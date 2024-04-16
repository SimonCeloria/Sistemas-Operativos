#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "tests/syscall_mock.h"
#include "builtin.h"
#include "command.h"
#include "strextra.h"
#include "execute.h"

typedef int fd_t;

// Cerrar los pipelines
static void close_all(fd_t fds[], unsigned int length)
{
    for (unsigned int i = 0; i < length * 2; i++)
    {
        close(fds[i]);
    }
}

// Ejecuta comando externo
static void exec_external(scommand cmd)
{
    assert(cmd != NULL && !scommand_is_empty(cmd));
    char *dir_in = scommand_get_redir_in(cmd);
    // Se camba stdin por el archivo de redireccion de entrada
    if (dir_in != NULL)
    {
        int file_in = open(dir_in, O_RDONLY, S_IRUSR);

        if (file_in == -1)
        {
            perror(dir_in);
            exit(EXIT_FAILURE);
        }

        int dup2_ = dup2(file_in, STDIN_FILENO);

        if (dup2_ == -1)
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        int close_ = close(file_in);

        if (close_ == -1)
        {
            perror("close");
            exit(EXIT_FAILURE);
        }
    }

    char *dir_out = scommand_get_redir_out(cmd);
    // Se cambia stdout por el archivo de redireccion de salida
    if (dir_out != NULL)
    {
        int file_out = open(dir_out, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
        if (file_out == -1)
        {
            perror(dir_in);
            exit(EXIT_FAILURE);
        }

        int dup2_ = dup2(file_out, STDOUT_FILENO);

        if (dup2_ == -1)
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        int close_ = close(file_out);

        if (close_ == -1)
        {
            perror("close");
            exit(EXIT_FAILURE);
        }
    }

    char **argv = scommand_to_argv(cmd);

    if (argv == NULL)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    execvp(argv[0], argv);

    printf("%s : Comando no encontrado\n", argv[0]);

    exit(EXIT_FAILURE);
}

// Execute command
static void execute_command(scommand cmd)
{
    assert(cmd != NULL);
    // Interno
    if (builtin_is_internal(cmd))
    {
        builtin_run(cmd);
        exit(EXIT_SUCCESS);
    }
    // Extrno
    else if (!scommand_is_empty(cmd))
    {
        exec_external(cmd);
    }
    // Vacio
    else
    {
        exit(EXIT_SUCCESS);
    }
}

// Ejecutar scommand
static void execute_scommand(pipeline apipe)
{
    assert(pipeline_length(apipe) == 1 && apipe != NULL);

    scommand scmd = pipeline_front(apipe);

    // Si es scmd interno
    if (builtin_is_internal(scmd))
    {
        builtin_run(scmd);
    }
    else
    {
        pid_t pid = fork();
        // En caso de fallar el fork
        if (pid < 0)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0)
        {
            exec_external(scmd);
        }
    }
}

// Ejecutar pipeline
static void execute_multiple(pipeline apipe)
{
    assert(apipe != NULL && pipeline_length(apipe) >= 2);

    unsigned int len_pipe = pipeline_length(apipe) - 1;

    fd_t *fds = malloc(sizeof(fd_t) * len_pipe * 2);
    // Falla malloc
    if (fds == NULL)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    for (unsigned int i = 0; i < len_pipe; i++)
    {
        int pipe_ = pipe(fds + i * 2);
        if (pipe_ < 0)
        {
            perror("pipe");
            close_all(fds, len_pipe);
            free(fds);
            fds = NULL;
            exit(EXIT_SUCCESS);
        }
    }
    // Crear las pipes
    unsigned int i = 0;
    bool error = false;

    // Ejecuta comandos
    while (!pipeline_is_empty(apipe) && !error)
    {
        pid_t pid = fork();
        // Falla el fork
        if (pid < 0)
        {
            perror("fork");
            error = true;
        }
        // Hilo hijo
        if (pid == 0)
        {
            if (pipeline_length(apipe) > 1)
            {
                int dup2_ = dup2(fds[i + 1], STDOUT_FILENO);
                if (dup2_ == -1)
                {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            if (i != 0)
            {
                int dup2_ = dup2(fds[i - 2], STDIN_FILENO);
                if (dup2_ == -1)
                {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            close_all(fds, len_pipe);
            execute_command(pipeline_front(apipe));
        }
        else if (pid > 0)
        {
            i = i + 2;
            pipeline_pop_front(apipe);
        }
    }
    close_all(fds, len_pipe);
    free(fds);
    fds = NULL;
}

// Ejecuta cualquier pipeline
static void execute_any_pipeline(pipeline apipe)
{
    assert(apipe != NULL);
    int pipe_length = pipeline_length(apipe);
    if (pipe_length == 1)
    {
        execute_scommand(apipe);
    }
    else if (pipe_length >= 2)
    {
        execute_multiple(apipe);
    }
}

// Ejecucion de cualquier pipeline bg o fg
void execute_pipeline(pipeline apipe)
{
    assert(apipe != NULL);
    int pipe_len = pipeline_length(apipe);

    // En caso de que se ejecute en foreground
    if (pipeline_get_wait(apipe))
    {
        execute_any_pipeline(apipe);
        while (pipe_len > 0)
        {
            wait(NULL);
            pipe_len--;
        }
    }
    // En caso de que se ejecute en background
    else
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            // Caso de que el fork falle
            perror("fork");
        }
        else if (pid == 0)
        {
            // El proceso hijo

            // Se conecta el stdin del hijo a un archivo vacio
            /* Como archivo vacio se usa una punta de lectura de pipe
               con punta de escritura cerrada */
            fd_t pipefds[2];
            int res_pipe = pipe(pipefds);
            if (res_pipe < 0)
            {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            fd_t punta_lectura = pipefds[0];
            fd_t punta_escritura = pipefds[1];

            close(punta_escritura);

            fd_t res_dup2 = dup2(punta_lectura, STDIN_FILENO);
            if (res_dup2 < 0)
            {
                perror("perror ");
                exit(EXIT_FAILURE);
            }

            // Ejecuta todos los comandos del pipeline
            execute_any_pipeline(apipe);

            // Y termina para que los hijos pasen a ser hijos del sistema
            exit(EXIT_SUCCESS);
        }
    }
}