#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#define MAX_INPUT    1024
#define MAX_ARGS       64
#define MAX_PIPES       5 
#define MAX_PROCS      64
#define TIME_QUANTUM   10

// Ejecución de comandos Bash con pipes

static int split_args(char *command, char **args) {
    int i = 0;
    char *tok = strtok(command, " \t\n");
    while (tok && i < MAX_ARGS - 1) {
        args[i++] = tok;
        tok = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
    return i;
}

static void exec_pipeline(char **cmds, int n) {
    int   pfd[MAX_PIPES - 1][2];
    pid_t pids[MAX_PIPES];

    for (int i = 0; i < n - 1; i++) {
        if (pipe(pfd[i]) == -1) { perror("pipe"); return; }
    }

    for (int i = 0; i < n; i++) {
        char  buf[MAX_INPUT];
        char *args[MAX_ARGS];
        strncpy(buf, cmds[i], MAX_INPUT - 1);
        buf[MAX_INPUT - 1] = '\0';
        split_args(buf, args);
        if (!args[0]) { pids[i] = -1; continue; }

        pids[i] = fork();
        if (pids[i] < 0) { perror("fork"); return; }

        if (pids[i] == 0) {
            if (i > 0)     dup2(pfd[i-1][0], STDIN_FILENO);
            if (i < n - 1) dup2(pfd[i][1],   STDOUT_FILENO);
            for (int j = 0; j < n - 1; j++) { close(pfd[j][0]); close(pfd[j][1]); }
            execvp(args[0], args);
            fprintf(stderr, "Comando no encontrado: %s\n", args[0]);
            exit(1);
        }
    }

    for (int i = 0; i < n - 1; i++) { close(pfd[i][0]); close(pfd[i][1]); }
    for (int i = 0; i < n; i++) if (pids[i] > 0) waitpid(pids[i], NULL, 0);
}

static void run_bash_cmd(char *input) {
    char *cmds[MAX_PIPES];
    int   n = 0;

    char *tok = strtok(input, "|");
    while (tok && n < MAX_PIPES) {
        while (*tok == ' ' || *tok == '\t') tok++;
        cmds[n++] = tok;
        tok = strtok(NULL, "|");
    }

    if (n == 1) {
        char *args[MAX_ARGS];
        split_args(cmds[0], args);
        if (!args[0]) return;
        pid_t pid = fork();
        if (pid == 0) {
            execvp(args[0], args);
            fprintf(stderr, "Comando no encontrado: %s\n", args[0]);
            exit(1);
        }
        wait(NULL);
    } else {
        exec_pipeline(cmds, n);
    }
}

// Calendarizacion de procesos

typedef enum { STATE_NEW, STATE_READY, STATE_TERMINATED } PState;

typedef struct {
    int    id;
    int    burst;
    int    size;
    PState state;
} Proc;

static Proc procs[MAX_PROCS];
static int  n_procs = 0;

static const char *state_str(PState s) {
    if (s == STATE_NEW)        return "new";
    if (s == STATE_READY)      return "ready";
    if (s == STATE_TERMINATED) return "terminated";
    return "?";
}

static int find_proc_idx(int id) {
    for (int i = 0; i < n_procs; i++)
        if (procs[i].id == id) return i;
    return -1;
}

static void cmd_mkprocess(int id, int burst, int size) {
    if (find_proc_idx(id) >= 0) {
        printf("Error: ya existe un proceso con identificador %d.\n", id);
        return;
    }
    if (burst <= 0) { printf("Error: burst time debe ser mayor a 0.\n"); return; }
    if (size  <= 0) { printf("Error: tamaño debe ser mayor a 0.\n");     return; }
    if (n_procs >= MAX_PROCS) { printf("Error: límite de procesos alcanzado.\n"); return; }

    procs[n_procs++] = (Proc){ id, burst, size, STATE_NEW };
    printf("Proceso %d creado (burst=%d, size=%d bloques, estado=new).\n",
           id, burst, size);
}

static void cmd_lstprocess(void) {
    if (n_procs == 0) { printf("(sin procesos)\n"); return; }
    printf("\n%-8s %-12s %-10s %-12s\n", "ID", "Burst Time", "Tamaño", "Estado");
    printf("------------------------------------------\n");
    for (int i = 0; i < n_procs; i++)
        printf("%-8d %-12d %-10d %-12s\n",
               procs[i].id, procs[i].burst, procs[i].size,
               state_str(procs[i].state));
    printf("\n");
}


typedef struct { int id; int burst; int orig_idx; } PInfo;

static int get_schedulable(PInfo **out) {
    int n = 0;
    for (int i = 0; i < n_procs; i++)
        if (procs[i].state == STATE_NEW || procs[i].state == STATE_READY) n++;
    if (n == 0) return 0;

    *out = malloc(n * sizeof(PInfo));
    int k = 0;
    for (int i = 0; i < n_procs; i++)
        if (procs[i].state == STATE_NEW || procs[i].state == STATE_READY)
            (*out)[k++] = (PInfo){ procs[i].id, procs[i].burst, i };
    return n;
}

static void mark_terminated(PInfo *arr, int n) {
    for (int i = 0; i < n; i++)
        procs[arr[i].orig_idx].state = STATE_TERMINATED;
}

static void print_sched_table(PInfo *arr, int *wt, int *tat, int n) {
    printf("\n%-8s %12s %16s\n", "ID", "Waiting Time", "Turnaround Time");
    double avg_wt = 0, avg_tat = 0;
    for (int i = 0; i < n; i++) {
        printf("%-8d %12d %16d\n", arr[i].id, wt[i], tat[i]);
        avg_wt  += wt[i];
        avg_tat += tat[i];
    }
    printf("%-8s %12.2f %16.2f\n", "PROMEDIO", avg_wt / n, avg_tat / n);
    printf("\n");
}

static void cmd_fcfs(void) {
    PInfo *arr = NULL;
    int n = get_schedulable(&arr);
    if (n == 0) { printf("No hay procesos listos para planificar.\n"); return; }

    printf("\n--- FCFS ---\n");
    int *wt  = calloc(n, sizeof(int));
    int *tat = calloc(n, sizeof(int));
    int t = 0;
    for (int i = 0; i < n; i++) {
        printf("t=%-4d  entra proceso %d\n", t, arr[i].id);
        wt[i] = t;
        t += arr[i].burst;
        tat[i] = t;
        printf("t=%-4d  sale proceso %d  (completado)\n", t, arr[i].id);
    }
    print_sched_table(arr, wt, tat, n);
    mark_terminated(arr, n);
    free(wt); free(tat); free(arr);
}

static void cmd_sjf(void) {
    PInfo *arr = NULL;
    int n = get_schedulable(&arr);
    if (n == 0) { printf("No hay procesos listos para planificar.\n"); return; }

    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - 1 - i; j++)
            if (arr[j].burst > arr[j+1].burst) {
                PInfo tmp = arr[j]; arr[j] = arr[j+1]; arr[j+1] = tmp;
            }

    printf("\n--- SJF ---\n");
    int *wt  = calloc(n, sizeof(int));
    int *tat = calloc(n, sizeof(int));
    int t = 0;
    for (int i = 0; i < n; i++) {
        printf("t=%-4d  entra proceso %d  (burst=%d)\n", t, arr[i].id, arr[i].burst);
        wt[i] = t;
        t += arr[i].burst;
        tat[i] = t;
        printf("t=%-4d  sale proceso %d  (completado)\n", t, arr[i].id);
    }
    print_sched_table(arr, wt, tat, n);
    mark_terminated(arr, n);
    free(wt); free(tat); free(arr);
}

static void cmd_rr(void) {
    PInfo *arr = NULL;
    int n = get_schedulable(&arr);
    if (n == 0) { printf("No hay procesos listos para planificar.\n"); return; }

    printf("\n--- Round-Robin (%dut) ---\n", TIME_QUANTUM);

    int *rem = malloc(n * sizeof(int));
    int *wt  = calloc(n, sizeof(int));
    int *tat = calloc(n, sizeof(int));
    for (int i = 0; i < n; i++) rem[i] = arr[i].burst;

    int t = 0, done = 0;
    while (done < n) {
        for (int i = 0; i < n; i++) {
            if (rem[i] <= 0) continue;
            int slice = (rem[i] < TIME_QUANTUM) ? rem[i] : TIME_QUANTUM;
            printf("Proceso %d entra, restante=%d\n", arr[i].id, rem[i]);
            rem[i] -= slice;
            t += slice;
            if (rem[i] == 0) {
                tat[i] = t;
                wt[i]  = tat[i] - arr[i].burst;
                done++;
                printf("Proceso %d sale, completado\n", arr[i].id);
            } else {
                printf("Proceso %d sale, restante=%d\n", arr[i].id, rem[i]);
            }
        }
    }
    print_sched_table(arr, wt, tat, n);
    mark_terminated(arr, n);
    free(rem); free(wt); free(tat); free(arr);
}

static void cmd_my_kill(int id) {
    int idx = find_proc_idx(id);
    if (idx < 0) { printf("Error: proceso %d no encontrado.\n", id); return; }
    for (int i = idx; i < n_procs - 1; i++) procs[i] = procs[i+1];
    n_procs--;
    printf("Proceso %d eliminado.\n", id);
}

int main(void) {
    char input[MAX_INPUT];

    while (1) {
        printf("shell> ");
        fflush(stdout);

        if (!fgets(input, MAX_INPUT, stdin)) break;
        input[strcspn(input, "\n")] = '\0';
        if (input[0] == '\0') continue;

        // exit
        if (strcmp(input, "exit") == 0) { printf("Bye\n"); break; }

        // mkprocess <id> <burst_time> <tamaño>
        if (strncmp(input, "mkprocess", 9) == 0 &&
            (input[9] == ' ' || input[9] == '\t' || input[9] == '\0')) {
            int id, burst, size;
            if (sscanf(input + 9, "%d %d %d", &id, &burst, &size) == 3)
                cmd_mkprocess(id, burst, size);
            else
                printf("Uso: mkprocess <id> <burst_time> <tamaño>\n");
            continue;
        }

        // lstprocess
        if (strcmp(input, "lstprocess") == 0) { cmd_lstprocess(); continue; }

        // Algoritmos de calendarizacion
        if (strcmp(input, "fcfs") == 0) { cmd_fcfs(); continue; }
        if (strcmp(input, "sjf")  == 0) { cmd_sjf();  continue; }
        if (strcmp(input, "rr")   == 0) { cmd_rr();   continue; }

        // my_kill <id>
        if (strncmp(input, "my_kill", 7) == 0 &&
            (input[7] == ' ' || input[7] == '\t' || input[7] == '\0')) {
            int id;
            if (sscanf(input + 7, "%d", &id) == 1)
                cmd_my_kill(id);
            else
                printf("Uso: my_kill <id>\n");
            continue;
        }

        // Comando bash con pipes
        {
            char buf[MAX_INPUT];
            strncpy(buf, input, MAX_INPUT - 1);
            buf[MAX_INPUT - 1] = '\0';
            run_bash_cmd(buf);
        }
    }

    return 0;
}
