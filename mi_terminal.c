#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define BUF_SIZE 1024
#define INPUT_SIZE 256

void mycat(char *filename) {
    int fd = open(filename, O_RDONLY);;
    char buffer[BUF_SIZE];
    ssize_t n_bytes;

    if (fd == -1) {
        write(1, "No se encuentra un archivo con ese nombre\n", 43);
        return;
    }

    while ((n_bytes = read(fd, buffer, BUF_SIZE)) > 0) {
        for (ssize_t i = 0; i < n_bytes; i++) {
            if (buffer[i] == '$') {
                close(fd);
                return;
            }
            write(1, &buffer[i], 1);
        }
    }

    close(fd);
}

void mycat_write(char *filename) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (filename == NULL) {
        write(1, "Debes especificar un archivo\n", 29);
        return;
    }

    if (fd == -1) {
        write(1, "Error al abrir el archivo\n", 26);
        return;
    }

    char c;
    while (read(0, &c, 1) > 0) {  
        if (c == '$') {
            while (read(0, &c, 1) > 0 && c != '\n');
            write(fd, "\n", 1);
            break;
        }
        write(fd, &c, 1);
    }


    close(fd);
    write(1, "Escritura completada\n", 22);
}

void mycp(char *source_filename, char *destination_filename) {
    int fd = open(source_filename, O_RDONLY);    
    char buffer[BUF_SIZE];
    ssize_t n_bytes;

    if (fd == -1) {
        write(1, "Error al abrir el archivo origen\n", 33);
        return;
    }

    int fd2 = open(destination_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    while ((n_bytes = read(fd, buffer, BUF_SIZE)) > 0) {
        for (ssize_t i = 0; i < n_bytes; i++) {
            if (buffer[i] == '$') {
                close(fd);
                return;
            }
            write(fd2, &buffer[i], 1);
        }
    }

    close(fd);
    close(fd2);
    write(1, "Escritura completada\n", 22);
}

void myrm(char *filename) {
    int fd = open(filename, O_RDONLY);;

    if (fd == -1) {
        write(1, "No se encuentra un archivo con ese nombre\n", 43);
        return;
    }

    if (unlink(filename) == -1) {
        write(1, "Error al eliminar el archivo\n", 29);
    } else {
        write(1, "Archivo eliminado\n", 18);
    }
}

int main() {
    char input[INPUT_SIZE];

    while (1) {
        write(1, "mi_terminal> ", 13);

        ssize_t n = read(0, input, INPUT_SIZE - 1);
        if (n <= 0) break;

        if (input[n-1] == '\n') input[n-1] = '\0';

        if (strcmp(input, "exit") == 0) break;

        if (strncmp(input, "mycat ", 6) == 0) {
            char *filename = NULL;
            filename = input + 6;
            mycat(filename);

        } else if (strncmp(input, "mycat>", 6) == 0) {
            char *filename = input + 7;
            mycat_write(filename);

        } else if (strncmp(input, "mycp", 4) == 0) {
            char source_filename[100];
            char destination_filename[100];

            if (sscanf(input, "mycp %s %s", source_filename, destination_filename) == 2) {
                if (strcmp(source_filename, destination_filename) == 0) {
                    write(1, "Error: origen y destino son el mismo archivo\n", 45);
                } else {
                    mycp(source_filename, destination_filename);
                }
            } else {
                write(1, "Debes especificar origen y destino\n", 36);
            }
            
        } else if (strncmp(input, "myrm", 4) == 0) {
            char *filename = NULL;
            filename = input + 5;

            myrm(filename);
        
        
        } else {
            write(1, "Comando no reconocido\n", 23);
        }
    }

    return 0;
}