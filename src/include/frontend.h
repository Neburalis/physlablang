#ifndef FRONTEND_H
#define FRONTEND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ast.h"
#include "var_list.h"

typedef struct {
    const char       *name;
    char             *buf;
    size_t            buf_len;
    NODE_T           *root;
    TOKEN_T          *tokens;
    size_t            token_count;
    size_t            token_capacity;
    varlist::VarList *vars;
    bool              owns_vars,
                      owns_name,
                      owns_buf;
} FRONT_COMPL_T;

int lexer_load_file(FRONT_COMPL_T *ctx, const char *filename);
int lexer_from_buffer(FRONT_COMPL_T *ctx, const char *name, const char *text, size_t bytes);
void lexer_reset(FRONT_COMPL_T *ctx);
void dump_lexer_tokens(const FRONT_COMPL_T *ctx, const char *title);

/**
 * @brief Выполняет синтаксический разбор массива токенов и строит AST.
 * @param ctx[in,out] контекст компилятора с заполненным лексическим буфером.
 * @return 0 при успехе, -1 при ошибке.
 */
int parse_tokens(FRONT_COMPL_T *ctx);

/**
 * @brief Расширяет динамический массив токенов при необходимости.
 * @param ctx[in]       контекст компилятора.
 * @param need[in]      требуемый размер.
 * @return 0 при успехе, -1 при нехватке памяти.
 */
int ensure_token_cap(FRONT_COMPL_T *ctx, size_t need);

/**
 * @brief Создает запись о токене в массиве.
 * @param ctx[in,out]   контекст компилятора.
 * @param type[in]      тип узла.
 * @param value[in]     значение узла.
 * @param text[in]      указатель на исходную подстроку.
 * @param len[in]       длина подстроки.
 * @param line[in]      номер строки.
 * @param pos[in]       позиция в строке.
 * @return 0 при успехе, -1 при ошибке.
 */
int add_token(FRONT_COMPL_T *ctx,
    NODE_TYPE type, NODE_VALUE_T value,
    const char *text, size_t len, int32_t line, int32_t pos
);

/**
 * @brief Сохраняет текущее AST в файл в префиксной форме.
 * @param ctx[in]   контекст с построенным деревом.
 * @param path[in]  путь к файлу.
 * @return 0 при успехе, -1 при ошибке.
 */
int save_ast_to_file(const FRONT_COMPL_T *ctx, const char *path);

#endif //FRONTEND_H