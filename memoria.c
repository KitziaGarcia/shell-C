#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* Divide un bloque: la primera parte queda para el proceso, el resto queda libre */
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
           "Bloque", "Dir.Base", "Tamaño", "Límite", "Estado", "Proceso");
    printf("-------------------------------------------------------------\n");
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
    printf("-------------------------------------------------------------\n\n");
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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <tamaño_memoria>\n", argv[0]);
        return 1;
    }
    int total = atoi(argv[1]);
    if (total <= 0) {
        fprintf(stderr, "Error: el tamaño debe ser un entero positivo\n");
        return 1;
    }

    init_memory(total);

    printf("=== Simulador de Asignación de Memoria Contigua ===\n");
    printf("Memoria total: %d unidades\n\n", total);
    printf("Comandos disponibles:\n");
    printf("  solicitar <pid> <tamaño> <estrategia>   ff=First Fit  bf=Best Fit  wf=Worst Fit\n");
    printf("  liberar <pid>\n");
    printf("  compactar\n");
    printf("  reporte\n");
    printf("  salir\n\n");

    char line[256];
    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) break;
        line[strcspn(line, "\n")] = '\0';

        if (line[0] == '\0') continue;

        if (strncmp(line, "solicitar", 9) == 0) {
            int pid, size;
            char strat[16];
            if (sscanf(line + 9, "%d %d %15s", &pid, &size, strat) != 3) {
                printf("Error: uso: solicitar <pid> <tamaño> <estrategia>\n");
                continue;
            }
            if (size <= 0) {
                printf("Error: el tamaño debe ser positivo\n");
                continue;
            }
            /* Verificar que el PID no esté ya en memoria */
            int exists = 0;
            for (MemoryBlock *b = memory_head; b != NULL; b = b->next)
                if (b->status == OCCUPIED && b->process != NULL && b->process->id == pid)
                    { exists = 1; break; }
            if (exists) {
                printf("Error: el proceso %d ya está en memoria\n", pid);
                continue;
            }

            Process *proc = malloc(sizeof(Process));
            proc->id = pid;
            proc->size = size;

            int ok = 0;
            if      (strcmp(strat, "ff") == 0) ok = first_fit(proc);
            else if (strcmp(strat, "bf") == 0) ok = best_fit(proc);
            else if (strcmp(strat, "wf") == 0) ok = worst_fit(proc);
            else {
                printf("Error: estrategia desconocida '%s'. Use ff, bf o wf\n", strat);
                free(proc);
                continue;
            }

            if (ok)
                printf("Proceso %d asignado (%d unidades, estrategia=%s)\n", pid, size, strat);
            else {
                printf("Error: no hay espacio suficiente para el proceso %d (%d unidades)\n", pid, size);
                free(proc);
            }

        } else if (strncmp(line, "liberar", 7) == 0) {
            int pid;
            if (sscanf(line + 7, "%d", &pid) != 1) {
                printf("Error: uso: liberar <pid>\n");
                continue;
            }
            if (free_process(pid))
                printf("Proceso %d liberado\n", pid);
            else
                printf("Error: proceso %d no encontrado en memoria\n", pid);

        } else if (strcmp(line, "compactar") == 0) {
            compact();
            printf("Memoria compactada\n");

        } else if (strcmp(line, "reporte") == 0) {
            report();

        } else if (strcmp(line, "salir") == 0) {
            break;

        } else {
            printf("Comando desconocido: '%s'\n", line);
        }
    }

    cleanup();
    printf("Simulador terminado.\n");
    return 0;
}
