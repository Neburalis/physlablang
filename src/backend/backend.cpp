#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "base.h"
#include "io_utils.h"

global const char *REGISTERS[8] = {"RAX", "RBX", "RCX", "RDX", "RTX", "DED", "INSIDE", "CURVA"};

global size_t g_if_counter = 0;
global size_t g_while_counter = 0;
global size_t g_do_counter = 0;
global size_t g_tmp_counter = 0;

typedef struct {
    const mystr::mystr_t *name;
    const char           *reg;
} binding_t;

typedef struct {
    const NODE_T         *func_node;
    const mystr::mystr_t *func_name;
    varlist::VarList     *globals;
    binding_t             bindings[8];
    size_t                bind_count;
    const char           *param_regs[8];
    size_t                param_count;
} func_ctx_t;

function void make_label(char *buf, size_t cap, const char *prefix, size_t id, const char *suffix);
function const mystr::mystr_t *literal_name(const varlist::VarList *vars, const NODE_T *node);
function int find_binding(const func_ctx_t *ctx, const mystr::mystr_t *name);
function const char *binding_reg(const func_ctx_t *ctx, const mystr::mystr_t *name);
function int add_binding(func_ctx_t *ctx, const mystr::mystr_t *name, const char **out_reg);
function const char *ensure_binding(func_ctx_t *ctx, const mystr::mystr_t *name);
function int collect_params(func_ctx_t *ctx, const NODE_T *node);
function void collect_args_in_order(const NODE_T *node, const NODE_T **dst, size_t *count, size_t cap);
function int emit_builtin_draw(func_ctx_t *ctx, const NODE_T *args, FILE *out);
function int emit_builtin_set_pixel(func_ctx_t *ctx, const NODE_T *args, FILE *out);
function const char *alloc_temp_reg(const func_ctx_t *ctx);
function int emit_call(func_ctx_t *ctx, const NODE_T *node, FILE *out);
function int emit_assignment(func_ctx_t *ctx, const NODE_T *node, FILE *out, bool keep);
function int emit_comparison_value(func_ctx_t *ctx, const NODE_T *node, FILE *out);
function int emit_expression(func_ctx_t *ctx, const NODE_T *node, FILE *out);
function int emit_conditional(func_ctx_t *ctx, const NODE_T *node, const char *true_lbl, const char *false_lbl, FILE *out);
function int emit_statement(func_ctx_t *ctx, const NODE_T *node, FILE *out, bool *did_ret);
function int emit_function(const varlist::VarList *globals, const NODE_T *node, FILE *out);
function int emit_function_list(const varlist::VarList *globals, const NODE_T *node, FILE *out);
function int emit_builtin_draw(func_ctx_t *ctx, const NODE_T *args, FILE *out);
function int emit_builtin_set_pixel(func_ctx_t *ctx, const NODE_T *args, FILE *out);
function const char *alloc_temp_reg(const func_ctx_t *ctx);


typedef int (*emit_builtin_func)(func_ctx_t *ctx, const NODE_T *args, FILE *out);

global struct {mystr::mystr_t name; emit_builtin_func func;} builtin_funcs[] = {
    {.name = mystr::construct("DRAW"),      .func = emit_builtin_draw},
    {.name = mystr::construct("SET_PIXEL"), .func = emit_builtin_set_pixel},
};

/**
 * @brief Формирует текст метки вида :prefix<id>suffix.
 */
function void make_label(char *buf, size_t cap, const char *prefix, size_t id, const char *suffix) {
    if (!buf || cap == 0) return;
    if (!suffix) suffix = "";
    snprintf(buf, cap, ":%s%zu%s", prefix ? prefix : "", id, suffix);
}

/**
 * @brief Возвращает строку по literal-узлу.
 */
function const mystr::mystr_t *literal_name(const varlist::VarList *vars, const NODE_T *node) {
    if (!node || !vars || node->type != LITERAL_T) return nullptr;
    return varlist::get(vars, node->value.id);
}

/**
 * @brief Ищет индекс биндинга по имени.
 */
function int find_binding(const func_ctx_t *ctx, const mystr::mystr_t *name) {
    if (!ctx || !name) return -1;
    for (size_t i = 0; i < ctx->bind_count; ++i) {
        if (ctx->bindings[i].name && ctx->bindings[i].name->is_same(name))
            return (int) i;
    }
    return -1;
}

/**
 * @brief Возвращает регистр, связанный с именем, или NULL.
 */
function const char *binding_reg(const func_ctx_t *ctx, const mystr::mystr_t *name) {
    int idx = find_binding(ctx, name);
    return (idx >= 0) ? ctx->bindings[idx].reg : nullptr;
}

/**
 * @brief Назначает следующий свободный регистр имени переменной.
 */
function int add_binding(func_ctx_t *ctx, const mystr::mystr_t *name, const char **out_reg) {
    if (!ctx || !name) return -1;
    if (ctx->bind_count >= ARRAY_COUNT(ctx->bindings)) {
        fprintf(stderr, "функция %s требует %zu переменных; максимум 8 (только регистровый бэкенд).\n",
                ctx->func_name && ctx->func_name->str ? ctx->func_name->str : "<unnamed>",
                ctx->bind_count + 1);
        return -1;
    }
    size_t idx = ctx->bind_count;
    ctx->bindings[idx].name = name;
    ctx->bindings[idx].reg = REGISTERS[idx];
    ctx->bind_count++;
    if (out_reg) *out_reg = ctx->bindings[idx].reg;
    return 0;
}

/**
 * @brief Возвращает регистр переменной, создавая биндинг при необходимости.
 */
function const char *ensure_binding(func_ctx_t *ctx, const mystr::mystr_t *name) {
    const char *reg = binding_reg(ctx, name);
    if (reg) return reg;
    if (add_binding(ctx, name, &reg)) return nullptr;
    return reg;
}

/**
 * @brief Рекурсивно назначает регистры параметрам функции.
 */
function int collect_params(func_ctx_t *ctx, const NODE_T *node) {
    if (!node) return 0;
    if (node->type == DELIMITER_T && node->value.delimiter == DELIMITER::COMA) {
        if (collect_params(ctx, node->left)) return -1;
        if (collect_params(ctx, node->right)) return -1;
        return 0;
    }
    const mystr::mystr_t *nm = literal_name(ctx->globals, node);
    if (!nm) return -1;
    const char *reg = ensure_binding(ctx, nm);
    if (!reg) return -1;
    if (ctx->param_count < ARRAY_COUNT(ctx->param_regs))
        ctx->param_regs[ctx->param_count++] = reg;
    return 0;
}

function bool reg_used(const func_ctx_t *ctx, const char *reg) {
    if (!ctx || !reg) return true;
    for (size_t i = 0; i < ctx->bind_count; ++i) {
        if (ctx->bindings[i].reg && strcmp(ctx->bindings[i].reg, reg) == 0)
            return true;
    }
    return false;
}

function const char *alloc_temp_reg(const func_ctx_t *ctx) {
    for (size_t i = 0; i < ARRAY_COUNT(REGISTERS); ++i) {
        if (!reg_used(ctx, REGISTERS[i]))
            return REGISTERS[i];
    }
    return nullptr;
}

/**
 * @brief Собирает аргументы вызова слева направо в массив.
 */
function void collect_args_in_order(const NODE_T *node, const NODE_T **dst, size_t *count, size_t cap) {
    if (!node || !dst || !count || *count >= cap) return;
    if (node->type == DELIMITER_T && node->value.delimiter == DELIMITER::COMA) {
        collect_args_in_order(node->left, dst, count, cap);
        collect_args_in_order(node->right, dst, count, cap);
        return;
    }
    dst[(*count)++] = node;
}

function int emit_builtin_draw(func_ctx_t *ctx, const NODE_T *args, FILE *out) {
    (void) ctx;
    const NODE_T *ordered[2] = {};
    size_t count = 0;
    collect_args_in_order(args, ordered, &count, ARRAY_COUNT(ordered));
    if (count != 1) {
        fprintf(stderr, "DRAW ожидает ровно 1 числовой аргумент задержки\n");
        return -1;
    }
    const NODE_T *arg = ordered[0];
    if (arg->type != NUMBER_T) {
        fprintf(stderr, "целевой процессор пока не поддерживает DRAW с нечисловым аргументом\n");
        return -1;
    }
    fprintf(out, "DRAW %.0f\n", arg->value.num);
    return 0;
}

function int emit_builtin_set_pixel(func_ctx_t *ctx, const NODE_T *args, FILE *out) {
    const NODE_T *ordered[3] = {};
    size_t count = 0;
    collect_args_in_order(args, ordered, &count, ARRAY_COUNT(ordered));
    if (count != 2) {
        fprintf(stderr, "SET_PIXEL ожидает 2 аргумента: значение, индекс\n");
        return -1;
    }
    const NODE_T *val_node = ordered[0];
    const NODE_T *idx_node = ordered[1];

    const char *tmp_reg = alloc_temp_reg(ctx);
    if (!tmp_reg) {
        fprintf(stderr, "нет свободных регистров для SET_PIXEL\n");
        return -1;
    }

    /* addr -> tmp_reg */
    if (emit_expression(ctx, idx_node, out)) return -1;
    fprintf(out, "POPR %s\n", tmp_reg);

    /* val -> mem[tmp_reg] */
    if (emit_expression(ctx, val_node, out)) return -1;
    fprintf(out, "POPM [%s]\n", tmp_reg);
    return 0;
}

/**
 * @brief Генерирует вызов пользовательской функции.
 */
function int emit_call(func_ctx_t *ctx, const NODE_T *node, FILE *out) {
    if (!ctx || !node || !out) return -1;
    const NODE_T *name_node = node->left;
    const NODE_T *args = node->right;
    const mystr::mystr_t *fname = literal_name(ctx->globals, name_node);
    if (!fname || !fname->str) return -1;

    for (int builtin_idx = 0; builtin_idx < ARRAY_COUNT(builtin_funcs); ++builtin_idx) {
        if (builtin_funcs[builtin_idx].name.is_same(fname))
            return builtin_funcs[builtin_idx].func(ctx, args, out);
    }

    const NODE_T *ordered[16] = {};
    size_t count = 0;
    collect_args_in_order(args, ordered, &count, ARRAY_COUNT(ordered));
    while (count > 0) {
        const NODE_T *arg = ordered[--count];
        if (emit_expression(ctx, arg, out)) return -1;
    }

    fprintf(out, "CALL :%s\n", fname->str);
    return 0;
}

/**
 * @brief Генерирует присваивание lhs = rhs.
 */
function int emit_assignment(func_ctx_t *ctx, const NODE_T *node, FILE *out, bool keep) {
    if (!ctx || !node || !out) return -1;
    const NODE_T *lhs = node->left;
    const NODE_T *rhs = node->right;
    const mystr::mystr_t *nm = literal_name(ctx->globals, lhs);
    if (!nm) return -1;
    const char *reg = ensure_binding(ctx, nm);
    if (!reg) return -1;
    if (emit_expression(ctx, rhs, out)) return -1;
    fprintf(out, "POPR %s\n", reg);
    if (keep)
        fprintf(out, "PUSHR %s\n", reg);
    return 0;
}

/**
 * @brief Вычисляет сравнение и кладет 0/1 в стек.
 */
function int emit_comparison_value(func_ctx_t *ctx, const NODE_T *node, FILE *out) {
    if (!ctx || !node || !out) return -1;
    char true_lbl[32] = "", false_lbl[32] = "", end_lbl[32] = "";
    make_label(true_lbl, sizeof(true_lbl), "cmp_true_", ++g_tmp_counter, "");
    make_label(false_lbl, sizeof(false_lbl), "cmp_false_", g_tmp_counter, "");
    make_label(end_lbl, sizeof(end_lbl), "cmp_end_", g_tmp_counter, "");
    if (emit_conditional(ctx, node, true_lbl, false_lbl, out)) return -1;
    fprintf(out, "%s\nPUSH 0\nJMP %s\n%s\nPUSH 1\n%s\n", false_lbl, end_lbl, true_lbl, end_lbl);
    return 0;
}

/**
 * @brief Генерирует стековый код для выражения.
 */
function int emit_expression(func_ctx_t *ctx, const NODE_T *node, FILE *out) {
    if (!ctx || !node || !out) return -1;
    switch (node->type) {
        case NUMBER_T:
            fprintf(out, "PUSH %.15g\n", node->value.num);
            return 0;
        case LITERAL_T: {
            const mystr::mystr_t *nm = literal_name(ctx->globals, node);
            const char *reg = binding_reg(ctx, nm);
            if (!nm || !reg) {
                fprintf(stderr, "целевой процессор пока не поддерживает строковые литералы\n");
                return -1;
            }
            fprintf(out, "PUSHR %s\n", reg);
            return 0;
        }
        case OPERATOR_T: {
            switch (node->value.opr) {
                case OPERATOR::ADD:
                case OPERATOR::SUB:
                case OPERATOR::MUL:
                case OPERATOR::DIV:
                case OPERATOR::MOD: {
                    if (emit_expression(ctx, node->left, out)) return -1;
                    if (emit_expression(ctx, node->right, out)) return -1;
                    const char *op = (node->value.opr == OPERATOR::ADD) ? "ADD" :
                                     (node->value.opr == OPERATOR::SUB) ? "SUB" :
                                     (node->value.opr == OPERATOR::MUL) ? "MUL" :
                                     (node->value.opr == OPERATOR::DIV) ? "DIV" : "MOD";
                    fprintf(out, "%s\n", op);
                    return 0;
                }
                case OPERATOR::SQRT:
                case OPERATOR::SIN:
                case OPERATOR::COS: {
                    if (emit_expression(ctx, node->left, out)) return -1;
                    const char *op = (node->value.opr == OPERATOR::SQRT) ? "SQRT" :
                                     (node->value.opr == OPERATOR::SIN)  ? "SIN"  : "COS";
                    fprintf(out, "%s\n", op);
                    return 0;
                }
                case OPERATOR::ASSIGNMENT:
                    return emit_assignment(ctx, node, out, true);
                case OPERATOR::EQ: case OPERATOR::NEQ:
                case OPERATOR::BELOW: case OPERATOR::ABOVE:
                case OPERATOR::BELOW_EQ: case OPERATOR::ABOVE_EQ:
                case OPERATOR::AND: case OPERATOR::OR: case OPERATOR::NOT:
                    return emit_comparison_value(ctx, node, out);
                case OPERATOR::CONNECTOR:
                    if (emit_expression(ctx, node->left, out)) return -1;
                    return emit_expression(ctx, node->right, out);
                case OPERATOR::SET_PIXEL: {
                    const char *tmp_reg = alloc_temp_reg(ctx);
                    if (!tmp_reg) {
                        fprintf(stderr, "нет свободных регистров для SET_PIXEL\n");
                        return -1;
                    }
                    if (emit_expression(ctx, node->right, out)) return -1;
                    fprintf(out, "POPR %s\n", tmp_reg);
                    if (emit_expression(ctx, node->left, out)) return -1;
                    fprintf(out, "POPM [%s]\n", tmp_reg);
                    return 0;
                }
                case OPERATOR::DRAW: {
                    const NODE_T *arg = node->left;
                    if (!arg || arg->type != NUMBER_T) {
                        fprintf(stderr, "целевой процессор пока не поддерживает DRAW с нечисловым аргументом\n");
                        return -1;
                    }
                    fprintf(out, "DRAW %.0f\n", arg->value.num);
                    return 0;
                }
                case OPERATOR::IN:
                case OPERATOR::OUT:
                case OPERATOR::POW:
                case OPERATOR::LN:
                case OPERATOR::TAN:
                case OPERATOR::CTG:
                case OPERATOR::ASIN:
                case OPERATOR::ACOS:
                case OPERATOR::ATAN:
                case OPERATOR::ACTG:
                    fprintf(stderr, "неподдерживаемый оператор в выражении\n");
                    return -1;
                default:
                    break;
            }
            break;
        }
        case KEYWORD_T:
            if (node->value.keyword == KEYWORD::FUNC_CALL)
                return emit_call(ctx, node, out);
            break;
        default:
            break;
    }
    fprintf(stderr, "неподдерживаемый узел выражения\n");
    return -1;
}

/**
 * @brief Генерирует условный переход: при истине -> true_lbl, иначе -> false_lbl.
 */
function int emit_conditional(func_ctx_t *ctx, const NODE_T *node, const char *true_lbl, const char *false_lbl, FILE *out) {
    if (!ctx || !node || !true_lbl || !false_lbl) return -1;
    if (node->type == OPERATOR_T) {
        OPERATOR::OPERATOR op = node->value.opr;
        if (op == OPERATOR::AND) {
            char mid[32] = "";
            make_label(mid, sizeof(mid), "if_and_", ++g_tmp_counter, "");
            if (emit_conditional(ctx, node->left, mid, false_lbl, out)) return -1;
            fprintf(out, "%s\n", mid);
            return emit_conditional(ctx, node->right, true_lbl, false_lbl, out);
        }
        if (op == OPERATOR::OR) {
            char mid[32] = "";
            make_label(mid, sizeof(mid), "if_or_", ++g_tmp_counter, "");
            if (emit_conditional(ctx, node->left, true_lbl, mid, out)) return -1;
            fprintf(out, "%s\n", mid);
            return emit_conditional(ctx, node->right, true_lbl, false_lbl, out);
        }
        if (op == OPERATOR::NOT)
            return emit_conditional(ctx, node->left, false_lbl, true_lbl, out);
        if (op == OPERATOR::EQ || op == OPERATOR::NEQ || op == OPERATOR::BELOW || op == OPERATOR::ABOVE ||
            op == OPERATOR::BELOW_EQ || op == OPERATOR::ABOVE_EQ) {
            if (emit_expression(ctx, node->left, out)) return -1;
            if (emit_expression(ctx, node->right, out)) return -1;
            const char *jmp = nullptr;
            switch (op) {
                case OPERATOR::EQ:       jmp = "JE";  break;
                case OPERATOR::NEQ:      jmp = "JNE"; break;
                case OPERATOR::BELOW:    jmp = "JB";  break;
                case OPERATOR::ABOVE:    jmp = "JA";  break;
                case OPERATOR::BELOW_EQ: jmp = "JBE"; break;
                case OPERATOR::ABOVE_EQ: jmp = "JAE"; break;
                default: break;
            }
            if (!jmp) return -1;
            fprintf(out, "%s %s\n", jmp, true_lbl);
            fprintf(out, "JMP %s\n", false_lbl);
            return 0;
        }
    }
    if (emit_expression(ctx, node, out)) return -1;
    fprintf(out, "PUSH 0\nJNE %s\nJMP %s\n", true_lbl, false_lbl);
    return 0;
}

/**
 * @brief Генерирует код для операторов и выражений верхнего уровня.
 */
function int emit_statement(func_ctx_t *ctx, const NODE_T *node, FILE *out, bool *did_ret) {
    if (!ctx || !node || !out) return -1;
    if (did_ret) *did_ret = false;
    if (node->type == OPERATOR_T && node->value.opr == OPERATOR::CONNECTOR) {
        bool left_ret = false, right_ret = false;
        if (emit_statement(ctx, node->left, out, &left_ret)) return -1;
        if (emit_statement(ctx, node->right, out, &right_ret)) return -1;
        if (did_ret) *did_ret = (left_ret || right_ret);
        return 0;
    }
    if (node->type == OPERATOR_T && node->value.opr == OPERATOR::ASSIGNMENT)
        return emit_assignment(ctx, node, out, false);
    if (node->type == KEYWORD_T && node->value.keyword == KEYWORD::VAR_DECLARATION) {
        const mystr::mystr_t *nm = literal_name(ctx->globals, node->left);
        if (!nm) return -1;
        return ensure_binding(ctx, nm) ? 0 : -1;
    }
    if (node->type == KEYWORD_T && node->value.keyword == KEYWORD::RETURN) {
        if (emit_expression(ctx, node->left, out)) return -1;
        fprintf(out, "RET\n");
        if (did_ret) *did_ret = true;
        return 0;
    }
    if (node->type == OPERATOR_T && node->value.opr == OPERATOR::OUT) {
        if (emit_expression(ctx, node->left, out)) return -1;
        fprintf(out, "OUT\n");
        return 0;
    }
    if (node->type == OPERATOR_T && node->value.opr == OPERATOR::IN) {
        const mystr::mystr_t *nm = literal_name(ctx->globals, node->left);
        const char *reg = ensure_binding(ctx, nm);
        if (!reg) return -1;
        fprintf(out, "IN\nPOPR %s\n", reg);
        return 0;
    }
    if (node->type == KEYWORD_T && node->value.keyword == KEYWORD::FUNC_CALL)
        return emit_call(ctx, node, out);
    if (node->type == KEYWORD_T && node->value.keyword == KEYWORD::IF) {
        const NODE_T *branches = node->right;
        const NODE_T *then_ops = branches ? branches->left : nullptr;
        const NODE_T *else_ops = branches ? branches->right : nullptr;

        char then_lbl[32] = "", else_lbl[32] = "", end_lbl[32] = "";
        make_label(then_lbl, sizeof(then_lbl), "if_", ++g_if_counter, "_then");
        make_label(else_lbl, sizeof(else_lbl), "if_", g_if_counter, "");
        make_label(end_lbl, sizeof(end_lbl), "if_", g_if_counter, "_end");

        const char *false_target = else_ops ? else_lbl : end_lbl;
        if (emit_conditional(ctx, node->left, then_lbl, false_target, out)) return -1;

        fprintf(out, "%s\n", then_lbl);
        if (then_ops && emit_statement(ctx, then_ops, out, did_ret)) return -1;
        if (else_ops)
            fprintf(out, "JMP %s\n", end_lbl);

        fprintf(out, "%s\n", false_target);
        if (else_ops && emit_statement(ctx, else_ops, out, did_ret)) return -1;
        if (else_ops)
            fprintf(out, "%s\n", end_lbl);
        return 0;
    }
    if (node->type == KEYWORD_T && node->value.keyword == KEYWORD::WHILE) {
        char start_lbl[32] = "", body_lbl[32] = "", end_lbl[32] = "";
        make_label(start_lbl, sizeof(start_lbl), "while_", ++g_while_counter, "");
        make_label(body_lbl, sizeof(body_lbl), "while_", g_while_counter, "_body");
        make_label(end_lbl, sizeof(end_lbl), "while_", g_while_counter, "_end");
        fprintf(out, "%s\n", start_lbl);
        if (emit_conditional(ctx, node->left, body_lbl, end_lbl, out)) return -1;
        fprintf(out, "%s\n", body_lbl);
        if (emit_statement(ctx, node->right, out, did_ret)) return -1;
        fprintf(out, "JMP %s\n%s\n", start_lbl, end_lbl);
        return 0;
    }
    if (node->type == KEYWORD_T && node->value.keyword == KEYWORD::DO_WHILE) {
        char body_lbl[32] = "", end_lbl[32] = "";
        make_label(body_lbl, sizeof(body_lbl), "do-while_", ++g_do_counter, "");
        make_label(end_lbl, sizeof(end_lbl), "do-while_", g_do_counter, "_end");
        fprintf(out, "%s\n", body_lbl);
        if (emit_statement(ctx, node->right, out, did_ret)) return -1;
        if (emit_conditional(ctx, node->left, body_lbl, end_lbl, out)) return -1;
        fprintf(out, "%s\n", end_lbl);
        return 0;
    }
    return emit_expression(ctx, node, out);
}

/**
 * @brief Эмитирует тело функции и ее пролог/рет.
 */
function int emit_function(const varlist::VarList *globals, const NODE_T *node, FILE *out) {
    if (!node || !globals || !out) return -1;
    const mystr::mystr_t *fname = literal_name(globals, node);
    if (!fname || !fname->str) return -1;
    func_ctx_t ctx = {};
    ctx.func_node = node;
    ctx.func_name = fname;
    ctx.globals = (varlist::VarList *)globals;

    collect_params(&ctx, node->left);

    fprintf(out, ":%s\n", fname->str);
    for (size_t i = 0; i < ctx.param_count; ++i)
        fprintf(out, "POPR %s\n", ctx.param_regs[i]);

    bool body_ret = false;
    if (emit_statement(&ctx, node->right, out, &body_ret)) return -1;
    if (!body_ret)
        fprintf(out, "RET\n");
    return 0;
}

/**
 * @brief Обходит список функций, разделенных запятыми.
 */
function int emit_function_list(const varlist::VarList *globals, const NODE_T *node, FILE *out) {
    if (!node) return 0;
    if (node->type == DELIMITER_T && node->value.delimiter == DELIMITER::COMA) {
        if (emit_function_list(globals, node->left, out)) return -1;
        return emit_function_list(globals, node->right, out);
    }
    return emit_function(globals, node, out);
}

/**
 * @brief Точка входа генерации: main-тело + функции + HLT.
 */
int reverse_program(NODE_T *root, varlist::VarList *vars, FILE *out) {
    if (!root || !vars || !out) return -1;
    const NODE_T *funcs = nullptr;
    const NODE_T *body = nullptr;
    if (root->type == OPERATOR_T && root->value.opr == OPERATOR::CONNECTOR) {
        funcs = root->left;
        body = root->right;
    } else {
        body = root;
    }

    func_ctx_t main_ctx = {};
    main_ctx.globals = vars;
    main_ctx.func_name = nullptr;
    if (body && emit_statement(&main_ctx, body, out, nullptr)) return -1;
    fprintf(out, "HLT\n");

    if (emit_function_list(vars, funcs, out)) return -1;
    return 0;
}
