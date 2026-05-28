# Proyecto: Shell en C para Linux

## Contexto

En la carpeta del proyecto ya existen archivos `.c` individuales donde cada funcionalidad descrita a continuación fue implementada y probada de forma independiente. **No se deben modificar esos archivos.** El objetivo es tomar exactamente esa lógica ya implementada e integrarla en un único archivo llamado `main.c` que funcione como el shell completo.

> **Nota importante:** La funcionalidad de **memoria** (comandos `alloc`, `free`, `mstatus`, `compact`) **no debe implementarse por el momento.** Se integrará en una etapa posterior.

---

## Lo que se debe generar

### 1. `main.c`

Un único archivo en C que integre toda la lógica ya existente en los archivos individuales del proyecto. Este archivo debe implementar un shell interactivo para Linux con los siguientes módulos:

---

### Módulo 1 — Ejecución de comandos Bash

El shell debe poder ejecutar comandos básicos de Bash (como `ps`, `ls`, `grep`, `cut`, etc.) usando las llamadas al sistema `fork` y `execvp`.

Además, debe soportar **entubamiento (pipes)** con el operador `|`, permitiendo encadenar hasta **5 procesos** en una sola línea de comandos.

**Ejemplos:**
```
$ ls
$ ps aux | grep bash
$ cat archivo.txt | grep "error" | cut -d: -f2
```

---

### Módulo 2 — Simulación de creación y calendarización de procesos

#### Creación de procesos

```
mkprocess <identificador> <burst_time> <tamaño>
```

- `identificador`: valor numérico único que identifica el proceso (validar que no exista uno previo con el mismo ID).
- `burst_time`: tiempo que el proceso ocupará en CPU (en unidades de tiempo).
- `tamaño`: bloques de memoria que ocupará el proceso.
- Al crearlo, el proceso queda en estado **new**.

#### Listar procesos

```
lstprocess
```

Muestra todos los procesos creados con: identificador, burst time, tamaño en bloques y **estado actual** del proceso.

#### Algoritmos de calendarización

Solo pueden ejecutarse procesos que hayan sido cargados a memoria previamente (estado **ready**) mediante el comando `alloc` (ver Módulo 3). Al ser planificados y ejecutados, pasan a estado **terminated**.

```
FCFS
```
Ejecuta los procesos en orden de llegada (First-Come, First-Served). Muestra el orden de entrada/salida al CPU, el turnaround time y el waiting time promedio general y por proceso.

```
SJF
```
Ejecuta los procesos ordenados por menor burst time (Shortest-Job-First). Muestra la misma información que FCFS.

```
RR
```
Ejecuta los procesos con Round Robin usando un **time quantum de 10 unidades de tiempo**. Muestra la misma información que los algoritmos anteriores.

#### Eliminación de procesos

```
my_kill <identificador>
```

Elimina el proceso con el identificador dado. Si el proceso está en estado **ready** (en memoria), ejecuta internamente el equivalente a `free` antes de eliminarlo.

---

### Módulo 3 — Asignación de memoria (pendiente / no implementar aún)

> Este módulo **no debe implementarse en esta etapa.** Se dejará preparado para integración futura.

Los comandos relacionados a este módulo son: `alloc`, `free`, `mstatus` y `compact`.

Para que los algoritmos de calendarización funcionen en esta etapa, se puede asumir que todos los procesos en estado **new** están disponibles para ser planificados (simulando que ya están en memoria), o bien dejar los comandos de calendarización condicionados a que el proceso esté en estado `ready` y simplemente mostrar un mensaje si no hay procesos listos.

---

### 2. `README.md`

Junto con `main.c`, generar un archivo `README.md` con la siguiente información:

#### Secciones requeridas:

**¿Qué hace el programa?**
Descripción general del shell: qué es, qué permite hacer y cómo está estructurado internamente.

**¿Cómo compilarlo?**
Comando exacto para compilar `main.c` con `gcc`, incluyendo flags relevantes (como `-Wall` o `-lm` si aplica).

**¿Cómo ejecutarlo?**
Comando para lanzar el shell compilado desde la terminal.

**Referencia de comandos**
Tabla o lista detallada con cada comando disponible, su sintaxis, descripción y un ejemplo de uso.

**¿Cómo funciona internamente?**
Explicación de las decisiones de diseño más importantes: cómo se manejan los pipes, cómo se representa un proceso en memoria, cómo funciona cada algoritmo de calendarización, etc.

**Ejemplos de uso**
Secuencia de comandos de ejemplo que muestre un flujo completo: crear procesos, listarlos, cargarlos y ejecutar cada algoritmo.

---

## Restricciones y consideraciones

- Usar únicamente la lógica ya presente en los archivos individuales del proyecto; no reinventar ni cambiar el comportamiento existente.
- El archivo `main.c` debe ser autocontenido (un solo archivo fuente).
- El shell debe tener un prompt visible (por ejemplo `shell> `) y un comando `exit` para salir.
- El código debe compilar sin errores con `gcc` en un sistema Linux estándar.
- La memoria (módulo 3) queda **fuera del alcance** de esta entrega.