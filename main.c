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
#define MEMORY_SIZE   100

// Memoria
typedef struct Process {
    int id;
    int size;
} Process;

typedef enum { FREE, OCCUPIED } BlockStatus;

typedef struct MemoryBlock {
    int base_address;
    Process *process;
    int size;
    BlockStatus status;
    struct MemoryBlock *next;
} MemoryBlock;

static MemoryBlock *memory_head = NULL;

static void init_memory(int total_size) {
    memory_head = malloc(sizeof(MemoryBlock));
    memory_head->base_address = 0;
    memory_head->process = NULL;
    memory_head->size = total_size;
    memory_head->status = FREE;
    memory_head->next = NULL;
}

static void split_and_assign(MemoryBlock *block, Process *proc) {
    if (block->size > proc->size) {
        MemoryBlock *remainder = malloc(sizeof(MemoryBlock));
        remainder->base_address = block->base_address + proc->size;
        remainder->process = NULL;
        remainder->size = block->size - proc->size;
        remainder->status = FREE;
        remainder->next = block->next;
        block->next = remainder;
        block->size = proc->size;
    }
    block->process = proc;
    block->status = OCCUPIED;
}

static int first_fit(Process *proc) {
    for (MemoryBlock *b = memory_head; b != NULL; b = b->next) {
        if (b->status == FREE && b->size >= proc->size) {
            split_and_assign(b, proc);
            return 1;
        }
    }
    return 0;
}

static int best_fit(Process *proc) {
    MemoryBlock *best = NULL;
    for (MemoryBlock *b = memory_head; b != NULL; b = b->next) {
        if (b->status == FREE && b->size >= proc->size)
            if (best == NULL || b->size < best->size)
                best = b;
    }
    if (best == NULL) return 0;
    split_and_assign(best, proc);
    return 1;
}

static int worst_fit(Process *proc) {
    MemoryBlock *worst = NULL;
    for (MemoryBlock *b = memory_head; b != NULL; b = b->next) {
        if (b->status == FREE && b->size >= proc->size)
            if (worst == NULL || b->size > worst->size)
                worst = b;
    }
    if (worst == NULL) return 0;
    split_and_assign(worst, proc);
    return 1;
}

static int free_process(int pid) {
    for (MemoryBlock *b = memory_head; b != NULL; b = b->next) {
        if (b->status == OCCUPIED && b->process != NULL && b->process->id == pid) {
            free(b->process);
            b->process = NULL;
            b->status = FREE;
            return 1;
        }
    }
    return 0;
}

static void compact(void) {
    MemoryBlock *b = memory_head;
    while (b != NULL && b->next != NULL) {
        if (b->status == FREE && b->next->status == FREE) {
            MemoryBlock *del = b->next;
            b->size += del->size;
            b->next = del->next;
            free(del);
        } else {
            b = b->next;
        }
    }
}

static void report(void) {
    printf("\n%-8s %-14s %-10s %-10s %-10s %s\n",
           "Bloque", "Dir.Base", "Tamaño", "Límite", "Estado", "Proceso\n");
    int n = 0;
    for (MemoryBlock *b = memory_head; b != NULL; b = b->next, n++) {
        printf("%-8d %-14d %-10d %-10d %-10s",
               n,
               b->base_address,
               b->size,
               b->base_address + b->size - 1,
               b->status == FREE ? "LIBRE" : "OCUPADO");
        if (b->status == OCCUPIED && b->process != NULL)
            printf(" %d", b->process->id);
        printf("\n");
    }
    printf("\n");
}

static void cleanup(void) {
    MemoryBlock *b = memory_head;
    while (b != NULL) {
        MemoryBlock *next = b->next;
        if (b->process != NULL) free(b->process);
        free(b);
        b = next;
    }
}

// comandos Bash con pipes

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

// Calendarización de procesos

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
    printf("\n");
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
        if (procs[i].state == STATE_READY) n++;
    if (n == 0) return 0;

    *out = malloc(n * sizeof(PInfo));
    int k = 0;
    for (int i = 0; i < n_procs; i++)
        if (procs[i].state == STATE_READY)
            (*out)[k++] = (PInfo){ procs[i].id, procs[i].burst, i };
    return n;
}

static void mark_terminated(PInfo *arr, int n) {
    for (int i = 0; i < n; i++) {
        free_process(arr[i].id);
        procs[arr[i].orig_idx].state = STATE_TERMINATED;
    }
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
    if (n == 0) { printf("No hay procesos en estado ready para planificar.\n"); return; }

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
    if (n == 0) { printf("No hay procesos en estado ready para planificar.\n"); return; }

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
    if (n == 0) { printf("No hay procesos en estado ready para planificar.\n"); return; }

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

static void cmd_alloc(int id, const char *strategy) {
    int idx = find_proc_idx(id);
    if (idx < 0) {
        printf("Error: proceso %d no encontrado.\n", id);
        return;
    }
    if (procs[idx].state == STATE_READY) {
        printf("Error: proceso %d ya está en memoria.\n", id);
        return;
    }
    if (procs[idx].state == STATE_TERMINATED) {
        printf("Error: proceso %d está terminado.\n", id);
        return;
    }

    Process *proc = malloc(sizeof(Process));
    proc->id   = id;
    proc->size = procs[idx].size;

    int ok = 0;
    if      (strcmp(strategy, "first") == 0) ok = first_fit(proc);
    else if (strcmp(strategy, "best")  == 0) ok = best_fit(proc);
    else if (strcmp(strategy, "worst") == 0) ok = worst_fit(proc);
    else {
        printf("Error: estrategia desconocida '%s'. Use first, best o worst.\n", strategy);
        free(proc);
        return;
    }

    if (ok) {
        procs[idx].state = STATE_READY;
        printf("Proceso %d cargado en memoria (tamaño=%d bloques, estrategia=%s, estado=ready).\n",
               id, procs[idx].size, strategy);
    } else {
        printf("Error: no hay espacio suficiente para el proceso %d (%d bloques).\n",
               id, procs[idx].size);
        free(proc);
    }
}

static void cmd_free_mem(int id) {
    int idx = find_proc_idx(id);
    if (idx < 0) {
        printf("Error: proceso %d no encontrado.\n", id);
        return;
    }
    if (procs[idx].state != STATE_READY) {
        printf("Error: proceso %d no está en memoria.\n", id);
        return;
    }
    if (free_process(id)) {
        procs[idx].state = STATE_NEW;
        printf("Proceso %d liberado de memoria (estado=new).\n", id);
    } else {
        printf("Error: no se pudo liberar el proceso %d de memoria.\n", id);
    }
}

static void cmd_my_kill(int id) {
    int idx = find_proc_idx(id);
    if (idx < 0) { printf("Error: proceso %d no encontrado.\n", id); return; }
    if (procs[idx].state == STATE_READY)
        free_process(id);
    for (int i = idx; i < n_procs - 1; i++) procs[i] = procs[i+1];
    n_procs--;
    printf("Proceso %d eliminado.\n", id);
}

int main(void) {
    init_memory(MEMORY_SIZE);

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

        // alloc <id> <estrategia>
        if (strncmp(input, "alloc", 5) == 0 &&
            (input[5] == ' ' || input[5] == '\t' || input[5] == '\0')) {
            int id;
            char strat[16];
            if (sscanf(input + 5, "%d %15s", &id, strat) == 2)
                cmd_alloc(id, strat);
            else
                printf("Uso: alloc <id> <estrategia>  (first|best|worst)\n");
            continue;
        }

        // free <id>
        if (strncmp(input, "free", 4) == 0 &&
            (input[4] == ' ' || input[4] == '\t' || input[4] == '\0')) {
            int id;
            if (sscanf(input + 4, "%d", &id) == 1)
                cmd_free_mem(id);
            else
                printf("Uso: free <id>\n");
            continue;
        }

        // mstatus
        if (strcmp(input, "mstatus") == 0) { report(); continue; }

        // compact
        if (strcmp(input, "compact") == 0) {
            compact();
            printf("Memoria compactada.\n");
            continue;
        }

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

    cleanup();
    return 0;
}
