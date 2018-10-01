/* Chih-Hsiang Wang */
/* gcc -std=c89 -o wangchih.adventure wangchih.adventure.c -lpthread */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>

/* <<<<<<<<<<<<<<< Const + Structure >>>>>>>>>>>>>>> */
/* Define the const */
#define NUM_ROOM_USE 7

/* Varaible for thread */
pthread_t tid;
pthread_mutex_t lock;

/* Global variable */
int num_path = 0; /* used for storing the paths of game */
int file_idx = 0; /* used for array about stored files' information */
int after_time = 0;

/* Struct used to connect with other rooms */
struct Connect {
  char* connect_name;
  struct Room* other_room;
};

/* Sturct of each room */
struct Room {
  char* room_name;
  int num_connect;
  char* room_type;
  struct Connect* connect;
};

/* <<<<<<<<<<<<<<< Founction - Second Thread >>>>>>>>>>>>>>> */
void *ProduceTimeTxt(void* arg) {
    /* lock the main thread */
    pthread_mutex_lock(&lock);

    char outstr[200];
    time_t t;
    struct tm* tmp;

    /* create file to store time information */
    FILE* file = fopen("./currentTime.txt", "w");

    /* write in current time information */
    t = time(NULL);
    tmp = localtime(&t);
    strftime(outstr, sizeof(outstr), "%I:%M%p, %A, %B %d, %Y", tmp);
    printf("%s\n", outstr);
    fprintf(file, "%s", outstr);
    fclose(file);

    /* unlock the thread */
    pthread_mutex_unlock(&lock);
    return NULL;
}

/* <<<<<<<<<<<<<<< Founction - Get files' content >>>>>>>>>>>>>>> */
/* Store the necessary contents of file into array */
void StoreContent(struct Room* file_array, struct dirent* seed) {
  FILE* fp; /* used for open file */
  char* token;  /* used for strstr() */
  fp = fopen(seed->d_name, "r");
  char line[100];

  /* read one line for each step until the end of file */
  while(!feof(fp)) {
    fgets(line, 100, fp);

    /* extract information of room_name */
    if (strstr(line, "ROOM NAME") != NULL) {
      token = strtok(line, ":");
      token = strtok(NULL, " \n");
      strcpy(file_array[file_idx].room_name, token);
    }
    /* extract information of connect_name */
    else if (strstr(line, "CONNECTION") != NULL) {
      token = strtok(line, " ");
      token = strtok(NULL, " ");
      token = strtok(NULL, "\n");
      strcpy(file_array[file_idx].connect[file_array[file_idx].num_connect].connect_name, token);
      file_array[file_idx].num_connect++;
    }
    /* extract information of room_type */
    else if (strstr(line, "ROOM TYPE") != NULL) {
      token = strtok(line, ":");
      token = strtok(NULL, " \n");
      strcpy(file_array[file_idx].room_type, token);
    }
  }
  fclose(fp);
  /* Reference */
  /* read files - https:/www.youtube.com/watch?v=8nIilb2kiSU */
  /* strtok() - https:/www.tutorialspoint.com/c_standard_library/c_function_strtok.htm */
}

/* Open latest modified "wangchih.room" directory & get access to the files inside */
void OpenDirAndStore(struct Room* file_array) {
  DIR* dir = NULL;  /* directory stream handle */
  struct dirent* seed_parent; /* used for current directory */
  struct dirent* seed_child;  /* used for inside the specific directory */
  struct stat file_state; /* return the state of found file (file_state.st_mtime) */
  time_t latest_room_time; /* used to keep track of time of newest created room */
  char* latest_room_name; /* used to keep track of name of newest created room */
  int count = 0; /* used for escaping first comparison */

  /* open current directory */
  dir = opendir(".");
  if (dir == NULL) {
    printf("Error! Unable to open the directory.");
    exit(1);
  }

  /* browse through all directories and files */
  while ( (seed_parent = readdir(dir)) != NULL) {
    /* find latest directory consist of "wangchih.room" */
    if (strncmp(seed_parent->d_name, "wangchih.room", 13) == 0) {
      /* get the latest modified time of file */
      stat(seed_parent->d_name, &file_state);
      /* skip the first found room & skip the ealier modified room */
      if (count != 0 && latest_room_time >= file_state.st_mtime)
        continue;
      latest_room_time = file_state.st_mtime;
      latest_room_name = seed_parent->d_name;
      count += 1;
    }
  }
  /* change to the found directory */
  chdir(latest_room_name);
  /* open the latest rooms' directory */
  dir = opendir(".");
  if (dir == NULL) {
    printf("Error! Unable to open the directory.");
    exit(1);
  }
  /* browse through each file */
  while ( (seed_child = readdir(dir)) != NULL) {
    /* exclude directory "." & ".." */
    if (strlen(seed_child->d_name) <= 2)
      continue;
    /* remove the time file created before */
    if (strncmp(seed_child->d_name, "currentTime", 11) == 0)
      continue;
    /* store the necessary contents of file into array */
    StoreContent(file_array, seed_child);
    file_idx++; /* go to next idx of array for storing information */
  }
  closedir(dir);
  /* Reference */
  /* opendir() - https:/www.youtube.com/watch?v=vbAfIGR_5XM */
}

/* <<<<<<<<<<<<<<< Founction - Game Process >>>>>>>>>>>>>>> */
/* Reach the end of game & print out result */
void GoalAndResult(char path_array[100][20]) {
  int i = 0;

  printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
  printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", num_path);
  /* print out the paths taken */
  for (i = 0; i < num_path; i++) {
    printf("%s\n", path_array[i]);
  }
}

/* Print out the information for user to select next path */
void PrintDirection(struct Room* file_array) {
  int i = 0;

  /* situation after receiving input "time" */
  if (after_time == 1) {
    printf("\nWHERE TO? >");
    after_time = 0;
  }
  /* situation except for receiving input "time" */
  else {
    printf("CURRENT LOCATION: %s\n", file_array[file_idx].room_name);
    printf("POSSIBLE CONNECTIONS: ");

    /* print out each possible connections */
    for (i = 0; i < file_array[file_idx].num_connect; i++) {
      if (i < file_array[file_idx].num_connect - 1)
        printf("%s, ", file_array[file_idx].connect[i].connect_name);
      else
        printf("%s.\n", file_array[file_idx].connect[i].connect_name);
    }
    printf("WHERE TO? >");
  }
}

/* Running process of the game */
void GameProcess(struct Room* file_array, char path_array[100][20]) {
  int i = 0, j = 0;
  char input[100];
  int next_idx = 0; /* index of the next room */
  bool find_next = false; /* decide if input is valid or not */

  /* print out the information for user to select next path */
  PrintDirection(file_array);

  /* receive input from user */
  scanf("%99s", input); /* prevent scanf from reading in names that are longer than 99 characters */
  printf("\n");

  /* loop through each connections of current room */
  for (i = 0; i < file_array[file_idx].num_connect; i++) {
    /* valid input for path */
    if (strcmp(file_array[file_idx].connect[i].connect_name, input) == 0) {
      find_next = true;

      /* store the valid room_name into path array */
      strcpy(path_array[num_path], input);
      num_path++;

      /* get the index of the next room */
      for (j = 0; j < NUM_ROOM_USE; j++) {
        if (strcmp(file_array[j].room_name, input) == 0)
          next_idx = j;
      }

      /* reach the goal room */
      if (strcmp(file_array[next_idx].room_type, "END_ROOM") == 0){
        GoalAndResult(path_array);
        exit(0);
      }
      /* keep on finding the final room */
      else {
        file_idx = next_idx;
        GameProcess(file_array, path_array);
        break;
      }
    }
  }

  /* use thread to create current time file */
  if (strcmp(input, "time")==0) {
    /* unlock the line of thread */
    pthread_mutex_unlock(&lock);
    /* start the thead */
    pthread_join(tid, NULL);
    /* lock the thread line */
    pthread_mutex_lock(&lock);
    /* create the thread line */
    pthread_create(&tid, NULL, &ProduceTimeTxt, NULL);

    find_next = true;
    after_time = 1;
    GameProcess(file_array, path_array);
  }

  /* invalid input */
  if (find_next == false) {
    printf("HUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
    GameProcess(file_array, path_array);
  }
}

/* <<<<<<<<<<<<<<< Founction - Initilization & Memory Management >>>>>>>>>>>>>>> */
/* Creat an array to store the information of rooms and connections */
struct Room* CreateArray() {
  int i = 0, j = 0;

  struct Room* file_array = malloc(sizeof(struct Room) * NUM_ROOM_USE);

  /* initilize array */
  for (i = 0; i < NUM_ROOM_USE; i++) {
    file_array[i].room_name = malloc(sizeof(char*) * 20);
    file_array[i].num_connect = 0;
    file_array[i].room_type = malloc(sizeof(char*) * 20);
    file_array[i].connect = malloc(sizeof(struct Connect) * NUM_ROOM_USE-1);

    /* each room has multiple connections */
    for (j = 0; j < NUM_ROOM_USE-1; j++) {
      file_array[i].connect[j].connect_name = malloc(sizeof(char*) * 20);
    }
  }
  return file_array;
}

/* Free the array */
void FreeArray(struct Room* file_array) {
  int i, j;
  for (i = 0; i < NUM_ROOM_USE; i++) {
    for (j = 0; j < NUM_ROOM_USE-1; j++) {
      free(file_array[i].connect[j].connect_name);
    }
    free(file_array[i].room_name);
    free(file_array[i].room_type);
    free(file_array[i].connect);
  }
}

/* <<<<<<<<<<<<<<< Main >>>>>>>>>>>>>>> */
int main() {
  /* lock the main  */
  pthread_mutex_lock(&lock);
  pthread_create(&tid, NULL, &ProduceTimeTxt, NULL);

  int i = 0;
  struct Room* file_array = CreateArray(); /* create an array to store information for later game */
  char path_array [100][20]; /* to store the path taken [NUMBER_OF_STRINGS][STRING_LENGTH+1] */

  /* open specific directory & store the specific information into array */
  OpenDirAndStore(file_array);
  /* test code: print the information sotred in the array */
  /* ================================================= */
  /* int j = 0, k = 0;
  for (j = 0; j < NUM_ROOM_USE; j++) {
    printf("\nname: %s\n", file_array[j].room_name);
    printf("type: %s\n", file_array[j].room_type);
    for (k = 0; k < file_array[j].num_connect; k++) {
      printf("connect: %s\n", file_array[j].connect[k].connect_name);
    }
  } */
  /* ================================================= */

  /* find the start room */
  for (i = 0; i < NUM_ROOM_USE; i++) {
    if (strcmp(file_array[i].room_type, "START_ROOM") == 0)
      file_idx = i;
  }
  /* start the game */
  GameProcess(file_array, path_array);

  /* free the array of information storer */
  FreeArray(file_array);

  return 0;
}
