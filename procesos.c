#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pidA, pidB, pidC, pidD, pidE, pidF, pidG;

    printf("Soy nodo P, mi PID es: %d, soy el el padre de todos los siguientes procesos, los cuales realizan distintas tareas\n", getpid());
    fflush(stdout);

    pidA = fork();

    if (pidA == 0) {
        printf("\nSoy nodo A, mi PID es: %d, mi padre: %d\n", getpid(), getppid());
        fflush(stdout);

        pidB = fork();
        if (pidB == 0) {
            printf("\nSoy nodo B, mi PID es: %d, mi padre: %d\n", getpid(), getppid());
            printf("Nodo B - Directorio actual:");
            fflush(stdout);
            system("pwd");
            exit(0);
        }
        wait(NULL);

        pidC = fork();
        if (pidC == 0) {
            printf("\nSoy nodo C, mi PID es: %d, mi padre: %d\n", getpid(), getppid());
            printf("Nodo C - Contenido del directorio actual:\n");
            fflush(stdout);
            system("ls .");

            pidD = fork();
            if (pidD == 0) {
                printf("\nSoy nodo D, mi PID es: %d, mi padre: %d\n", getpid(), getppid());
                printf("Nodo D - Lista de procesos actuales:\n");
                fflush(stdout);
                system("ps");

                pidE = fork();
                if (pidE == 0) {
                    printf("\nSoy nodo E, mi PID es: %d, mi padre: %d\n", getpid(), getppid());
                    printf("Nodo E - Fecha del sistema: ");
                    fflush(stdout);
                    system("date");
                    exit(0);
                }
                wait(NULL);

                pidF = fork();
                if (pidF == 0) {
                    printf("\nSoy nodo F, mi PID es: %d, mi padre: %d\n", getpid(), getppid());
                    printf("Nodo F - Lista de procesos actuales:\n");
                    fflush(stdout);
                    system("ps");

                    pidG = fork();
                    if (pidG == 0) {
                        printf("\nSoy nodo G, mi PID es: %d, mi padre: %d\n", getpid(), getppid());
                        printf("Nodo G - Archivo creado: 'estructura_procesos'\n");
                        fflush(stdout);
                        system("touch estructura_procesos");
                        exit(0);
                    }
                    wait(NULL);
                    exit(0);
                }
                wait(NULL); 
                exit(0);
            }
            wait(NULL);
            exit(0);
        }
        wait(NULL);
        //system("clear");
        printf("\nNodo A - Clear\n");
        exit(0);
    }

    wait(NULL);
    printf("\nTodos los procesos han terminado\n");

    return 0;
}