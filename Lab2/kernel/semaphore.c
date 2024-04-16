#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

// Variables globales
#define sem_max 32
/* Acquire -> Pone una cerradura al proceso acquire(&p->lock)
 *  Release -> Libera la cerradura del proceso release(&p->lock)
 *  Wakeup  -> Despierta un proceso puesto a dormir por sleep
 *  Sleep   -> Detiene un proceso poniendolo a dormir
 *  Argint  -> Permite obtener un argumento dado por una syscall
 */

/*------------------------------------------Estructuras---------------------------------------------*/
// Estructura semaforo
struct semaphore
{
    int resources;
    int active;
    struct spinlock lk;
};

// Arreglo de semaforos
struct semaphore a[sem_max];
/*------------------------------------------------------------------------------------------------*/

/*-------Funcion para revisar si un semaforo esta usado y si devolver el siguiente semaforo-------*/

/*------------------------------------------------------------------------------------------------*/

/*----------------------------Syscalls para inicializar/cerrar semaforos--------------------------*/


uint64 sys_sem_open(void)
{
    int sem_id;
    int resources;
    argint(0, &sem_id);    // Tomo el primer argumento y lo guardo en sem_id
    argint(1, &resources); // Tomo el segundo argumento y lo guardo en resources
    if (sem_id > sem_max)
    {
        printf("ERROR, Semaphore non-exist, max semaphore is %d\n", sem_max);
        exit(0);
    }
    if (resources < 0)
    {
        printf("ERROR, Negative value\n");
        exit(0);
    }

    if (a[sem_id].active == 1)
    {
        sem_id = 0;
    }
    else if (a[sem_id].active == 0)
    {
        initlock(&a[sem_id].lk, "semaphore"); // Inicio el juego de inicio/fin de tiempo de escritura
        acquire(&a[sem_id].lk); // Esta en proceso un cambio
        a[sem_id].active = 1;            // Si el semaforo estaba desactivado lo activo
        a[sem_id].resources = resources; // Guardo los recursos del semaforo
        release(&a[sem_id].lk);      // Termina el cambio no puedo escribir mas en memoria
    }
    return sem_id; // Retorno el semaforo
}

static uint64 closed_sem(int sem_id)
{                           // Detengo el semaforo
    acquire(&a[sem_id].lk); // Esta en proceso un cambio
    a[sem_id].active = 0;   // Desactivo el semaforo
    release(&a[sem_id].lk); // Termina el cambio no puedo escribir mas en memoria
    return sem_id;
}

uint64 sys_sem_close(void)
{
    int sem_id;         // Que semaforo debo cerrar
    argint(0, &sem_id); // Tomo el primer argumento y lo guardo en sem
    if (sem_id < 0 || sem_id > sem_max)
    {
        printf("ERROR : Invalid semaphore\n", sem_max);
        exit(0);
    }
    return closed_sem(sem_id); // Retorno la llamada a funcion que cierra el semaforo
}
/*-------------------------------------------------------------------------------------------------*/

/*----------------------------Syscalls para manejar el flujo de semaforos--------------------------*/
uint64 sys_sem_up(void)
{
    int sem_id;
    argint(0, &sem_id);
    acquire(&a[sem_id].lk);

    if (a[sem_id].resources == 0) // Si el valor del semaforo es 0 desbloqueo procesos e incremento
    {
        // Desbloquear procesos
        wakeup(&a[sem_id]);
        a[sem_id].resources += 1;
    }
    else if (a[sem_id].resources > 0)
    {
        a[sem_id].resources += 1;
    }

    
    release(&a[sem_id].lk);
    return sem_id;
}

uint64 sys_sem_down(void)
{
    int sem_id;
    argint(0, &sem_id);
    acquire(&a[sem_id].lk);

    while (a[sem_id].resources == 0)
    {
        sleep(&a[sem_id], &a[sem_id].lk);
    }
    if (a[sem_id].resources >= 1) // Si el valor del semaforo es mayo 0 decremento
    {
        // Decremento recursos
        a[sem_id].resources -= 1;
    }
    else
    {
        a[sem_id].resources = 0;
    }

    release(&a[sem_id].lk);
    return sem_id;
}
/*-------------------------------------------------------------------------------------------------*/
