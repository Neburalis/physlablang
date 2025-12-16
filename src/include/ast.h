#ifndef AST_H
#define AST_H

#include <stddef.h>
#include <stdint.h>

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
        VAR_DECLARATION, VAR_ASSIGNMENT,
        LET_ASSIGNMENT,
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

#endif // AST_H