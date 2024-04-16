#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#include <stddef.h>

#define ERROR 0
#define SUCCESS 1

#define INIT_SEM_VALUE 0

int sem_ping;
int sem_pong;

static void ping_proc(int n_pings)
{
  for (int i = 0; i < n_pings; i++)
  {
    sem_down(sem_ping);
    printf("ping\n");
    sem_up(sem_pong);
  }
}

static void pong_proc(int n_pongs)
{
  for (int i = 0; i < n_pongs; i++)
  {
    sem_down(sem_pong);
    printf("\tpong\n");
    sem_up(sem_ping);
  }
}

int main(int argc, char *argv[])
{
  int n_pingpong;

  // Invalid args
  if (argc != 2)
  {
    fprintf(2, "ERROR:Too few arguments\n");
    exit(ERROR);
  }
  // Parseo argumentos
  n_pingpong = atoi(argv[1]);
  if (n_pingpong < 1)
  {
    printf("ERROR:Argument must be greater than 0\n");
    exit(ERROR);
  }
  int i = 1;
  int j = 2;
  sem_pong = sem_open(i, INIT_SEM_VALUE);
  sem_ping = sem_open(j, INIT_SEM_VALUE + 1);
  while (sem_pong != i && i < 32)
  {
    ++i;  
    sem_pong = sem_open(i, INIT_SEM_VALUE);
  }
  while (sem_ping != j && j < 32)
  {
    ++j;  
    sem_ping = sem_open(j, INIT_SEM_VALUE + 1);
  }
  if ((sem_pong != i) && (sem_ping != j))
  {
      printf("ERROR : semaphore cannot be initialized \n");
      exit(0);
  }
  
  int rc = fork();
  if (rc < 0)
  {
    printf("ERROR:fork failure");
  }
  else if (rc == 0)
  {
    ping_proc(n_pingpong);
  }
  else
  {
    pong_proc(n_pingpong);
  }

  sem_close(sem_ping);
  sem_close(sem_pong);

  exit(SUCCESS);
}
