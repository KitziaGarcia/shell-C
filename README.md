# Shell en C para Linux

## ¿Qué hace el programa?

`main.c` implementa un shell interactivo para Linux que integra dos funcionalidades en un único archivo fuente:

1. **Ejecución de comandos Bash** mediante `fork` + `execvp`, con soporte de entubamiento (`|`) para encadenar hasta 5 procesos.
2. **Simulación de procesos y calendarización**: permite crear procesos virtuales y planificarlos con los algoritmos FCFS, SJF y Round-Robin, rastreando su estado (`new` → `terminated`).

El módulo de asignación de memoria (`alloc`, `mstatus`, `compact`) está reservado para una entrega posterior y no está incluido.

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

El shell muestra el prompt `shell>` y acepta comandos hasta que se escribe `exit`.

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
| `FCFS` | Planifica los procesos en orden de llegada. Muestra el orden de ejecución, waiting time y turnaround time. Los procesos pasan a **terminated**. | `FCFS` |
| `SJF` | Planifica los procesos ordenados por menor burst time. Misma salida que FCFS. | `SJF` |
| `RR` | Planifica los procesos con Round-Robin (quantum = 10 ut). Misma salida que FCFS. | `RR` |
| `my_kill <id>` | Elimina el proceso con el identificador dado. | `my_kill 3` |

---

## ¿Cómo funciona internamente?

### Pipes múltiples (Módulo 1)

La función `run_bash_cmd` divide la línea de entrada por el carácter `|` con `strtok`, obteniendo un arreglo de hasta 5 segmentos. Si solo hay un segmento se ejecuta con `fork` + `execvp` directo. Si hay varios, `exec_pipeline` crea `n-1` pipes y lanza un hijo por cada segmento: cada hijo redirige su stdin/stdout con `dup2` según su posición en la cadena, cierra todos los descriptores sobrantes y ejecuta su comando con `execvp`. El padre cierra su copia de todos los pipes y espera a cada hijo con `waitpid`.

### Representación de procesos (Módulo 2)

Cada proceso se almacena en un arreglo estático de structs `Proc` con los campos `id`, `burst`, `size` y `state`. El estado es un `enum` con tres valores: `STATE_NEW`, `STATE_READY` y `STATE_TERMINATED`. En esta etapa (sin módulo de memoria), los procesos en estado `new` son tratados como planificables directamente.

### Algoritmos de calendarización

Antes de ejecutar cada algoritmo se recopilan en un arreglo auxiliar `PInfo` solo los procesos planificables (`new` o `ready`), sin modificar el arreglo original. Al terminar, los procesos se marcan como `terminated` en el arreglo principal.

- **FCFS**: recorre el arreglo en orden de inserción. `wt[i]` es el tiempo acumulado antes del proceso `i`; `tat[i] = wt[i] + burst[i]`.
- **SJF**: ordena el arreglo auxiliar por `burst` (bubble sort) y luego aplica la misma lógica que FCFS.
- **RR**: en cada pasada del ciclo externo asigna a cada proceso pendiente un slice de `min(restante, 10)` unidades. Cuando el restante llega a 0 calcula `tat` y `wt = tat - burst`.

---

## Ejemplos de uso

```
shell> mkprocess 1 30 2
Proceso 1 creado (burst=30, size=2 bloques, estado=new).

shell> mkprocess 2 10 1
Proceso 2 creado (burst=10, size=1 bloques, estado=new).

shell> mkprocess 3 20 3
Proceso 3 creado (burst=20, size=3 bloques, estado=new).

shell> lstprocess

ID       Burst Time   Tamaño     Estado
------------------------------------------
1        30           2          new
2        10           1          new
3        20           3          new

shell> SJF

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
1        30           2          terminated
2        10           1          terminated
3        20           3          terminated

shell> ls | grep main
main.c

shell> ps aux | grep bash | head -3

shell> exit
Bye!
```
