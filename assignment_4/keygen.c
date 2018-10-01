// Chih-Hsiang Wang
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
	char random_key;
	int key_length = 0;
	srand(time(0)); // use srand to produce random seed

	// check the number of arguments
	if (argc != 2) {
		fprintf(stderr, "invalid number of args\n");
		exit(1);
	}

	// store the number of keys from argv[1] to the varaible(decimal)
	// sscanf : http://ccckmit.wikidot.com/cp:sscanf
	sscanf(argv[1], "%d", &key_length);

	if (key_length <= 0) {
		fprintf(stderr, "invalid number of keys\n");
		exit(1);
	}

	int i = 0;
	for (i = 0; i < key_length; i++) {
		random_key = (char) (rand() % (90 - 64 + 1) + 64);

		// let the 27th word be the space
		if (random_key == '@')
			random_key = ' ';

		printf("%c", random_key);
	}
	printf("\n");

	return 0;
}
