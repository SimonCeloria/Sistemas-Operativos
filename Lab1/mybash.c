#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "command.h"
#include "execute.h"
#include "parser.h"
#include "parsing.h"
#include "builtin.h"

#define SIZE 1024

static void show_prompt(char *path)
{
    printf("\033[0;32m%s@Mybash:\033\033[1m\033[34m%s\033[0m$", getenv("LOGNAME"), getcwd(path, SIZE)); // Mejorable con modularizacion
    fflush(stdout);                                                                                     // Limpia el stdin (buffer de entrada)
}

int main(int argc, char *argv[])
{
    pipeline pipe;
    Parser input;
    bool quit = false;
    char *cwd = malloc(sizeof(char) * SIZE); // Alojo meoria para el path

    int c;

    input = parser_new(stdin);

    while (!quit)
    {
        getcwd(cwd, SIZE);
        show_prompt(cwd); // Muestro el prompt

        c = getchar(); // Here we bridge the terminal behavior

        if (c == EOF)
        {
            fprintf(stdout, "\nCtrl+D pressed. Exiting...\n");
            fflush(stdout);
            break;
        }

        ungetc(c, stdin); // Push the character back into stdin

        pipe = parse_pipeline(input);

        quit = parser_at_eof(input); // Consulto si el parser llego al final de archivo

        if (pipe != NULL)
        {
            // quit = quit || builtin_is_exit(pipe); // No existe builtin_is_exit pero taria bueno que existiera para si es exit salir -> Deberia revisar si el comando es exite
            execute_pipeline(pipe);
        }
        else if (!quit)
        {
            printf("Invalid command\n");
        }
        pipeline_destroy(pipe);
    }
    free(cwd); // Libero la memoria utilizada
    input = parser_destroy(input);
    return EXIT_SUCCESS;
}
