#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "frontend.h"
#include "ast.h"
#include "base.h"

struct parser_t {
    FRONT_COMPL_T *ctx;
    size_t         pos;
    bool           error;
};

static NODE_T *get_program                  (parser_t *p);
static NODE_T *get_report                   (parser_t *p);
static NODE_T *get_annotation               (parser_t *p);
static NODE_T *get_theoretical_background   (parser_t *p);
static NODE_T *get_function_declaration     (parser_t *p);
static NODE_T *get_identifier_list          (parser_t *p);
static NODE_T *get_experimental_procedure   (parser_t *p);
static NODE_T *get_results_discussion       (parser_t *p);
static NODE_T *get_conclusion               (parser_t *p);
static NODE_T *get_operators                (parser_t *p, KEYWORD::KEYWORD stop_kw);
static NODE_T *get_operator                 (parser_t *p);
static NODE_T *get_variable_declaration     (parser_t *p);
static NODE_T *get_assignment               (parser_t *p);
static NODE_T *get_conditional              (parser_t *p);
static NODE_T *get_loop                     (parser_t *p);
static NODE_T *get_io_statement             (parser_t *p);
static NODE_T *get_return_statement         (parser_t *p);
static NODE_T *get_function_call            (parser_t *p);
static NODE_T *get_arguments                (parser_t *p);
static NODE_T *get_expression               (parser_t *p);
static NODE_T *get_logical_expression       (parser_t *p);
static NODE_T *get_comparison_expression    (parser_t *p);
static NODE_T *get_math_expression          (parser_t *p);
static NODE_T *get_power                    (parser_t *p);
static NODE_T *get_term                     (parser_t *p);
static NODE_T *get_factor                   (parser_t *p);
static NODE_T *get_internal_func_call       (parser_t *p);

static TOKEN_T *get_token                   (parser_t *p);
static TOKEN_T *get_next                    (parser_t *p);
static TOKEN_T *next_token                  (parser_t *p);
static bool     is_end                      (parser_t *p);
static bool     match_keyword               (parser_t *p, KEYWORD::KEYWORD kw, TOKEN_T **out);
static bool     match_operator              (parser_t *p, OPERATOR::OPERATOR op, TOKEN_T **out);
static bool     match_delim                 (parser_t *p, DELIMITER::DELIMITER d, TOKEN_T **out);
static NODE_T  *make_synthetic              (parser_t *p, NODE_TYPE type, NODE_VALUE_T val);
static NODE_T  *make_connector              (parser_t *p, NODE_T *left, NODE_T *right);
static NODE_T  *make_comma_node             (parser_t *p, NODE_T *left, NODE_T *right);
static NODE_T  *clone_identifier_literal    (parser_t *p, const TOKEN_T *tok);
static NODE_T  *make_literal_from_id        (parser_t *p, size_t id, const TOKEN_T *src);
static NODE_T  *attach_comma_chain          (parser_t *p, NODE_T *first);

/**
 * @brief Создает AST для массива токенов и сохраняет его в ctx->root.
 * @param ctx[in,out] контекст лексера/парсера.
 * @return 0 при успехе, -1 при ошибке.
 */
int parse_tokens(FRONT_COMPL_T *ctx) {
    if (!ctx || !ctx->tokens) return -1;
    // стараемся зарезервировать место под синтетические узлы, чтобы не двигать указатели
    size_t reserve = ctx->token_count * 8 + 64;
    if (ensure_token_cap(ctx, reserve))
        return -1;
    parser_t p = { ctx, 0, false };
    NODE_T *root = get_program(&p);
    if (p.error || !root)
        return -1;
    ctx->root = root;
    recount_elements(root);
    root->parent = nullptr;
    return 0;
}

/**
 * @brief Возвращает текущий токен или nullptr.
 */
static TOKEN_T *get_token(parser_t *p) {
    if (!p || !p->ctx) return nullptr;
    return (p->pos < p->ctx->token_count) ? &p->ctx->tokens[p->pos] : nullptr;
}

/**
 * @brief Возвращает следующий токен без сдвига.
 */
static TOKEN_T *get_next(parser_t *p) {
    if (!p || !p->ctx) return nullptr;
    size_t idx = p->pos + 1;
    return (idx < p->ctx->token_count) ? &p->ctx->tokens[idx] : nullptr;
}

/**
 * @brief Сдвигает позицию на один токен и возвращает его.
 */
static TOKEN_T *next_token(parser_t *p) {
    if (!p || !p->ctx) return nullptr;
    if (p->pos >= p->ctx->token_count) return nullptr;
    return &p->ctx->tokens[p->pos++];
}

/**
 * @brief Проверяет, достигнут ли конец потока токенов.
 */
static bool is_end(parser_t *p) {
    return !p || !p->ctx || p->pos >= p->ctx->token_count;
}

/**
 * @brief Сопоставляет ключевое слово.
 */
static bool match_keyword(parser_t *p, KEYWORD::KEYWORD kw, TOKEN_T **out) {
    TOKEN_T *tok = get_token(p);
    if (tok && tok->node.type == KEYWORD_T && tok->node.value.keyword == kw) {
        if (out) *out = tok;
        ++p->pos;
        return true;
    }
    return false;
}

/**
 * @brief Сопоставляет оператор.
 */
static bool match_operator(parser_t *p, OPERATOR::OPERATOR op, TOKEN_T **out) {
    TOKEN_T *tok = get_token(p);
    if (tok && tok->node.type == OPERATOR_T && tok->node.value.opr == op) {
        if (out) *out = tok;
        ++p->pos;
        return true;
    }
    return false;
}

/**
 * @brief Сопоставляет разделитель.
 */
static bool match_delim(parser_t *p, DELIMITER::DELIMITER d, TOKEN_T **out) {
    TOKEN_T *tok = get_token(p);
    if (tok && tok->node.type == DELIMITER_T && tok->node.value.delimiter == d) {
        if (out) *out = tok;
        ++p->pos;
        return true;
    }
    return false;
}

/**
 * @brief Создает синтетический токен (который не содержался в исходном тексте) и возвращает его узел.
 */
static NODE_T *make_synthetic(parser_t *p, NODE_TYPE type, NODE_VALUE_T val) {
    if (!p || !p->ctx) return nullptr;
    if (add_token(p->ctx, type, val, nullptr, 0, 0, 0)) {
        p->error = true;
        return nullptr;
    }
    return &p->ctx->tokens[p->ctx->token_count - 1].node;
}

/**
 * @brief Создает узел CONNECTOR и связывает потомков.
 */
static NODE_T *make_connector(parser_t *p, NODE_T *left, NODE_T *right) {
    NODE_VALUE_T v = {};
    v.opr = OPERATOR::CONNECTOR;
    NODE_T *node = make_synthetic(p, OPERATOR_T, v);
    if (!node) return nullptr;
    set_children(node, left, right);
    return node;
}

/**
 * @brief Создает узел с разделителем COMA и связывает потомков.
 */
static NODE_T *make_comma_node(parser_t *p, NODE_T *left, NODE_T *right) {
    NODE_VALUE_T v = {};
    v.delimiter = DELIMITER::COMA;
    NODE_T *node = make_synthetic(p, DELIMITER_T, v);
    if (!node) return nullptr;
    set_children(node, left, right);
    return node;
}

/**
 * @brief Клонирует идентификатор как LITERAL-узел.
 */
static NODE_T *clone_identifier_literal(parser_t *p, const TOKEN_T *tok) {
    if (!tok || tok->node.type != IDENTIFIER_T) return nullptr;
    NODE_VALUE_T v = tok->node.value;
    return make_synthetic(p, LITERAL_T, v);
}

/**
 * @brief Создает LITERAL-узел по индексу VarList.
 */
static NODE_T *make_literal_from_id(parser_t *p, size_t id, const TOKEN_T *src) {
    NODE_VALUE_T v = {};
    v.id = id;
    const char *text = src ? src->text : nullptr;
    size_t len = src ? src->length : 0;
    if (add_token(p->ctx, LITERAL_T, v, text, len, 0, 0)) {
        p->error = true;
        return nullptr;
    }
    return &p->ctx->tokens[p->ctx->token_count - 1].node;
}

/**
 * @brief Устанавливает цепочку узлов с использованием существующих запятых.
 */
static NODE_T *attach_comma_chain(parser_t *p, NODE_T *first) {
    NODE_T *current = first;
    while (true) {
        TOKEN_T *comma_tok = nullptr;
        if (!match_delim(p, DELIMITER::COMA, &comma_tok))
            break;
        NODE_T *next_expr = get_expression(p);
        if (!next_expr) {
            p->error = true;
            return nullptr;
        }
        set_children(&comma_tok->node, current, next_expr);
        current = &comma_tok->node;
    }
    return current;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* Грамматические правила.                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * @brief program ::= report
 */
static NODE_T *get_program(parser_t *p) {
    return get_report(p);
}

/**
 * @brief report ::= "ЛАБОРАТОРНАЯ РАБОТА" literal annotation theoretical_background experimental_procedure results_discussion conclusion
 */
static NODE_T *get_report(parser_t *p) {
    TOKEN_T *lab_tok = nullptr;
    if (!match_keyword(p, KEYWORD::LAB, &lab_tok)) {
        p->error = true;
        return nullptr;
    }
    /* заголовочный литерал можно пропустить */
    TOKEN_T *lit_tok = get_token(p);
    if (lit_tok && lit_tok->node.type == LITERAL_T)
        next_token(p);

    if (!get_annotation(p)) return nullptr;
    NODE_T *funcs = get_theoretical_background(p);
    if (p->error) return nullptr;
    NODE_T *exp_body = get_experimental_procedure(p);
    if (p->error) return nullptr;
    NODE_T *res_body = get_results_discussion(p);
    if (p->error) return nullptr;
    if (!get_conclusion(p)) return nullptr;

    NODE_T *body = exp_body;
    if (res_body) {
        if (body)
            body = make_connector(p, body, res_body);
        else
            body = res_body;
    }

    if (!body && funcs)
        body = funcs;

    if (!funcs && !body)
        return nullptr;

    NODE_T *root = make_connector(p, funcs, body);
    return root;
}

/**
 * @brief annotation ::= "АННОТАЦИЯ" "ЦЕЛЬ:" literal ... "КОНЕЦ АННОТАЦИИ"
 */
static NODE_T *get_annotation(parser_t *p) {
    TOKEN_T *ann = nullptr;
    if (!match_keyword(p, KEYWORD::ANNOTATION, &ann)) {
        p->error = true;
        return nullptr;
    }
    /* упрощенно: пропускаем все до END_ANNOTATION */
    while (!is_end(p)) {
        TOKEN_T *tok = get_token(p);
        if (tok && tok->node.type == KEYWORD_T && tok->node.value.keyword == KEYWORD::END_ANNOTATION) {
            next_token(p);
            return ann ? &ann->node : nullptr;
        }
        next_token(p);
    }
    p->error = true;
    return nullptr;
}

/**
 * @brief theoretical_background ::= "ТЕОРЕТИЧЕСКИЕ СВЕДЕНИЯ" (literal* function_declaration+)+ "КОНЕЦ ТЕОРИИ"
 */
static NODE_T *get_theoretical_background(parser_t *p) {
    TOKEN_T *start = nullptr;
    if (!match_keyword(p, KEYWORD::THEORETICAL, &start)) {
        p->error = true;
        return nullptr;
    }
    NODE_T *func_list = nullptr;
    while (!is_end(p)) {
        TOKEN_T *tok = get_token(p);
        if (tok && tok->node.type == KEYWORD_T && tok->node.value.keyword == KEYWORD::END_THEORETICAL) {
            next_token(p);
            return func_list;
        }
        /* пропускаем литералы */
        if (tok && tok->node.type == LITERAL_T) {
            next_token(p);
            continue;
        }
        NODE_T *f = get_function_declaration(p);
        if (!f) {
            p->error = true;
            return nullptr;
        }
        if (func_list)
            func_list = make_comma_node(p, func_list, f);
        else
            func_list = f;
    }
    p->error = true;
    return nullptr;
}

/**
 * @brief function_declaration ::= "ФОРМУЛА" identifier "(" identifier_list ")" operators "КОНЕЦ ФОРМУЛЫ"
 */
static NODE_T *get_function_declaration(parser_t *p) {
    TOKEN_T *kw = nullptr;
    if (!match_keyword(p, KEYWORD::FORMULA, &kw)) {
        p->error = true;
        return nullptr;
    }
    TOKEN_T *name_tok = get_token(p);
    if (!name_tok || name_tok->node.type != IDENTIFIER_T) {
        p->error = true;
        return nullptr;
    }
    next_token(p);
    NODE_T *name_lit = clone_identifier_literal(p, name_tok);
    if (!name_lit) return nullptr;

    if (!match_delim(p, DELIMITER::PAR_OPEN, nullptr)) {
        p->error = true;
        return nullptr;
    }
    NODE_T *params = get_identifier_list(p);
    if (!match_delim(p, DELIMITER::PAR_CLOSE, nullptr)) {
        p->error = true;
        return nullptr;
    }

    NODE_T *body = get_operators(p, KEYWORD::END_FORMULA);
    if (p->error) return nullptr;
    if (!match_keyword(p, KEYWORD::END_FORMULA, nullptr)) {
        p->error = true;
        return nullptr;
    }

    /* имя функции хранится в корне, параметры слева, тело справа */
    set_children(name_lit, params, body);
    return name_lit;
}

/**
 * @brief identifier_list ::= identifier ("," identifier)*
 */
static NODE_T *get_identifier_list(parser_t *p) {
    TOKEN_T *tok = get_token(p);
    if (!tok || tok->node.type != IDENTIFIER_T)
        return nullptr;
    next_token(p);
    NODE_T *first = clone_identifier_literal(p, tok);
    if (!first) return nullptr;
    NODE_T *current = first;
    while (true) {
        TOKEN_T *comma_tok = nullptr;
        if (!match_delim(p, DELIMITER::COMA, &comma_tok))
            break;
        TOKEN_T *next_id = get_token(p);
        if (!next_id || next_id->node.type != IDENTIFIER_T) {
            p->error = true;
            return nullptr;
        }
        next_token(p);
        NODE_T *lit = clone_identifier_literal(p, next_id);
        if (!lit) return nullptr;
        set_children(&comma_tok->node, current, lit);
        current = &comma_tok->node;
    }
    return current;
}

/**
 * @brief experimental_procedure ::= "ХОД РАБОТЫ" operators "КОНЕЦ РАБОТЫ"
 */
static NODE_T *get_experimental_procedure(parser_t *p) {
    if (!match_keyword(p, KEYWORD::EXPERIMENTAL, nullptr)) {
        p->error = true;
        return nullptr;
    }
    NODE_T *ops = get_operators(p, KEYWORD::END_EXPERIMENTAL);
    if (p->error) return nullptr;
    if (!match_keyword(p, KEYWORD::END_EXPERIMENTAL, nullptr)) {
        p->error = true;
        return nullptr;
    }
    return ops;
}

/**
 * @brief results_discussion ::= "ОБСУЖДЕНИЕ РЕЗУЛЬТАТОВ" operators "КОНЕЦ РЕЗУЛЬТАТОВ"
 */
static NODE_T *get_results_discussion(parser_t *p) {
    if (!match_keyword(p, KEYWORD::RESULTS, nullptr)) {
        p->error = true;
        return nullptr;
    }
    NODE_T *ops = get_operators(p, KEYWORD::END_RESULTS);
    if (p->error) return nullptr;
    if (!match_keyword(p, KEYWORD::END_RESULTS, nullptr)) {
        p->error = true;
        return nullptr;
    }
    return ops;
}

/**
 * @brief conclusion ::= "ВЫВОДЫ" literal "КОНЕЦ ВЫВОДОВ" | "КОНЕЦ ВЫВОДА"
 */
static NODE_T *get_conclusion(parser_t *p) {
    if (!match_keyword(p, KEYWORD::CONCLUSION, nullptr)) {
        p->error = true;
        return nullptr;
    }
    TOKEN_T *lit = get_token(p);
    if (lit && lit->node.type == LITERAL_T)
        next_token(p);
    TOKEN_T *end_tok = get_token(p);
    if (!end_tok || end_tok->node.type != KEYWORD_T ||
        (end_tok->node.value.keyword != KEYWORD::END_CONCLUSION)) {
        p->error = true;
        return nullptr;
    }
    next_token(p);
    return lit ? &lit->node : nullptr;
}

/**
 * @brief operators ::= operator+
 */
static NODE_T *get_operators(parser_t *p, KEYWORD::KEYWORD stop_kw) {
    NODE_T *first = nullptr;
    while (!is_end(p)) {
        TOKEN_T *tok = get_token(p);
        if (is_operator_stop(tok, stop_kw))
            break;
        if (!is_operator_start(tok))
            break;
        NODE_T *op = get_operator(p);
        if (!op) {
            p->error = true;
            return nullptr;
        }
        if (first)
            first = make_connector(p, first, op);
        else
            first = op;
    }
    return first;
}

/**
 * @brief operator ::= variable_declaration | assignment | function_call | conditional | loop | io_statement | return_statement
 */
static NODE_T *get_operator(parser_t *p) {
    TOKEN_T *tok = get_token(p);
    if (!tok) return nullptr;
    if (tok->node.type == KEYWORD_T) {
        switch (tok->node.value.keyword) {
            case KEYWORD::VAR_DECLARATION: return get_variable_declaration(p);
            case KEYWORD::IF:             return get_conditional(p);
            case KEYWORD::WHILE:
            case KEYWORD::WHILE_CONDITION:return get_loop(p);
            case KEYWORD::RETURN:         return get_return_statement(p);
            case KEYWORD::FUNC_CALL:      return get_function_call(p);
            default: break;
        }
        if (keyword_is_io(tok))
            return get_io_statement(p);
    }
    /* попытка присваивания или вызова по идентификатору */
    if (tok->node.type == IDENTIFIER_T) {
        /* проверим шаблон id = expr */
        size_t save = p->pos;
        TOKEN_T *id_tok = next_token(p);
        TOKEN_T *next = get_token(p);
        if (next && next->node.type == OPERATOR_T && next->node.value.opr == OPERATOR::ASSIGNMENT) {
            p->pos = save;
            return get_assignment(p);
        }
        p->pos = save;
        return get_function_call(p);
    }
    /* логическое: может быть выражением в качестве оператора */
    return get_expression(p);
}

/**
 * @brief variable_declaration ::= "ВЕЛИЧИНА" identifier | "ВЕЛИЧИНА" identifier "=" expression
 */
static NODE_T *get_variable_declaration(parser_t *p) {
    TOKEN_T *kw = nullptr;
    if (!match_keyword(p, KEYWORD::VAR_DECLARATION, &kw)) {
        p->error = true;
        return nullptr;
    }
    TOKEN_T *id_tok = get_token(p);
    if (!id_tok || id_tok->node.type != IDENTIFIER_T) {
        p->error = true;
        return nullptr;
    }
    next_token(p);
    NODE_T *decl_name = clone_identifier_literal(p, id_tok);
    if (!decl_name) return nullptr;
    set_children(&kw->node, decl_name, nullptr);

    TOKEN_T *next = get_token(p);
    if (!next || next->node.type != OPERATOR_T || next->node.value.opr != OPERATOR::ASSIGNMENT)
        return &kw->node;

    next_token(p); /* consume '=' */
    NODE_T *rhs = get_expression(p);
    if (!rhs) {
        p->error = true;
        return nullptr;
    }
    NODE_T *assign_name = clone_identifier_literal(p, id_tok);
    if (!assign_name) return nullptr;
    NODE_T *assign_node = &next->node;
    set_children(assign_node, assign_name, rhs);
    NODE_T *conn = make_connector(p, &kw->node, assign_node);
    return conn;
}

/**
 * @brief assignment ::= identifier "=" expression
 */
static NODE_T *get_assignment(parser_t *p) {
    TOKEN_T *id_tok = get_token(p);
    if (!id_tok || id_tok->node.type != IDENTIFIER_T)
        return nullptr;
    next_token(p);
    TOKEN_T *eq_tok = nullptr;
    if (!match_operator(p, OPERATOR::ASSIGNMENT, &eq_tok)) {
        p->pos -= 1; /* rollback id */
        return nullptr;
    }
    NODE_T *lhs = clone_identifier_literal(p, id_tok);
    if (!lhs) return nullptr;
    NODE_T *rhs = get_expression(p);
    if (!rhs) {
        p->error = true;
        return nullptr;
    }
    set_children(&eq_tok->node, lhs, rhs);
    return &eq_tok->node;
}

/**
 * @brief conditional ::= "ЕСЛИ" expression "ТО" operators ("ИНАЧЕ" operators)?
 */
static NODE_T *get_conditional(parser_t *p) {
    TOKEN_T *if_tok = nullptr;
    if (!match_keyword(p, KEYWORD::IF, &if_tok)) {
        p->error = true;
        return nullptr;
    }
    NODE_T *cond = get_expression(p);
    if (!cond) {
        p->error = true;
        return nullptr;
    }
    TOKEN_T *then_tok = nullptr;
    if (!match_keyword(p, KEYWORD::THEN, &then_tok)) {
        p->error = true;
        return nullptr;
    }
    NODE_T *then_ops = get_operators(p, KEYWORD::ELSE);
    if (p->error) return nullptr;
    NODE_T *else_ops = nullptr;
    if (match_keyword(p, KEYWORD::ELSE, nullptr)) {
        else_ops = get_operators(p, KEYWORD::ELSE);
    }
    set_children(&then_tok->node, then_ops, else_ops);
    set_children(&if_tok->node, cond, &then_tok->node);
    return &if_tok->node;
}

/**
 * @brief loop ::= "ПОКА" expression "ПОВТОРЯЕМ" operators "СТОП" | "ПОВТОРЯЕМ" operators "ПОКА" expression "СТОП"
 */
static NODE_T *get_loop(parser_t *p) {
    TOKEN_T *first = get_token(p);
    if (!first) return nullptr;
    if (first->node.type != KEYWORD_T) return nullptr;

    /* while: ПОКА expr ПОВТОРЯЕМ ops СТОП */
    if (first->node.value.keyword == KEYWORD::WHILE_CONDITION) {
        next_token(p);
        NODE_T *cond = get_expression(p);
        if (!cond) { p->error = true; return nullptr; }
        TOKEN_T *body_kw = nullptr;
        if (!match_keyword(p, KEYWORD::WHILE, &body_kw)) { p->error = true; return nullptr; }
        NODE_T *body = get_operators(p, KEYWORD::END_WHILE);
        if (!match_keyword(p, KEYWORD::END_WHILE, nullptr)) { p->error = true; return nullptr; }
        set_children(&body_kw->node, cond, body);
        return &body_kw->node;
    }

    /* do-while: ПОВТОРЯЕМ ops ПОКА expr СТОП */
    if (first->node.value.keyword == KEYWORD::WHILE) {
        next_token(p);
        NODE_T *body = get_operators(p, KEYWORD::WHILE_CONDITION);
        TOKEN_T *cond_kw = nullptr;
        if (!match_keyword(p, KEYWORD::WHILE_CONDITION, &cond_kw)) { p->error = true; return nullptr; }
        NODE_T *cond = get_expression(p);
        if (!cond) { p->error = true; return nullptr; }
        if (!match_keyword(p, KEYWORD::END_WHILE, nullptr)) { p->error = true; return nullptr; }
        cond_kw->node.value.keyword = KEYWORD::DO_WHILE;
        set_children(&cond_kw->node, cond, body);
        return &cond_kw->node;
    }

    return nullptr;
}

/**
 * @brief io_statement ::= "ВЫВЕСТИ" expression | "ПОКАЗАТЬ" expression | "ИЗМЕРИТЬ" identifier
 */
static NODE_T *get_io_statement(parser_t *p) {
    TOKEN_T *tok = next_token(p);
    if (!tok) return nullptr;
    OPERATOR::OPERATOR op = OPERATOR::OUT;
    if (tok->node.type == KEYWORD_T) {
        /* значения OUT/IN закодированы в поле keyword */
        int raw = (int) tok->node.value.keyword;
        if (raw == (int) OPERATOR::IN) op = OPERATOR::IN;
        else op = OPERATOR::OUT;
    } else if (tok->node.type == OPERATOR_T) {
        op = tok->node.value.opr;
    }
    NODE_T *arg = nullptr;
    if (op == OPERATOR::IN) {
        TOKEN_T *id = get_token(p);
        if (!id || id->node.type != IDENTIFIER_T) { p->error = true; return nullptr; }
        next_token(p);
        arg = clone_identifier_literal(p, id);
    } else {
        arg = get_expression(p);
    }
    if (!arg) { p->error = true; return nullptr; }
    tok->node.type = OPERATOR_T;
    tok->node.value.opr = op;
    set_children(&tok->node, arg, nullptr);
    return &tok->node;
}

/**
 * @brief return_statement ::= "ВОЗВРАТИТЬ" expression
 */
static NODE_T *get_return_statement(parser_t *p) {
    TOKEN_T *kw = nullptr;
    if (!match_keyword(p, KEYWORD::RETURN, &kw)) {
        p->error = true;
        return nullptr;
    }
    NODE_T *expr = get_expression(p);
    if (!expr) { p->error = true; return nullptr; }
    set_children(&kw->node, expr, nullptr);
    return &kw->node;
}

/**
 * @brief function_call ::= identifier arguments | identifier KEYWORD.FUNC_CALL arguments | identifier "(" arguments ")"
 */
static NODE_T *get_function_call(parser_t *p) {
    size_t start = p->pos;
    TOKEN_T *id_tok = get_token(p);
    if (!id_tok || id_tok->node.type != IDENTIFIER_T)
        return nullptr;
    next_token(p);

    bool had_keyword = false;
    TOKEN_T *call_kw = nullptr;
    if (match_keyword(p, KEYWORD::FUNC_CALL, &call_kw))
        had_keyword = true;

    bool has_parens = false;
    if (match_delim(p, DELIMITER::PAR_OPEN, nullptr))
        has_parens = true;

    NODE_T *args = get_arguments(p);
    if (!args) {
        p->pos = start;
        return nullptr;
    }
    if (has_parens) {
        if (!match_delim(p, DELIMITER::PAR_CLOSE, nullptr)) {
            p->error = true;
            return nullptr;
        }
    }

    NODE_T *name_lit = clone_identifier_literal(p, id_tok);
    if (!name_lit) return nullptr;

    NODE_T *root = nullptr;
    if (call_kw)
        root = &call_kw->node;
    else {
        NODE_VALUE_T v = {};
        v.keyword = KEYWORD::FUNC_CALL;
        root = make_synthetic(p, KEYWORD_T, v);
    }
    set_children(root, name_lit, args);
    return root;
}

/**
 * @brief arguments ::= expression ("," expression)*
 */
static NODE_T *get_arguments(parser_t *p) {
    NODE_T *first = get_expression(p);
    if (!first) return nullptr;
    return attach_comma_chain(p, first);
}

/**
 * @brief logical_expression ::= comparison_expression ((AND|OR) comparison_expression)*
 */
static NODE_T *get_logical_expression(parser_t *p) {
    NODE_T *lhs = get_comparison_expression(p);
    if (!lhs) return nullptr;
    while (true) {
        TOKEN_T *op_tok = get_token(p);
        if (!op_tok || op_tok->node.type != OPERATOR_T)
            break;
        OPERATOR::OPERATOR op = op_tok->node.value.opr;
        if (op != OPERATOR::AND && op != OPERATOR::OR)
            break;
        next_token(p);
        NODE_T *rhs = get_comparison_expression(p);
        if (!rhs) { p->error = true; return nullptr; }
        set_children(&op_tok->node, lhs, rhs);
        lhs = &op_tok->node;
    }
    return lhs;
}

/**
 * @brief comparison_expression ::= math_expression (comp math_expression)?
 */
static NODE_T *get_comparison_expression(parser_t *p) {
    NODE_T *lhs = get_math_expression(p);
    if (!lhs) return nullptr;
    TOKEN_T *op_tok = get_token(p);
    if (!op_tok || op_tok->node.type != OPERATOR_T)
        return lhs;
    switch (op_tok->node.value.opr) {
        case OPERATOR::EQ:
        case OPERATOR::NEQ:
        case OPERATOR::BELOW:
        case OPERATOR::ABOVE:
        case OPERATOR::BELOW_EQ:
        case OPERATOR::ABOVE_EQ:
            break;
        default:
            return lhs;
    }
    next_token(p);
    NODE_T *rhs = get_math_expression(p);
    if (!rhs) { p->error = true; return nullptr; }
    set_children(&op_tok->node, lhs, rhs);
    return &op_tok->node;
}

/**
 * @brief math_expression ::= term (("+"|"-") term)*
 */
static NODE_T *get_math_expression(parser_t *p) {
    NODE_T *lhs = get_term(p);
    if (!lhs) return nullptr;
    while (true) {
        TOKEN_T *op_tok = get_token(p);
        if (!op_tok || op_tok->node.type != OPERATOR_T)
            break;
        OPERATOR::OPERATOR op = op_tok->node.value.opr;
        if (op != OPERATOR::ADD && op != OPERATOR::SUB)
            break;
        next_token(p);
        NODE_T *rhs = get_term(p);
        if (!rhs) { p->error = true; return nullptr; }
        set_children(&op_tok->node, lhs, rhs);
        lhs = &op_tok->node;
    }
    return lhs;
}

/**
 * @brief term ::= factor (("*"|"/"|"%") factor)*
 */
static NODE_T *get_term(parser_t *p) {
    NODE_T *lhs = get_power(p);
    if (!lhs) return nullptr;
    while (true) {
        TOKEN_T *op_tok = get_token(p);
        if (!op_tok || op_tok->node.type != OPERATOR_T)
            break;
        OPERATOR::OPERATOR op = op_tok->node.value.opr;
        if (op != OPERATOR::MUL && op != OPERATOR::DIV && op != OPERATOR::MOD)
            break;
        next_token(p);
        NODE_T *rhs = get_factor(p);
        if (!rhs) { p->error = true; return nullptr; }
        set_children(&op_tok->node, lhs, rhs);
        lhs = &op_tok->node;
    }
    return lhs;
}

/**
 * @brief power ::= factor ("^" factor)*
 */
static NODE_T *get_power(parser_t *p) {
    NODE_T *base = get_factor(p);
    if (!base) return nullptr;
    while (true) {
        TOKEN_T *tok = get_token(p);
        if (!tok || tok->node.type != OPERATOR_T || tok->node.value.opr != OPERATOR::POW)
            break;
        next_token(p);
        NODE_T *exp = get_factor(p);
        if (!exp) { p->error = true; return nullptr; }
        set_children(&tok->node, base, exp);
        base = &tok->node;
    }
    return base;
}

/**
 * @brief factor ::= internal_func | function_call | number | identifier | literal | "(" expression ")"
 */
static NODE_T *get_factor(parser_t *p) {
    NODE_T *node = get_internal_func_call(p);
    if (node) return node;

    /* функция пользователя */
    size_t save = p->pos;
    node = get_function_call(p);
    if (node) return node;
    p->pos = save;

    TOKEN_T *tok = get_token(p);
    if (!tok) return nullptr;

    if (match_delim(p, DELIMITER::PAR_OPEN, nullptr)) {
        NODE_T *expr = get_expression(p);
        if (!match_delim(p, DELIMITER::PAR_CLOSE, nullptr)) { p->error = true; return nullptr; }
        return expr;
    }

    if (tok->node.type == NUMBER_T) {
        next_token(p);
        return &tok->node;
    }

    if (tok->node.type == LITERAL_T) {
        next_token(p);
        return &tok->node;
    }

    if (tok->node.type == IDENTIFIER_T) {
        next_token(p);
        return clone_identifier_literal(p, tok);
    }

    /* строка в кавычках */
    if (tok->node.type == DELIMITER_T && tok->node.value.delimiter == DELIMITER::QUOTE) {
        next_token(p);
        TOKEN_T *lit = get_token(p);
        if (lit && lit->node.type == LITERAL_T) {
            next_token(p);
            match_delim(p, DELIMITER::QUOTE, nullptr);
            return &lit->node;
        }
    }

    return nullptr;
}

/**
 * @brief internal_func ::= unary_func "(" expression ")" | binary_func "(" expression "," expression ")"
 */
static NODE_T *get_internal_func_call(parser_t *p) {
    TOKEN_T *tok = get_token(p);
    if (!tok || tok->node.type != OPERATOR_T)
        return nullptr;
    OPERATOR::OPERATOR op = tok->node.value.opr;
    if (!is_unary_builtin(op) && !is_binary_builtin(op))
        return nullptr;
    next_token(p);
    if (!match_delim(p, DELIMITER::PAR_OPEN, nullptr)) { p->error = true; return nullptr; }
    if (is_unary_builtin(op)) {
        NODE_T *arg = get_expression(p);
        if (!match_delim(p, DELIMITER::PAR_CLOSE, nullptr)) { p->error = true; return nullptr; }
        set_children(&tok->node, arg, nullptr);
        return &tok->node;
    }
    NODE_T *lhs = get_expression(p);
    TOKEN_T *comma = nullptr;
    if (!match_delim(p, DELIMITER::COMA, &comma)) { p->error = true; return nullptr; }
    NODE_T *rhs = get_expression(p);
    if (!match_delim(p, DELIMITER::PAR_CLOSE, nullptr)) { p->error = true; return nullptr; }
    set_children(&tok->node, lhs, rhs);
    return &tok->node;
}

/**
 * @brief expression ::= assignment | logical_expression
 */
static NODE_T *get_expression(parser_t *p) {
    size_t save = p->pos;
    NODE_T *assign = get_assignment(p);
    if (assign) return assign;
    p->pos = save;
    return get_logical_expression(p);
}
