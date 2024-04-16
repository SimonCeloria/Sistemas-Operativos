# Sistemas operativos
## Laboratorio 3

Codigo del laboratorio [Bitbuket](https://bitbucket.org/sistop-famaf/so23lab3g30/src/master/)

Grupo 30 integrantes:

- Simón Celoria
- Lazaro Cugat
- Ezequiel Lauret
- Santiago Morales  
## Primera parte respuestas:

> ¿Qué política de planificación utiliza xv6-riscv para elegir el próximo proceso a ejecutarse?

La política de planificación de procesos implementada por el sistema operativo xv6-riscv es conocida como Round-Robin (RR). Esta estrategia se basa en el mantenimiento de una tabla de procesos en el estado "RUNNABLE", y el sistema operativo, siguiendo una política predefinida, selecciona cuál de los procesos en ese estado debe ejecutarse a continuación. Una vez seleccionado, el proceso cambia su estado a "RUNNING" y se le asigna un período de tiempo de CPU predeterminado, conocido como "quantum", durante el cual se le permite ejecutarse. Al finalizar este quantum, el sistema operativo se encarga de ceder el control al siguiente proceso en la cola de "RUNNABLE".

Es importante destacar que un proceso tiene la capacidad de liberar voluntariamente la CPU por sí mismo, bien puede ser porque pasa al estado "SLEEP" esperando un IO, o porque utilizo la llamada a sistema yield() para liberar el CPU antes de finalizar el quantum.

Este comportamiento permite una gestión eficiente de los recursos del sistema y garantiza que otros procesos puedan ser ejecutados cuando sea necesario.

> ¿Cuánto dura un quantum en xv6-riscv?

La duración del quantum en el sistema operativo xv6-riscv se encuentra especificada en el código fuente ubicado en la carpeta "kernel," dentro del archivo "start.c." Más precisamente, se define en la función "timer_init," que se encuentra en la línea 69 de dicho archivo. La variable encargada de representar la duración del quantum se denomina "interval" y tiene un valor establecido en 1000000 (Posee una escala 1/10 por lo que él quantum dura 100 msec). Este valor se traduce en la cantidad de ciclos que se ejecutan, lo que equivale a una décima parte de un segundo en el entorno de emulación QEMU.

> ¿Cuánto dura un cambio de contexto en xv6-riscv?

*¿Qué ocurre en un context switch?*

A grandes rasgos un cambio de contexto no solo consiste en salvar/restaurar los registros que contienen la información de los procesos. 
Un cambio conlleva además lo que realiza la función switch(), ésta guarda el estado del proceso con una serie de 14 registros con las instrucciones sd/ld, también pone a correr el proceso que bien puede ejecutarse en el 1er CPU o bien en cualquier otro CPU (suponiendo que no se está ejecutando en una PC monocore). Aunque pasa en poco tiempo el cambio de contexto es un tiempo importante a tener en cuenta por el gran volumen de ejecuciones que maneja un procesador.
A continuación mostramos un pequeño diagrama de la ruta que puede tomar un cambio de contexto: 
**Link del diagrama: https://github.com/FamafLover/famaf_comunitario/blob/main/Context.png**

Luego realizamos una serie de pruebas para intentar estimar el tiempo que conlleva un cambio de contexto en xv6.

Para poder resolver esta pregunta recurrimos al libro OSTEP, el cual menciona en el capitulo 7 : 
> As you can see, the length of the time slice is critical for RR. The shorter it is, the better the performance of RR under the response-time metric. However, making the time slice too short is problematic: suddenly the cost of context switching will dominate overall performance.

El objetivo de este estudio consiste en reducir el quantum del sistema operativo de manera que el costo del cambio de contexto se acerque tanto al quantum, de tal forma que no permita la ejecución de procesos. Se llevaron a cabo diversas pruebas para determinar el valor óptimo del quantum que minimizaría este costo y se observó su impacto en el rendimiento del sistema operativo.

**Prueba i:**
Se disminuyó el valor del quantum de 1,000,000 a 1,000. Se observó una disminución en el rendimiento, pero el sistema operativo xv6 continuó funcionando correctamente. Se ejecutó el programa "pingpong" para evaluar su comportamiento, y se confirmó que funcionaba sin problemas.

**Prueba ii:**
Se redujo aún más el valor del quantum en intervalos de 100 (700, 600, 500, 400, 300, 200). Cada reducción incrementaba el tiempo de inicialización, pero se logró ejecutar el programa "pingpong". Se observó que a medida que el intervalo disminuía, el sistema operativo se volvía más lento, indicando un aumento significativo en el costo del cambio de contexto.

**Prueba iii:**
Se realizó una prueba con un intervalo de 100, lo que resultó en un tiempo de inicialización considerablemente alto. Al ejecutar el programa "pingpong" en este intervalo, se observó que cada ping/pong se imprimía aproximadamente una vez por segundo, lo que evidencia un rendimiento ineficiente.

**Prueba iv:**
En una última prueba, se intentó con un intervalo de 80, pero después de varios intentos, no se logró la inicialización.

**Conclusiones:**
En las pruebas realizadas, se puede concluir que el cambio de contexto tiene una duración menor de 100.


>¿El cambio de contexto consume tiempo de un quantum?

Cierto, cuando se produce una cambio de contexto, se requiere que el sistema operativo dedique recursos a guardar el estado del proceso en ejecución, cargar el estado del proceso programado para ejecutarse a continuación y llevar a cabo diversas tareas relacionadas con la gestión de procesos. Este conjunto de acciones adicionales conlleva un costo temporal significativo, lo cual, en consecuencia, reduce el tiempo efectivo disponible para la ejecución del propio proceso.

>¿Hay alguna forma de que a un proceso se le asigne menos tiempo?

Si es posible atravez de la modificacion del quantum en la siguiente linea de codigo (con escala 1/10):
```sh
// ask the CLINT for a timer interrupt.
  int interval = 1000000; // cycles; about 1/10th second in qemu.
  *(uint64*)CLINT_MTIMECMP(id) = *(uint64*)CLINT_MTIME + interval;
```
>¿Cúales son los estados en los que un proceso pueden permanecer en xv6-
riscv y que los hace cambiar de estado?

Los estados en los que los procesos pueden permanecer en xv6-riscv son :
enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };
RUNNING: En el código del scheduler es donde se ve modificado el estado del proceso cambiandolo a RUNNING mostrando que actualmente está siendo ejecutado.

*UNUSED*: Al inicializar la proc table o bien al realizar un free proc (línea 155), todos los procesos son seteados en UNUSED como podemos ver en el siguiente código (archivo proc.c línea 56).
```sh
// initialize the proc table.
void procinit(void){
    struct proc *p;
    initlock(&pid_lock, "nextpid");
    initlock(&wait_lock, "wait_lock");
    for(p = proc; p < &proc[NPROC]; p++) {
        initlock(&p->lock, "proc");
        p->state = UNUSED; <- Modificacion del estado del proceso
        p->kstack = KSTACK((int) (p - proc));
    }
}
```
*USED*: Cómo se puede ver en el archivo proc.c en la función allocproc linea 110, la función busca por procesos en estado UNUSED y una vez encuentra una llama a la función found y en la funcion found se setea el estado del proceso en USED además de realizar otras acciones. Adjunto fragmento del codigo found:

```sh
...
found:
p->pid = allocpid();
p->state = USED; <- Modificacion del estado del proceso
...
```

*RUNNABLE*: El estado runnable se puede modificar en varias funciones bien pueden ser userinit, fork, wakeup, yield o kill todas estas seteando el proceso en runnable luego de realizar sus debidas funciones. 

*SLEEPING*: La función sleep es la encargada de poner los estados a dormir y cambiar el estado a SLEEPING, en el archivo proc.c linea 536 y los motivos pueden ser varios como por ejemplo esperar un I/O.

*ZOMBIE*: La función exit() cambia el estado del proceso a ZOMBIE en el archivo proc.c en la linea 379, la función esta acompañada de un comentario que dice lo siguiente:

```sh
// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
```
Sale del proceso actual, un proceso del cual se salió permanece en estado zombie hasta que un padre llama a la función wait ().

# Segunda parte respuestas:
## Caracterizacion de experimentos

La siguiente es una tabla estandar brindada por el profesor para caracterizar los experimentos tanto de io-bench como cpu-bench.

| Tabla de caracterizacion de experimentos| |
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | Normal |
| Politica scheduler | RR |
| Caso | x |
|.            |           |
| Promedio MFLOPS /100T 1er cpubench |  |
| Promedio MFLOPS /100T 2do cpubench | |
| Promedio OPW/100T | |
| Promedio OPR/100T | |
|.            |           |
| Cant. select IO |  |
| Cant. select 1er CPU |  |
| Cant. select 2do CPU |  |

---  
# Experimentos
## Primer prueba un solo io-bench:

**Datos de entrada**
```sh
                                        3: 3392 OPW100T, 3392 OPR100T
                                        3: 3295 OPW100T, 3295 OPR100T
                                        3: 3008 OPW100T, 3008 OPR100T
                                        3: 2851 OPW100T, 2851 OPR100T
                                        3: 3264 OPW100T, 3264 OPR100T
                                        3: 3231 OPW100T, 3231 OPR100T
                                        3: 3104 OPW100T, 3104 OPR100T
                                        3: 3295 OPW100T, 3295 OPR100T
                                        3: 2978 OPW100T, 2978 OPR100T
                                        3: 3200 OPW100T, 3200 OPR100T
                                        3: 3104 OPW100T, 3104 OPR100T
                                        3: 3200 OPW100T, 3200 OPR100T
                                        3: 3168 OPW100T, 3168 OPR100T
                                        3: 3264 OPW100T, 3264 OPR100T
                                        3: 3328 OPW100T, 3328 OPR100T
                                        3: 3295 OPW100T, 3295 OPR100T
                                        3: 3295 OPW100T, 3295 OPR100T
                                        3: 3200 OPW100T, 3200 OPR100T
                                        3: 3231 OPW100T, 3231 OPR100T
Termino iobench 3: total ops 122112u -->        Priority:0
 Times executed:200509
 Last time executed:2130
```

| Tabla de caracterizacion de experimentos| 1 io-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | Normal |
| Politica scheduler | RR |
| Caso | 1 solo io-bench |
|.            |           |
| Promedio MFLOPS /100T 1er cpubench | Es solo iobench |
| Promedio MFLOPS /100T 2do cpubench | Es solo iobench |
| Promedio OPW/100T | 3.194,89 OPW100T|
| Promedio OPR/100T | 3.194,89 OPW100T|
|.            |           |
| Cant. select IO (Times executed) | 200509 |
| Cant. select 2do IO (Times executed) | 210479 |
| Cant. select 2do CPU | - |

**Conclusiones:** Al ejecutar un solo io-bench el promedio de escritura y lectura es igual. Como se puede observar el promedio es muy similar a la cantidad de OPW/OPT por lo que se podria entender que al ejecutarse un solo io-bench la relacion entre el quanto y el proceso se mantienen cercanas

## Segunda prueba un solo cpu-bench:

**Datos de entrada**

*1er cpubench*  
```
3: 712 MFLOP100T
3: 725 MFLOP100T
3: 725 MFLOP100T
3: 719 MFLOP100T
3: 725 MFLOP100T
3: 725 MFLOP100T
3: 725 MFLOP100T
3: 725 MFLOP100T
3: 700 MFLOP100T
3: 688 MFLOP100T
3: 725 MFLOP100T
3: 712 MFLOP100T
3: 712 MFLOP100T
3: 688 MFLOP100T
3: 706 MFLOP100T
3: 706 MFLOP100T
3: 706 MFLOP100T
Termino cpubench 3: total ops 805306368u -->            Priority:0
Times executed:2109
Last time executed:2154
```
*2do cpubench*
```
5: 732 MFLOP100T
5: 738 MFLOP100T
5: 738 MFLOP100T
5: 745 MFLOP100T
5: 738 MFLOP100T
5: 725 MFLOP100T
5: 719 MFLOP100T
5: 732 MFLOP100T
5: 732 MFLOP100T
5: 738 MFLOP100T
5: 732 MFLOP100T
5: 732 MFLOP100T
5: 706 MFLOP100T
5: 738 MFLOP100T
5: 745 MFLOP100T
5: 732 MFLOP100T
5: 745 MFLOP100T
5: 738 MFLOP100T
Termino cpubench 5: total ops 1610612736u -->           Priority:0
Times executed:2100
Last time executed:9559
```

| Tabla de caracterizacion de experimentos| 1 io-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | Normal |
| Politica scheduler | RR |
| Caso | 1 solo io-bench |
|.            |           |
| Promedio MFLOPS /100T 1er cpubench | 713 MFLOPS/100T|
| Promedio MFLOPS /100T 2do cpubench | 733 MFLOPS/100T |
| Promedio OPW/100T | Es solo cpu-bench|
| Promedio OPR/100T | Es solo cpu-bench|
|.            |           |
| Cant. select IO (Times executed) | - |
| Cant. select 1er CPU | 2109 |
| Cant. select 2do CPU | 2100 |

**Observacion:** En este caso en la segunda ejecucion hubo una diferencia importante en el last time executed con respecto a la primera ejecucion pero en general los datos se mantienen constantes en las ejecuciones.

## Tercera prueba un iobench-cpubench:

**Datos de entrada**

*1er iobench-cpubench*
```
5: 745 MFLOP100T
5: 738 MFLOP100T
5: 745 MFLOP100T
                                        3: 58 OPW100T, 58 OPR100T
5: 738 MFLOP100T
5: 725 MFLOP100T
                                        3: 33 OPW100T, 33 OPR100T
5: 725 MFLOP100T
5: 732 MFLOP100T
                                        3: 33 OPW100T, 33 OPR100T
5: 725 MFLOP100T
                                        3: 33 OPW100T, 33 OPR100T
5: 738 MFLOP100T
5: 745 MFLOP100T
                                        3: 33 OPW100T, 33 OPR100T
5: 738 MFLOP100T
5: 700 MFLOP100T
                                        3: 33 OPW100T, 33 OPR100T
5: 682 MFLOP100T
5: 712 MFLOP100T
                                        3: 33 OPW100T, 33 OPR100T
5: 712 MFLOP100T
                                        3: 33 OPW100T, 33 OPR100T
5: 700 MFLOP100T
5: 738 MFLOP100T
                                        3: 33 OPW100T, 33 OPR100T
5: 732 MFLOP100T
Termino cpubench 5: total ops 1610612736u -->   Priority:0
Times executed:2127
Last time executed:2206
                                        3: 33 OPW100T, 33 OPR100T
Termino iobench 3: total ops 1408u -->  Priority:0
Times executed:2239
Last time executed:2208
```

*2do iobench-cpubench*

```
12: 706 MFLOP100T
12: 700 MFLOP100T
12: 719 MFLOP100T
                                        14: 62 OPW100T, 62 OPR100T
12: 712 MFLOP100T
12: 719 MFLOP100T
                                        14: 33 OPW100T, 33 OPR100T
12: 706 MFLOP100T
                                        14: 33 OPW100T, 33 OPR100T
12: 719 MFLOP100T
12: 700 MFLOP100T
                                        14: 33 OPW100T, 33 OPR100T
12: 725 MFLOP100T
12: 700 MFLOP100T
                                        14: 33 OPW100T, 33 OPR100T
12: 712 MFLOP100T
                                        14: 33 OPW100T, 33 OPR100T
12: 700 MFLOP100T
12: 719 MFLOP100T
                                        14: 33 OPW100T, 33 OPR100T
12: 712 MFLOP100T
12: 712 MFLOP100T
                                        14: 33 OPW100T, 33 OPR100T
12: 725 MFLOP100T
                                        14: 33 OPW100T, 33 OPR100T
12: 712 MFLOP100T
Termino cpubench 12: total ops 805306368u -->
Priority:0
Times executed:2110
Last time executed:13752
                                        14: 33 OPW100T, 33 OPR100T
Termino iobench 14: total ops 1408u --> Priority:0
Times executed:2224
Last time executed:13753
```

| Tabla de caracterizacion de experimentos| iobench-cpubench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | Normal |
| Politica scheduler | RR |
| Caso | 1 iobench-cpubench |
|.            |           |
| Promedio MFLOPS /100T 1er cpubench | 710 MFLOPS/100T|
| Promedio MFLOPS /100T 2do cpubench | 711 MFLOPS/100T|
| Promedio OPW/100T | 35.9 |
| Promedio OPR/100T | 35.9 |
|.            |           |
| Cant. select IO (Times executed) | 2224 |
| Cant. select 1er CPU | 2111 |
| Cant. select 2do CPU | 2110 |

## Cuarta prueba cpubench-iobench:

**Datos de entrada**

*1er cpubench & iobench*
```
5: 745 MFLOP100T
5: 738 MFLOP100T
5: 745 MFLOP100T
                                        3: 58 OPW100T, 58 OPR100T
5: 738 MFLOP100T
5: 725 MFLOP100T
                                        3: 33 OPW100T, 33 OPR100T
5: 725 MFLOP100T
5: 732 MFLOP100T
                                        3: 33 OPW100T, 33 OPR100T
5: 725 MFLOP100T
                                        3: 33 OPW100T, 33 OPR100T
5: 738 MFLOP100T
5: 745 MFLOP100T
                                        3: 33 OPW100T, 33 OPR100T
5: 738 MFLOP100T
5: 700 MFLOP100T
                                        3: 33 OPW100T, 33 OPR100T
5: 682 MFLOP100T
5: 712 MFLOP100T
                                        3: 33 OPW100T, 33 OPR100T
5: 712 MFLOP100T
                                        3: 33 OPW100T, 33 OPR100T
5: 700 MFLOP100T
5: 738 MFLOP100T
                                        3: 33 OPW100T, 33 OPR100T
5: 732 MFLOP100T
Termino cpubench 5: total ops 1610612736u --> Priority:0
Times executed:2127
Last time executed:2206
                                        3: 33 OPW100T, 33 OPR100T
Termino iobench 3: total ops 1408u -->  Priority:0
Times executed:2239
Last time executed:2208
```

*2do cpubench & iobench*
```
8: 738 MFLOP100T
8: 738 MFLOP100T
8: 745 MFLOP100T
                                        6: 60 OPW100T, 60 OPR100T
8: 732 MFLOP100T
8: 738 MFLOP100T
                                        6: 33 OPW100T, 33 OPR100T
8: 738 MFLOP100T
8: 745 MFLOP100T
                                        6: 33 OPW100T, 33 OPR100T
8: 738 MFLOP100T
                                        6: 33 OPW100T, 33 OPR100T
8: 738 MFLOP100T
8: 738 MFLOP100T
                                        6: 33 OPW100T, 33 OPR100T
8: 738 MFLOP100T
8: 738 MFLOP100T
                                        6: 33 OPW100T, 33 OPR100T
8: 738 MFLOP100T
8: 738 MFLOP100T
                                        6: 33 OPW100T, 33 OPR100T
8: 738 MFLOP100T
                                        6: 33 OPW100T, 33 OPR100T
8: 738 MFLOP100T
8: 738 MFLOP100T
                                        6: 33 OPW100T, 33 OPR100T
8: 738 MFLOP100T
Termino cpubench 8: total ops 1610612736u --> Priority:0
Times executed:2115
Last time executed:5018
                                        6: 33 OPW100T, 33 OPR100T
Termino iobench 6: total ops 1408u -->  Priority:0
Times executed:2225
Last time executed:5019
```

| Tabla de caracterizacion de experimentos| cpubench & iobench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | Normal |
| Politica scheduler | RR |
| Caso | 1 cpu-bench y 1 io-bench |
|.            |           |
| Promedio MFLOPS /100T 1er cpubench | 726 MFLOPS/100T|
| Promedio MFLOPS /100T 2do cpubench | 738 MFLOPS/100T|
| Promedio OPW/100T | 35.5|
| Promedio OPR/100T | 35.5|
|.            |           |
| Cant. select IO (Times executed) | 2225 |
| Cant. select 1er CPU | 2127 |
| Cant. select 2do CPU | 2115 |

### Prueba 5
Datos:
```
5: 569 MFLOP100T
7: 564 MFLOP100T
5: 890 MFLOP100T
7: 898 MFLOP100T
7: 906 MFLOP100T
5: 898 MFLOP100T
7: 906 MFLOP100T
5: 906 MFLOP100T
7: 915 MFLOP100T
5: 906 MFLOP100T
7: 906 MFLOP100T
5: 898 MFLOP100T
7: 906 MFLOP100T
5: 898 MFLOP100T
                                        3: 32 OPW100T, 32 OPR100T
7: 898 MFLOP100T
5: 890 MFLOP100T
7: 906 MFLOP100T
5: 898 MFLOP100T
7: 898 MFLOP100T
5: 890 MFLOP100T
                                        3: 16 OPW100T, 16 OPR100T
7: 890 MFLOP100T
5: 883 MFLOP100T
7: 906 MFLOP100T
5: 906 MFLOP100T
7: 898 MFLOP100T
5: 906 MFLOP100T
                                        3: 16 OPW100T, 16 OPR100T
7: 898 MFLOP100T
5: 883 MFLOP100T
7: 932 MFLOP100T
5: 906 MFLOP100T
7: 875 MFLOP100T
5: 883 MFLOP100T
7: 906 MFLOP100T
5: 906 MFLOP100T
                                        3: 16 OPW100T, 16 OPR100T
Termino cpubench 7: total ops 3825205248u --> Priority:0
Times executed:1051
Last time executed:2197
Termino cpubench 5: total ops 3825205248u --> Priority:0
Times executed:1063
Last time executed:2204
                                        3: 16 OPW100T, 16 OPR100T
Termino iobench 3: total ops 768u -->   Priority:0
Times executed:1218
Last time executed:2206
```

| Tabla de caracterizacion de experimentos| 1 cpu-bench 1 cpu-bench y 1 io-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | 10 veces más pequeño |
| Politica scheduler | RR |
| Caso | 1 cpu-bench 1 cpu-bench y 1 io-bench |
|.            |           |
| Promedio MFLOPS/100T 1er cpubench (proceso 5)| 877 MFLOPS/100T|
| Promedio MFLOPS/100T 2do cpubench (proceso 7)| 876 MFLOPS/100T|
| Promedio OPW/100T |  19  OPW/100T|
| Promedio OPR/100T |  19  OPW/100T|
|.            |           |
| Cant. select IO (Times executed) | 1218 |
| Cant. select 1er CPU (7) | 2197 |
| Cant. select 2do CPU (5)| 1063 |

## Repito todas las pruebas con un quanto 10 mas pequeño

### Prueba 1:
Datos:
```
                                        3: 3648 OPW100T, 3648 OPR100T
                                        3: 3648 OPW100T, 3648 OPR100T
                                        3: 3421 OPW100T, 3421 OPR100T
                                        3: 3392 OPW100T, 3392 OPR100T
                                        3: 2851 OPW100T, 2851 OPR100T
                                        3: 2534 OPW100T, 2534 OPR100T
                                        3: 3584 OPW100T, 3584 OPR100T
                                        3: 3520 OPW100T, 3520 OPR100T
                                        3: 3485 OPW100T, 3485 OPR100T
                                        3: 3584 OPW100T, 3584 OPR100T
                                        3: 3611 OPW100T, 3611 OPR100T
                                        3: 3584 OPW100T, 3584 OPR100T
                                        3: 3611 OPW100T, 3611 OPR100T
                                        3: 3584 OPW100T, 3584 OPR100T
                                        3: 3584 OPW100T, 3584 OPR100T
                                        3: 3611 OPW100T, 3611 OPR100T
                                        3: 3584 OPW100T, 3584 OPR100T
                                        3: 3865 OPW100T, 3865 OPR100T
                                        3: 3520 OPW100T, 3520 OPR100T
Termino iobench 3: total ops 132992u -->        Priority:0  
Times executed:224184  
Last time executed:26587  
```

| Tabla de caracterizacion de experimentos| 1 io-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | 10 veces más pequeño |
| Politica scheduler | RR |
| Caso | 1 io-bench |
|.            |           |
| Promedio OPW/100T | 3485 OPW/100T|
| Promedio OPR/100T | 3485 OPW/100T|
|.            |           |
| Cant. select IO (Times executed) | 224184 |
| Cant. select 1er CPU | - |
| Cant. select 2do CPU | - |

**Observacion:** Se ejecuto la misma cantidad de veces que con un quanto 10 veces mayor

### Prueba 2:

Datos:
```sh
4: 694 MFLOP100T
4: 700 MFLOP100T
4: 712 MFLOP100T
4: 712 MFLOP100T
4: 700 MFLOP100T
4: 700 MFLOP100T
4: 719 MFLOP100T
4: 700 MFLOP100T
4: 688 MFLOP100T
4: 694 MFLOP100T
4: 712 MFLOP100T
4: 694 MFLOP100T
4: 706 MFLOP100T
4: 712 MFLOP100T
4: 719 MFLOP100T
4: 706 MFLOP100T
4: 688 MFLOP100T
Termino cpubench 4: total ops 805306368u -->
Priority:0
Times executed:21034
Last time executed:73051
```


| Tabla de caracterizacion de experimentos| 1 cpu-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | 10 veces más pequeño |
| Politica scheduler | RR |
| Caso | 1 cpu-bench |
|.            |           |
| Promedio MFLOPS/100T | 703 MFLOPS/100T|
|.            |           |
| Cant. select 1er CPU | 21034 |

**Observacion:** Esta vez el orden de cantidad de veces que fue elegido el proceso aumento por 10 a diferencia del iobench

### Prueba 3:

Datos:
```
5: 644 MFLOP100T

                                        7: 396 OPW100T, 396 OPR100T
                                        7: 333 OPW100T, 333 OPR100T
5: 639 MFLOP100T

                                        7: 333 OPW100T, 333 OPR100T
5: 660 MFLOP100T

                                        7: 333 OPW100T, 333 OPR100T
5: 654 MFLOP100T

                                        7: 336 OPW100T, 336 OPR100T
5: 654 MFLOP100T

                                        7: 333 OPW100T, 333 OPR100T
5: 660 MFLOP100T

                                        7: 333 OPW100T, 333 OPR100T
5: 649 MFLOP100T

                                        7: 336 OPW100T, 336 OPR100T
5: 660 MFLOP100T

                                        7: 333 OPW100T, 333 OPR100T
5: 660 MFLOP100T

                                        7: 333 OPW100T, 333 OPR100T
5: 665 MFLOP100T

                                        7: 336 OPW100T, 336 OPR100T
5: 654 MFLOP100T

                                        7: 333 OPW100T, 333 OPR100T
5: 660 MFLOP100T

                                        7: 333 OPW100T, 333 OPR100T
5: 660 MFLOP100T

                                        7: 331 OPW100T, 331 OPR100T
5: 665 MFLOP100T

                                        7: 333 OPW100T, 333 OPR100T
5: 639 MFLOP100T

                                        7: 333 OPW100T, 333 OPR100T
                                        7: 331 OPW100T, 331 OPR100T
5: 624 MFLOP100T  
Termino cpubench 5: total ops 0u -->            Priority:0  
Times executed:21007  
Last time executed:123776  
Termino iobench 7: total ops 1318$ 4u -->       Priority:0  
Times executed:21006  
Last time executed:123782  
```

| Tabla de caracterizacion de experimentos| 1 io-bench y 1 cpu-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | 10 veces más pequeño |
| Politica scheduler | RR |
| Caso | 1 io-bench y 1 cpu-bench |
|.            |           |
| Promedio MFLOPS/100T | 652 MFLOPS/100T|
| Promedio OPW/100T |  337  OPW/100T|
| Promedio OPR/100T |  337  OPW/100T|
|.            |           |
| Cant. select IO (Times executed) | 21006 |
| Cant. select 1er CPU | 21007 |

### Prueba 4:

Datos:
```
5: 665 MFLOP100T
                                        3: 386 OPW100T, 386 OPR100T
5: 665 MFLOP100T
                                        3: 336 OPW100T, 336 OPR100T
5: 654 MFLOP100T
                                        3: 333 OPW100T, 333 OPR100T
                                        3: 336 OPW100T, 336 OPR100T
5: 660 MFLOP100T
                                        3: 333 OPW100T, 333 OPR100T
5: 654 MFLOP100T
                                        3: 333 OPW100T, 333 OPR100T
5: 682 MFLOP100T
                                        3: 333 OPW100T, 333 OPR100T
5: 639 MFLOP100T
                                        3: 325 OPW100T, 325 OPR100T
5: 644 MFLOP100T
                                        3: 333 OPW100T, 333 OPR100T
5: 649 MFLOP100T
                                        3: 333 OPW100T, 333 OPR100T
5: 671 MFLOP100T
                                        3: 328 OPW100T, 328 OPR100T
5: 649 MFLOP100T
                                        3: 336 OPW100T, 336 OPR100T
5: 676 MFLOP100T
                                        3: 333 OPW100T, 333 OPR100T
5: 671 MFLOP100T
                                        3: 333 OPW100T, 333 OPR100T
5: 665 MFLOP100T
                                        3: 336 OPW100T, 336 OPR100T
5: 639 MFLOP100T
                                        3: 322 OPW100T, 322 OPR100T
5: 592 MFLOP100T
                                        3: 325 OPW100T, 325 OPR100T
Termino iobench 3: total ops 13184u --> Priority:0
Times executed:20841
Last time executed:29721
$ Termino cpubench 5: total ops 4093640704u --> Priority:0
Times executed:21201
Last time executed:29956
```

| Tabla de caracterizacion de experimentos| 1 cpu-bench y 1 io-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | 10 veces más pequeño |
| Politica scheduler | RR |
| Caso | 1 cpu-bench y 1 io-bench |
|.            |           |
| Promedio MFLOPS/100T | 653 MFLOPS/100T|
| Promedio OPW/100T |  334  OPW/100T|
| Promedio OPR/100T |  334  OPW/100T|
|.            |           |
| Cant. select IO (Times executed) | 20841 |
| Cant. select 1er CPU | 21201 |


### Prueba 5:

Datos:
```
11: 350 MFLOP100T
9: 327 MFLOP100T
                                        7: 222 OPW100T, 222 OPR100T
11: 353 MFLOP100T
9: 330 MFLOP100T
                                        7: 166 OPW100T, 166 OPR100T
11: 369 MFLOP100T
9: 394 MFLOP100T
                                        7: 168 OPW100T, 168 OPR100T
11: 906 MFLOP100T
9: 838 MFLOP100T
                                        7: 166 OPW100T, 166 OPR100T
11: 890 MFLOP100T
9: 831 MFLOP100T
                                        7: 166 OPW100T, 166 OPR100T
11: 883 MFLOP100T
9: 831 MFLOP100T
                                        7: 166 OPW100T, 166 OPR100T
11: 890 MFLOP100T
9: 831 MFLOP100T
                                        7: 166 OPW100T, 166 OPR100T
11: 898 MFLOP100T
9: 831 MFLOP100T
                                        7: 168 OPW100T, 168 OPR100T
11: 883 MFLOP100T
                                        7: 166 OPW100T, 166 OPR100T
9: 825 MFLOP100T
11: 875 MFLOP100T
9: 805 MFLOP100T
                                        7: 166 OPW100T, 166 OPR100T
11: 867 MFLOP100T
9: 825 MFLOP100T
                                        7: 166 OPW100T, 166 OPR100T
11: 890 MFLOP100T
9: 831 MFLOP100T
                                        7: 166 OPW100T, 166 OPR100T
11: 890 MFLOP100T
                                        7: 166 OPW100T, 166 OPR100T
9: 838 MFLOP100T
11: 890 MFLOP100T
                                        7: 166 OPW100T, 166 OPR100T
9: 831 MFLOP100T
11: 898 MFLOP100T
                                        7: 168 OPW100T, 168 OPR100T
9: 853 MFLOP100T
11: 898 MFLOP100T
                                        7: 166 OPW100T, 166 OPR100T
9: 845 MFLOP100T
11: 906 MFLOP100T
                                        7: 166 OPW100T, 166 OPR100T
Termino cpubench 11: total ops 2415919104u --> Priority:0
Times executed:10528
Last time executed:67601
Termino cpubench 9: total ops 1207959552u --> Priority:0
Times executed:10554
Last time executed:67625
Termino iobench 7: total ops 6656u -->  Priority:0
Times executed:10630
Last time executed:67634
```

| Tabla de caracterizacion de experimentos| 1 cpu-bench 1 cpu-bench y 1 io-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | 10 veces más pequeño |
| Politica scheduler | RR |
| Caso | 1 cpu-bench 1 cpu-bench y 1 io-bench |
|.            |           |
| Promedio MFLOPS/100T 1er cpubench (proceso 11)| 790 MFLOPS/100T|
| Promedio MFLOPS/100T 2do cpubench (proceso 9)| 741 MFLOPS/100T|
| Promedio OPW/100T |  169  OPW/100T|
| Promedio OPR/100T |  169  OPW/100T|
|.            |           |
| Cant. select IO (Times executed) | 10630 |
| Cant. select 1er CPU | 10528 |
| Cant. select 2do CPU | 10554 |

## Repito pruebas de la parte 2 con la implementacion de MLFQ

### Prueba 1:

Datos:
```
3: 706 MFLOP100T
3: 654 MFLOP100T
3: 654 MFLOP100T
3: 660 MFLOP100T
3: 660 MFLOP100T
3: 665 MFLOP100T
3: 706 MFLOP100T
3: 671 MFLOP100T
3: 649 MFLOP100T
3: 654 MFLOP100T
3: 644 MFLOP100T
3: 665 MFLOP100T
3: 629 MFLOP100T
3: 654 MFLOP100T
3: 654 MFLOP100T
3: 649 MFLOP100T
Termino cpubench 3: total ops 0u --> Priority : 0, Times executed : 2115, Last time executed : 2148
```

| Tabla de caracterizacion de experimentos| 1 cpu-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | Normal |
| Politica scheduler | MLFQ |
| Caso | 1 cpu-bench|
|.            |           |
| Promedio MFLOPS/100T 1er cpubench| 660  MFLOPS/100T|
|.            |           |
| Cant. select 1er CPU | 2115 |


### Prueba 2:

Datos:
```
                                        4: 2978 OPW100T, 2978 OPR100T
                                        4: 2914 OPW100T, 2914 OPR100T
                                        4: 2635 OPW100T, 2635 OPR100T
                                        4: 2661 OPW100T, 2661 OPR100T
                                        4: 2944 OPW100T, 2944 OPR100T
                                        4: 3104 OPW100T, 3104 OPR100T
                                        4: 3104 OPW100T, 3104 OPR100T
                                        4: 3072 OPW100T, 3072 OPR100T
                                        4: 3136 OPW100T, 3136 OPR100T
                                        4: 3231 OPW100T, 3231 OPR100T
                                        4: 3136 OPW100T, 3136 OPR100T
                                        4: 3168 OPW100T, 3168 OPR100T
                                        4: 3072 OPW100T, 3072 OPR100T
                                        4: 3015 OPW100T, 3015 OPR100T
                                        4: 3008 OPW100T, 3008 OPR100T
                                        4: 3200 OPW100T, 3200 OPR100T
                                        4: 3104 OPW100T, 3104 OPR100T
                                        4: 3200 OPW100T, 3200 OPR100T
                                        4: 3168 OPW100T, 3168 OPR100T
Termino iobench 4: total ops 116608u -->        
Priority : 2,
Times executed : 191092, 
Last time executed : 4338
```

| Tabla de caracterizacion de experimentos| 1 io-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | Normal|
| Politica scheduler | MLFQ |
| Caso |  1 io-bench |
|.            |           |
| Promedio OPW/100T |  3044  OPW/100T|
| Promedio OPR/100T |  3044  OPW/100T|
|.            |           |
| Cant. select IO (Times executed) | 191092 |

### Prueba 3:

Datos:
```
5: 649 MFLOP100T
5: 688 MFLOP100T
5: 654 MFLOP100T
                                        7: 62 OPW100T, 62 OPR100T
5: 649 MFLOP100T
                                        7: 33 OPW100T, 33 OPR100T
5: 665 MFLOP100T
5: 654 MFLOP100T
                                        7: 33 OPW100T, 33 OPR100T
5: 671 MFLOP100T
                                        7: 33 OPW100T, 33 OPR100T
5: 665 MFLOP100T
5: 665 MFLOP100T
                                        7: 34 OPW100T, 34 OPR100T
5: 660 MFLOP100T
5: 660 MFLOP100T
                                        7: 33 OPW100T, 33 OPR100T
5: 671 MFLOP100T
                                        7: 33 OPW100T, 33 OPR100T
5: 660 MFLOP100T
5: 671 MFLOP100T
                                        7: 33 OPW100T, 33 OPR100T
5: 660 MFLOP100T
                                        7: 33 OPW100T, 33 OPR100T
5: 688 MFLOP100T
Termino cpubench 5: total ops 0u --> Priority : 0, Times executed : 2129, Last time executed : 6604                    7: 33 OPW100T, 33 OPR100T
Termino iobench 7: total ops 1408u -->  Priority : 2, Times executed : 2224, Last time executed : 6605
```

| Tabla de caracterizacion de experimentos| 1 io-bench y 1 cpu-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | Normal |
| Politica scheduler | MLFQ |
| Caso | 1 io-bench y 1 cpu-bench |
|.            |           |
| Promedio MFLOPS/100T 1er cpubench| 664 MFLOPS/100T|
| Promedio OPW/100T |  36  OPW/100T|
| Promedio OPR/100T |  36  OPW/100T|
|.            |           |
| Cant. select IO (Times executed) | 2224 |
| Cant. select 1er CPU (Proceso 5) | 2129 |

### Prueba 4:

Datos:
```
10: 665 MFLOP100T
10: 688 MFLOP100T
10: 676 MFLOP100T
                                        8: 62 OPW100T, 62 OPR100T
10: 694 MFLOP100T
                                        8: 33 OPW100T, 33 OPR100T
10: 688 MFLOP100T
10: 676 MFLOP100T
                                        8: 33 OPW100T, 33 OPR100T
10: 688 MFLOP100T
10: 676 MFLOP100T
                                        8: 33 OPW100T, 33 OPR100T
10: 665 MFLOP100T
                                        8: 33 OPW100T, 33 OPR100T
10: 682 MFLOP100T
10: 671 MFLOP100T
                                        8: 33 OPW100T, 33 OPR100T
10: 676 MFLOP100T
                                        8: 33 OPW100T, 33 OPR100T
10: 665 MFLOP100T
10: 694 MFLOP100T
                                        8: 33 OPW100T, 33 OPR100T
10: 682 MFLOP100T
10: 688 MFLOP100T
                                        8: 33 OPW100T, 33 OPR100T
Termino cpubench 10: total ops 0u --> Priority : 0, Times executed : 2111, Last time executed : 9318                   8: 33 OPW100T, 33 OPR100T
Termino iobench 8: total ops 1408u -->  Priority : 1, Times executed : 2226, Last time executed : 9320
```

| Tabla de caracterizacion de experimentos| 1 cpu-bench y 1 io-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | Normal |
| Politica scheduler | MLFQ |
| Caso | 1 cpu-bench y 1 io-bench |
|.            |           |
| Promedio MFLOPS/100T 1er cpubench| 679 MFLOPS/100T|
| Promedio OPW/100T |  36  OPW/100T|
| Promedio OPR/100T |  36  OPW/100T|
|.            |           |
| Cant. select IO (Times executed) | 2226 |
| Cant. select 1er CPU (Proceso 10) | 2111 |

### Prueba 5:

Datos:
```
13: 564 MFLOP100T
15: 525 MFLOP100T
13: 845 MFLOP100T
15: 845 MFLOP100T
13: 853 MFLOP100T
15: 838 MFLOP100T
                                        11: 60 OPW100T, 60 OPR100T
13: 838 MFLOP100T
15: 838 MFLOP100T
13: 825 MFLOP100T
                                        11: 33 OPW100T, 33 OPR100T
15: 838 MFLOP100T
13: 825 MFLOP100T
15: 825 MFLOP100T
                                        11: 33 OPW100T, 33 OPR100T
15: 860 MFLOP100T
13: 853 MFLOP100T
13: 860 MFLOP100T
15: 853 MFLOP100T
                                        11: 33 OPW100T, 33 OPR100T
13: 845 MFLOP100T
15: 853 MFLOP100T
                                        11: 33 OPW100T, 33 OPR100T
13: 845 MFLOP100T
15: 838 MFLOP100T
15: 860 MFLOP100T
13: 853 MFLOP100T
                                        11: 33 OPW100T, 33 OPR100T
15: 853 MFLOP100T
13: 845 MFLOP100T
                                        11: 33 OPW100T, 33 OPR100T
15: 811 MFLOP100T
13: 811 MFLOP100T
15: 831 MFLOP100T
13: 838 MFLOP100T
                                        11: 33 OPW100T, 33 OPR100T
15: 818 MFLOP100T
13: 798 MFLOP100T
15: 825 MFLOP100T
13: 825 MFLOP100T
                                        11: 33 OPW100T, 33 OPR100T
Termino cpubench 15: total ops 2818572288u --> Priority : 0,
Times executed : 1057, Last time executed : 13201
Termino cpubench 13: total ops 2818572288u --> Priority : 0,
Times executed : 1060, Last time executed : 13203               
11: 33 OPW100T, 33 OPR100T
Termino iobench 11: total ops 1408u --> Priority : 2, 
Times executed : 2226, 
Last time executed : 13205
```

| Tabla de caracterizacion de experimentos| 1 cpu-bench 1 cpu-bench y 1 io-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | Normal |
| Politica scheduler | MLFQ |
| Caso | 1 cpu-bench 1 cpu-bench y 1 io-bench |
|.            |           |
| Promedio MFLOPS/100T 1er cpubench (13)| 819 MFLOPS/100T|
| Promedio MFLOPS/100T 2do cpubench (15)| 818 MFLOPS/100T|
| Promedio OPW/100T |  36  OPW/100T|
| Promedio OPR/100T |  36  OPW/100T|
|.            |           |
| Cant. select IO (Times executed) | 2226 |
| Cant. select 1er CPU (Proceso 13) | 1057 |
| Cant. select 1er CPU (Proceso 15) | 1060 |

## Repito pruebas con quanto 10 veces menor

### Prueba 1:

Datos:
```
3: 639 MFLOP100T
3: 603 MFLOP100T
3: 525 MFLOP100T
3: 634 MFLOP100T
3: 644 MFLOP100T
3: 644 MFLOP100T
3: 634 MFLOP100T
3: 597 MFLOP100T
3: 603 MFLOP100T
3: 629 MFLOP100T
3: 619 MFLOP100T
3: 624 MFLOP100T
3: 649 MFLOP100T
3: 649 MFLOP100T
3: 614 MFLOP100T
3: 639 MFLOP100T
Termino cpubench 3: total ops 3489660928u --> Priority : 0, Times executed : 20971, Last time executed : 21230
```

| Tabla de caracterizacion de experimentos| 1 cpu-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | 10 veces menor |
| Politica scheduler | MLFQ |
| Caso | 1 cpu-bench|
|.            |           |
| Promedio MFLOPS/100T 1er cpubench| 621  MFLOPS/100T|
|.            |           |
| Cant. select 1er CPU | 20971 |


### Prueba 2:

Datos:
```
                                        4: 3231 OPW100T, 3231 OPR100T
                                        4: 3008 OPW100T, 3008 OPR100T
                                        4: 3136 OPW100T, 3136 OPR100T
                                        4: 3168 OPW100T, 3168 OPR100T
                                        4: 2816 OPW100T, 2816 OPR100T
                                        4: 3104 OPW100T, 3104 OPR100T
                                        4: 3358 OPW100T, 3358 OPR100T
                                        4: 3264 OPW100T, 3264 OPR100T
                                        4: 3231 OPW100T, 3231 OPR100T
                                        4: 3295 OPW100T, 3295 OPR100T
                                        4: 3328 OPW100T, 3328 OPR100T
                                        4: 3264 OPW100T, 3264 OPR100T
                                        4: 3328 OPW100T, 3328 OPR100T
                                        4: 3328 OPW100T, 3328 OPR100T
                                        4: 3264 OPW100T, 3264 OPR100T
                                        4: 3231 OPW100T, 3231 OPR100T
                                        4: 3200 OPW100T, 3200 OPR100T
                                        4: 3328 OPW100T, 3328 OPR100T
                                        4: 3295 OPW100T, 3295 OPR100T
Termino iobench 4: total ops 122880u -->        Priority : 1, Times executed : 207932, Last time executed : 43474
```

| Tabla de caracterizacion de experimentos| 1 io-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | 10 veces menor|
| Politica scheduler | MLFQ |
| Caso |  1 io-bench |
|.            |           |
| Promedio OPW/100T |  3067  OPW/100T|
| Promedio OPR/100T |  3067  OPW/100T|
|.            |           |
| Cant. select IO (Times executed) | 207932 |


### Prueba 3:

Datos:
```
5: 610 MFLOP100T
                                        7: 386 OPW100T, 386 OPR100T
                                        7: 333 OPW100T, 333 OPR100T
5: 614 MFLOP100T
                                        7: 333 OPW100T, 333 OPR100T
5: 624 MFLOP100T
                                        7: 336 OPW100T, 336 OPR100T
5: 624 MFLOP100T
                                        7: 333 OPW100T, 333 OPR100T
5: 624 MFLOP100T
                                        7: 333 OPW100T, 333 OPR100T
5: 634 MFLOP100T
                                        7: 336 OPW100T, 336 OPR100T
5: 603 MFLOP100T
                                        7: 331 OPW100T, 331 OPR100T
5: 654 MFLOP100T
                                        7: 336 OPW100T, 336 OPR100T
5: 614 MFLOP100T
                                        7: 333 OPW100T, 333 OPR100T
5: 619 MFLOP100T
                                        7: 333 OPW100T, 333 OPR100T
5: 654 MFLOP100T
                                        7: 333 OPW100T, 333 OPR100T
                                        7: 333 OPW100T, 333 OPR100T
5: 619 MFLOP100T
                                        7: 336 OPW100T, 336 OPR100T
5: 634 MFLOP100T
                                        7: 333 OPW100T, 333 OPR100T
5: 644 MFLOP100T
                                        7: 333 OPW100T, 333 OPR100T
5: 634 MFLOP100T
                                        7: 336 OPW100T, 336 OPR100T
Termino iobench 7: total ops 13184u --> Priority : 1, Times executed : 21033, Last time executed : 75605Termino cpubench 5: total ops 3288334336u --> Priority : 0, Times executed : 21196, Last time executed : 75749
```

| Tabla de caracterizacion de experimentos| 1 io-bench y 1 cpu-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | 10 veces más pequeño |
| Politica scheduler | MLFQ |
| Caso | 1 io-bench y 1 cpu-bench |
|.            |           |
| Promedio MFLOPS/100T | 627 MFLOPS/100T|
| Promedio OPW/100T |  306  OPW/100T|
| Promedio OPR/100T |  306  OPW/100T|
|.            |           |
| Cant. select IO (Times executed) | 21196 |
| Cant. select 1er CPU | 21033 |


### Prueba 4:

Datos:
```
10: 634 MFLOP100T
                                        8: 386 OPW100T, 386 OPR100T
                                        8: 333 OPW100T, 333 OPR100T
10: 614 MFLOP100T
                                        8: 333 OPW100T, 333 OPR100T
10: 634 MFLOP100T
                                        8: 333 OPW100T, 333 OPR100T
10: 649 MFLOP100T
                                        8: 331 OPW100T, 331 OPR100T
10: 619 MFLOP100T
                                        8: 331 OPW100T, 331 OPR100T
10: 614 MFLOP100T
                                        8: 331 OPW100T, 331 OPR100T
10: 634 MFLOP100T
                                        8: 331 OPW100T, 331 OPR100T
10: 624 MFLOP100T
                                        8: 333 OPW100T, 333 OPR100T
10: 624 MFLOP100T
                                        8: 333 OPW100T, 333 OPR100T
10: 629 MFLOP100T
                                        8: 333 OPW100T, 333 OPR100T
                                        8: 336 OPW100T, 336 OPR100T
10: 639 MFLOP100T
                                        8: 333 OPW100T, 333 OPR100T
10: 660 MFLOP100T
                                        8: 333 OPW100T, 333 OPR100T
10: 624 MFLOP100T
                                        8: 333 OPW100T, 333 OPR100T
10: 619 MFLOP100T
                                        8: 336 OPW100T, 336 OPR100T
10: 610 MFLOP100T
                                        8: 331 OPW100T, 331 OPR100T
Termino iobench 8: total ops 13184u --> Priority : 1, Times executed : 21036, Last time executed : 103454$ Termino cpubench 10: total ops 3489660928u --> Priority : 0, Times executed : 21138, Last time executed : 103487
```

| Tabla de caracterizacion de experimentos| 1 cpu-bench y 1 io-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | 10 veces menor |
| Politica scheduler | MLFQ |
| Caso | 1 cpu-bench y 1 io-bench |
|.            |           |
| Promedio MFLOPS/100T 1er cpubench| 628 MFLOPS/100T|
| Promedio OPW/100T |  335  OPW/100T|
| Promedio OPR/100T |  335  OPW/100T|
|.            |           |
| Cant. select IO (Times executed) | 21036 |
| Cant. select 1er CPU (Proceso 10) | 21138 |


### Prueba 5:

Datos:
```
16: 554 MFLOP100T
14: 554 MFLOP100T
                                        12: 382 OPW100T, 382 OPR100T
16: 781 MFLOP100T
14: 781 MFLOP100T
                                        12: 331 OPW100T, 331 OPR100T
16: 766 MFLOP100T
14: 774 MFLOP100T
                                        12: 333 OPW100T, 333 OPR100T
16: 774 MFLOP100T
14: 766 MFLOP100T
                                        12: 333 OPW100T, 333 OPR100T
16: 789 MFLOP100T
14: 774 MFLOP100T
                                        12: 328 OPW100T, 328 OPR100T
16: 789 MFLOP100T
14: 774 MFLOP100T
                                        12: 333 OPW100T, 333 OPR100T
16: 781 MFLOP100T
14: 781 MFLOP100T
                                        12: 333 OPW100T, 333 OPR100T
16: 818 MFLOP100T
14: 811 MFLOP100T
                                        12: 333 OPW100T, 333 OPR100T
16: 831 MFLOP100T
14: 825 MFLOP100T
                                        12: 333 OPW100T, 333 OPR100T
16: 781 MFLOP100T
14: 774 MFLOP100T
                                        12: 331 OPW100T, 331 OPR100T
16: 766 MFLOP100T
14: 774 MFLOP100T
16: 805 MFLOP100T
                                        12: 333 OPW100T, 333 OPR100T
14: 781 MFLOP100T
16: 805 MFLOP100T
14: 805 MFLOP100T
                                        12: 333 OPW100T, 333 OPR100T
16: 766 MFLOP100T
14: 774 MFLOP100T
                                        12: 333 OPW100T, 333 OPR100T
16: 781 MFLOP100T
14: 774 MFLOP100T
                                        12: 333 OPW100T, 333 OPR100T
16: 774 MFLOP100T
14: 759 MFLOP100T
                                        12: 331 OPW100T, 331 OPR100T
16: 789 MFLOP100T
14: 774 MFLOP100T
                                        12: 333 OPW100T, 333 OPR100T
16: 781 MFLOP100T
14: 759 MFLOP100T
                                        12: 331 OPW100T, 331 OPR100T
16: 805 MFLOP100T
Termino iobench 12: total ops 13184u -->        Priority : 1, Times executed : 20853, Last time executed : 154069$ Termino cpubench 14: total ops 1811939328u --> Priority : 0, Times executed : 10524, Last time executed : 154168Termino cpubench 16: total ops 2617245696u --> Priority : 0, Times executed : 10573, Last time executed : 154218
```

| Tabla de caracterizacion de experimentos| 1 cpu-bench 1 cpu-bench y 1 io-bench|
| ------ | ------ |
| Hardware | Intel Core i3-10100F, S-1200, 3.60GHz with Radeon RX580 4gb |
| Software | Qemu version 6.2.0 |
| Quantum | 10 veces menor |
| Politica scheduler | MLFQ |
| Caso | 1 cpu-bench 1 cpu-bench y 1 io-bench |
|.            |           |
| Promedio MFLOPS/100T 1er cpubench (16)| 775 MFLOPS/100T|
| Promedio MFLOPS/100T 2do cpubench (14)| 765 MFLOPS/100T|
| Promedio OPW/100T |  335  OPW/100T|
| Promedio OPR/100T |  335  OPW/100T|
|.            |           |
| Cant. select IO (Times executed) | 20853 |
| Cant. select 1er CPU (Proceso 16) | 10524 |
| Cant. select 1er CPU (Proceso 14) | 10573 |

### Conclusiones de los experimentos realizados:

A la hora realizar los test de RR se observa claramente que se reparten los recursos independientemente del tipo de proceso, lo que puede producir muchos problemas a la hora de recibir un feedback al usar inputs/outputs (sensacion de lentitud al realizar acciones en la pc) para esto es que surge la idea de MLFQ (Multilevel feedback queue) que implementa una prioridad en los procesos que liberan el cpu (normalmente por espera de inputs/outputs) que es lo que implementamos al modificar las prioridades en funciones como yield y hacer eleccion del proceso con mas prioridad dentro del scheduler.

En los experimentos (principalmente en el ultimo) se observa que la cantidad de veces que es elegido iobench duplica a ambos cpubench y ambos terminan con una prioridad menor a la de iobench.
Con la implementacion actual existe una alta posibilidad de que haya procesos sufriendo starvation por la falta de implementacion de un priority boost que suba la prioridad de los procesos que llevan mucho tiempo sin ser ejecutados, o bien un priority boost que reinicie todas las prioridades. Ambos son enfoques validos para resolver el problema de starvation (Procesos que quedan con la prioridad minima y nunca llegan a ser ejecutados porque hay procesos con alta prioridad monopolizando el cpu).

## Punto estrella*
Implementamos un priority boost en el scheduler el cual mejora la prioridad de los procesos mediante una actualizacion de las prioridades cada cierto tiempo (En este caso cada 1000 ticks).


   [dill]: <https://github.com/joemccann/dillinger>
   [git-repo-url]: <https://github.com/joemccann/dillinger.git>
   [john gruber]: <http://daringfireball.net>
   [df1]: <http://daringfireball.net/projects/markdown/>
   [markdown-it]: <https://github.com/markdown-it/markdown-it>
   [Ace Editor]: <http://ace.ajax.org>
   [node.js]: <http://nodejs.org>
   [Twitter Bootstrap]: <http://twitter.github.com/bootstrap/>
   [jQuery]: <http://jquery.com>
   [@tjholowaychuk]: <http://twitter.com/tjholowaychuk>
   [express]: <http://expressjs.com>
   [AngularJS]: <http://angularjs.org>
   [Gulp]: <http://gulpjs.com>

   [PlDb]: <https://github.com/joemccann/dillinger/tree/master/plugins/dropbox/README.md>
   [PlGh]: <https://github.com/joemccann/dillinger/tree/master/plugins/github/README.md>
   [PlGd]: <https://github.com/joemccann/dillinger/tree/master/plugins/googledrive/README.md>
   [PlOd]: <https://github.com/joemccann/dillinger/tree/master/plugins/onedrive/README.md>
   [PlMe]: <https://github.com/joemccann/dillinger/tree/master/plugins/medium/README.md>
   [PlGa]: <https://github.com/RahulHP/dillinger/blob/master/plugins/googleanalytics/README.md>

