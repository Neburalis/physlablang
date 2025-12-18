#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ast.h"
#include "base.h"
#include "io_utils.h"

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
        case OPERATOR::DRAW:
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
    switch (op) {
        case OPERATOR::POW:
        case OPERATOR::SET_PIXEL:
            return true;
        default:
            return false;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*  AST loader                                                          */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef struct {
    const char       *buf;
    size_t            len;
    size_t            pos;
    varlist::VarList *vars;
    bool              error;
} ast_loader_t;

static void skip_ws(ast_loader_t *p) {
    while (p && p->pos < p->len) {
        unsigned char c = (unsigned char) p->buf[p->pos];
        if (!isspace(c))
            break;
        ++p->pos;
    }
}

static int read_token(ast_loader_t *p, char *out, size_t cap, bool *is_str) {
    if (!p || !out || cap == 0) return 0;
    skip_ws(p);
    if (p->pos >= p->len) return 0;
    if (is_str) *is_str = false;
    char c = p->buf[p->pos];
    if (c == '(' || c == ')') {
        out[0] = c;
        out[1] = '\0';
        ++p->pos;
        return 1;
    }
    if (c == '"') {
        ++p->pos;
        size_t wrote = 0;
        while (p->pos < p->len && p->buf[p->pos] != '"') {
            if (wrote + 1 < cap)
                out[wrote++] = p->buf[p->pos];
            ++p->pos;
        }
        if (p->pos >= p->len)
            return 0;
        ++p->pos; /* consume closing quote */
        out[wrote] = '\0';
        if (is_str) *is_str = true;
        return 1;
    }
    size_t wrote = 0;
    while (p->pos < p->len) {
        c = p->buf[p->pos];
        if (isspace((unsigned char)c) || c == '(' || c == ')')
            break;
        if (wrote + 1 < cap)
            out[wrote++] = c;
        ++p->pos;
    }
    out[wrote] = '\0';
    return wrote > 0;
}

static size_t intern_literal(ast_loader_t *p, const char *text) {
    if (!p || !p->vars || !text) return (size_t)-1;
    char *dup = strdup(text);
    if (!dup) return (size_t)-1;
    mystr::mystr_t str = mystr::construct(dup);
    size_t idx = varlist::add(p->vars, &str);
    free(dup);
    return idx;
}

static bool parse_keyword(const char *tok, KEYWORD::KEYWORD *out) {
    if (!tok || !out) return false;
    struct { const char *name; KEYWORD::KEYWORD kw; } table[] = {
        {"LAB", KEYWORD::LAB},
        {"ANNOTATION", KEYWORD::ANNOTATION},
        {"END_ANNOTATION", KEYWORD::END_ANNOTATION},
        {"GOAL_LITERAL", KEYWORD::GOAL_LITERAL},
        {"THEORETICAL", KEYWORD::THEORETICAL},
        {"END_THEORETICAL", KEYWORD::END_THEORETICAL},
        {"EXPERIMENTAL", KEYWORD::EXPERIMENTAL},
        {"END_EXPERIMENTAL", KEYWORD::END_EXPERIMENTAL},
        {"RESULTS", KEYWORD::RESULTS},
        {"END_RESULTS", KEYWORD::END_RESULTS},
        {"CONCLUSION", KEYWORD::CONCLUSION},
        {"END_CONCLUSION", KEYWORD::END_CONCLUSION},
        {"IF", KEYWORD::IF},
        {"ELSE", KEYWORD::ELSE},
        {"THEN", KEYWORD::THEN},
        {"WHILE", KEYWORD::WHILE},
        {"DO-WHILE", KEYWORD::DO_WHILE},
        {"WHILE_CONDITION", KEYWORD::WHILE_CONDITION},
        {"END_WHILE", KEYWORD::END_WHILE},
        {"FORMULA", KEYWORD::FORMULA},
        {"END_FORMULA", KEYWORD::END_FORMULA},
        {"VAR_DECLARATION", KEYWORD::VAR_DECLARATION},
        {"FUNC_CALL", KEYWORD::FUNC_CALL},
        {"RETURN", KEYWORD::RETURN},
    };
    for (size_t i = 0; i < ARRAY_COUNT(table); ++i) {
        if (strcmp(tok, table[i].name) == 0) {
            *out = table[i].kw;
            return true;
        }
    }
    return false;
}

static bool parse_operator(const char *tok, OPERATOR::OPERATOR *out) {
    if (!tok || !out) return false;
    struct { const char *name; OPERATOR::OPERATOR op; } table[] = {
        {"+", OPERATOR::ADD},
        {"-", OPERATOR::SUB},
        {"*", OPERATOR::MUL},
        {"/", OPERATOR::DIV},
        {"^", OPERATOR::POW},
        {"LN", OPERATOR::LN},
        {"SIN", OPERATOR::SIN},
        {"COS", OPERATOR::COS},
        {"TAN", OPERATOR::TAN},
        {"CTG", OPERATOR::CTG},
        {"ASIN", OPERATOR::ASIN},
        {"ACOS", OPERATOR::ACOS},
        {"ATAN", OPERATOR::ATAN},
        {"ACTG", OPERATOR::ACTG},
        {"SQRT", OPERATOR::SQRT},
        {"DRAW", OPERATOR::DRAW},
        {"SET_PIXEL", OPERATOR::SET_PIXEL},
        {"%", OPERATOR::MOD},
        {"==", OPERATOR::EQ},
        {"!=", OPERATOR::NEQ},
        {"<", OPERATOR::BELOW},
        {">", OPERATOR::ABOVE},
        {"<=", OPERATOR::BELOW_EQ},
        {">=", OPERATOR::ABOVE_EQ},
        {"AND", OPERATOR::AND},
        {"OR", OPERATOR::OR},
        {"!", OPERATOR::NOT},
        {"IN", OPERATOR::IN},
        {"OUT", OPERATOR::OUT},
        {"=", OPERATOR::ASSIGNMENT},
        {";", OPERATOR::CONNECTOR},
    };
    for (size_t i = 0; i < ARRAY_COUNT(table); ++i) {
        if (strcmp(tok, table[i].name) == 0) {
            *out = table[i].op;
            return true;
        }
    }
    return false;
}

static bool parse_delimiter(const char *tok, DELIMITER::DELIMITER *out) {
    if (!tok || !out) return false;
    if (strcmp(tok, ",") == 0) { *out = DELIMITER::COMA; return true; }
    if (strcmp(tok, "PAR_OPEN") == 0) { *out = DELIMITER::PAR_OPEN; return true; }
    if (strcmp(tok, "PAR_CLOSE") == 0) { *out = DELIMITER::PAR_CLOSE; return true; }
    if (strcmp(tok, "QUOTE") == 0) { *out = DELIMITER::QUOTE; return true; }
    if (strcmp(tok, "COLON") == 0) { *out = DELIMITER::COLON; return true; }
    return false;
}

static NODE_T *parse_node(ast_loader_t *p);

static NODE_T *parse_leaf(ast_loader_t *p, const char *tok, bool is_str) {
    if (!p || !tok) return nullptr;

    if (is_str) {
        size_t id = intern_literal(p, tok);
        if (id == (size_t)-1) { p->error = true; return nullptr; }
        NODE_VALUE_T v = {}; v.id = id; return new_node(LITERAL_T, v, nullptr, nullptr);
    }

    if (strcmp(tok, "nil") == 0) return nullptr;

    char *endptr = nullptr;
    double num = strtod(tok, &endptr);
    if (endptr && endptr != tok && *endptr == '\0') {
        NODE_VALUE_T v = {}; v.num = num; return new_node(NUMBER_T, v, nullptr, nullptr);
    }

    if (strcmp(tok, "IDENTIFIER") == 0) {
        char buf[128] = ""; bool dummy = false;
        if (!read_token(p, buf, sizeof(buf), &dummy)) { p->error = true; return nullptr; }
        size_t id = (size_t) strtoull(buf, nullptr, 10);
        NODE_VALUE_T v = {}; v.id = id; return new_node(IDENTIFIER_T, v, nullptr, nullptr);
    }

    KEYWORD::KEYWORD kw = {};
    if (parse_keyword(tok, &kw)) {
        NODE_VALUE_T v = {}; v.keyword = kw; return new_node(KEYWORD_T, v, nullptr, nullptr);
    }

    OPERATOR::OPERATOR op = {};
    if (parse_operator(tok, &op)) {
        NODE_VALUE_T v = {}; v.opr = op; return new_node(OPERATOR_T, v, nullptr, nullptr);
    }

    DELIMITER::DELIMITER d = {};
    if (parse_delimiter(tok, &d)) {
        NODE_VALUE_T v = {}; v.delimiter = d; return new_node(DELIMITER_T, v, nullptr, nullptr);
    }

    p->error = true;
    return nullptr;
}

static NODE_T *parse_node(ast_loader_t *p) {
    if (!p) return nullptr;
    char tok[256] = "";
    bool is_str = false;
    if (!read_token(p, tok, sizeof(tok), &is_str)) {
        p->error = true;
        return nullptr;
    }
    if (strcmp(tok, "nil") == 0)
        return nullptr;
    if (tok[0] != '(') {
        p->error = true;
        return nullptr;
    }
    if (!read_token(p, tok, sizeof(tok), &is_str)) {
        p->error = true;
        return nullptr;
    }

    NODE_T *head = parse_leaf(p, tok, is_str);
    if (!head) {
        p->error = true;
        return nullptr;
    }

    NODE_T *left = parse_node(p);
    NODE_T *right = parse_node(p);
    char closing[8] = ""; bool dummy = false;
    if (!read_token(p, closing, sizeof(closing), &dummy) || closing[0] != ')') {
        p->error = true;
        destruct_node(left);
        destruct_node(right);
        destruct_node(head);
        return nullptr;
    }
    set_children(head, left, right);
    return head;
}

int load_ast_from_buffer(const char *text, size_t len, NODE_T **root_out, varlist::VarList *vars_out) {
    if (!text || !root_out || !vars_out) return -1;
    *root_out = nullptr;
    varlist::init(vars_out);

    ast_loader_t loader = {text, len, 0, vars_out, false};
    NODE_T *root = parse_node(&loader);
    if (loader.error || !root) {
        destroy_ast(root, vars_out);
        return -1;
    }
    recount_elements(root);
    *root_out = root;
    return 0;
}

int load_ast_from_file(const char *path, NODE_T **root_out, varlist::VarList *vars_out) {
    if (!path || !root_out || !vars_out) return -1;
    size_t buf_len = 0;
    char *buf = read_file_to_buf(path, &buf_len);
    if (!buf || !buf_len)
        return -1;
    int rc = load_ast_from_buffer(buf, buf_len, root_out, vars_out);
    free(buf);
    return rc;
}

void destroy_ast(NODE_T *root, varlist::VarList *vars) {
    destruct_node(root);
    if (vars)
        varlist::destruct(vars);
}