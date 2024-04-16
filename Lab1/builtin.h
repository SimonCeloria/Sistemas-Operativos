#ifndef _BUILTIN_H_
#define _BUILTIN_H_

#include <stdbool.h>

#include "command.h"


void builtin_run_cd(scommand cmd);
/*
* Ejecuta el comando cd
*/

void builtin_run_help(void);
/*
* Ejecuta el comando help
*/
void builtin_run_exit(void);
/*
* Ejecuta el comando exit
*/

bool is_a_special_path(char *path);
/*
* Verifica si el comando es un comando especial (".", "..", "/", " ", "~")
*/

bool builtin_is_internal(scommand cmd);
/*
 * Indica si el comando alojado en `cmd` es un comando interno
 *
 * REQUIRES: cmd != NULL
 *
 */


bool builtin_alone(pipeline p);
/*
 * Indica si el pipeline tiene solo un elemento y si este se corresponde a un
 * comando interno.
 *
 * REQUIRES: p != NULL
 *
 * ENSURES:
 *
 * builtin_alone(p) == pipeline_length(p) == 1 &&
 *                     builtin_is_internal(pipeline_front(p))
 *
 *
 */

void builtin_run(scommand cmd);
/*
 * Ejecuta un comando interno
 *
 * REQUIRES: {builtin_is_internal(cmd)}
 *
 */

#endif
