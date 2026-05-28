#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64

int split(char *command, char **args) {
    int i = 0;
    char *token = strtok(command, " \t\n");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
    return i;
}

void exec_simple_cmd(char *command) {
    char *args[MAX_ARGS];
    split(command, args);
    if (args[0] == NULL) return;

    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        fprintf(stderr, "Comando no encontrado: %s\n", args[0]);
        exit(1);
    }
    wait(NULL);
}

void exec_pipe(char *cmd1, char *cmd2) {
    char *args1[MAX_ARGS];
    char *args2[MAX_ARGS];
    split(cmd1, args1);
    split(cmd2, args2);

    int pfd[2];
    if (pipe(pfd) == -1) {
        perror("pipe");
        return;
    }

    // va a ejevcutar y escribir
    pid_t pidA = fork();
    if (pidA == 0) {
        close(pfd[0]);
        dup2(pfd[1], STDOUT_FILENO);
        close(pfd[1]); 
        execvp(args1[0], args1);
        fprintf(stderr, "Comando no encontrado: %s\n", args1[0]);
        exit(1);
    }

    // vaa a leer y ejecutar
    pid_t pidB = fork();
    if (pidB == 0) {
       close(pfd[1]);   
        dup2(pfd[0], STDIN_FILENO);
        close(pfd[0]);    
        execvp(args2[0], args2);
        fprintf(stderr, "Comando no encontrado: %s\n", args2[0]);
        exit(1);
    }

    close(pfd[0]);
    close(pfd[1]);
    wait(NULL);
    wait(NULL);
}

int main() {
    char input[MAX_INPUT];

    while (1) {
        printf("mi_terminal> ");
        fflush(stdout); 

        if (fgets(input, MAX_INPUT, stdin) == NULL) break;

        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0) continue; 

        if (strcmp(input, "exit") == 0) {  
            break;
        }

        char *pipe_pos = strchr(input, '|');

        if (pipe_pos != NULL) {  
            *pipe_pos = '\0';         
            char *cmd1 = input;       // apunta al primero
            char *cmd2 = pipe_pos + 1; // apunta al que va despues del |
            exec_pipe(cmd1, cmd2);
        } else {
            exec_simple_cmd(input); 
        }
    }

    return 0;
}