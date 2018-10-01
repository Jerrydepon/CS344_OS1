// Chih-Hsiang Wang
// gcc -o smallsh smallsh.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

// <<<<<<<<<<<<<<< Global Variables >>>>>>>>>>>>>>>
char* std_in = NULL; // store the arg after <
char* std_out = NULL; // store the arg after >
int if_stdin = 0; // decide if need to redirect with std_in 
int if_stdout = 0; // decide if need to redirect with std_out
int foreground = 0; // forefround-only mode or not
int background = 0; // if or not user add & to enter background mode
int child_exit_method; // decide whether child exist normally or by signal
int tmp_back_pid = 0; // memorize the background pid
int num_array; // for testing

// <<<<<<<<<<<<<<< Signal Handling >>>>>>>>>>>>>>>
// Avoid CRTL-C impact while in parent
void Do_nothing() { 
  	return;
}
// Catch CRTL-C interrupt signal in child
void Interrupt(int sig) {
	int interrupt_signal = WTERMSIG(sig);
    	printf("terminated by signal %d\n", interrupt_signal);
}
// Exit status & Terminated signal
void Status_shell() {
	// child process terminates normally
	if (WIFEXITED(child_exit_method) != 0) {
		int exit_status = WEXITSTATUS(child_exit_method);
		printf("exit value %d\n", exit_status);
	}
	// child process was terminated by a signal
	else if (WIFSIGNALED(child_exit_method) != 0) {
		int termSignal = WTERMSIG(child_exit_method);
		printf("terminated by signal %d\n", termSignal);
	}
}
// Change to foreground-only mode or not by CRTL-Z
void Change_foreground(int sigNum) {
        // exit foreground-only mode
        if(foreground == 1) {
                foreground = 0;
                printf("\nExiting foreground-only mode\n:");
        }
        // enter foreground-only mode
        else if (foreground == 0) {
                foreground = 1;
                printf("\nEntering foreground-only mode (& is now ignored)\n:");
        }
}

// <<<<<<<<<<<<<<< Some Functions >>>>>>>>>>>>>>>
// Change directory
void Cd_shell(char** args_array) {
        // do nothing if there are more than one arg after "cd"
        if (args_array[2] != NULL)
                return;
        else {
                // change directory to HOME if there is no specific arg
                if (args_array[1] == NULL)
                        args_array[1] = getenv("HOME");
                // change to the directory, return 0 if succeed, return 1 if fail
                if (chdir(args_array[1]) != 0)
                        perror("cd error");
        }
}
// Find the zombie(defunt) once they complete running
void Find_defunt() {
        // check if any process has completed, return 0 if none have
        pid_t completed_pid = 0;
        completed_pid = waitpid(-1, &child_exit_method, WNOHANG);

        // no zombie exist
        if (completed_pid == -1)
                return;

        // find the completed zombie
        if (completed_pid != 0) {
                printf("background pid %d is done: ", tmp_back_pid);
                Status_shell();
		fflush(stdout);
        }
}
// replace words: https://www.geeksforgeeks.org/c-program-replace-word-text-another-given-word/
// In the test script, need to use cd testdir$$ & change $$ into shell ID
char* ReplaceWord(const char* all_words, const char* old_word, const char* new_word) {
	int i = 0;
	int count = 0; // number of old word
    	int new_len = strlen(new_word);
    	int old_len = strlen(old_word);
    	char *result; // new directory name to be return

	// loop string until the end
    	for (i = 0; all_words[i] != '\0'; i++) {
        	// count the number needed to be changed & the idx to be changed
        	if (strstr(&all_words[i], old_word) == &all_words[i]) {
            		count++;
            		i += old_len;
        	}
    	}
	
	// malloc for the final return string
	result = malloc(sizeof(char) * (i + count* (new_len - old_len)));	

    	i = 0;
    	// loop string until the end
    	while (*all_words) {
        	// replace old words with new words
        	if (strstr(all_words, old_word) == all_words) {
            		strcpy(&result[i], new_word);
            		i += new_len;
            		all_words += old_len;
        	}
        	else
            		result[i++] = *all_words++;
    	}
	//put \0 in the end of the string
    	result[i] = '\0';
    	
	return result;
}

// <<<<<<<<<<<<<<< Redirecting >>>>>>>>>>>>>>>
// redirect: https://www.youtube.com/watch?v=fL-zXw_oLbw
// < , read and print out
void Stdin_file(char* file_name) {
	int ret = 0;
	int file = 0;
	
	// open file for read
	file = open(file_name, O_RDONLY);
	if (file == -1) {
		printf("cannot open %s for input\n", file_name);
		exit(1);
	}

	// duplicate open file descriptor onto another file descriptor
	ret = dup2(file, 0);
	if (ret < 0)
		perror("dup2 error");
	
	close(file);
}
// reference: http://joe.is-programmer.com/posts/17463.html
// > , write in
void Stdout_file(char* file_name) {
	int ret = 0;
	int file = 0;

	// open file for write, if no file exist, then create one
	file = open(file_name, O_CREAT | O_WRONLY, 0744);
	if (file == -1)
		perror("open file error");
	
	// duplicate open file descriptor onto another file descriptor
	ret = dup2(file, 1);
	if (ret < 0)
		perror("dup2 error");
	
	close(file);	
}

// <<<<<<<<<<<<<<< Exec & Fork >>>>>>>>>>>>>>>
// Fork a child & execute arguments
void Fork_args(char** args_array) {
	pid_t spawn_pid = -5;
	child_exit_method = -5;

	spawn_pid = fork();
	switch(spawn_pid) {
		// error forking
		case -1:
			perror("Fork error");
			break;
		// child process
		case 0:
			// redirect of std_in
			if (if_stdin == 1) 
				Stdin_file(std_in);
			// redirect of std_out
			if (if_stdout == 1)  
				Stdout_file(std_out);
			// execute args
			if (execvp(args_array[0], args_array) < 0) {
				perror(args_array[0]);
				exit(1);
			}
		// parent process
		default:
			// print out background pid & store it until it is done
			if (background == 1) { 
				printf("background pid is %d\n", spawn_pid);
				tmp_back_pid = spawn_pid;
				background = 0;
				return;
			}
			// block the process until the specific child process is terminated
			else {
				// if interrupt by CRTL-C in child
				signal(SIGINT, Interrupt);
			
				waitpid(spawn_pid, &child_exit_method, 0);
			}
			break;
	}
}
// Execute the arguments
int Exec_args(char** args_array) {
	int num_builtin = 0;
	// no args
	if (args_array[0] == NULL)
		return 1;
	// ignore command begin with #
	else if (strstr(args_array[0], "#") != NULL)
		return 1;
	// exit the shell
	else if (strcmp(args_array[0], "exit") == 0) {
		// return 0 to exit shell
		return 0;
	}
	// move on to change directory function
	else if (strcmp(args_array[0], "cd") == 0) {
		Cd_shell(args_array);
		return 1;
	}
	// move on to print status function
	else if (strcmp(args_array[0], "status") == 0) {	
		background = 0;
		Status_shell();
		return 1;
	}
	// if there is arg which is not built in function & exit
	else {	
		Fork_args(args_array);
		return 1;
	}
}

// <<<<<<<<<<<<<<< Get Arguments >>>>>>>>>>>>>>>
// Split the line into args & check for the special case
char** Split_line(char* line) {
	char** args_array = malloc(sizeof(char*) * 512); // to store the keyin arguments
	char* token; // used for iterate through the splitted words
	int idx = 0; // idx for array
	num_array = 0;	// total number of elemnets in the array	

	// split the line first by the space
	token = strtok(line, " \n"); // remember \n to avoid \n stored into array

	// loop until there is no arg left
	do {
		// no arg
		if (token == NULL)
			break;
		// redirecting stdin
		else if (strcmp(token, "<") == 0) {
			// move to the arg(file_name) after <
			token = strtok(NULL, " \n");
			// no arg after "<"
			if (token == NULL)
				return args_array;
			// remember the file_name 
			else {
				std_in = malloc(sizeof(char) * strlen(token));
				strcpy(std_in, token);
				if_stdin = 1; // set for deciding in child process
			}		
		}
		// redirecting stdout
		else if (strcmp(token, ">") == 0) {
			// move to the arg(file_name) after >
			token = strtok(NULL, " \n");
			// no arg after ">"
			if (token == NULL)
				return args_array;
			// remember the file_name 
			else {
				std_out = malloc(sizeof(char) * strlen(token));
				strcpy(std_out, token);
				if_stdout = 1; // set for deciding in child process
			}
		}
		// change to run in background
		else if (strcmp(token, "&") == 0 && strcmp(args_array[0], "echo") != 0) {
			background = 1;
			// ignore the & if in foreground-only mode
			if (foreground == 1)
				background = 0;	
		}
		// process ID of the shell, use $$ to represen
		else if (strstr(token, "$$") != NULL) {
			char* pid_string = malloc(sizeof(char) * 512);
			pid_t smallsh_ID = getpid();
			// convert int to string 
            		sprintf(pid_string, "%d", smallsh_ID);
			
			// convert $$ in the words into pid number
			pid_string = ReplaceWord(token, "$$", pid_string);
			// store the testdir + pid number(string) into array
			args_array[idx] = pid_string;
			idx++;
			num_array++;
		}
		// other command
		else {
			args_array[idx] = token;
			idx++;
			num_array++;
		}
		
		// move on to next arg
		token = strtok(NULL, " \n");

	} while(token);
	
	// test code
	//====================================================//
	//for (int k = 0; k < 4; k++)
	//	printf("args[%d] %s\n", k,args_array[k]);
	//====================================================//
	return args_array;
}
// Get the whole line & split it into words & store in the array
char** Get_args() {
	char** args_array; // to store the keyin arguments
	char* line = NULL; // to store a line
	ssize_t buffer_size = 0; //  the same as size_t, but is a signed type. able to represent -1
	// ssize_t: https://jameshfisher.com/2017/02/22/ssize_t.html
	
	// get a line from user input
	getline(&line, &buffer_size, stdin);
	fflush(stdout); // clear the buffer
	
	// split the line into words & store them into the array 
	args_array = Split_line(line);	
	
	return args_array;
}

// <<<<<<<<<<<<<<< Main Process >>>>>>>>>>>>>>>
// Input & parse the command each time
void Shell_loop() {
	char** args_array; // to store the key-in arguments
	int check_exit = 0; // to decide whether to exit
	
	// receiving command until exit
	do {
		// avoid CRTL-C impact while in parent
		signal(SIGINT, Do_nothing);

		if_stdin = 0;
		if_stdout = 0;

		printf(":");
	
		// store input from user into array
		args_array = Get_args();
		
		// test code
		//====================================================//
		//for (int i = 0; i < num_array; i++) {
		//	if (i == num_array - 1)
		//		printf("arg%d: %s\n", i, args_array[i]);
		//	else {
		//		printf("arg%d: %s ", i, args_array[i]);
		//	}
		//	
		//}	
		//printf("if_stdin: %d, if_stdout: %d\n", if_stdin, if_stdout);
		//====================================================//
		
		// execute the arguments, return 1 or 0 to decide running loop or not	
		check_exit = Exec_args(args_array);
		
		// find the zombie(defunt) once they complete running
		Find_defunt();	

	} while (check_exit);
}
// write a small shell: https://brennan.io/2015/01/16/write-a-shell-in-c/
// Main
int main() {
	// catch signal: https://stackoverflow.com/questions/4217037/catch-ctrl-c-in-c
	// avoid CRTL-C impact while in parent
	signal(SIGINT, Do_nothing);
	// catch CRTL-Z signal
	signal(SIGTSTP, Change_foreground);
	// loop the shell until receive exit	
	Shell_loop();

	return 0;
}
