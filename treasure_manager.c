#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> 
#include <unistd.h> 
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#define DATA_FILE "data.bin"
#define LOG_FILE "logs.txt"

#define PATH_SIZE 256
#define CMD_SIZE 128

#define ID_SIZE 16
#define NAME_SIZE 32
#define CLUE_SIZE 128

typedef struct {
    char id[ID_SIZE];
    char username[NAME_SIZE];
    float latitude;
    float longitude;
    char clue[CLUE_SIZE];
    int value;
} Treasure;

void log_cmd(char* hunt_id, char* cmd) {
    char file_path[PATH_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, LOG_FILE);

    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (fd < 0) {
        fprintf(stderr, "Error opening log file '%s': %s\n", file_path, strerror(errno));
        return;
    }

    dprintf(fd, "%s\n", cmd);
    close(fd);

    char symlink_name[PATH_SIZE];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);

    symlink(file_path, symlink_name);
}

void add_treasure(char* hunt_id) {
    mkdir(hunt_id, 0755);

    char file_path[PATH_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, DATA_FILE);

    Treasure t;

    printf("Treasure ID: ");
    fgets(t.id, ID_SIZE, stdin);
    t.id[strcspn(t.id, "\n")] = '\0';

    int fd_check = open(file_path, O_RDONLY);

    if (fd_check >= 0) {
        Treasure existing;

        while (read(fd_check, &existing, sizeof(Treasure)) == sizeof(Treasure)) {
            if (strcmp(existing.id, t.id) == 0) {
                printf("Treasure already exists.\n");
                close(fd_check);
                return;
            }
        }

        close(fd_check);
    }

    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (fd < 0) {
        fprintf(stderr, "Error opening treasure file '%s': %s\n", file_path, strerror(errno));
        return;
    }

    printf("Username: ");
    fgets(t.username, NAME_SIZE, stdin);
    t.username[strcspn(t.username, "\n")] = '\0';

    printf("Latitude: ");
    scanf("%f", &t.latitude);
    getchar();

    printf("Longitude: ");
    scanf("%f", &t.longitude);
    getchar();

    printf("Clue: ");
    fgets(t.clue, CLUE_SIZE, stdin);
    t.clue[strcspn(t.clue, "\n")] = '\0';

    printf("Value: ");
    scanf("%d", &t.value);
    getchar();

    if (write(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("Treasure added successfully.\n");

        char cmd[CMD_SIZE];
        snprintf(cmd, sizeof(cmd), "Added treasure %s.", t.id);
        log_cmd(hunt_id, cmd);
    } else {
        fprintf(stderr, "Error writing treasure to file '%s': %s\n", file_path, strerror(errno));
    }

    close(fd);
}

void view_treasure(char* hunt_id, char* target_id) {
    char file_path[PATH_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, DATA_FILE);

    int fd = open(file_path, O_RDONLY);

    if (fd < 0) {
        fprintf(stderr, "Error opening treasure file '%s': %s\n", file_path, strerror(errno));
        return;
    }

    Treasure t;
    int found = 0;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.id, target_id) == 0) {
            printf("ID: %s | User: %s | Lat: %.2f | Long: %.2f | Clue: %s | Value: %d\n",
                   t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
            found = 1;
            break;
        }
    }

    close(fd);

    if (!found) {
        printf("Treasure not found.\n");
    } else {
        char cmd[CMD_SIZE];
        snprintf(cmd, sizeof(cmd), "Viewed treasure %s.", target_id);
        log_cmd(hunt_id, cmd);
    }
}

void remove_treasure(char* hunt_id, char* target_id) {
    char file_path[PATH_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, DATA_FILE);

    int fd = open(file_path, O_RDONLY);

    if (fd < 0) {
        fprintf(stderr, "Error opening treasure file '%s': %s\n", file_path, strerror(errno));
        return;
    }

    char temp_path[PATH_SIZE];
    snprintf(temp_path, sizeof(temp_path), "%s/temp.bin", hunt_id);

    int temp_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (temp_fd < 0) {
        fprintf(stderr, "Error creating temp file '%s': %s\n", temp_path, strerror(errno));
        close(fd);
        return;
    }

    Treasure t;
    int removed = 0;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.id, target_id) != 0) {
            write(temp_fd, &t, sizeof(Treasure));
        } else {
            removed = 1;
        }
    }

    close(fd);
    close(temp_fd);

    if (removed) {
        rename(temp_path, file_path);

        char cmd[PATH_SIZE];
        snprintf(cmd, sizeof(cmd), "Removed treasure %s.", target_id);
        log_cmd(hunt_id, cmd);

        printf("Treasure removed successfully.\n");
    } else {
        unlink(temp_path);
        printf("Treasure not found.\n");
    }
}

void list_treasures(char* hunt_id) {
    char file_path[PATH_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, DATA_FILE);

    struct stat st;

    if (stat(file_path, &st) < 0) {
        fprintf(stderr, "Error retrieving file stats for '%s': %s\n", file_path, strerror(errno));
        return;
    }

    printf("ID: %s\nSize: %ld bytes\nLast modified: %s\n", hunt_id, st.st_size, ctime(&st.st_mtime));

    int fd = open(file_path, O_RDONLY);

    if (fd < 0) {
        fprintf(stderr, "Error opening treasure file '%s': %s\n", file_path, strerror(errno));
        return;
    }

    Treasure t;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %s | User: %s | Lat: %.2f | Long: %.2f | Value: %d\n",
               t.id, t.username, t.latitude, t.longitude, t.value);
    }

    close(fd);

    char cmd[CMD_SIZE];
    snprintf(cmd, sizeof(cmd), "Listed treasures from hunt %s.", hunt_id);
    log_cmd(hunt_id, cmd);
}

void remove_hunt(char* hunt_id) {
    char file_path[PATH_SIZE];

    struct stat st;
    if (stat(hunt_id, &st) < 0) {
        printf("Hunt not found.\n");
        return;
    }

    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, DATA_FILE);
    unlink(file_path);

    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, LOG_FILE);
    unlink(file_path);

    rmdir(hunt_id);

    char symlink_name[PATH_SIZE];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);
    unlink(symlink_name);

    printf("Hunt removed succesfully.\n");
}

void print_usage(char* arg) {
    printf("Usage:\n");
    printf("   %s --add <hunt_id>\n", arg);
    printf("   %s --list <hunt_id>\n", arg);
    printf("   %s --view <hunt_id> <treasure_id>\n", arg);
    printf("   %s --remove_treasure <hunt_id> <treasure_id>\n", arg);
    printf("   %s --remove_hunt <hunt_id>\n", arg);
}

void process_cmd(char* program, char* arg1, char* arg2, char* arg3) {
    if (!arg1) {
        print_usage(program);
        exit(1);
    }

    if (strcmp(arg1, "--add") == 0 && arg2) {
        add_treasure(arg2);
    } else if (strcmp(arg1, "--list") == 0 && arg2) {
        list_treasures(arg2);
    } else if (strcmp(arg1, "--view") == 0 && arg2 && arg3) {
        view_treasure(arg2, arg3);
    } else if (strcmp(arg1, "--remove_treasure") == 0 && arg2 && arg3) {
        remove_treasure(arg2, arg3);
    } else if (strcmp(arg1, "--remove_hunt") == 0 && arg2) {
        remove_hunt(arg2);
    } else {
        print_usage(program);
        exit(1);
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        print_usage(argv[0]);
        exit(1);
    }

    char* arg1 = argv[1];
    char* arg2 = argv[2];
    char* arg3 = NULL;

    if (argc >= 4) {
        arg3 = argv[3];
    }

    process_cmd(argv[0], arg1, arg2, arg3);

    return 0;
}
