#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "base.h"

/** Проверяет, что токен является заданным ключевым словом. */
bool is_keyword_tok(const TOKEN_T *tok, KEYWORD::KEYWORD kw) {
    return tok && tok->node.type == KEYWORD_T && tok->node.value.keyword == kw;
}

/** Проверяет, что токен является заданным оператором. */
bool is_operator_tok(const TOKEN_T *tok, OPERATOR::OPERATOR op) {
    return tok && tok->node.type == OPERATOR_T && tok->node.value.opr == op;
}

/** Проверяет, что токен является указанным разделителем. */
bool is_delim_tok(const TOKEN_T *tok, DELIMITER::DELIMITER d) {
    return tok && tok->node.type == DELIMITER_T && tok->node.value.delimiter == d;
}

/**
 * @brief Выделяет новый узел AST.
 */
NODE_T *alloc_new_node(void) {
    NODE_T *node = TYPED_CALLOC(1, NODE_T);
    if (!node) return nullptr;
    node->signature = signature;
    node->left = nullptr;
    node->right = nullptr;
    node->parent = nullptr;
    node->elements = 0;
    return node;
}

/**
 * @brief Создает узел и назначает потомков.
 */
NODE_T *new_node(NODE_TYPE type, NODE_VALUE_T value, NODE_T *left, NODE_T *right) {
    NODE_T *node = alloc_new_node();
    if (!node) return nullptr;
    node->type = type;
    node->value = value;
    node->left = left;
    node->right = right;
    if (left) {
        left->parent = node;
        node->elements += left->elements + 1;
    }
    if (right) {
        right->parent = node;
        node->elements += right->elements + 1;
    }
    return node;
}

/**
 * @brief Возвращает true если оба потомка заданы.
 */
bool is_leaf(const NODE_T *node) {
    return node && node->left && node->right;
}

/**
 * @brief Рекурсивно удаляет поддерево.
 */
void destruct_node(NODE_T *node) {
    if (!node) return;
    destruct_node(node->left);
    destruct_node(node->right);
    free(node);
}

/**
 * @brief Устанавливает потомков у узла и обновляет обратные ссылки.
 */
void set_children(NODE_T *parent, NODE_T *left, NODE_T *right) {
    if (!parent) return;
    parent->left = left;
    parent->right = right;
    if (left) left->parent = parent;
    if (right) right->parent = parent;
}

/**
 * @brief Пересчитывает поле elements для поддерева.
 */
size_t recount_elements(NODE_T *node) {
    if (!node) return 0;
    size_t total = 0;
    if (node->left) {
        node->left->parent = node;
        total += recount_elements(node->left) + 1;
    }
    if (node->right) {
        node->right->parent = node;
        total += recount_elements(node->right) + 1;
    }
    node->elements = total;
    return total;
}

/**
 * @brief Проверяет, может ли токен начинать оператор.
 */
bool is_operator_start(const TOKEN_T *tok) {
    if (!tok) return false;
    if (tok->node.type == KEYWORD_T) {
        switch (tok->node.value.keyword) {
            case KEYWORD::VAR_DECLARATION:
            case KEYWORD::IF:
            case KEYWORD::WHILE:
            case KEYWORD::WHILE_CONDITION:
            case KEYWORD::RETURN:
            case KEYWORD::FUNC_CALL:
                return true;
            default:
                break;
        }
        if (keyword_is_io(tok))
            return true;
        return false;
    }
    if (tok->node.type == IDENTIFIER_T)
        return true;
    if (tok->node.type == OPERATOR_T) {
        switch (tok->node.value.opr) {
            case OPERATOR::OUT:
            case OPERATOR::IN:
                return true;
            default:
                break;
        }
    }
    return false;
}

/**
 * @brief Проверяет, должен ли парсер остановить чтение операторов.
 */
bool is_operator_stop(const TOKEN_T *tok, KEYWORD::KEYWORD stop_kw) {
    if (!tok) return true;
    if (tok->node.type != KEYWORD_T)
        return false;
    KEYWORD::KEYWORD kw = tok->node.value.keyword;
    if (kw == stop_kw)
        return true;
    switch (kw) {
        case KEYWORD::ELSE:
        case KEYWORD::END_WHILE:
        case KEYWORD::END_FORMULA:
        case KEYWORD::END_EXPERIMENTAL:
        case KEYWORD::END_RESULTS:
        case KEYWORD::END_THEORETICAL:
        case KEYWORD::END_CONCLUSION:
        case KEYWORD::END_ANNOTATION:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Проверяет, является ли оператор унарной встроенной функцией.
 */
bool is_unary_builtin(OPERATOR::OPERATOR op) {
    switch (op) {
        case OPERATOR::SIN:
        case OPERATOR::COS:
        case OPERATOR::TAN:
        case OPERATOR::CTG:
        case OPERATOR::ASIN:
        case OPERATOR::ACOS:
        case OPERATOR::ATAN:
        case OPERATOR::ACTG:
        case OPERATOR::SQRT:
        case OPERATOR::LN:
        case OPERATOR::NOT:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Проверяет, является ли оператор бинарной встроенной функцией.
 */
bool is_binary_builtin(OPERATOR::OPERATOR op) {
    return op == OPERATOR::POW;
}