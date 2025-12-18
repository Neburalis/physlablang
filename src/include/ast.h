#ifndef AST_H
#define AST_H

#include <stddef.h>
#include <stdint.h>

#include "var_list.h"

const int32_t signature = (int32_t) 0x50C0C1;

enum NODE_TYPE {
    KEYWORD_T,
    IDENTIFIER_T,
    NUMBER_T,
    LITERAL_T,
    OPERATOR_T,
    DELIMITER_T,
};

namespace KEYWORD {
    enum KEYWORD {
        LAB,
        ANNOTATION, END_ANNOTATION,
        GOAL_LITERAL,
        THEORETICAL, END_THEORETICAL,
        EXPERIMENTAL, END_EXPERIMENTAL,
        RESULTS, END_RESULTS,
        CONCLUSION, END_CONCLUSION,
        IF, ELSE, THEN,
        WHILE, DO_WHILE, WHILE_CONDITION, END_WHILE,
        FORMULA, END_FORMULA,
        VAR_DECLARATION,
        FUNC_CALL,
        RETURN,
    };
}

namespace OPERATOR {
    enum OPERATOR {
        // арифметика
        ADD, SUB, MUL, DIV,
        POW,
        LN,
        SIN, COS, TAN, CTG,
        ASIN, ACOS, ATAN, ACTG,
        SQRT,
        MOD,
        // сравнение
        EQ, NEQ,
        BELOW, ABOVE,
        BELOW_EQ, ABOVE_EQ,
        // логические
        AND, OR, NOT,
        // io
        IN, OUT,
        // graphic
        SET_PIXEL, DRAW,

        ASSIGNMENT,

        CONNECTOR,
    };
}

namespace DELIMITER {
    enum DELIMITER {
        PAR_OPEN, PAR_CLOSE,
        QUOTE, COMA, COLON,
    };
}

typedef union {
    KEYWORD::KEYWORD        keyword;  // KEYWORD_T
    size_t                  id;       // IDENTIFIER_T, LITERAL_T
    double                  num;      // NUMBER_T
    OPERATOR::OPERATOR      opr;      // OPERATOR_T
    DELIMITER::DELIMITER    delimiter;// DELIMITER_T
} NODE_VALUE_T;

typedef struct NODE_T {
    int32_t         signature;

    NODE_TYPE       type;
    NODE_VALUE_T    value;

    size_t          elements;

    NODE_T          *left, *right,
                    *parent;
} NODE_T;

typedef struct {
    NODE_T       node;
    const char  *text;
    size_t       length;
    int32_t      line;
    int32_t      pos;
    const char  *filename;
} TOKEN_T;

/**
 * @brief Выделяет новый AST-узел с обнулением.
 * @return указатель на узел или nullptr.
 */
NODE_T *alloc_new_node();
/**
 * @brief Создает AST-узел и связывает потомков.
 * @param type тип узла.
 * @param value сохраненное значение.
 * @param left левый потомок или nullptr.
 * @param right правый потомок или nullptr.
 * @return указатель на готовый узел или nullptr.
 */
NODE_T *new_node(NODE_TYPE type, NODE_VALUE_T value, NODE_T *left, NODE_T *right);
/**
 * @brief Рекурсивно освобождает поддерево.
 * @param node корень удаляемого поддерева.
 */
void destruct_node(NODE_T *node);
/**
 * @brief Проверяет наличие обоих потомков.
 * @param node проверяемый узел.
 * @return true если левый и правый существуют.
 */
bool is_leaf(const NODE_T *node);

bool is_keyword_tok(const TOKEN_T *tok, KEYWORD::KEYWORD kw);
bool is_operator_tok(const TOKEN_T *tok, OPERATOR::OPERATOR op);
bool is_delim_tok(const TOKEN_T *tok, DELIMITER::DELIMITER d);

/**
 * @brief Устанавливает потомков у узла и обновляет обратные ссылки.
 */
void set_children(NODE_T *parent, NODE_T *left, NODE_T *right);

/**
 * @brief Пересчитывает поле elements для поддерева.
 */
size_t recount_elements(NODE_T *node);

/**
 * @brief Возвращает true если ключевое слово на самом деле кодирует IN/OUT.
 */
static inline bool keyword_is_io(const TOKEN_T *tok) {
    if (!tok || tok->node.type != KEYWORD_T)
        return false;
    int raw = (int) tok->node.value.keyword;
    return raw == (int) OPERATOR::OUT || raw == (int) OPERATOR::IN;
}

/**
 * @brief Проверяет, может ли токен начинать оператор.
 */
bool is_operator_start(const TOKEN_T *tok);

/**
 * @brief Проверяет, должен ли парсер остановить чтение операторов.
 */
bool is_operator_stop(const TOKEN_T *tok, KEYWORD::KEYWORD stop_kw);

bool     is_unary_builtin(OPERATOR::OPERATOR op);
bool     is_binary_builtin(OPERATOR::OPERATOR op);

/**
 * @brief Загружает AST из текстового дампа (префиксная форма) в память.
 * @param text  указатель на буфер с содержимым файла.
 * @param len   длина буфера в байтах.
 * @param root_out куда поместить корень дерева.
 * @param vars_out указатель на VarList; будет инициализирован.
 * @return 0 при успехе, -1 при ошибке.
 */
int load_ast_from_buffer(const char *text, size_t len, NODE_T **root_out, varlist::VarList *vars_out);

/**
 * @brief Загружает AST из файла.
 * @param path путь к .ast файлу.
 * @param root_out куда поместить корень дерева.
 * @param vars_out указатель на VarList; будет инициализирован.
 * @return 0 при успехе, -1 при ошибке.
 */
int load_ast_from_file(const char *path, NODE_T **root_out, varlist::VarList *vars_out);

/**
 * @brief Освобождает дерево и таблицу имён, созданные загрузчиком AST.
 */
void destroy_ast(NODE_T *root, varlist::VarList *vars);

#endif // AST_H