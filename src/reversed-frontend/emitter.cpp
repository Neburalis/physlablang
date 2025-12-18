#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rev-front.h"
#include "base.h"

function const char *builtin_name(OPERATOR::OPERATOR op) {
    switch (op) {
        case OPERATOR::SIN:  return "sin";
        case OPERATOR::COS:  return "cos";
        case OPERATOR::TAN:  return "tg";
        case OPERATOR::CTG:  return "ctg";
        case OPERATOR::ASIN: return "arcsin";
        case OPERATOR::ACOS: return "arccos";
        case OPERATOR::ATAN: return "arctan";
        case OPERATOR::ACTG: return "arcctg";
        case OPERATOR::SQRT: return "sqrt";
        case OPERATOR::LN:   return "ln";
        default:             return "";
    }
}

function int precedence(NODE_T *node) {
    if (!node) return 0;
    if (node->type == KEYWORD_T && node->value.keyword == KEYWORD::FUNC_CALL)
        return 7;
    if (node->type != OPERATOR_T)
        return 7;
    switch (node->value.opr) {
        case OPERATOR::OR:   return 1;
        case OPERATOR::AND:  return 2;
        case OPERATOR::EQ: case OPERATOR::NEQ:
        case OPERATOR::BELOW: case OPERATOR::ABOVE:
        case OPERATOR::BELOW_EQ: case OPERATOR::ABOVE_EQ:
            return 3;
        case OPERATOR::ADD: case OPERATOR::SUB:
            return 4;
        case OPERATOR::MUL: case OPERATOR::DIV: case OPERATOR::MOD:
            return 5;
        case OPERATOR::POW:
            return 6;
        case OPERATOR::NOT:
            return 7;
        default:
            return 7;
    }
}

function void mark_id(size_t id, char *known, size_t cap) {
    if (known && id < cap)
        known[id] = 1;
}

function void mark_comma_chain(NODE_T *node, char *known, size_t cap) {
    if (!node) return;
    if (node->type == DELIMITER_T && node->value.delimiter == DELIMITER::COMA) {
        mark_comma_chain(node->left, known, cap);
        mark_comma_chain(node->right, known, cap);
        return;
    }
    if (node->type == LITERAL_T || node->type == IDENTIFIER_T)
        mark_id(node->value.id, known, cap);
}

function void collect_declared(NODE_T *node, char *known, size_t cap) {
    if (!node) return;
    if (node->type == KEYWORD_T) {
        switch (node->value.keyword) {
            case KEYWORD::VAR_DECLARATION:
                if (node->left && (node->left->type == LITERAL_T || node->left->type == IDENTIFIER_T))
                    mark_id(node->left->value.id, known, cap);
                break;
            case KEYWORD::RETURN:
            case KEYWORD::IF:
            case KEYWORD::ELSE:
            case KEYWORD::THEN:
            case KEYWORD::WHILE:
            case KEYWORD::DO_WHILE:
                break;
            case KEYWORD::FUNC_CALL:
                break;
            default:
                break;
        }
    }
    if (node->type == OPERATOR_T && node->value.opr == OPERATOR::ASSIGNMENT) {
        if (node->left && (node->left->type == LITERAL_T || node->left->type == IDENTIFIER_T))
            mark_id(node->left->value.id, known, cap);
    }
    if (node->type == LITERAL_T) {
        /* функция: id хранится здесь; параметры в left */
        if (node->parent && node->parent->type == DELIMITER_T && node->parent->value.delimiter == DELIMITER::COMA)
            mark_id(node->value.id, known, cap);
    }
    if (node->type == LITERAL_T && node->left && node->parent == nullptr) {
        mark_id(node->value.id, known, cap);
    }
    if (node->type == LITERAL_T && node->left) {
        /* предположительно функция */
        mark_id(node->value.id, known, cap);
        mark_comma_chain(node->left, known, cap);
    }
    collect_declared(node->left, known, cap);
    collect_declared(node->right, known, cap);
}

function int emit_literal(FILE *out, varlist::VarList *vars, char *known, size_t cap, size_t id) {
    const mystr::mystr_t *entry = varlist::get(vars, id);
    const char *name = (entry && entry->str) ? entry->str : "";
    int is_decl = (known && id < cap && known[id]);
    if (is_decl)
        return fprintf(out, "%s", name);
    return fprintf(out, "\"%s\"", name);
}

function int emit_expr(FILE *out, NODE_T *node, varlist::VarList *vars, char *known, size_t cap, int parent_prec);

function int emit_builtin(FILE *out, OPERATOR::OPERATOR op, NODE_T *arg, varlist::VarList *vars, char *known, size_t cap) {
    const char *name = builtin_name(op);
    if (!name || !name[0])
        return -1;
    if (fprintf(out, "%s(", name) < 0)
        return -1;
    if (emit_expr(out, arg, vars, known, cap, 7) < 0)
        return -1;
    return fprintf(out, ")");
}

function int emit_pow(FILE *out, NODE_T *node, varlist::VarList *vars, char *known, size_t cap, int my_prec) {
    if (emit_expr(out, node->left, vars, known, cap, my_prec) < 0)
        return -1;
    if (fputs(" ^ ", out) < 0)
        return -1;
    return emit_expr(out, node->right, vars, known, cap, my_prec - 1);
}

function int emit_expr(FILE *out, NODE_T *node, varlist::VarList *vars, char *known, size_t cap, int parent_prec) {
    if (!out || !node) return -1;
    int my_prec = precedence(node);
    int need_paren = my_prec < parent_prec;

    if (need_paren && fputc('(', out) == EOF)
        return -1;

    if (node->type == NUMBER_T) {
        if (fprintf(out, "%g", node->value.num) < 0)
            return -1;
    } else if (node->type == LITERAL_T || node->type == IDENTIFIER_T) {
        if (emit_literal(out, vars, known, cap, node->value.id) < 0)
            return -1;
    } else if (node->type == KEYWORD_T && node->value.keyword == KEYWORD::FUNC_CALL) {
        NODE_T *name_node = node->left;
        if (!name_node || (name_node->type != LITERAL_T && name_node->type != IDENTIFIER_T))
            return -1;
        if (emit_literal(out, vars, known, cap, name_node->value.id) < 0)
            return -1;
        if (fputs(" ПРИМЕНЯЕМ ", out) < 0)
            return -1;
        NODE_T *args = node->right;
        while (args && args->type == DELIMITER_T && args->value.delimiter == DELIMITER::COMA) {
            if (emit_expr(out, args->left, vars, known, cap, 7) < 0)
                return -1;
            if (args->right && fputs(", ", out) < 0)
                return -1;
            args = args->right;
        }
        if (args) {
            if (emit_expr(out, args, vars, known, cap, 7) < 0)
                return -1;
        }
    } else if (node->type == OPERATOR_T) {
        switch (node->value.opr) {
            case OPERATOR::ADD: case OPERATOR::SUB: case OPERATOR::MUL:
            case OPERATOR::DIV: case OPERATOR::MOD: {
                if (emit_expr(out, node->left, vars, known, cap, my_prec) < 0)
                    return -1;
                const char *sep = " ? ";
                switch (node->value.opr) {
                    case OPERATOR::ADD: sep = " + "; break;
                    case OPERATOR::SUB: sep = " - "; break;
                    case OPERATOR::MUL: sep = " * "; break;
                    case OPERATOR::DIV: sep = " / "; break;
                    case OPERATOR::MOD: sep = " % "; break;
                    default: break;
                }
                if (fputs(sep, out) < 0)
                    return -1;
                if (emit_expr(out, node->right, vars, known, cap, my_prec) < 0)
                    return -1;
                break;
            }
            case OPERATOR::POW:
                if (emit_pow(out, node, vars, known, cap, my_prec) < 0)
                    return -1;
                break;
            case OPERATOR::EQ: case OPERATOR::NEQ:
            case OPERATOR::BELOW: case OPERATOR::ABOVE:
            case OPERATOR::BELOW_EQ: case OPERATOR::ABOVE_EQ: {
                const char *sep = "==";
                switch (node->value.opr) {
                    case OPERATOR::EQ: sep = " == "; break;
                    case OPERATOR::NEQ: sep = " != "; break;
                    case OPERATOR::BELOW: sep = " < "; break;
                    case OPERATOR::ABOVE: sep = " > "; break;
                    case OPERATOR::BELOW_EQ: sep = " <= "; break;
                    case OPERATOR::ABOVE_EQ: sep = " >= "; break;
                    default: break;
                }
                if (emit_expr(out, node->left, vars, known, cap, my_prec) < 0)
                    return -1;
                if (fputs(sep, out) < 0)
                    return -1;
                if (emit_expr(out, node->right, vars, known, cap, my_prec + 1) < 0)
                    return -1;
                break;
            }
            case OPERATOR::AND:
            case OPERATOR::OR: {
                const char *word = (node->value.opr == OPERATOR::AND) ? " И " : " ИЛИ ";
                if (emit_expr(out, node->left, vars, known, cap, my_prec) < 0)
                    return -1;
                if (fputs(word, out) < 0)
                    return -1;
                if (emit_expr(out, node->right, vars, known, cap, my_prec + 1) < 0)
                    return -1;
                break;
            }
            case OPERATOR::NOT:
                if (fputs("НЕ ", out) < 0)
                    return -1;
                if (emit_expr(out, node->left ? node->left : node->right, vars, known, cap, my_prec) < 0)
                    return -1;
                break;
            case OPERATOR::IN:
            case OPERATOR::OUT:
                if (node->value.opr == OPERATOR::IN) {
                    if (fputs("ИЗМЕРИТЬ ", out) < 0)
                        return -1;
                } else {
                    if (fputs("ПОКАЗАТЬ ", out) < 0)
                        return -1;
                }
                if (emit_expr(out, node->left, vars, known, cap, 7) < 0)
                    return -1;
                break;
            case OPERATOR::LN:
            case OPERATOR::SIN:
            case OPERATOR::COS:
            case OPERATOR::TAN:
            case OPERATOR::CTG:
            case OPERATOR::ASIN:
            case OPERATOR::ACOS:
            case OPERATOR::ATAN:
            case OPERATOR::ACTG:
            case OPERATOR::SQRT:
                if (emit_builtin(out, node->value.opr, node->left ? node->left : node->right, vars, known, cap) < 0)
                    return -1;
                break;
            default:
                return -1;
        }
    }

    if (need_paren && fputc(')', out) == EOF)
        return -1;
    return 0;
}

function int emit_line_start(FILE *out, int indent) {
    for (int i = 0; i < indent; ++i) {
        if (fputc(' ', out) == EOF)
            return -1;
    }
    return 0;
}

function int emit_operator(FILE *out, NODE_T *node, varlist::VarList *vars, char *known, size_t cap, int indent);

function int emit_connector(FILE *out, NODE_T *node, varlist::VarList *vars, char *known, size_t cap, int indent) {
    if (!node) return 0;
    if (node->type == OPERATOR_T && node->value.opr == OPERATOR::CONNECTOR) {
        if (emit_connector(out, node->left, vars, known, cap, indent) < 0)
            return -1;
        if (node->right && fputc('\n', out) == EOF)
            return -1;
        return emit_connector(out, node->right, vars, known, cap, indent);
    }
    if (emit_operator(out, node, vars, known, cap, indent) < 0)
        return -1;
    return 0;
}

function int emit_if(FILE *out, NODE_T *node, varlist::VarList *vars, char *known, size_t cap, int indent) {
    if (!node || !node->right) return -1;
    if (emit_line_start(out, indent) < 0) return -1;
    if (fputs("ЕСЛИ ", out) < 0) return -1;
    if (emit_expr(out, node->left, vars, known, cap, 0) < 0) return -1;
    if (fputs(" ТО\n", out) < 0) return -1;

    NODE_T *then_branch = node->right->left;
    NODE_T *else_branch = node->right->right;
    if (emit_connector(out, then_branch, vars, known, cap, indent + 4) < 0)
        return -1;
    if (else_branch) {
        if (fputc('\n', out) == EOF) return -1;
        if (emit_line_start(out, indent) < 0) return -1;
        if (fputs("ИНАЧЕ\n", out) < 0) return -1;
        if (emit_connector(out, else_branch, vars, known, cap, indent + 4) < 0)
            return -1;
    }
    return 0;
}

function int emit_while(FILE *out, NODE_T *node, varlist::VarList *vars, char *known, size_t cap, int indent) {
    if (!node) return -1;
    if (emit_line_start(out, indent) < 0) return -1;
    if (fputs("ПОКА ", out) < 0) return -1;
    if (emit_expr(out, node->left, vars, known, cap, 0) < 0) return -1;
    if (fputs(" ПОВТОРЯЕМ\n", out) < 0) return -1;
    if (emit_connector(out, node->right, vars, known, cap, indent + 4) < 0)
        return -1;
    if (fputc('\n', out) == EOF) return -1;
    if (emit_line_start(out, indent) < 0) return -1;
    if (fputs("СТОП", out) < 0) return -1;
    return 0;
}

function int emit_do_while(FILE *out, NODE_T *node, varlist::VarList *vars, char *known, size_t cap, int indent) {
    if (!node) return -1;
    if (emit_line_start(out, indent) < 0) return -1;
    if (fputs("ПОВТОРЯЕМ\n", out) < 0) return -1;
    if (emit_connector(out, node->right, vars, known, cap, indent + 4) < 0)
        return -1;
    if (fputc('\n', out) == EOF) return -1;
    if (emit_line_start(out, indent) < 0) return -1;
    if (fputs("ПОКА ", out) < 0) return -1;
    if (emit_expr(out, node->left, vars, known, cap, 0) < 0) return -1;
    if (fputs(" СТОП", out) < 0) return -1;
    return 0;
}

function int emit_operator(FILE *out, NODE_T *node, varlist::VarList *vars, char *known, size_t cap, int indent) {
    if (!node) return -1;
    if (node->type == KEYWORD_T) {
        switch (node->value.keyword) {
            case KEYWORD::VAR_DECLARATION:
                if (emit_line_start(out, indent) < 0) return -1;
                if (fputs("ВЕЛИЧИНА ", out) < 0) return -1;
                if (!node->left) return -1;
                if (emit_literal(out, vars, known, cap, node->left->value.id) < 0) return -1;
                return 0;
            case KEYWORD::RETURN:
                if (emit_line_start(out, indent) < 0) return -1;
                if (fputs("ВОЗВРАТИТЬ ", out) < 0) return -1;
                return emit_expr(out, node->left, vars, known, cap, 0);
            case KEYWORD::IF:
                return emit_if(out, node, vars, known, cap, indent);
            case KEYWORD::WHILE:
                return emit_while(out, node, vars, known, cap, indent);
            case KEYWORD::DO_WHILE:
                return emit_do_while(out, node, vars, known, cap, indent);
            case KEYWORD::FUNC_CALL:
                if (emit_line_start(out, indent) < 0) return -1;
                return emit_expr(out, node, vars, known, cap, 0);
            default:
                break;
        }
    }
    if (node->type == OPERATOR_T) {
        if (node->value.opr == OPERATOR::ASSIGNMENT) {
            if (emit_line_start(out, indent) < 0) return -1;
            if (emit_expr(out, node->left, vars, known, cap, 0) < 0) return -1;
            if (fputs(" = ", out) < 0) return -1;
            return emit_expr(out, node->right, vars, known, cap, 0);
        }
        if (node->value.opr == OPERATOR::OUT || node->value.opr == OPERATOR::IN) {
            if (emit_line_start(out, indent) < 0) return -1;
            return emit_expr(out, node, vars, known, cap, 0);
        }
    }
    return -1;
}

function int emit_params(FILE *out, NODE_T *node, varlist::VarList *vars, char *known, size_t cap) {
    NODE_T *p = node;
    while (p && p->type == DELIMITER_T && p->value.delimiter == DELIMITER::COMA) {
        if (emit_literal(out, vars, known, cap, p->left->value.id) < 0)
            return -1;
        if (p->right && fputs(", ", out) < 0)
            return -1;
        p = p->right;
    }
    if (p) {
        if (emit_literal(out, vars, known, cap, p->value.id) < 0)
            return -1;
    }
    return 0;
}

function int emit_function(FILE *out, NODE_T *node, varlist::VarList *vars, char *known, size_t cap) {
    if (!node || node->type != LITERAL_T) return -1;
    if (fputs("ФОРМУЛА ", out) < 0) return -1;
    if (emit_literal(out, vars, known, cap, node->value.id) < 0) return -1;
    if (fputc(' ', out) == EOF) return -1;
    if (fputc('(', out) == EOF) return -1;
    if (node->left) {
        if (emit_params(out, node->left, vars, known, cap) < 0) return -1;
    }
    if (fputs(")\n", out) < 0) return -1;
    if (emit_connector(out, node->right, vars, known, cap, 4) < 0) return -1;
    if (fputc('\n', out) == EOF) return -1;
    if (fputs("КОНЕЦ ФОРМУЛЫ", out) < 0) return -1;
    return 0;
}

function int emit_function_list(FILE *out, NODE_T *node, varlist::VarList *vars, char *known, size_t cap) {
    if (!node) return 0;
    if (node->type == DELIMITER_T && node->value.delimiter == DELIMITER::COMA) {
        if (emit_function_list(out, node->left, vars, known, cap) < 0)
            return -1;
        if (fputc('\n', out) == EOF) return -1;
        return emit_function_list(out, node->right, vars, known, cap);
    }
    return emit_function(out, node, vars, known, cap);
}

function NODE_T *extract_exp(NODE_T *root) {
    if (!root) return nullptr;
    if (root->type == OPERATOR_T && root->value.opr == OPERATOR::CONNECTOR)
        return root->left;
    return root;
}

function NODE_T *extract_res(NODE_T *root) {
    if (!root) return nullptr;
    if (root->type == OPERATOR_T && root->value.opr == OPERATOR::CONNECTOR)
        return root->right;
    return nullptr;
}

int emit_program(NODE_T *root, varlist::VarList *vars, FILE *out) {
    if (!root || !vars || !out)
        return -1;

    size_t sym_cap = varlist::size(vars);
    char *known = nullptr;
    if (sym_cap) {
        known = (char *) calloc(sym_cap, sizeof(char));
        if (!known)
            return -1;
    }

    collect_declared(root, known, sym_cap);

    do {

        if (fprintf(out, "ЛАБОРАТОРНАЯ РАБОТА %s\n\n", "Восстановленная") < 0)
            break;
        if (fputs("АННОТАЦИЯ\nЦЕЛЬ: восстановлено из AST\nКОНЕЦ АННОТАЦИИ\n\n", out) < 0)
            break;

        if (fputs("ТЕОРЕТИЧЕСКИЕ СВЕДЕНИЯ\n", out) < 0)
            break;
        if (emit_function_list(out, root->left, vars, known, sym_cap) < 0)
            break;
        if (fputs("\nКОНЕЦ ТЕОРИИ\n\n", out) < 0)
            break;

        NODE_T *exp_ops = extract_exp(root->right);
        NODE_T *res_ops = extract_res(root->right);

        if (fputs("ХОД РАБОТЫ\n", out) < 0)
            break;
        if (exp_ops && emit_connector(out, exp_ops, vars, known, sym_cap, 0) < 0)
            break;
        if (fputs("\nКОНЕЦ РАБОТЫ\n\n", out) < 0)
            break;

        if (res_ops) {
            if (fputs("ОБСУЖДЕНИЕ РЕЗУЛЬТАТОВ\n", out) < 0)
                break;
            if (emit_connector(out, res_ops, vars, known, sym_cap, 0) < 0)
                break;
            if (fputs("\nКОНЕЦ РЕЗУЛЬТАТОВ\n\n", out) < 0)
                break;
        }

        if (fputs("ВЫВОДЫ\nВосстановлено автоматически\nКОНЕЦ ВЫВОДОВ\n", out) < 0)
            break;
        if (fputs("AI generated for reference only\n", out) < 0)
            break;

        free(known);
        return 0;

    } while(0);
    free(known);
    return -1;
}
