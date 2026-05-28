#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME 64
#define TIME_QUANTUM 10

typedef struct Process {
    char name[MAX_NAME];
    int  burst;
    struct Process *next;
} Process;

Process *head = NULL;


void list_append(Process **h, const char *name, int burst) {
    Process *p = malloc(sizeof(Process));
    strncpy(p->name, name, MAX_NAME - 1);
    p->name[MAX_NAME - 1] = '\0';
    p->burst = burst;
    p->next  = NULL;

    if (!*h) {
        *h = p; return; 
    }

    Process *cur = *h;
    while (cur->next) cur = cur->next;
    cur->next = p;
}

int list_size(Process *h) {
    int n = 0;
    while (h) { 
        n++; 
        h = h->next; 
    }
    return n;
}

void list_free(Process **h) {
    while (*h) {
        Process *tmp = *h;
        *h = (*h)->next;
        free(tmp);
    }
}

// Copiar la lista a un arreglo para no modificar la original
typedef struct { char name[MAX_NAME]; int burst; } PInfo;

int list_to_array(Process *h, PInfo **arr) {
    int n = list_size(h);
    if (n == 0) return 0;
    *arr = malloc(n * sizeof(PInfo));

    for (int i = 0; i < n; i++, h = h->next) {
        strncpy((*arr)[i].name, h->name, MAX_NAME);
        (*arr)[i].burst = h->burst;
    }

    return n;
}

// Los resultados
void print_table(const char *names[], int wt[], int tat[], int n) {
    printf("\n%-16s %12s %16s\n", "Proceso", "Waiting Time", "Turnaround Time");
    double avg_wt = 0, avg_tat = 0;
    
    for (int i = 0; i < n; i++) {
        printf("%-16s %12d %16d\n", names[i], wt[i], tat[i]);
        avg_wt  += wt[i];
        avg_tat += tat[i];
    }

    printf("%-16s %12.2f %16.2f\n", "PROMEDIO", avg_wt / n, avg_tat / n);
    printf("\n");
}


void cmd_mkprocess(const char *name, int burst) {
    if (burst <= 0) { 
        printf("Error: el burst time debe ser mayor a 0.\n"); 
        return; 
    }
    list_append(&head, name, burst);
    printf("Proceso '%s' creado con burst time %d ut.\n", name, burst);
}

void cmd_lstprocss(void) {
    if (!head) { 
        printf("(lista vacía)\n");
        return; 
    }

    printf("\n  %-16s %s\n", "Proceso", "Burst Time");
    
    for (Process *p = head; p; p = p->next) {
        printf("  %-16s %d\n", p->name, p->burst);
    }
    printf("\n");
}

void cmd_rmprocess(const char *name) {
    Process *cur = head, *prev = NULL;
    while (cur) {
        if (strcmp(cur->name, name) == 0) {
            if (prev) {
                prev->next = cur->next;
            } else {
                head = cur->next;
            }

            free(cur);
            printf("Proceso '%s' eliminado.\n", name);
            return;
        }
        prev = cur; 
        cur = cur->next;
    }
    printf("Proceso '%s' no encontrado.\n", name);
}

void cmd_fcfs(void) {
    PInfo *arr = NULL;
    int n = list_to_array(head, &arr);
    if (n == 0) { 
        printf("Lista vacía.\n"); 
        return; 
    }

    printf("\n--- FCFS ---\n");
    int *wt = calloc(n, sizeof(int));
    int *tat = calloc(n, sizeof(int));
    const char **names = malloc(n * sizeof(char *));

    int time = 0;
    for (int i = 0; i < n; i++) {
        names[i] = arr[i].name;
        printf("t=%-4d  entra %s\n", time, arr[i].name);
        wt[i] = time;
        time += arr[i].burst;
        tat[i] = time;
        printf("t=%-4d  sale %s  (completado)\n", time, arr[i].name);
    }
    print_table(names, wt, tat, n);

    free(wt);
    free(tat); 
    free(names); 
    free(arr);
    list_free(&head);
    printf("Lista de procesos vaciada.\n\n");
}

void cmd_sjf(void) {
    PInfo *arr = NULL;
    int n = list_to_array(head, &arr);
    if (n == 0) { 
        printf("Lista vacía.\n"); 
        return; 
    }

    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            if (arr[j].burst > arr[j+1].burst) {
                PInfo tmp = arr[j]; 
                arr[j] = arr[j+1]; 
                arr[j+1] = tmp;
            }
        }
    }

    printf("\n--- SJF ---\n");
    int *wt = calloc(n, sizeof(int));
    int *tat = calloc(n, sizeof(int));
    const char **names = malloc(n * sizeof(char *));

    int time = 0;
    for (int i = 0; i < n; i++) {
        names[i] = arr[i].name;
        printf("t=%-4d  entra %s  (burst=%d)\n", time, arr[i].name, arr[i].burst);
        wt[i] = time;
        time += arr[i].burst;
        tat[i] = time;
        printf("t=%-4d  sale %s  (completado)\n", time, arr[i].name);
    }
    print_table(names, wt, tat, n);

    free(wt); 
    free(tat); 
    free(names); 
    free(arr);
    list_free(&head);
    printf("Lista de procesos vaciada.\n\n");
}

void cmd_rr(void) {
    PInfo *arr = NULL;
    int n = list_to_array(head, &arr);
    if (n == 0) { printf("Lista vacía.\n"); return; }

    printf("\n--- Round-Robin (%dut) ---\n", TIME_QUANTUM);

    int *rem = malloc(n * sizeof(int));
    int *tat = calloc(n, sizeof(int));
    int *wt = calloc(n, sizeof(int));
    const char **names = malloc(n * sizeof(char *));

    for (int i = 0; i < n; i++) {
        rem[i] = arr[i].burst;
        names[i] = arr[i].name;
    }

    int time = 0, done = 0;

    while (done < n) {
        for (int i = 0; i < n; i++) {
            if (rem[i] <= 0) continue;
            int slice = rem[i] < TIME_QUANTUM ? rem[i] : TIME_QUANTUM;
            printf("%s entra, restante = %d\n", names[i], rem[i]);
            rem[i] -= slice;
            time += slice;

            if (rem[i] == 0) {
                tat[i] = time;
                wt[i] = tat[i] - arr[i].burst;
                done++;
                printf("%s sale, completado\n", names[i]);
            } else {
                printf("%s sale, restante = %d\n", names[i], rem[i]);
            }
        }
    }

    print_table(names, wt, tat, n);

    free(rem); free(wt); free(tat); free(names); free(arr);
    list_free(&head);
    printf("Lista de procesos vaciada.\n\n");
}

int main(void) {
    char line[256];
    while (1) {
        printf("terminal> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;

        line[strcspn(line, "\n")] = '\0';
        char *cmd = strtok(line, " \t");
        if (!cmd) continue;

        if (strcmp(cmd, "exit") == 0) {
            printf("Bye\n");
            break;
        } else if (strcmp(cmd, "lstprocss") == 0) {
            cmd_lstprocss();
        } else if (strcmp(cmd, "fcfs") == 0) {
            cmd_fcfs();
        } else if (strcmp(cmd, "sjf") == 0) {
            cmd_sjf();
        } else if (strcmp(cmd, "rr") == 0) {
            cmd_rr();
        } else if (strcmp(cmd, "mkprocess") == 0) {
            char *name  = strtok(NULL, " \t");
            char *burst = strtok(NULL, " \t");
            cmd_mkprocess(name, atoi(burst));
        } else if (strcmp(cmd, "rmprocess") == 0) {
            char *name = strtok(NULL, " \t");
            cmd_rmprocess(name);
        } else {
            printf("Comando desconocido: '%s'\n", cmd);
        }
    }

    list_free(&head);
    return 0;
}