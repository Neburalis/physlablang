#include <ctype.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "enhanced_string.h"
#include "stringNthong.h"
#include "io_utils.h"
#include "base.h"
#include "var_list.h"
#include "frontend.h"

// NODE_T *alloc_new_node() {
//     NODE_T *new_node = TYPED_CALLOC(1, NODE_T);
//     if (new_node == nullptr) {
//         return nullptr;
//     }
//     new_node->signature=signature;
//     return new_node;
// }
//
// NODE_T *new_node(NODE_TYPE type, NODE_VALUE_T value, NODE_T *left, NODE_T *right) {
//     NODE_T *node = alloc_new_node();
//     if (!node)
//         return nullptr;
//     node->type  = type;
//     node->value = value;
//     node->left  = left;
//     node->right = right;
//     if (right) {
//         right->parent = node;
//         node->elements += right->elements + 1;
//     }
//     if (left) {
//         left->parent = node;
//         node->elements += left->elements + 1;
//     }
//     return node;
// }
//
// void destruct(NODE_T *node) {
//     if (!node)
//         return;
//     destruct(node->left);
//     destruct(node->right);
//     FREE(node);
// }
//
// void destruct(FRONT_COMPL_T *eqtree) {
//     if (!eqtree)
//         return;
//     destruct(eqtree->root);
//     if (eqtree->owns_name && eqtree->name)
//         free((void *) eqtree->name);
//     eqtree->name = nullptr;
//     if (eqtree->vars) {
//         varlist::destruct(eqtree->vars);
//         if (eqtree->owns_vars)
//             FREE(eqtree->vars);
//     }
//     FREE(eqtree);
// }
//
// void destruct(FRONT_COMPL_T **eq_arr) {
//     size_t idx = 1;
//     FRONT_COMPL_T *cur_tree = eq_arr[idx];
//     while(cur_tree != nullptr) {
//         destruct(cur_tree);
//         eq_arr[idx] = nullptr;
//         ++idx;
//         cur_tree = eq_arr[idx];
//     }
// }

// bool node_type_from_token(const char *token, NODE_TYPE *out) {
//     if (!token || !*token || !out)
//         return false;
//     if (!token[1] && strchr("+-*/", token[0])) {
//         *out = OP_T;
//         return true;
//     }
//     OPERATOR tmp = {};
//     if (operator_from_token(token, &tmp)) {
//         *out = OP_T;
//         return true;
//     }
//     char *end = nullptr;
//     strtod(token, &end);
//     if (end && end != token && *end == '\0') {
//         *out = NUM_T;
//         return true;
//     }
//     if (isalpha((unsigned char)token[0])) {
//         for (const unsigned char *c = (const unsigned char *)token; *c; ++c) {
//             if (!isalnum(*c) && *c != '_')
//                 return false;
//         }
//         *out = VAR_T;
//         return true;
//     }
//     return false;
// }
//
// bool operator_from_token(const char *token, OPERATOR *op) {
//     if (!token || !op || !*token)
//         return false;
// #define MATCH(tok, val) if (strcmp(token, (tok)) == 0) { *(op) = (val); return true; }
//     MATCH("+", ADD);
//     MATCH("-", SUB);
//     MATCH("*", MUL);
//     MATCH("/", DIV);
//     MATCH("^", POW);
//     MATCH("ln", LN);
//     MATCH("log", LOG);
//     MATCH("sin", SIN);
//     MATCH("cos", COS);
//     MATCH("tan", TAN);
//     MATCH("tg", TAN);
//     MATCH("cot", CTG);
//     MATCH("ctg", CTG);
//     MATCH("arcsin", ASIN);
//     MATCH("arccos", ACOS);
//     MATCH("arctan", ATAN);
//     MATCH("arctg", ATAN);
//     MATCH("actg", ACTG);
//     MATCH("arcctg", ACTG);
//     MATCH("sqrt", SQRT);
//     MATCH("sinh", SINH);
//     MATCH("sh", SINH);
//     MATCH("cosh", COSH);
//     MATCH("ch", COSH);
//     MATCH("tanh", TANH);
//     MATCH("th", TANH);
//     MATCH("coth", CTH);
//     MATCH("cth", CTH);
// #undef MATCH
//     return false;
// }
//
// bool is_leaf(const NODE_T *node) {
//     return node && node->left && node->right;
// }
//
// function double eval_node(const NODE_T *node, const double *vals, size_t vals_num) {
//     if (!node) return 0;
//     switch (node->type) {
//         case NUM_T: return node->value.num;
//         case VAR_T:
//             POSASSERT(vals != nullptr);
//             POSASSERT(node->value.var < vals_num);
//             return vals[node->value.var];
//         case OP_T: {
//             double l = node->left ? eval_node(node->left, vals, vals_num) : 0;
//             double r = node->right ? eval_node(node->right, vals, vals_num) : 0;
//             switch (node->value.opr) {
//                 case ADD:  return l + r;
//                 case SUB:  return l - r;
//                 case MUL:  return l * r;
//                 case DIV:  return l / r;
//                 case POW:  return pow(l, r);
//                 case LOG:  return log(l) / log(r);
//                 case LN:   return log(l);
//                 case SIN:  return sin(l);
//                 case COS:  return cos(l);
//                 case TAN:  return tan(l);
//                 case CTG:  return 1.0 / tan(l);
//                 case ASIN: return asin(l);
//                 case ACOS: return acos(l);
//                 case ATAN: return atan(l);
//                 case ACTG: return atan(1.0 / l);
//                 case SQRT: return sqrt(l);
//                 case SINH: return sinh(l);
//                 case COSH: return cosh(l);
//                 case TANH: return tanh(l);
//                 case CTH:  return 1.0 / tanh(l);
//                 default:   ERROR_MSG("eval_node: unknown operator: %d", node->value.opr); return 0;
//             }
//         }
//     }
// }
//
// EQ_POINT_T read_point_data(const FRONT_COMPL_T *eqtree) {
//     EQ_POINT_T res = {eqtree, nullptr, 0, 0};
//     if (!eqtree) return res;
//     varlist::VarList *vars = eqtree->vars;
//     if (!vars) return res;
//     res.vars_count = varlist::size(vars);
//     if (!res.vars_count) return res;
//     res.point = TYPED_CALLOC(res.vars_count, double);
//     VERIFY(res.point != nullptr, ERROR_MSG("read_point_data: failed to allocate values"); return res;);
//     for (size_t i = 0; i < res.vars_count; ++i) {
//         const mystr::mystr_t *name = varlist::get(vars, i);
//         const char *label = (name && name->str) ? name->str : "var";
//         printf("Enter %s: ", label);
//         fflush(stdout);
//         scanf("%lf", &res.point[i]);
//     }
//     return res;
// }
//
// EQ_POINT_T *calc_in_point(EQ_POINT_T *point) {
//     if (!point->tree || !point->tree->root) return point;
//     point->result = eval_node(point->tree->root, point->point, point->vars_count);
//     return point;
// }

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* AST serialization                                                     */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void indent(FILE *fp, unsigned int depth) {
    for (unsigned int i = 0; i < depth; ++i)
        fputs("    ", fp);
}

#define STR_CASE_(x) case x: return #x;
#define CASE_(x, y) case x: return y;

static const char *keyword_name(KEYWORD::KEYWORD kw) {
    switch (kw) {
        STR_CASE_(KEYWORD::LAB);
        STR_CASE_(KEYWORD::ANNOTATION);
        STR_CASE_(KEYWORD::END_ANNOTATION);
        STR_CASE_(KEYWORD::GOAL_LITERAL);
        STR_CASE_(KEYWORD::THEORETICAL);
        STR_CASE_(KEYWORD::END_THEORETICAL);
        STR_CASE_(KEYWORD::EXPERIMENTAL);
        STR_CASE_(KEYWORD::END_EXPERIMENTAL);
        STR_CASE_(KEYWORD::RESULTS);
        STR_CASE_(KEYWORD::END_RESULTS);
        STR_CASE_(KEYWORD::CONCLUSION);
        STR_CASE_(KEYWORD::END_CONCLUSION);
        CASE_(KEYWORD::IF, "IF");
        CASE_(KEYWORD::ELSE, "ELSE");
        CASE_(KEYWORD::THEN, "THEN");
        CASE_(KEYWORD::WHILE, "WHILE");
        CASE_(KEYWORD::DO_WHILE, "DO-WHILE");
        STR_CASE_(KEYWORD::WHILE_CONDITION);
        STR_CASE_(KEYWORD::END_WHILE);
        STR_CASE_(KEYWORD::FORMULA);
        STR_CASE_(KEYWORD::END_FORMULA);
        CASE_(KEYWORD::VAR_DECLARATION, "VAR_DECLARATION");
        CASE_(KEYWORD::FUNC_CALL, "FUNC_CALL");
        CASE_(KEYWORD::RETURN, "RETURN");
        default:                         return "KEYWORD";
    }
}

static const char *operator_name(OPERATOR::OPERATOR op) {
    switch (op) {
        CASE_(OPERATOR::ADD, "+");
        CASE_(OPERATOR::SUB, "-");
        CASE_(OPERATOR::MUL, "*");
        CASE_(OPERATOR::DIV, "/");
        CASE_(OPERATOR::POW, "^");
        CASE_(OPERATOR::LN, "LN");
        CASE_(OPERATOR::SIN, "SIN");
        CASE_(OPERATOR::COS, "COS");
        CASE_(OPERATOR::TAN, "TAN");
        CASE_(OPERATOR::CTG, "CTG");
        CASE_(OPERATOR::ASIN, "ASIN");
        CASE_(OPERATOR::ACOS, "ACOS");
        CASE_(OPERATOR::ATAN, "ATAN");
        CASE_(OPERATOR::ACTG, "ACTG");
        CASE_(OPERATOR::SQRT, "SQRT");
        CASE_(OPERATOR::EQ, "==");
        CASE_(OPERATOR::NEQ, "!=");
        CASE_(OPERATOR::BELOW, "<");
        CASE_(OPERATOR::ABOVE, ">");
        CASE_(OPERATOR::BELOW_EQ, "<=");
        CASE_(OPERATOR::ABOVE_EQ, ">=");
        CASE_(OPERATOR::AND, "AND");
        CASE_(OPERATOR::OR, "OR");
        CASE_(OPERATOR::NOT, "!");
        CASE_(OPERATOR::MOD, "%");
        CASE_(OPERATOR::IN, "IN");
        CASE_(OPERATOR::OUT, "OUT");
        CASE_(OPERATOR::ASSIGNMENT, "=");
        CASE_(OPERATOR::CONNECTOR, ";");
        default:                   return "OP";
    }
}

static const char *delimiter_name(DELIMITER::DELIMITER d) {
    switch (d) {
        STR_CASE_(DELIMITER::PAR_OPEN);
        STR_CASE_(DELIMITER::PAR_CLOSE);
        STR_CASE_(DELIMITER::QUOTE);
        CASE_(DELIMITER::COMA, ",");
        STR_CASE_(DELIMITER::COLON);
        default:                   return "DELIM";
    }
}

#undef STR_CASE_
#undef CASE_

static void format_literal(const FRONT_COMPL_T *ctx, size_t id, char *buf, size_t cap) {
    if (!buf || !cap) return;
    buf[0] = '\0';
    const mystr::mystr_t *entry = (ctx && ctx->vars) ? varlist::get(ctx->vars, id) : nullptr;
    if (entry && entry->str)
        snprintf(buf, cap, "%s", entry->str);
    else
        snprintf(buf, cap, "id_%zu", id);
}

static void write_node(FILE *fp, const FRONT_COMPL_T *ctx, const NODE_T *node, unsigned int depth) {
    if (!node) {
        indent(fp, depth);
        fputs("nil", fp);
        return;
    }

    indent(fp, depth);
    // fputc('\n', fp);

    depth++;

    switch (node->type) {
        case NUMBER_T:
            fprintf(fp, "%g", node->value.num);
            return;
        case OPERATOR_T:
            fputc('(', fp);
            fputc('\n', fp);
            ifprintf(depth, fp, "%s", operator_name(node->value.opr));
            break;
        case KEYWORD_T:
            fputc('(', fp);
            fputc('\n', fp);
            ifprintf(depth, fp, "%s", keyword_name(node->value.keyword));
            break;
        case LITERAL_T: {
            fputc('(', fp);
            fputc('\n', fp);
            char tmp[128];
            format_literal(ctx, node->value.id, tmp, sizeof(tmp));
            ifprintf(depth, fp, "LITERAL %zu \"%s\"", node->value.id, tmp);
            break;
        }
        case IDENTIFIER_T:
            fputc('(', fp);
            fputc('\n', fp);
            ifprintf(depth, fp, "IDENTIFIER %zu", node->value.id);
            break;
        case DELIMITER_T:
            fputc('(', fp);
            fputc('\n', fp);
            ifprintf(depth, fp, "%s", delimiter_name(node->value.delimiter));
            break;
        default:
            fputc('(', fp);
            fputc('\n', fp);
            ifprintf(depth, fp, "UNKNOWN");
            break;
    }

    if (!node->left)
        fputc('nil ', fp);
    else {
        fputc('\n', fp);
        write_node(fp, ctx, node->left, depth);
    }
    if (!node->right)
        fputc('nil ', fp);
    else {
        fputc('\n', fp);
        write_node(fp, ctx, node->right, depth);
    }
    fputc('\n', fp);
    ifprintf(--depth, fp, ")");
}

/**
 * @brief Сохраняет текущее AST в текстовый файл в префиксной форме.
 * @param ctx   контекст фронтенда с деревом.
 * @param path  путь к файлу.
 * @return 0 при успехе, -1 при ошибке.
 */
int save_ast_to_file(const FRONT_COMPL_T *ctx, const char *path) {
    if (!ctx || !ctx->root || !path)
        return -1;
    FILE *fp = fopen(path, "w");
    if (!fp)
        return -1;
    write_node(fp, ctx, ctx->root, 0);
    fputc('\n', fp);
    fclose(fp);
    return 0;
}