#ifndef REV_FRONT_H
#define REV_FRONT_H

#include <stdio.h>

#include "ast.h"
#include "var_list.h"

/**
 * @brief Генерирует текст PhysLab по загруженному AST.
 *
 * @param root Корень AST, полученный из load_ast_from_file().
 * @param vars Указатель на заполненный VarList.
 * @param out  Открытый поток вывода; не закрывается внутри.
 * @return 0 при успехе, -1 при ошибке записи/аргументов.
 */
int emit_program(NODE_T *root, varlist::VarList *vars, FILE *out);

#endif // REV_FRONT_H
