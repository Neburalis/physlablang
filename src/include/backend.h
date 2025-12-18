#ifndef BACKEND_H
#define BACKEND_H

#include "ast.h"

/**
 * @brief Точка входа генерации: main-тело + функции + HLT.
 */
int reverse_program(NODE_T *root, varlist::VarList *vars, FILE *out);

#endif // BACKEND_H