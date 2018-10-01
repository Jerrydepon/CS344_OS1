// Chih-Hsiang Wang
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>

// ============================ Error Handling =============================
// Error function used for reporting issues
void error(const char* msg) {
	fprintf(stderr, "%s\n", msg);
}

// ============================ Return Size =============================
// Retrun the size of the file
int FileSize(char* file) {
	int file_length = 0;

	// open the file & get the length of text
	FILE* fp = fopen(file, "rb"); // use "rb" to ignore newline
	if (fp != NULL) {
		// fseek() & ftell(): https://www.geeksforgeeks.org/fseek-in-c-with-example/
		// move pointer to the end
		fseek(fp, 0, SEEK_END);
		// store the position of pointer
		file_length = ftell(fp);

		fclose(fp);
	}
	return file_length;
}

// ============================ Main - Client =============================
int main(int argc, char *argv[]) {
	int socketFD, portNumber;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	int text_size, text_length, key_size;
	char t_buffer[71000], k_buffer[71000], c_buffer[142000], d_buffer[71000];
	memset(t_buffer, '\0', sizeof(t_buffer)); // Clear out the buffer for text
	memset(k_buffer, '\0', sizeof(k_buffer)); // Clear out the buffer for key
	memset(c_buffer, '\0', sizeof(c_buffer)); // Clear out the buffer for complete message
	memset(d_buffer, '\0', sizeof(d_buffer)); // Clear out the buffer for decrypted data

	// Check usage & args
	if (argc < 4) {
		fprintf(stderr, "USAGE: %s text key port\n", argv[0]);
		exit(0);
	}

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) {
		fprintf(stderr, "CLIENT: ERROR, no such host\n");
		exit(0);
	}
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// =========================================================
	// Get the size of text in plaintext for the buffer
	text_size = FileSize(argv[1]);
	if(text_size == 0) {
		error("no such file or no contents in the text file");
		exit(1);
	}
	text_size += 2; // include the size for terminating characters

	// Get the size of key file
	key_size = FileSize(argv[2]);
	if (key_size == 0) {
		error("no such file or no contents in the key file");
		exit(1);
	}
	key_size += 2; // include the size for terminating characters

	// check if the number of key is enough
	if (key_size+1 < text_size) {
		fprintf(stderr, "Error: key '%s' is too short\n", argv[2]);
		exit(1);
	}

	// =========================================================
	// Store the text in the plaintext into buffer
	FILE* fp = fopen(argv[1], "rb"); // use "rb" to ignore newline
	if (fp == NULL) {
		error("plaintext does not exist\n");
		exit(1);
	}
	else {
		// move pointer to the end
		fseek(fp, 0, SEEK_END);
		// store the position of pointer
		text_length = ftell(fp);

		// move pointer to the starting
		fseek(fp, 0, SEEK_SET);
		// read the file
		fread(t_buffer, text_length, 1, fp);

		// replace newline & check the character
		int i = 0, ascii;
		for (i = 0; i < text_length; i++) {
			ascii = (int)t_buffer[i];
			// replace newline with '*'
			if(t_buffer[i] == '\n') {
				t_buffer[i] = '*';
				strcat(t_buffer, "*");
			}
			// check character
			else {
				if((ascii < 65 || ascii > 90) && ascii != 32 && ascii != 64) {
					error("Error: invalid character in plaintext (decrypt)\n");
					fclose(fp);
					exit(1);
				}
			}
		}
		fclose(fp);
	}

	// =========================================================
	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) {
		error("CLIENT: ERROR opening socket");
		exit(1);
	}

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) { // Connect socket to address
		error("CLIENT: ERROR connecting to server");
		exit(1);
	}

	// =========================================================
	// Store the key into buffer
	FILE* fp_2 = fopen(argv[2], "rb"); // use "rb" to ignore newline
	if (fp_2 == NULL) {
		error("key does not exist\n");
		exit(1);
	}
	else {
		// move pointer to the starting
		fseek(fp_2, 0, SEEK_SET);
		// read the file
		fread(k_buffer, text_size-3, 1, fp_2);

		// replace newline & check the character
		int i = 0, ascii_2 = 0, is_replace = 0;
		for (i = 0; i < text_size-3; i++) {
			ascii_2 = (int)k_buffer[i];
			// replace newline with '@'
			if (k_buffer[i] == '\n') {
				k_buffer[i] = '@';
				strcat(k_buffer, "@");
				is_replace = 1;
			}
			else {
				if((ascii_2 < 65 || ascii_2 > 90) && ascii_2 != 32 && ascii_2 != 64) {
					error("invalid character in key\n");
					fclose(fp_2);
				}
			}
		}
		// if no '\n' in the buffer
		if(is_replace == 0) {
			k_buffer[text_size-3] = '@';
			strcat(k_buffer,"@");
		}
		fclose(fp_2);
	}

	// // =========================================================
	// // Decide if connecting to the right(otp_dec_d) or wrong(otp_enc_d) server
	// char message_send[] = {"I"};
	// char message_recv[2] = {};
	// send(socketFD, message_send, sizeof(message_send), 0);
	// recv(socketFD, message_recv, sizeof(message_recv), 0);
	// // printf("dec: %s\n", message_recv);
	// if (strcmp(message_recv, "e") == 0) {
	// 	fprintf(stderr, "Error: could not contact otp_enc_d on port %s\n", argv[3]);
	// 	exit(2);
	// }
	// fflush(stdin);
	// fflush(stdout);

	// =========================================================
	// Store the content of text and key into complete buffer
	strcat(c_buffer, t_buffer);
	strcat(c_buffer, k_buffer);

	// Send data to server to for decryting
	int data_wirtten;
	int idx = 0;
	int buffer_size = sizeof(c_buffer) - 1;

	// Write data until c_buffer is filled
	while (idx < sizeof(c_buffer) - 1) {
		data_wirtten = send(socketFD, c_buffer+idx, buffer_size, 0);
		if (data_wirtten < 0) {
			error("CLIENT: ERROR writing to socket");
			exit(1);
		}
		idx += data_wirtten;
		buffer_size -= data_wirtten;
	}

	// =========================================================
	// Read the decrypted data from server
	int data_read;
	int idx_2 = 0;
	int buffer2_size = sizeof(d_buffer) - 1;

	// receive data until d_buffer is filled
	while (idx_2 < sizeof(d_buffer) - 1) {
		data_read = recv(socketFD, d_buffer+idx_2, buffer2_size, 0);
		if (data_read < 0) {
			error("CLIENT: ERROR reading from socket");
			exit(1);
		}
		idx_2 += data_read;
		buffer2_size -= data_read;
	}

	// change the "@@" at the end of d_buffer into termination symbol
	int term_pos = strstr(d_buffer, "@@") - d_buffer;
	d_buffer[term_pos] = '\0';

	// print out the data from decryption
	printf("%s\n", d_buffer);

	close(socketFD);
	return 0;
}
