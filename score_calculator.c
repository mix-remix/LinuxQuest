#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#define DATA_FILE "data.bin"

#define PATH_SIZE 256
#define BUFF_SIZE 512

#define ID_SIZE 16
#define NAME_SIZE 32
#define CLUE_SIZE 128

#define MAX_USERS 100

typedef struct {
    char id[ID_SIZE];
    char username[NAME_SIZE];
    float latitude;
    float longitude;
    char clue[CLUE_SIZE];
    int value;
} Treasure;

typedef struct {
    char username[NAME_SIZE];
    int score;
} User;

int find_index(char* name, User* users, int n) {
    for (int i = 0; i < n; i++) {
        if (strcmp(users[i].username, name) == 0) {
            return i;
        }
    }

    return -1;
}

void process_file(char* hunt_id) {
    char file_path[PATH_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, DATA_FILE);

    int fd = open(file_path, O_RDONLY);

    if (fd < 0) {
        fprintf(stderr, "Error opening '%s': %s\n", file_path, strerror(errno));
        return;
    }

    User users[MAX_USERS];
    int count = 0;

    Treasure t;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        int index = find_index(t.username, users, count);

        if (index >= 0) {
            users[index].score += t.value;
        } else {
            if (count >= MAX_USERS) {
                fprintf(stderr, "Too many users.\n");
                close(fd);
                return;
            }

            strncpy(users[count].username, t.username, NAME_SIZE - 1);
            users[count].username[NAME_SIZE - 1] = '\0';
            users[count].score = t.value;

            count++;
        }
    }

    for (int i = 0; i < count; i++) {
        printf("%s - %d  ", users[i].username, users[i].score);
    }

    close(fd);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        exit(1);
    }

    process_file(argv[1]);

    return 0;
}
