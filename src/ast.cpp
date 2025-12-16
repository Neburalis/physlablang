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