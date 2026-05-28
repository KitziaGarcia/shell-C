# Shell en C para Linux

## ¿Qué hace el programa?

`main.c` implementa un shell interactivo para Linux que integra tres funcionalidades en un único archivo fuente:

1. **Ejecución de comandos Bash** mediante `fork` + `execvp`, con soporte de entubamiento (`|`) para encadenar hasta 5 procesos.
2. **Simulación de procesos y calendarización**: permite crear procesos virtuales y planificarlos con los algoritmos FCFS, SJF y Round-Robin. Solo los procesos en estado **ready** (cargados en memoria) pueden ser planificados.
3. **Módulo de memoria**: asignación contigua de memoria con estrategias First Fit, Best Fit y Worst Fit, liberación de bloques y compactación.

---

## ¿Cómo compilarlo?

```bash
gcc -Wall -o shell main.c
```

---

## ¿Cómo ejecutarlo?

```bash
./shell
```

El shell muestra el prompt `shell>` y acepta comandos hasta que se escribe `exit`. La memoria se inicializa automáticamente con `100` bloques (constante `MEMORY_SIZE` en el código).

---

## Referencia de comandos

### Generales

| Comando | Descripción |
|---------|-------------|
| `exit`  | Sale del shell. |

### Módulo 1 — Bash y pipes

| Sintaxis | Descripción | Ejemplo |
|----------|-------------|---------|
| `<comando>` | Ejecuta cualquier comando de Bash disponible en el sistema. | `ls -l` |
| `<cmd1> \| <cmd2> \| ...` | Ejecuta una cadena de hasta 5 comandos conectados por pipes. | `ps aux \| grep bash \| cut -d' ' -f1` |

### Módulo 2 — Procesos y calendarización

| Sintaxis | Descripción | Ejemplo |
|----------|-------------|---------|
| `mkprocess <id> <burst_time> <tamaño>` | Crea un proceso con identificador numérico único, tiempo de ráfaga y tamaño en bloques. Queda en estado **new**. | `mkprocess 1 25 4` |
| `lstprocess` | Lista todos los procesos con su ID, burst time, tamaño y estado actual. | `lstprocess` |
| `fcfs` | Planifica los procesos **ready** en orden de llegada. Muestra el orden de ejecución, waiting time y turnaround time. Los procesos pasan a **terminated** y se liberan de memoria. | `fcfs` |
| `sjf` | Planifica los procesos **ready** ordenados por menor burst time. Misma salida que fcfs. | `sjf` |
| `rr` | Planifica los procesos **ready** con Round-Robin (quantum = 10 ut). Misma salida que fcfs. | `rr` |
| `my_kill <id>` | Elimina el proceso con el identificador dado. Si el proceso está en estado **ready**, lo libera de memoria antes de eliminarlo. | `my_kill 3` |

### Módulo 3 — Memoria

| Sintaxis | Descripción | Ejemplo |
|----------|-------------|---------|
| `alloc <id> <estrategia>` | Carga en memoria el proceso indicado usando la estrategia de asignación contigua especificada. El proceso pasa a estado **ready**. Estrategias: `first` (First Fit), `best` (Best Fit), `worst` (Worst Fit). | `alloc 1 best` |
| `free <id>` | Libera el bloque de memoria del proceso indicado. El proceso vuelve a estado **new** pero no se elimina de la lista. | `free 2` |
| `mstatus` | Muestra el estado actual de la memoria: número de bloque, dirección base, tamaño, límite, estado (LIBRE/OCUPADO) y proceso asociado. | `mstatus` |
| `compact` | Fusiona bloques libres adyacentes para reducir la fragmentación externa. | `compact` |

---

## ¿Cómo funciona internamente?

### Pipes múltiples (Módulo 1)

La función `run_bash_cmd` divide la línea de entrada por el carácter `|` con `strtok`, obteniendo un arreglo de hasta 5 segmentos. Si solo hay un segmento se ejecuta con `fork` + `execvp` directo. Si hay varios, `exec_pipeline` crea `n-1` pipes y lanza un hijo por cada segmento: cada hijo redirige su stdin/stdout con `dup2` según su posición en la cadena, cierra todos los descriptores sobrantes y ejecuta su comando con `execvp`. El padre cierra su copia de todos los pipes y espera a cada hijo con `waitpid`.

### Representación de procesos (Módulo 2)

Cada proceso se almacena en un arreglo estático de structs `Proc` con los campos `id`, `burst`, `size` y `state`. El estado es un `enum` con tres valores: `STATE_NEW`, `STATE_READY` y `STATE_TERMINATED`. Solo los procesos en estado `ready` (cargados en memoria con `alloc`) son elegibles para los algoritmos de calendarización.

### Algoritmos de calendarización

Antes de ejecutar cada algoritmo se recopilan en un arreglo auxiliar `PInfo` solo los procesos en estado `ready`, sin modificar el arreglo original. Al terminar, los procesos se liberan de memoria y se marcan como `terminated` en el arreglo principal.

- **FCFS**: recorre el arreglo en orden de inserción. `wt[i]` es el tiempo acumulado antes del proceso `i`; `tat[i] = wt[i] + burst[i]`.
- **SJF**: ordena el arreglo auxiliar por `burst` (bubble sort) y luego aplica la misma lógica que FCFS.
- **RR**: en cada pasada del ciclo externo asigna a cada proceso pendiente un slice de `min(restante, 10)` unidades. Cuando el restante llega a 0 calcula `tat` y `wt = tat - burst`.

### Módulo de memoria (Módulo 3)

La memoria se representa como una lista enlazada de structs `MemoryBlock`, cada uno con dirección base, tamaño, estado (`FREE`/`OCCUPIED`) y puntero al proceso que lo ocupa. Al arrancar el shell se inicializa un único bloque libre de tamaño `MEMORY_SIZE`.

- **`alloc`**: busca un bloque libre según la estrategia elegida, lo divide si sobra espacio (`split_and_assign`) y marca el bloque como ocupado. El proceso pasa a `ready`.
- **`free`**: recorre la lista buscando el bloque del proceso y lo marca como libre. El proceso vuelve a `new`.
- **`compact`**: fusiona bloques libres consecutivos en uno solo para consolidar el espacio disponible.
- **`mstatus`**: imprime la tabla completa de bloques con su estado actual.

---

## Ejemplos de uso

```
shell> mkprocess 1 30 10
Proceso 1 creado (burst=30, size=10 bloques, estado=new).

shell> mkprocess 2 10 5
Proceso 2 creado (burst=10, size=5 bloques, estado=new).

shell> mkprocess 3 20 8
Proceso 3 creado (burst=20, size=8 bloques, estado=new).

shell> alloc 1 first
Proceso 1 cargado en memoria (tamaño=10 bloques, estrategia=first, estado=ready).

shell> alloc 2 best
Proceso 2 cargado en memoria (tamaño=5 bloques, estrategia=best, estado=ready).

shell> alloc 3 worst
Proceso 3 cargado en memoria (tamaño=8 bloques, estrategia=worst, estado=ready).

shell> mstatus

Bloque   Dir.Base       Tamaño     Límite     Estado     Proceso

0        0              10         9          OCUPADO    1
1        10             5          14         OCUPADO    2
2        15             8          22         OCUPADO    3
3        23             77         99         LIBRE

shell> lstprocess

ID       Burst Time   Tamaño     Estado
------------------------------------------
1        30           10         ready
2        10           5          ready
3        20           8          ready

shell> sjf

--- SJF ---
t=0     entra proceso 2  (burst=10)
t=10    sale proceso 2  (completado)
t=10    entra proceso 3  (burst=20)
t=30    sale proceso 3  (completado)
t=30    entra proceso 1  (burst=30)
t=60    sale proceso 1  (completado)

ID       Waiting Time  Turnaround Time
2               0               10
3              10               30
1              30               60
PROMEDIO       13.33            33.33

shell> lstprocess

ID       Burst Time   Tamaño     Estado
------------------------------------------
1        30           10         terminated
2        10           5          terminated
3        20           8          terminated

shell> mstatus

Bloque   Dir.Base       Tamaño     Límite     Estado     Proceso

0        0              100        99         LIBRE

shell> mkprocess 4 15 6
Proceso 4 creado (burst=15, size=6 bloques, estado=new).

shell> alloc 4 best
Proceso 4 cargado en memoria (tamaño=6 bloques, estrategia=best, estado=ready).

shell> free 4
Proceso 4 liberado de memoria (estado=new).

shell> my_kill 4
Proceso 4 eliminado.

shell> ls | grep main
main.c

shell> exit
Bye
```
