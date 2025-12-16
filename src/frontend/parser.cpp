#include <ctype.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "differentiator.h"
#include "enhanced_string.h"
#include "stringNthong.h"
#include "io_utils.h"
#include "base.h"
#include "var_list.h"

#define PARSE_FAIL(p, ...)            \
    do {                              \
        (p)->error = true;            \
        ERROR_MSG(__VA_ARGS__);       \
    } while (0)

typedef struct {
    const char         *buf;
    size_t              len;
    size_t              pos;
    bool                error;
    varlist::VarList   *vars;
} parser_t;

// Dumps parser state for verbose debugging output.
function void debug_parse_print(const parser_t *p, const char *reason);

// Extracts the tree name from the parser buffer.
function bool extract_tree_name(parser_t *p, const char **eq_tree_name, char **owned_name);

// Skips whitespace characters in the parser buffer.
function void skip_spaces(parser_t *p);
#define ss_ skip_spaces(p)

// Recursive-descent entry points.
function NODE_T *get_grammar   (parser_t *p);
function NODE_T *get_expression(parser_t *p);
function NODE_T *get_term      (parser_t *p);
function NODE_T *get_power     (parser_t *p);
function NODE_T *get_primary   (parser_t *p);
function NODE_T *get_unarycall (parser_t *p);
function NODE_T *get_binarycall(parser_t *p);
function NODE_T *get_variable  (parser_t *p);
function NODE_T *get_number    (parser_t *p);
function bool    get_binaryfunc(parser_t *p, OPERATOR *out);
function bool    get_unaryfunc (parser_t *p, OPERATOR *out);

// Helper routines.
function bool    is_at_end     (const parser_t *p);
function char    peek_char     (const parser_t *p);
function bool    match_char    (parser_t *p, char ch);
function char   *collect_identifier(parser_t *p);
function NODE_T *make_binary_node(parser_t *p, OPERATOR op, NODE_T *lhs, NODE_T *rhs);
function NODE_T *make_unary_node (parser_t *p, OPERATOR op, NODE_T *arg);
function bool    is_unary_operator(OPERATOR op);
function bool    is_binary_operator(OPERATOR op);
function bool    store_variable(parser_t *p, NODE_T *node, char *token);
function bool extract_graph_range(parser_t *p, graph_range_t *range);

#define CREATE_NEW_EQ_TREE()                                \
    EQ_TREE_T *new_eq_tree = TYPED_CALLOC(1, EQ_TREE_T);    \
    VERIFY(new_eq_tree, destruct(root); return nullptr;);   \
    new_eq_tree->name = eq_tree_name;                       \
    new_eq_tree->root = root;                               \
    new_eq_tree->vars = vars;                               \
    new_eq_tree->owns_name = owned_name != nullptr;         \
    owned_name = nullptr;

// Reads expression file into an equation tree structure.
EQ_TREE_T *load_tree_from_file(const char *filename, const char *eq_tree_name, varlist::VarList *vars, graph_range_t *range) {
    VERIFY(filename != nullptr, ERROR_MSG("filename is nullptr");  return nullptr;);
    VERIFY(vars     != nullptr, ERROR_MSG("vars list is nullptr"); return nullptr;);
    char *buffer = read_file_to_buf(filename, nullptr);
    if (!buffer) {
        ERROR_MSG("Can't read expression file '%s'\n", filename);
        return nullptr;
    }
    char *owned_name = nullptr;
    size_t buffer_len = strlen(buffer);
    parser_t parser = {buffer, buffer_len, 0, false, nullptr};
    if (!extract_tree_name(&parser, &eq_tree_name, &owned_name)) {
        FREE(owned_name);
        FREE(buffer);
        return nullptr;
    }
    if (!extract_graph_range(&parser, range)) {
        FREE(owned_name);
        FREE(buffer);
        return nullptr;
    }
    varlist::init(vars);
    parser.vars = vars;
    NODE_T *root = get_grammar(&parser);
    if (parser.error) {
        destruct(root);
        FREE(owned_name);
        FREE(buffer);
        return nullptr;
    }
    FREE(buffer);

    CREATE_NEW_EQ_TREE();
    return new_eq_tree;
}

#undef CREATE_NEW_EQ_TREE

// Extracts the tree name from the parser buffer.
function bool extract_tree_name(parser_t *p, const char **eq_tree_name, char **owned_name) {
    ss_;
    if (p->pos >= p->len || p->buf[p->pos] == '(')
        return true;
    const char *line = p->buf + p->pos;
    const char *line_end = strchr(line, '\n');
    if (!line_end) {
        ERROR_MSG("Name must be on own line\n");
        return false;
    }
    size_t name_len = (size_t)(line_end - line);
    if (name_len > 0) {
        *owned_name = TYPED_CALLOC(name_len + 1, char);
        if (*owned_name) {
            memcpy(*owned_name, line, name_len);
            *eq_tree_name = *owned_name;
        }
    }
    p->pos = (size_t)(line_end - p->buf) + 1;
    ss_;
    return true;
}

function bool extract_graph_range(parser_t *p, graph_range_t *range) {
    double x_min = NAN, x_max = NAN, y_min = NAN, y_max = NAN;
    if (strncmp(p->buf + p->pos, "xrange", 6) == 0) {
        int count = sscanf(p->buf + p->pos, "xrange [%lg : %lg] ", &x_min, &x_max);
        p->pos += count;
        const char *line_end = strchr(p->buf + p->pos, '\n');
        // DEBUG_PRINT("x_min = %lg, x_max = %lg\n", x_min, x_max);
        if (line_end) {
            p->pos += line_end - (p->buf + p->pos) + 1;
        } else return false;
    }
    if (strncmp(p->buf + p->pos, "yrange", 6) == 0) {
        int count = sscanf(p->buf + p->pos, "yrange [%lg : %lg] ", &y_min, &y_max);
        p->pos += count;
        const char *line_end = strchr(p->buf + p->pos, '\n');
        if (line_end)
            p->pos += line_end - (p->buf + p->pos) + 1;
        else return false;
    }
    range->x_max=x_max;
    range->x_min=x_min;
    range->y_max=y_max;
    range->y_min=y_min;
    return true;
}

// Loads a tree with a default name wrapper.
EQ_TREE_T *load_tree_from_file(const char *filename, varlist::VarList *vars, graph_range_t *range) {
    return load_tree_from_file(filename, "New equation tree", vars, range);
}

function bool is_at_end(const parser_t *p) {
    return p->pos >= p->len;
}

function char peek_char(const parser_t *p) {
    return is_at_end(p) ? '\0' : p->buf[p->pos];
}

function bool match_char(parser_t *p, char ch) {
    if (peek_char(p) != ch)
        return false;
    ++p->pos;
    return true;
}

function void skip_spaces(parser_t *p) {
    while (!is_at_end(p) && isspace((unsigned char) p->buf[p->pos]))
        ++p->pos;
}

function char *collect_identifier(parser_t *p) {
    if (is_at_end(p))
        return nullptr;
    size_t start = p->pos;
    char c = p->buf[p->pos];
    if (!isalpha((unsigned char) c))
        return nullptr;
    ++p->pos;
    while (!is_at_end(p)) {
        char sym = p->buf[p->pos];
        if (!isalnum((unsigned char) sym) && sym != '_')
            break;
        ++p->pos;
    }
    size_t span = p->pos - start;
    char *name = TYPED_CALLOC(span + 1, char);
    if (!name) {
        PARSE_FAIL(p, "No memory for identifier\n");
        return nullptr;
    }
    memcpy(name, p->buf + start, span);
    name[span] = '\0';
    return name;
}

function bool is_unary_operator(OPERATOR op) {
    switch (op) {
        case LN:
        case SIN:
        case COS:
        case TAN:
        case CTG:
        case ASIN:
        case ACOS:
        case ATAN:
        case ACTG:
        case SQRT:
        case SINH:
        case COSH:
        case TANH:
        case CTH:
            return true;
        default:
            return false;
    }
}

function bool is_binary_operator(OPERATOR op) {
    switch (op) {
        case LOG:
            return true;
        default:
            return false;
    }
}

function NODE_T *make_binary_node(parser_t *p, OPERATOR op, NODE_T *lhs, NODE_T *rhs) {
    NODE_T *node = new_node(OP_T, (NODE_VALUE_T) {.opr = op}, lhs, rhs);
    if (!node) {
        PARSE_FAIL(p, "Failed to allocate binary node\n");
        destruct(lhs);
        destruct(rhs);
    }
    return node;
}

function NODE_T *make_unary_node(parser_t *p, OPERATOR op, NODE_T *arg) {
    NODE_T *node = new_node(OP_T, (NODE_VALUE_T) {.opr = op}, arg, nullptr);
    if (!node) {
        PARSE_FAIL(p, "Failed to allocate unary node\n");
        destruct(arg);
    }
    return node;
}

function bool get_unaryfunc(parser_t *p, OPERATOR *out) {
    size_t start = p->pos;
    char *name = collect_identifier(p);
    if (!name)
        return false;

    OPERATOR op = {};
    bool ok = operator_from_token(name, &op) && is_unary_operator(op);
    FREE(name);
    if (!ok) {
        p->pos = start;
        return false;
    }
    *out = op;
    return true;
}

function bool get_binaryfunc(parser_t *p, OPERATOR *out) {
    size_t start = p->pos;
    char *name = collect_identifier(p);
    if (!name)
        return false;
    OPERATOR op = {};
    bool ok = operator_from_token(name, &op) && is_binary_operator(op);
    FREE(name);
    if (!ok) {
        p->pos = start;
        return false;
    }
    *out = op;
    return true;
}

function NODE_T *get_number(parser_t *p) {
    ss_;
    size_t start = p->pos;
    if (is_at_end(p))
        return nullptr;
    char c = p->buf[p->pos];
    if (!isdigit((unsigned char) c)) {
        if (!(c == '-' && p->pos + 1 < p->len && isdigit((unsigned char) p->buf[p->pos + 1]))) // Если встретили минус а за ним не число
            return nullptr;
    }

    char *end = nullptr;
    double val = strtod(p->buf + p->pos, &end);
    if (!end || end == p->buf + p->pos) {
        p->pos = start;
        return nullptr;
    }
    size_t consumed = (size_t) (end - (p->buf + p->pos));
    p->pos += consumed;

    NODE_T *node = new_node(NUM_T, (NODE_VALUE_T) {.num = val}, nullptr, nullptr);
    if (!node) {
        PARSE_FAIL(p, "Failed to allocate number node\n");
        return nullptr;
    }
    return node;
}

function NODE_T *get_variable(parser_t *p) {
    ss_;
    size_t start = p->pos;
    char *name = collect_identifier(p);
    if (!name)
        return nullptr;

    OPERATOR op = {};
    bool reserved = operator_from_token(name, &op) && (is_unary_operator(op) || is_binary_operator(op));
    if (reserved) {
        FREE(name);
        p->pos = start;
        return nullptr;
    }
    NODE_T *node = new_node(VAR_T, (NODE_VALUE_T) {}, nullptr, nullptr);
    if (!node) {
        PARSE_FAIL(p, "Failed to allocate variable node\n");
        FREE(name);
        return nullptr;
    }
    if (!store_variable(p, node, name)) {
        PARSE_FAIL(p, "Failed to register variable\n");
        FREE(name);
        destruct(node);
        return nullptr;
    }
    FREE(name);
    return node;
}

function NODE_T *get_binarycall(parser_t *p) {
    ss_;
    size_t start = p->pos;
    OPERATOR op = {};
    if (!get_binaryfunc(p, &op))
        return nullptr;
    ss_;
    if (!match_char(p, '(')) {
        p->pos = start;
        return nullptr;
    }
    NODE_T *first = get_expression(p);
    if (!first) {
        if (!p->error)
            p->pos = start;
        return nullptr;
    }
    ss_;
    if (!match_char(p, ',')) {
        PARSE_FAIL(p, "Expected ',' in binary call\n");
        destruct(first);
        return nullptr;
    }
    NODE_T *second = get_expression(p);
    if (!second) {
        destruct(first);
        return nullptr;
    }
    ss_;
    if (!match_char(p, ')')) {
        PARSE_FAIL(p, "Expected ')' after binary call\n");
        destruct(first);
        destruct(second);
        return nullptr;
    }
    return make_binary_node(p, op, first, second);
}

function NODE_T *get_unarycall(parser_t *p) {
    ss_;
    size_t start = p->pos;
    OPERATOR op = {};
    if (!get_unaryfunc(p, &op))
        return nullptr;
    ss_;
    if (!match_char(p, '(')) {
        p->pos = start;
        return nullptr;
    }
    NODE_T *arg = get_expression(p);
    if (!arg) {
        if (!p->error)
            p->pos = start;
        return nullptr;
    }
    ss_;
    if (!match_char(p, ')')) {
        PARSE_FAIL(p, "Expected ')' after unary call\n");
        destruct(arg);
        return nullptr;
    }
    return make_unary_node(p, op, arg);
}

function NODE_T *get_primary(parser_t *p) {
    ss_;
    if (match_char(p, '(')) {
        NODE_T *node = get_expression(p);
        if (!node)
            return nullptr;
        ss_;
        if (!match_char(p, ')')) {
            PARSE_FAIL(p, "Expected ')' to close group\n");
            destruct(node);
            return nullptr;
        }
        return node;
    }

    NODE_T *parsed = get_number(p);
    if (parsed)
        return parsed;

    parsed = get_unarycall(p);
    if (parsed)
        return parsed;

    parsed = get_binarycall(p);
    if (parsed)
        return parsed;

    parsed = get_variable(p);
    if (parsed)
        return parsed;

    PARSE_FAIL(p, "Primary expected at offset %zu\n", p->pos);
    return nullptr;
}

function NODE_T *get_power(parser_t *p) {
    NODE_T *base = get_primary(p);
    if (!base)
        return nullptr;
    ss_;
    if (!match_char(p, '^'))
        return base;
    NODE_T *exp = get_primary(p);
    if (!exp) {
        PARSE_FAIL(p, "Required exp after ^ symbol\n");
        destruct(base);
        return nullptr;
    }
    return make_binary_node(p, POW, base, exp);
}

function NODE_T *get_term(parser_t *p) {
    NODE_T *lhs = get_power(p);
    if (!lhs)
        return nullptr;
    while (true) {
        ss_;
        char op_char = peek_char(p);
        OPERATOR op = {};
        if (op_char == '*') op = MUL;
        else if (op_char == '/') op = DIV;
        else break;

        ++p->pos;
        NODE_T *rhs = get_power(p);
        if (!rhs) {
            destruct(lhs);
            return nullptr;
        }
        lhs = make_binary_node(p, op, lhs, rhs);
        if (!lhs)
            return nullptr;
    }
    return lhs;
}

function NODE_T *get_expression(parser_t *p) {
    NODE_T *lhs = get_term(p);
    if (!lhs)
        return nullptr;
    while (true) {
        ss_;
        char op_char = peek_char(p);
        OPERATOR op = {};
        if (op_char == '+') op = ADD;
        else if (op_char == '-') op = SUB;
        else break;

        ++p->pos;
        NODE_T *rhs = get_term(p);
        if (!rhs) {
            destruct(lhs);
            return nullptr;
        }
        lhs = make_binary_node(p, op, lhs, rhs);
        if (!lhs)
            return nullptr;
    }
    return lhs;
}

function NODE_T *get_grammar(parser_t *p) {
    NODE_T *root = get_expression(p);
    if (!root)
        return nullptr;
    ss_;
    if (!is_at_end(p)) {
        PARSE_FAIL(p, "Unexpected trailing data at offset %zu\n", p->pos);
        destruct(root);
        return nullptr;
    }
    return root;
}

// Registers variable name and stores its index.
function bool store_variable(parser_t *p, NODE_T *node, char *token) {
    debug_parse_print(p, "store_variable");
    if (!p->vars)
        return false;
    mystr::mystr_t name = mystr::construct(token);
    size_t idx = varlist::add(p->vars, &name);
    return idx != varlist::NPOS ? (node->value.var = idx, true) : false;
}

// Dumps parser state for verbose debugging output.
function void debug_parse_print(const parser_t *p, const char *reason) {
    size_t pos = p->pos < p->len ? p->pos : p->len;
    printf("file loading dump %s\n", reason);
    printf(BRIGHT_BLACK("%.*s"), (int)pos, p->buf);
    if (pos < p->len) {
        printf(GREEN("%c"), p->buf[pos]);
        if (pos + 1 < p->len) printf("%s", p->buf + pos + 1);
    }
    putchar('\n');
}

#undef ss_