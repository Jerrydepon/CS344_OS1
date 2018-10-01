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

// ============================ Main - Server =============================
int main(int argc, char *argv[]) {
	int listenSocketFD, establishedConnectionFD, portNumber;
	socklen_t sizeOfClientInfo;
	struct sockaddr_in serverAddress, clientAddress;
	char c_buffer[142000], t_buffer[71000], k_buffer[71000], d_buffer[71000];
	memset(c_buffer, '\0', sizeof(c_buffer)); // Clear out the buffer for complete message
	memset(t_buffer, '\0', sizeof(t_buffer)); // Clear out the buffer for text
	memset(k_buffer, '\0', sizeof(k_buffer)); // Clear out the buffer for key
	memset(d_buffer, '\0', sizeof(d_buffer)); // Clear out the buffer for decrypted data

	// Check usage & args
	if (argc < 2) {
		fprintf(stderr, "USAGE: %s port\n", argv[0]);
		exit(1);
	}

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) {
		error("ERROR opening socket");
		exit(1);
	}

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) { // Connect socket to port
		error("ERROR on binding");
		exit(1);
	}
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	// =========================================================
	// Keep running the server for connecting
	// int is_once = 0;
	while(1) {
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) {
			error("ERROR on accept");
			exit(1);
		}

		// // =========================================================
		// // Decide if connecting to the right(otp_dec_d) or wrong(otp_enc_d) server
		// if (is_once == 0) {
		// 	char message_send[] = {"d"};
		// 	char message_recv[5] = {};
		// 	recv(establishedConnectionFD, message_recv, sizeof(message_recv), 0);
		// 	// printf("dec_d: %s\n", message_recv);
		// 	send(establishedConnectionFD, message_send, sizeof(message_send), 0);
		// 	fflush(stdin);
		// 	fflush(stdout);
		//
		// 	is_once = 1;
		// 	close(establishedConnectionFD);
		// 	continue;
		// }

		// =========================================================
		// Get the message from the client
		int data_read;
		int idx = 0;
		int buffer_size = sizeof(c_buffer) - 1;

		// receive data until c_buffer is filled
		while(idx < sizeof(c_buffer) - 1) {
			data_read = recv(establishedConnectionFD, c_buffer+idx, buffer_size, 0);
			if (data_read < 0) {
				error("SERVER: ERROR reading from socket");
				exit(1);
			}
			idx += data_read;
			buffer_size -= data_read;
		}

		// =========================================================
		// Handling complete message from client into text & key
		char* end_location = strstr(c_buffer, "**");
		int term_lenth = strlen("**");
		// copy only text from c_buffer to t_buffer
		strncpy(t_buffer, c_buffer, end_location-c_buffer);
		// fill the key buffer
		sprintf(k_buffer, "%s", end_location+term_lenth);

		// replace "@@" from the end of key to '\0'
		int term_pos = strstr(k_buffer, "@@") - k_buffer;
		k_buffer[term_pos] = '\0';

		// =========================================================
		// Decrypt the text by the key (OTP)
		int i = 0;
		for(i = 0; i < strlen(t_buffer); i++) {
			int text_i = (int)t_buffer[i];
			int key_i = (int)k_buffer[i];
			int decrypt_i;

			// text, replace space into 26th character
			if(text_i == 32)
				text_i = 26;
			// text, if not space, minus 65 ('A')
			else
				text_i -= 65;

			// key, replace space into 26th character
			if(key_i == 32)
				key_i = 26;
			// key, if not space, minus 65 ('A')
			else
				key_i -= 65;

			// decryption by minusing text by key
			decrypt_i = text_i - key_i;
			// decreption, make sure the range is from 0 to 26
			if (decrypt_i < 0)
				decrypt_i += 27;
			// decryption, transform the 27th into space
			if(decrypt_i == 26)
				decrypt_i = 32;
			// decryption, transform back to 'A' to 'Z'
			else
				decrypt_i += 65;

			// decryption, transform back to char from int and store into d_buffer
			d_buffer[i] = (char)decrypt_i;
		}

		// add termination symbol
		strcat(d_buffer, "@@");

		// =========================================================
		// Send decryted data back to client
		int data_wirtten;
		int idx_2 = 0;
		int buffer2_size = sizeof(d_buffer) - 1;
		while(idx_2 < sizeof(d_buffer) - 1) {
			data_wirtten = send(establishedConnectionFD, d_buffer+idx_2, buffer2_size, 0);
			if (data_wirtten < 0) {
				error("SERVER: ERROR writing to socket");
				exit(1);
			}
			idx_2 += data_wirtten;
			buffer2_size -= data_wirtten;
		}

		close(establishedConnectionFD); // Close the existing socket which is connected to the client
	}

	close(listenSocketFD); // Close the listening socket
	return 0;
}
