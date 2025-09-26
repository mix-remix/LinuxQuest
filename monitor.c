#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <dirent.h>

#define DATA_FILE "data.bin"
#define CMD_FILE "commands.txt"

#define BUFF_SIZE 512
#define PATH_SIZE 256
#define INPUT_SIZE 256

#define ID_SIZE 16
#define NAME_SIZE 32
#define CLUE_SIZE 128

#define END_FLAG "END"

typedef struct {
    char id[ID_SIZE];
    char username[NAME_SIZE];
    float latitude;
    float longitude;
    char clue[CLUE_SIZE];
    int value;
} Treasure;

int sig_received = 0;

void sigusr1_handler(int sig_code) {
    sig_received = 1;
}

void list_hunts() {
    DIR* dir = opendir(".");

    if (!dir) {
        fprintf(stderr, "Failed to open directory: %s\n", strerror(errno));
        fflush(stdout);
        return;
    }

    struct dirent* entry;
    struct stat st;

    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (stat(entry->d_name, &st) == 0 && S_ISDIR(st.st_mode)) {
            char file_path[PATH_SIZE];
            snprintf(file_path, sizeof(file_path), "%s/%s", entry->d_name, DATA_FILE);

            if (access(file_path, F_OK) == 0) {
                int fd = open(file_path, O_RDONLY);

                if (fd < 0) {
                    fprintf(stderr, "Failed to open file '%s': %s\n", file_path, strerror(errno));
                    continue;
                }

                Treasure t;
                int count = 0;

                while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
                    count++;
                }

                close(fd);

                printf("%s: %d %s\n", entry->d_name, count, (count == 1) ? "treasure" : "treasures");
            }
        }
    }

    closedir(dir);
    fflush(stdout);
}

void list_treasures(char* hunt_id) {
    char file_path[PATH_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, DATA_FILE);

    int fd = open(file_path, O_RDONLY);

    if (fd < 0) {
        printf("Hunt not found.\n");
        fflush(stdout);
        return;
    }

    Treasure t;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %s | User: %s | Value: %d\n", t.id, t.username, t.value);
    }

    close(fd);
    fflush(stdout);
}

void view_treasure(char* hunt_id, char* treasure_id) {
    char file_path[PATH_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, DATA_FILE);

    int fd = open(file_path, O_RDONLY);

    if (fd < 0) {
        printf("Hunt not found.\n");
        fflush(stdout);
        return;
    }

    Treasure t;
    int found = 0;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.id, treasure_id) == 0) {
            printf("ID: %s\nUser: %s\nLat: %.2f\nLong: %.2f\nClue: %s\nValue: %d\n",
                   t.id, t.username, t.latitude, t.longitude, t.clue, t.value);

            found = 1;
            break;
        }
    }

    if (!found) {
        printf("Treasure not found.\n");
    }

    close(fd);
    fflush(stdout);
}

void process_cmd() {
    int fd = open(CMD_FILE, O_RDONLY);

    if (fd < 0) {
        printf("Failed to open %s: %s\n%s\n", CMD_FILE, strerror(errno), END_FLAG);
        fflush(stdout);
        return;
    }

    char input[INPUT_SIZE];
    int n = read(fd, input, sizeof(input) - 1);
    close(fd);

    if (n <= 0) {
        printf("Invalid command file\n%s\n", END_FLAG);
        fflush(stdout);
        return;
    }

    input[n] = '\0';
    input[strcspn(input, "\n")] = 0;

    char* cmd = strtok(input, " ");
    char* arg1 = strtok(NULL, " ");
    char* arg2 = strtok(NULL, " ");

    if (cmd == NULL) {
        printf("No command received.\n%s\n", END_FLAG);
        fflush(stdout);
        return;
    }
    if (strcmp(cmd, "list_hunts") == 0) {
        list_hunts();
    } else if (strcmp(cmd, "list_treasures") == 0 && arg1) {
        list_treasures(arg1);
    } else if (strcmp(cmd, "view_treasure") == 0 && arg1 && arg2) {
        view_treasure(arg1, arg2);
    } else if (strcmp(cmd, "stop_monitor") == 0) {
        printf("Monitor stopping...\n");
        fflush(stdout);
        usleep(2000000);
        printf("%s\n", END_FLAG);
        fflush(stdout);
        exit(0);
    } else {
        printf("Unknown command.\n");
    }

    printf("%s\n", END_FLAG);
    fflush(stdout);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <write_fd>\n", argv[0]);
        exit(1);
    }

    int write_fd = atoi(argv[1]);

    if (dup2(write_fd, STDOUT_FILENO) < 0) {
        fprintf(stderr, "Failed to redirect stdout.\n");
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    while (1) {
        pause();

        if (sig_received) {
            sig_received = 0;
            process_cmd();
        }
    }

    return 0;
}