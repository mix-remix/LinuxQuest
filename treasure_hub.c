#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>

#define DATA_FILE "data.bin"
#define CMD_FILE "commands.txt"

#define BUFF_SIZE 512
#define PATH_SIZE 256
#define CMD_SIZE 128
#define INPUT_SIZE 256

#define MONITOR_EXEC "./monitor"
#define SCORE_EXEC "./score_calculator"
#define MONITOR_NAME "monitor"
#define SCORE_NAME "score_calculator"

#define END_FLAG "END"

int monitor_running = 0;
int monitor_fd[2];
pid_t monitor_pid = -1;

void start_monitor() {
    if (monitor_running) {
        printf("Monitor already running.\n");
        return;
    }

    if (pipe(monitor_fd) < 0) {
        fprintf(stderr, "Pipe creation failed: %s\n", strerror(errno));
        return;
    }

    monitor_pid = fork();

    if (monitor_pid < 0) {
        fprintf(stderr, "Fork failed: %s\n", strerror(errno));
        return;
    }
    if (monitor_pid == 0) {
        close(monitor_fd[0]);

        char fd_buff[BUFF_SIZE];
        snprintf(fd_buff, sizeof(fd_buff), "%d", monitor_fd[1]);

        execl(MONITOR_EXEC, MONITOR_NAME, fd_buff, NULL);

        fprintf(stderr, "Monitor exec failed: %s\n", strerror(errno));
        exit(1);
    }

    close(monitor_fd[1]);
    monitor_running = 1;

    printf("Monitor started.\n");
}

void send_cmd(char* cmd) {
    if (!monitor_running) {
        printf("Monitor is not running.\n");
        return;
    }

    int cmd_fd = open(CMD_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (cmd_fd < 0) {
        fprintf(stderr, "Failed to write to %s: %s\n", CMD_FILE, strerror(errno));
        return;
    }

    write(cmd_fd, cmd, strlen(cmd));
    close(cmd_fd);

    kill(monitor_pid, SIGUSR1);

    char buff[BUFF_SIZE];

    while (1) {
        int n = read(monitor_fd[0], buff, sizeof(buff) - 1);

        if (n <= 0) {
            if (n < 0) {
                fprintf(stderr, "Read error: %s\n", strerror(errno));
            }
            break;
        }

        buff[n] = '\0';

        char* end_ptr = strstr(buff, END_FLAG);
        if (end_ptr != NULL) {
            *end_ptr = '\0';
            printf("%s", buff);
            break;
        }

        printf("%s", buff);
    }
}

void stop_monitor() {
    if (!monitor_running) {
        printf("Monitor is not running. Use start_monitor first.\n");
        return;
    }

    send_cmd("stop_monitor");
    waitpid(monitor_pid, NULL, 0);

    monitor_running = 0;

    printf("Monitor stopped.\n");
}

void sigchld_handler(int sig_code) {
    int status;
    pid_t pid = wait(&status);

    if (pid == monitor_pid) {
        monitor_running = 0;
        monitor_pid = -1;
    }
}

void calculate_scores() {
    DIR* dir = opendir(".");

    if (!dir) {
        fprintf(stderr, "Failed to open current directory: %s\n", strerror(errno));
        return;
    }

    struct dirent* entry;

    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_DIR && (strcmp(entry->d_name, ".")) != 0 && (strcmp(entry->d_name, "..")) != 0) {

            char file_path[PATH_SIZE];
            snprintf(file_path, sizeof(file_path), "%s/%s", entry->d_name, DATA_FILE);

            if (access(file_path, F_OK) != 0) {
                continue;
            }

            int score_fd[2];

            if (pipe(score_fd) < 0) {
                fprintf(stderr, "Pipe creation failed for score_calculator: %s\n", strerror(errno));
                continue;
            }

            pid_t score_pid = fork();

            if (score_pid < 0) {
                fprintf(stderr, "Fork failed for score_calculator: %s\n", strerror(errno));
                close(score_fd[0]);
                close(score_fd[1]);
                continue;
            }
            if (score_pid == 0) {
                close(score_fd[0]);
                dup2(score_fd[1], STDOUT_FILENO);
                close(score_fd[1]);

                execl(SCORE_EXEC, SCORE_NAME, entry->d_name, NULL);

                fprintf(stderr, "Score calculator exec failed: %s\n", strerror(errno));
                exit(1);
            }

            close(score_fd[1]);

            char buff[BUFF_SIZE];
            int n;

            printf("Scores for %s: ", entry->d_name);

            while ((n = read(score_fd[0], buff, sizeof(buff) - 1)) > 0) {
                buff[n] = '\0';
                printf("%s", buff);
            }

            close(score_fd[0]);
            waitpid(score_pid, NULL, 0);

            printf("\n");
        }
    }

    closedir(dir);
}

void print_usage() {
    printf("Usage:\n");
    printf("   >> start_monitor\n");
    printf("   >> list_hunts\n");
    printf("   >> list_treasures <hunt_id>\n");
    printf("   >> view_treasure <hunt_id> <treasure_id>\n");
    printf("   >> calculate_score\n");
    printf("   >> stop_monitor\n");
    printf("   >> exit\n");
}

void process_cmd() {
    char input[INPUT_SIZE];

    if (fgets(input, sizeof(input), stdin) == 0) {
        printf("Error reading input.\n");
        return;
    }

    input[strcspn(input, "\n")] = 0;

    char* cmd = strtok(input, " ");
    char* arg1 = strtok(NULL, " ");
    char* arg2 = strtok(NULL, " ");

    if (cmd == NULL) {
        printf("You must enter a command.\n");
        return;
    }
    if (strcmp(cmd, "start_monitor") == 0) {
        start_monitor();
    } else if (strcmp(cmd, "stop_monitor") == 0) {
        stop_monitor();
    } else if (strcmp(cmd, "list_hunts") == 0) {
        send_cmd("list_hunts");
    } else if (strcmp(cmd, "list_treasures") == 0 && arg1) {
        char cmd[CMD_SIZE];
        snprintf(cmd, sizeof(cmd), "list_treasures %s", arg1);
        send_cmd(cmd);
    } else if (strcmp(cmd, "view_treasure") == 0 && arg1 && arg2) {
        char cmd[CMD_SIZE];
        snprintf(cmd, sizeof(cmd), "view_treasure %s %s", arg1, arg2);
        send_cmd(cmd);
    } else if (strcmp(cmd, "calculate_score") == 0) {
        calculate_scores();
    } else if (strcmp(cmd, "exit") == 0) {
        if (monitor_running) {
            printf("Monitor is still running. Use stop_monitor first.\n");
        } else {
            exit(0);
        }
    } else {
        print_usage();
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGCHLD, &sa, NULL);

    while (1) {
        printf(">> ");
        fflush(stdout);
        process_cmd();
    }

    return 0;
}
