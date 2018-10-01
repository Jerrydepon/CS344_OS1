/* Chih-Hsiang Wang */
/* gcc -std=c89 -o wangchih.buildrooms wangchih.buildrooms.c */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/* <<<<<<<<<<<<<<< Const + Structure >>>>>>>>>>>>>>> */
/* Define the const for number of total rooms and selected rooms */
#define NUM_ROOM_LIST 10
#define NUM_ROOM_USE 7

/* List of rooms' names */
char* room_names[NUM_ROOM_LIST] = {
  "Pikachu",
  "Bulbasaur",
  "Charmander",
  "Squirtle",
  "Jigglypuff",
  "Psyduck",
  "Dragonite",
  "Eevee",
  "Gyarados",
  "Mewtwo"
};

/* Struct used to connect with other rooms */
struct Connect {
  struct Room* other_room;
};

/* Sturct of each room */
struct Room {
  char* room_name;
  int num_connect;
  struct Connect* connect;
  char* room_type;
};

/* <<<<<<<<<<<<<<< Founction - Create Rooms >>>>>>>>>>>>>>> */
/* Create all the rooms we will use */
void CreateRooms(struct Room* room_array) {
  int room_idx = 0;
  int rand_idx[NUM_ROOM_USE];
  int i = 0, j = 0;

  /* iterate through NUM_ROOM_USE and create each room */
  for (i = 0; i < NUM_ROOM_USE; i++) {
    /* make a random number from 0 to 9, for the use of random room name */
    srand(time(NULL));
    /* avoid same name of rooms */
    do {
      rand_idx[i] = rand() % NUM_ROOM_LIST;
      for (j = 0; j < i; j++) {
        if (rand_idx[i] == rand_idx[j])
          break;
      }
    } while (j != i);

    /* make a space for Room */
    struct Room* room = malloc(sizeof(struct Room));
    /* make an array to store connected information */
    struct Connect* connect_room = malloc(sizeof(struct Connect) * NUM_ROOM_USE-1);
    /* create information for 1.name 2.number of connect 3.array of connected room 4.type */
    room->room_name = room_names[rand_idx[i]];
    room->num_connect = 0;
    room->connect = connect_room;
    /* decide the type of room */
    if (room_idx == 0)
      room->room_type = "START_ROOM";
    else if (room_idx == NUM_ROOM_USE-1)
      room->room_type = "END_ROOM";
    else
      room->room_type = "MID_ROOM";

    /* put the room into array, and plus 1 to index of room */
    room_array[i] = *room;
    room_idx++;
  }
}

/* <<<<<<<<<<<<<<< Founction - Connect Rooms >>>>>>>>>>>>>>> */
/* Returns true if all rooms have 3 to 6 outbound connections, false otherwise */
bool IsGraphFull(struct Room* room_array) {
  int i = 0;
  for (i = 0; i < NUM_ROOM_USE; i++) {
    if (room_array[i].num_connect < 3 || room_array[i].num_connect > 6)
      return false;
  }
  return true;
}

/* Returns a idx of random Room, does NOT validate if connection can be added */
int GetRandomRoom() {
  /* make a random number from 0 to 6, for the use to select random room */
  int rand_idx = 0;
  rand_idx = rand() % NUM_ROOM_USE;

  return rand_idx;
}

/* Returns true if a connection can be added from Room x (< 6 outbound connections), false otherwise */
bool CanAddConnectionFrom(struct Room* x) {
  if (x->num_connect < NUM_ROOM_USE-1)
    return true;
  else
    return false;
}

/* Returns true if Rooms x and y are the same Room, false otherwise */
bool IsSameRoom(struct Room* x, struct Room* y)
{
  if (x->room_name == y->room_name)
    return true;
  else
    return false;
}

/* Returns true if a connection from Room x to Room y already exists, false otherwise */
bool ConnectionAlreadyExists(struct Room* x, struct Room* y) {
  int i = 0;
  for (i = 0; i < x->num_connect; i++) {
    if (x->connect[i].other_room == y)
      return true;
  }
  return false;
}

/* Connects Rooms x and y together, does not check if this connection is valid */
void ConnectRoom(struct Room* x, struct Room* y) {
  x->connect[x->num_connect].other_room = y;
  x->num_connect++;
  y->connect[y->num_connect].other_room = x;
  y->num_connect++;
}

/* Adds a random, valid outbound connection from a Room to another Room */
void AddRandomConnection(struct Room* room_array) {
  int x, y;
  /* randomly find a room x which can still add connection */
  while(true) {
    x = GetRandomRoom();
    if (CanAddConnectionFrom(&room_array[x]) == true)
      break;
  }
  /* randomly find a room y which can still add connection & not the same room & connection not already exist */
  do {
    y = GetRandomRoom();
  } while(CanAddConnectionFrom(&room_array[y]) == false || IsSameRoom(&room_array[x], &room_array[y]) == true || ConnectionAlreadyExists(&room_array[x], &room_array[y]) == true);

  /* coonect selected two rooms */
  ConnectRoom(&room_array[x], &room_array[y]);

  /* test code: print the name of room & number of connect during steps of connection */
  /* ================================================= */
  /* printf("x:%s  ", room_array[x].room_name);
  printf("y:%s  ", room_array[y].room_name);
  printf("x:%d  ", room_array[x].num_connect);
  printf("y:%d  \n", room_array[y].num_connect); */
  /* ================================================= */
}

/* <<<<<<<<<<<<<<< Founction - Create Directory+Files >>>>>>>>>>>>>>> */
/* Create a directory and the files inside the directory (number of used rooms) */
void CreateDirAndFiles(struct Room* room_array) {
  int i = 0, j = 0;
  char directory_name[25];  /* make a array to store name of directory */
  pid_t pid = getpid();     /* get the uniqe process ID lest coincide with other directory name */
  /* concatenate the file name with process ID */
  sprintf(directory_name, "wangchih.rooms.%d", pid);
  /* making a directory, mode = 0700 */
  mkdir(directory_name, 0700);
  /* get into directory for creating files in it */
  chdir(directory_name);

  /* write the text into each file */
  for (i = 0; i < NUM_ROOM_USE; i++) {
    FILE* fp;
    fp = fopen(room_array[i].room_name, "w");
    /* write the text in the file */
    fprintf(fp, "ROOM NAME: %s\n", room_array[i].room_name);
    for (j = 0; j < room_array[i].num_connect; j++) {
      fprintf(fp, "CONNECTION %d: %s\n", j+1, room_array[i].connect[j].other_room->room_name);
    }
    fprintf(fp, "ROOM TYPE: %s", room_array[i].room_type);
    /* close the files */
    fclose(fp);
  }
  /* return to the parent directory */
  chdir("..");
}

/* <<<<<<<<<<<<<<< Founction - Free Memory >>>>>>>>>>>>>>> */
void FreeArray(struct Room* room_array) {
  int i = 0;
  for (i = 0; i < NUM_ROOM_USE; i++) {
    free(room_array[i].connect);
  }
  free(room_array);
}

/* <<<<<<<<<<<<<<< Main >>>>>>>>>>>>>>> */
int main() {
  srand(time(NULL));
  struct Room* room_array = malloc(sizeof(struct Room) * NUM_ROOM_USE);
  /* create all the room we will use */
  CreateRooms(room_array);

  /* create all connections in graph */
  while (IsGraphFull(room_array) == false) {
    AddRandomConnection(room_array);
  }
  /* test code: print out each room & its connected rooms after connection */
  /* ================================================= */
  /* for(int i = 0; i < NUM_ROOM_USE; i++){
  printf("room name: %s\n",room_array[i].room_name );
    for(int j = 0; j < room_array[i].num_connect; j++) {
     printf("connecnt room: %s\n", room_array[i].connect[j].other_room->room_name);
    }
  } */
  /* ================================================= */

  /* create a directory and the files inside the directory */
  CreateDirAndFiles(room_array);

  /* free memory */
  FreeArray(room_array);

  /* ================================================= */
  /* printf("Done\n"); */
  /* ================================================= */
  return 0;
}
