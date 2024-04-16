#include "command.h"
#include "strextra.h"
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

struct scommand_s
{
    GList *list;
    char *input;
    char *output;
};

struct pipeline_s
{
    GList *commands;
    bool foreground; // indica si el shell debe esperar la terminación del comando antes de permitir ejecutar uno nuevo
};

// Verificar las devoluciones de los asserts que dicen result o funciones vacias

scommand scommand_new(void)
{
    scommand new_scommand;
    new_scommand = malloc(sizeof(struct scommand_s));
    new_scommand->list = NULL;
    new_scommand->input = NULL;
    new_scommand->output = NULL;
    assert(new_scommand != NULL && scommand_is_empty(new_scommand) &&
           scommand_get_redir_in(new_scommand) == NULL &&
           scommand_get_redir_out(new_scommand) == NULL);
    return new_scommand;
}

scommand scommand_destroy(scommand self)
{
    assert(self != NULL);
    g_list_free_full(self->list, free);
    free(self->input);
    free(self->output);
    self->input = NULL;
    self->output = NULL;
    free(self);
    self = NULL;
    assert(self == NULL);
    return self;
}

/* Modificadores */

void scommand_push_back(scommand self, char *argument)
{
    assert(self != NULL && argument != NULL);
    self->list = g_list_append(self->list, argument);
    assert(!scommand_is_empty(self));
}

void scommand_pop_front(scommand self)
{
    assert(self != NULL && !scommand_is_empty(self));
    GList *destroy_me = self->list;
    self->list = g_list_next(self->list);
    free(g_list_nth_data(destroy_me, 0));
    destroy_me = NULL;
}

void scommand_set_redir_in(scommand self, char *filename)
{
    assert(self != NULL);
    free(self->input);
    self->input = filename;
}

void scommand_set_redir_out(scommand self, char *filename)
{
    assert(self != NULL);
    free(self->output);
    self->output = filename;
}

/* Proyectores */

bool scommand_is_empty(const scommand self)
{
    assert(self != NULL);
    return (g_list_length(self->list) == 0);
}

unsigned int scommand_length(const scommand self)
{
    assert(self != NULL);
    unsigned int length = g_list_length(self->list);
    assert((length == 0) == (scommand_is_empty(self)));
    return length;
}

char *scommand_front(const scommand self)
{
    assert(self != NULL && !scommand_is_empty(self));
    char *front = g_list_nth_data(self->list, 0);
    assert(front != NULL);
    return front;
}

char *scommand_get_redir_in(const scommand self)
{
    assert(self != NULL);
    return self->input;
}

char *scommand_get_redir_out(const scommand self)
{
    assert(self != NULL);
    return self->output;
}

static char *scommand_next_argument(scommand self)
{
    assert(self != NULL && !scommand_is_empty(self));

    char *result = g_list_nth_data(self->list, 0);

    self->list = g_list_remove(self->list, result);

    assert(result != NULL);
    return (result);
}

char *scommand_to_string(const scommand self)
{
    assert(self != NULL);
    unsigned int length = scommand_length(self);
    char *result = strdup("");
    if (!scommand_is_empty(self))
    {
        result = str_rm(result, g_list_nth_data(self->list, 0));
        result = str_rm(result, " ");

        for (unsigned int i = 1; i < length; i++)
        {
            result = str_rm(result, g_list_nth_data(self->list, i));
            result = str_rm(result, " ");
        }
        if (self->input != NULL)
        {
            result = str_rm(result, " < ");
            result = str_rm(result, self->input);
        }
        if (self->output != NULL)
        {
            result = str_rm(result, " > ");
            result = str_rm(result, self->output);
        }
    }
    assert(scommand_is_empty(self) ||
           scommand_get_redir_in(self) == NULL ||
           scommand_get_redir_out(self) == NULL ||
           strlen(result) > 0);
    return result;
}

// PIPELINE -------------------------------------------------------------

pipeline pipeline_new(void)
{
    // reservo memoria para el nuevo pipeline
    pipeline p = malloc(sizeof(struct pipeline_s));
    p->commands = NULL;
    p->foreground = true; // esta en espera de un comando
    // asegura:
    assert(p != NULL && pipeline_is_empty(p) && pipeline_get_wait(p));
    return p;
}

// pendiente
static void scommand_destroy_self(void *self)
{
    scommand destroy = self;
    destroy = scommand_destroy(destroy);
}

pipeline pipeline_destroy(pipeline self)
{
    assert(self != NULL);
    g_list_free_full(self->commands, scommand_destroy_self);
    self->commands = NULL;
    free(self);
    self = NULL;
    assert(self == NULL);
    return self;
}

/* Modificadores */

void pipeline_push_back(pipeline self, scommand sc)
{
    assert(self != NULL && sc != NULL);

    self->commands = g_list_append(self->commands, sc);

    assert(!pipeline_is_empty(self));
}

void pipeline_pop_front(pipeline self)
{
    assert(self != NULL && !pipeline_is_empty(self));
    // Obtener el primer comando de la lista
    assert(self != NULL && !pipeline_is_empty(self));
    GList *aux = NULL;
    gpointer data_aux;
    aux = self->commands;
    self->commands = g_list_next(self->commands);
    data_aux = g_list_nth_data(aux, 0);
    g_list_free_1(data_aux);
    aux = NULL;
}

void pipeline_set_wait(pipeline self, const bool w)
{
    assert(self != NULL);
    // defino la condicion del foreground
    self->foreground = w;
}

/* Proyectores */

bool pipeline_is_empty(const pipeline self)
{
    assert(self != NULL);

    return (g_list_length(self->commands) == 0);
}

unsigned int pipeline_length(const pipeline self)
{
    assert(self != NULL);
    unsigned int length = g_list_length(self->commands);
    assert((length == 0) == pipeline_is_empty(self));
    return length;
}

scommand pipeline_front(const pipeline self)
{
    assert(self != NULL && !pipeline_is_empty(self));
    // defino cmd := primer comando
    scommand cmd = g_list_nth_data(self->commands, 0);
    assert(cmd != NULL);
    return cmd;
}

bool pipeline_get_wait(const pipeline self)
{
    assert(self != NULL);
    // consulto si tengo q esperar o no
    return self->foreground;
}

char *pipeline_to_string(const pipeline self)
{
    assert(self != NULL);
    GList *cmd = self->commands;
    char *result = NULL;
    result = strdup("");
    if (cmd != NULL)
    {
        char *fst_cmd = scommand_to_string(g_list_nth_data(cmd, 0));

        result = str_rm(result, fst_cmd);
        free(fst_cmd);

        cmd = g_list_next(cmd);
        while (cmd != NULL)
        {
            char *concat_cmd = scommand_to_string(g_list_nth_data(cmd, 0));
            assert(concat_cmd != NULL);
            result = str_rm(result, " | ");

            result = str_rm(result, concat_cmd);
            free(concat_cmd);
            cmd = g_list_next(cmd);
        }
        if (!pipeline_get_wait(self))
        {
            result = str_rm(result, " &");
        }
    }

    assert(pipeline_is_empty(self) || pipeline_get_wait(self) || strlen(result) > 0);
    return result;
}

// Auxiliares
char **scommand_to_argv(const scommand self)
{
    assert(self != NULL);
    unsigned int n = scommand_length(self);
    char **argv = malloc(sizeof(char *) * n + 1);
    for (unsigned int i = 0; i < n; i++)
    {
        char *arg = scommand_next_argument(self);
        argv[i] = arg;
    }
    argv[n] = NULL;

    assert(argv != NULL);
    return argv;
}
