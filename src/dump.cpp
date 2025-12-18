#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "io_utils.h"
#include "base.h"
#include "logger.h"
#include "frontend.h"

const char *node_type_name(const NODE_T *node) {
    if (!node) return "UNKNOWN";
    switch (node->type) {
        case NUMBER_T: return "NUMBER";
        case OPERATOR_T:  return "OPERATOR";
        case IDENTIFIER_T: return "VARIABLE";
        case KEYWORD_T: return "KEYWORD";
        case LITERAL_T: return "LITERAL";
        case DELIMITER_T: return "DELIMITER";
        default:    return "UNKNOWN";
    }
}

#define STR_CASE_(x) case x: return #x;

static const char *lexer_node_type_name(NODE_TYPE type) {
    switch (type) {
        STR_CASE_(KEYWORD_T);
        STR_CASE_(IDENTIFIER_T);
        STR_CASE_(NUMBER_T);
        STR_CASE_(LITERAL_T);
        STR_CASE_(OPERATOR_T);
        STR_CASE_(DELIMITER_T);
        default:            return "NODE_TYPE::UNKNOWN";
    }
}

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
        STR_CASE_(KEYWORD::IF);
        STR_CASE_(KEYWORD::ELSE);
        STR_CASE_(KEYWORD::THEN);
        STR_CASE_(KEYWORD::WHILE);
        STR_CASE_(KEYWORD::DO_WHILE);
        STR_CASE_(KEYWORD::WHILE_CONDITION);
        STR_CASE_(KEYWORD::END_WHILE);
        STR_CASE_(KEYWORD::FORMULA);
        STR_CASE_(KEYWORD::END_FORMULA);
        STR_CASE_(KEYWORD::VAR_DECLARATION);
        STR_CASE_(KEYWORD::FUNC_CALL);
        STR_CASE_(KEYWORD::RETURN);
        default:                         return "KEYWORD::UNKNOWN";
    }
}

static const char *operator_name(OPERATOR::OPERATOR op) {
    switch (op) {
        STR_CASE_(OPERATOR::ADD);
        STR_CASE_(OPERATOR::SUB);
        STR_CASE_(OPERATOR::MUL);
        STR_CASE_(OPERATOR::DIV);
        STR_CASE_(OPERATOR::POW);
        STR_CASE_(OPERATOR::LN);
        STR_CASE_(OPERATOR::SIN);
        STR_CASE_(OPERATOR::COS);
        STR_CASE_(OPERATOR::TAN);
        STR_CASE_(OPERATOR::CTG);
        STR_CASE_(OPERATOR::ASIN);
        STR_CASE_(OPERATOR::ACOS);
        STR_CASE_(OPERATOR::ATAN);
        STR_CASE_(OPERATOR::ACTG);
        STR_CASE_(OPERATOR::SQRT);
        STR_CASE_(OPERATOR::EQ);
        STR_CASE_(OPERATOR::NEQ);
        STR_CASE_(OPERATOR::BELOW);
        STR_CASE_(OPERATOR::ABOVE);
        STR_CASE_(OPERATOR::BELOW_EQ);
        STR_CASE_(OPERATOR::ABOVE_EQ);
        STR_CASE_(OPERATOR::AND);
        STR_CASE_(OPERATOR::OR);
        STR_CASE_(OPERATOR::NOT);
        STR_CASE_(OPERATOR::MOD);
        STR_CASE_(OPERATOR::IN);
        STR_CASE_(OPERATOR::OUT);
        STR_CASE_(OPERATOR::ASSIGNMENT);
        STR_CASE_(OPERATOR::CONNECTOR);
        default:                   return "OPERATOR::UNKNOWN";
    }
}

static const char *delimiter_name(DELIMITER::DELIMITER delim) {
    switch (delim) {
        STR_CASE_(DELIMITER::PAR_OPEN);
        STR_CASE_(DELIMITER::PAR_CLOSE);
        STR_CASE_(DELIMITER::QUOTE);
        STR_CASE_(DELIMITER::COMA);
        STR_CASE_(DELIMITER::COLON);
        default:                    return "DELIMITER::UNKNOWN";
    }
}

#undef STR_CASE_

function const char *node_type_color(const NODE_T *node) {
    if (!node) return "#f2f2f2";
    switch (node->type) {
        case NUMBER_T: return "#fff2cc";
        case OPERATOR_T:  return "#cfe2ff";
        case IDENTIFIER_T: return "#d4f8d4";
        case LITERAL_T: return "#d4f8d4";
        case KEYWORD_T: return "#ffd9b3";
        case DELIMITER_T: return "#e0e0e0";
        default:    return "#f2f2f2";
    }
}

function const char *node_type_shape(const NODE_T *node) {
    if (!node) return "hexagon";
    switch (node->type) {
        case NUMBER_T: return "box";
        case OPERATOR_T:  return "ellipse";
        case IDENTIFIER_T: return "diamond";
        case LITERAL_T: return "diamond";
        case KEYWORD_T: return "ellipse";
        case DELIMITER_T: return "octagon";
        default:    return "hexagon";
    }
}

function const char *node_type_font_size(const NODE_T *node) {
    if (!node) return "10pt";
    switch (node->type) {
        case NUMBER_T: return "10pt";
        case OPERATOR_T:  return "12pt";
        case IDENTIFIER_T: return "10pt";
        case LITERAL_T: return "10pt";
        case KEYWORD_T: return "12pt";
        case DELIMITER_T: return "10pt";
        default:    return "10pt";
    }
}

const char *operator_symbol(OPERATOR::OPERATOR op) {
    switch (op) {
        case OPERATOR::ADD:   return "+";
        case OPERATOR::SUB:   return "-";
        case OPERATOR::MUL:   return "*";
        case OPERATOR::DIV:   return "/";
        case OPERATOR::POW:   return "^";
        case OPERATOR::LN:    return "ln";
        case OPERATOR::SIN:   return "sin";
        case OPERATOR::COS:   return "cos";
        case OPERATOR::TAN:   return "tan";
        case OPERATOR::CTG:   return "ctg";
        case OPERATOR::ASIN:  return "arcsin";
        case OPERATOR::ACOS:  return "arccos";
        case OPERATOR::ATAN:  return "arctan";
        case OPERATOR::ACTG:  return "arccot";
        case OPERATOR::SQRT:  return "sqrt";
        default:    return "?";
    }
}

function void format_node_value(const FRONT_COMPL_T *ctx, const NODE_T *node, char *buf, size_t size) {
    if (!node || !buf || !size) return;
    switch (node->type) {
        case NUMBER_T:
            snprintf(buf, size, "%g", node->value.num);
            break;
        case OPERATOR_T:
            snprintf(buf, size, "%s", operator_name(node->value.opr));
            break;
        case KEYWORD_T:
            snprintf(buf, size, "%s", keyword_name(node->value.keyword));
            break;
        case IDENTIFIER_T:
        case LITERAL_T: {
            const mystr::mystr_t *entry = (ctx && ctx->vars) ? varlist::get(ctx->vars, node->value.id) : nullptr;
            if (entry && entry->str)
                snprintf(buf, size, "%s", entry->str);
            else
                snprintf(buf, size, "id_%zu", node->value.id);
            break;
        }
        case DELIMITER_T:
            snprintf(buf, size, "%s", delimiter_name(node->value.delimiter));
            break;
        default:
            snprintf(buf, size, "-");
            break;
    }
}

function int write_node_full(const FRONT_COMPL_T *ctx, const NODE_T *subtree, FILE *fp, int *id_counter) {
    if (!subtree || !fp || !id_counter) return -1;
    int my_id = (*id_counter)++;

    char value_buf[128] = "";
    format_node_value(ctx, subtree, value_buf, sizeof(value_buf));

    const char *color = node_type_color(subtree);

        fprintf(fp,
            "\tnode%d [label=\"{ type=%s | value=%s | { left=%p | right=%p } | parent=%p | size=%zu }\", shape=record, style=filled, fillcolor=\"%s\"];\n",
            my_id,
            node_type_name(subtree),
            value_buf,
            (void *)subtree->left,
            (void *)subtree->right,
            (void *)subtree->parent,
            subtree->elements,
            color);

    if (subtree->left) {
        int left_id = write_node_full(ctx, subtree->left, fp, id_counter);
        if (left_id >= 0)
            fprintf(fp, "\tnode%d -> node%d [color=\"#0c0ccc\", label=\"L\", constraint=true];\n", my_id, left_id);
    }

    if (subtree->right) {
        int right_id = write_node_full(ctx, subtree->right, fp, id_counter);
        if (right_id >= 0)
            fprintf(fp, "\tnode%d -> node%d [color=\"#3dad3d\", label=\"R\", constraint=true];\n", my_id, right_id);
    }

    return my_id;
}

function int write_node_simple(const FRONT_COMPL_T *ctx, const NODE_T *subtree, FILE *fp, int *id_counter) {
    if (!subtree || !fp || !id_counter) return -1;
    int my_id = (*id_counter)++;

    char value_buf[128] = "";
    format_node_value(ctx, subtree, value_buf, sizeof(value_buf));

    const char *shape = node_type_shape(subtree);
    const char *color = node_type_color(subtree);
    const char *fontsize = node_type_font_size(subtree);

        fprintf(fp,
            "\tnode%d [label=\"%s\", shape=%s, style=filled, fillcolor=\"%s\", fontsize=\"%s\"];\n",
            my_id,
            value_buf,
            shape,
            color,
            fontsize
        );

    if (subtree->left) {
        int left_id = write_node_simple(ctx, subtree->left, fp, id_counter);
        if (left_id >= 0)
            fprintf(fp, "\tnode%d -> node%d [color=\"#0c0ccc\", label=\"L\", constraint=true];\n", my_id, left_id);
    }

    if (subtree->right) {
        int right_id = write_node_simple(ctx, subtree->right, fp, id_counter);
        if (right_id >= 0)
            fprintf(fp, "\tnode%d -> node%d [color=\"#3dad3d\", label=\"R\", constraint=true];\n", my_id, right_id);
    }

    return my_id;
}

function void generate_dot_dump(const FRONT_COMPL_T *ctx, bool is_simple, FILE *fp) {
    if (!fp) return;
    fprintf(fp,
            "digraph EquationTree {\n"
            "\trankdir=TB;\n"
            "\tnode [fontname=\"Helvetica\", fontsize=10];\n"
            "\tedge [arrowsize=0.8];\n"
            "\tgraph [splines=true, concentrate=false];\n\n");

    if (!ctx || !ctx->root) {
        fprintf(fp, "\t// empty tree\n\n\tlabel = \"empty tree\";\n");
        fprintf(fp, "}\n");
        return;
    }

    int id_counter = 0;
    if (is_simple)
        write_node_simple(ctx, ctx->root, fp, &id_counter);
    else
        write_node_full(ctx, ctx->root, fp, &id_counter);
    fprintf(fp, "}\n");
}

function int generate_files(const FRONT_COMPL_T *ctx, bool is_simple, const char *dir, char *out_basename, size_t out_size) {
    local size_t dump_counter = 0;
    const char *outdir = (dir && dir[0] != '\0') ? dir : ".";
    size_t outdir_len = strlen(outdir);

    char dot_path[512] = "";
    if (outdir_len && outdir[outdir_len - 1] == '/')
        snprintf(dot_path, sizeof(dot_path), "%stree_dump_%zu.dot.tmp", outdir, dump_counter);
    else
        snprintf(dot_path, sizeof(dot_path), "%s/tree_dump_%zu.dot.tmp", outdir, dump_counter);

    FILE *fp = fopen(dot_path, "w");
    if (!fp) return -1;

    generate_dot_dump(ctx, is_simple, fp);
    fclose(fp);

    char image_basename[256] = "";
    snprintf(image_basename, sizeof(image_basename), "tree_dump_%zu.svg", dump_counter);

    char svg_path[512] = "";
    if (outdir_len && outdir[outdir_len - 1] == '/')
        snprintf(svg_path, sizeof(svg_path), "%s%s", outdir, image_basename);
    else
        snprintf(svg_path, sizeof(svg_path), "%s/%s", outdir, image_basename);

    char command[1024] = "";
    snprintf(command, sizeof(command), "dot -Tsvg %s -o %s", dot_path, svg_path);
    (void) system(command);

    if (out_basename && out_size > 0) {
        strncpy(out_basename, image_basename, out_size);
        out_basename[out_size - 1] = '\0';
    }

    ++dump_counter;
    return 0;
}

function void dump_internal(const FRONT_COMPL_T *ctx, bool is_simple, const char *fmt, va_list ap) {
    const char *dir = logger_get_active_dir();
    char basename[256] = "";
    int rc = generate_files(ctx, is_simple, dir, basename, sizeof(basename));

    FILE *log_file = logger_get_file();
    if (!log_file)
        return;

    if (fmt != nullptr) {
        fprintf(log_file, "<p>");
        vfprintf(log_file, fmt, ap);
        fprintf(log_file, "</p>\n");
        fflush(log_file);
    }
    else if (ctx && ctx->name) {
        fprintf(log_file, "<p>Dump of %s</p>", ctx->name);
        fflush(log_file);
    }

    if (rc == 0 && basename[0] != '\0')
        fprintf(log_file, "<img src=\"%s\">\n", basename);
    else
        fprintf(log_file, "<p>SVG not generated</p>\n");

    fflush(log_file);
}

void full_dump(const FRONT_COMPL_T *ctx) {
    va_list ap = {};
    dump_internal(ctx, false, nullptr, ap);
}

void full_dump(const FRONT_COMPL_T *ctx, const char *fmt, ...) {
    va_list ap = {};
    va_start(ap, fmt);
    dump_internal(ctx, false, fmt, ap);
    va_end(ap);
}

void simple_dump(const FRONT_COMPL_T *ctx) {
    va_list ap = {};
    dump_internal(ctx, true, nullptr, ap);
}

void simple_dump(const FRONT_COMPL_T *ctx, const char *fmt, ...) {
    va_list ap = {};
    va_start(ap, fmt);
    dump_internal(ctx, true, fmt, ap);
    va_end(ap);
}



static void html_escape(FILE *out, const char *text, size_t len) {
    if (!out || !text) return;
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char) text[i];
        switch (c) {
            case '&': fputs("&amp;", out); break;
            case '<': fputs("&lt;", out); break;
            case '>': fputs("&gt;", out); break;
            case '"': fputs("&quot;", out); break;
            case '\'': fputs("&#39;", out); break;
            case '\n': fputs("&#10;", out); break;
            case '\r': fputs("&#13;", out); break;
            case '\t': fputs("&#9;", out); break;
            default: fputc(c, out); break;
        }
    }
}

static void print_token_value(const FRONT_COMPL_T *ctx, const TOKEN_T *tok, FILE *out) {
    if (!tok || !out) return;
    switch (tok->node.type) {
        case KEYWORD_T: {
            // fputs("KEYWORD::", out);
            fputs(keyword_name(tok->node.value.keyword), out);
            if (tok->text && tok->length) {
                fputs(" (", out);
                html_escape(out, tok->text, tok->length);
                fputc(')', out);
            }
            break;
        }
        case IDENTIFIER_T: {
            fprintf(out, "IDENTIFIER idx=%zu", tok->node.value.id);
            if (ctx && ctx->vars) {
                const mystr::mystr_t *entry = varlist::get(ctx->vars, tok->node.value.id);
                if (entry && entry->str) {
                    fputs(" [", out);
                    html_escape(out, entry->str, entry->len);
                    fputc(']', out);
                }
            }
            break;
        }
        case LITERAL_T: {
            fprintf(out, "LITERAL idx=%zu", tok->node.value.id);
            if (ctx && ctx->vars) {
                const mystr::mystr_t *entry = varlist::get(ctx->vars, tok->node.value.id);
                if (entry && entry->str) {
                    fputs(" [", out);
                    html_escape(out, entry->str, entry->len);
                    fputc(']', out);
                }
            }
            break;
        }
        case NUMBER_T: {
            fprintf(out, "NUMBER %g", tok->node.value.num);
            break;
        }
        case OPERATOR_T: {
            // fputs("OPERATOR::", out);
            fputs(operator_name(tok->node.value.opr), out);
            if (tok->text && tok->length) {
                fputs(" (", out);
                html_escape(out, tok->text, tok->length);
                fputc(')', out);
            }
            break;
        }
        case DELIMITER_T: {
            // fputs("DELIMITER::", out);
            fputs(delimiter_name(tok->node.value.delimiter), out);
            if (tok->text && tok->length) {
                fputs(" (", out);
                html_escape(out, tok->text, tok->length);
                fputc(')', out);
            }
            break;
        }
        default:
            fputs("UNKNOWN", out);
            break;
    }
}

void dump_lexer_tokens(const FRONT_COMPL_T *ctx, const char *title) {
    FILE *log_file = logger_get_file();
    if (!ctx || !log_file) return;

    const char *heading = title;
    if (!heading || !heading[0]) heading = ctx->name ? ctx->name : "lexer tokens";

    fprintf(log_file, "<section class=\"lexer-tokens\">\n");
    fprintf(log_file, "<h2>Lexer tokens: ");
    html_escape(log_file, heading, strlen(heading));
    fprintf(log_file, " (%zu)</h2>\n", ctx->token_count);

    if (!ctx->tokens || ctx->token_count == 0) {
        fputs("<p>No tokens.</p>\n</section>\n", log_file);
        fflush(log_file);
        return;
    }

    fputs("<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\">\n", log_file);
    fputs("<thead><tr>"
          "<th>#</th>"
          "<th>Type</th>"
          "<th>Value</th>"
          "<th>Location</th>"
          "<th>Source</th>"
          "<th>NODE_T</th>"
          "</tr></thead><tbody>\n", log_file);

    for (size_t i = 0; i < ctx->token_count; ++i) {
        const TOKEN_T *tok = &ctx->tokens[i];
        fprintf(log_file, "<tr><td>%zu</td>", i);

        fprintf(log_file, "<td>%s</td>", lexer_node_type_name(tok->node.type));

        fputs("<td>", log_file);
        print_token_value(ctx, tok, log_file);
        fputs("</td>", log_file);

        fputs("<td>", log_file);
        if (tok->line > 0 && tok->pos > 0) {
            const char *href = tok->filename ? tok->filename : ctx->name;
            if (href && href[0]) {
                fputs("<a href=\"", log_file);
                html_escape(log_file, href, strlen(href));
                fprintf(log_file, "#L%d\">%d:%d</a>", tok->line, tok->line, tok->pos);
            } else {
                fprintf(log_file, "%d:%d", tok->line, tok->pos);
            }
        } else {
            fputs("&mdash;", log_file);
        }
        fputs("</td>", log_file);

        fputs("<td><code>", log_file);
        if (tok->text && tok->length) {
            size_t snippet = tok->length;
            if (snippet > 128) snippet = 128;
            html_escape(log_file, tok->text, snippet);
            if (snippet < tok->length) fputs("&hellip;", log_file);
        } else {
            fputs("&nbsp;", log_file);
        }
        fputs("</code></td>", log_file);

        fprintf(log_file,
                "<td>ptr=%p sig=0x%X elems=%zu left=%p right=%p parent=%p</td>",
                (void *)&tok->node,
                (unsigned int) tok->node.signature,
                tok->node.elements,
                (void *) tok->node.left,
                (void *) tok->node.right,
                (void *) tok->node.parent);

        fputs("</tr>\n", log_file);
    }

    fputs("</tbody></table>\n</section>\n", log_file);
    fflush(log_file);
}

function int latex_prec_op(OPERATOR::OPERATOR op) {
    switch (op) {
        case OPERATOR::ADD:
        case OPERATOR::SUB:   return 1;
        case OPERATOR::MUL:
        case OPERATOR::DIV:   return 2;
        case OPERATOR::POW:   return 3;
        default:    return 4;
    }
}

function int latex_prec(const NODE_T *node) {
    if (!node || node->type != OPERATOR_T) return 5;
    return latex_prec_op(node->value.opr);
}

function bool need_parens(OPERATOR::OPERATOR parent, const NODE_T *child, bool right_side) {
    if (!child || child->type != OPERATOR_T) return false;
    if (parent == OPERATOR::DIV) return false;
    int parent_prec = latex_prec_op(parent);
    int child_prec  = latex_prec(child);
    if (parent == OPERATOR::POW) {
        return right_side ? (child_prec <= parent_prec) : (child_prec < parent_prec);
    }
    if (child_prec < parent_prec) return true;
    if (child_prec > parent_prec) return false;
    if (parent == OPERATOR::SUB && right_side) return true;
    return false;
}

function const char *latex_func_name(OPERATOR::OPERATOR op) {
    switch (op) {
        case OPERATOR::LN:   return "\\ln";
        case OPERATOR::SIN:  return "\\sin";
        case OPERATOR::COS:  return "\\cos";
        case OPERATOR::TAN:  return "\\tan";
        case OPERATOR::CTG:  return "\\cot";
        case OPERATOR::ASIN: return "\\arcsin";
        case OPERATOR::ACOS: return "\\arccos";
        case OPERATOR::ATAN: return "\\arctan";
        case OPERATOR::ACTG: return "\\arccot";
        default:   return nullptr;
    }
}

// function size_t latex_size(EQ_TREE_T *tree, NODE_T *node) {
//     if (!tree || !node) return 0;
//     switch (node->type) {
//         case NUMBER_T:
//             return 48;
//         case IDENTIFIER_T: {
//             const mystr::mystr_t *name = tree->vars ? varlist::get(tree->vars, node->value.var) : nullptr;
//             return (name && name->str) ? strlen(name->str) : 0;
//         }
//         case OPERATOR_T: {
//             size_t left_len = node->left ? latex_size(tree, node->left) : 0;
//             size_t right_len = node->right ? latex_size(tree, node->right) : 0;
//             switch (node->value.opr) {
//                 case DIV:
//                     return left_len + right_len + 9;
//                 case ADD:
//                 case SUB: {
//                     size_t extra = (need_parens(node->value.opr, node->left, false) ? 2 : 0) +
//                                    (need_parens(node->value.opr, node->right, true) ? 2 : 0);
//                     return left_len + right_len + extra + 3;
//                 }
//                 case MUL: {
//                     size_t extra = (need_parens(node->value.opr, node->left, false) ? 2 : 0) +
//                                    (need_parens(node->value.opr, node->right, true) ? 2 : 0);
//                     return left_len + right_len + extra + 7;
//                 }
//                 case POW: {
//                     size_t extra = (need_parens(node->value.opr, node->left, false) ? 2 : 0);
//                     size_t exp_extra = (need_parens(node->value.opr, node->right, true) ? 2 : 0);
//                     if (right_len == 0) return left_len + extra;
//                     return left_len + right_len + extra + exp_extra + 3;
//                 }
//                 case LOG: {
//                     size_t base_part = 0;
//                     if (node->right && right_len) base_part = 3 + right_len; // _{ base }
//                     return 4 + base_part + 2 + left_len;
//                 }
//                 case LN:
//                 case SIN:
//                 case COS:
//                 case TAN:
//                 case CTG:
//                 case ASIN:
//                 case ACOS:
//                 case ATAN:
//                 case ACTG:
//                 case SINH:
//                 case COSH:
//                 case TANH:
//                 case CTH: {
//                     const char *name = latex_func_name(node->value.opr);
//                     size_t name_len = name ? strlen(name) : 0;
//                     return name_len + 2 + left_len;
//                 }
//                 case SQRT:
//                     return 6 + left_len + 1;
//                 default:
//                     return left_len + right_len;
//             }
//         }
//         default:
//             return 0;
//     }
// }

function void append_char(char **dst, char ch) {
    if (!dst || !*dst) return;
    **dst = ch;
    ++(*dst);
}

function void append_str(char **dst, const char *src) {
    if (!dst || !*dst || !src) return;
    size_t len = strlen(src);
    memcpy(*dst, src, len);
    *dst += len;
}

// function void latex_emit(EQ_TREE_T *tree, NODE_T *node, char **out) {
//     if (!tree || !node || !out || !*out) return;
//     switch (node->type) {
//         case NUMBER_T: {
//             if (node->value.num < 0)
//                 append_char(out, '(');
//             char raw[64] = "";
//             int written = snprintf(raw, sizeof(raw), "%.15g", node->value.num);
//             if (written <= 0) break;
//             char *exp = strchr(raw, 'e');
//             if (!exp) exp = strchr(raw, 'E');
//             if (!exp) {
//                 append_str(out, raw);
//                 if (node->value.num < 0)
//                     append_char(out, ')');
//                 break;
//             }
//             int exponent = atoi(exp + 1);
//             *exp = '\0';
//             bool drop_mantissa = (strcmp(raw, "1") == 0);
//             if (!drop_mantissa) {
//                 append_str(out, raw);
//                 append_str(out, " \\cdot ");
//             }
//             append_str(out, "10^{");
//             char exp_buf[16] = "";
//             snprintf(exp_buf, sizeof(exp_buf), "%d", exponent);
//             append_str(out, exp_buf);
//             append_char(out, '}');
//             if (node->value.num < 0)
//                     append_char(out, ')');
//             break;
//         }
//         case IDENTIFIER_T: {
//             const mystr::mystr_t *name = tree->vars ? varlist::get(tree->vars, node->value.var) : nullptr;
//             if (name && name->str) append_str(out, name->str);
//             break;
//         }
//         case OPERATOR_T: {
//             switch (node->value.opr) {
//                 case DIV:
//                     append_str(out, "\\frac{");
//                     latex_emit(tree, node->left, out);
//                     append_str(out, "}{");
//                     latex_emit(tree, node->right, out);
//                     append_char(out, '}');
//                     return;
//                 case POW: {
//                     bool wrap_left = need_parens(node->value.opr, node->left, false);
//                     bool wrap_right = need_parens(node->value.opr, node->right, true);
//                     if (wrap_left) append_char(out, '(');
//                     latex_emit(tree, node->left, out);
//                     if (wrap_left) append_char(out, ')');
//                     if (node->right) {
//                         append_str(out, "^{");
//                         if (wrap_right) append_char(out, '(');
//                         latex_emit(tree, node->right, out);
//                         if (wrap_right) append_char(out, ')');
//                         append_char(out, '}');
//                     }
//                     return;
//                 }
//                 case LOG:
//                     append_str(out, "\\log");
//                     if (node->right) {
//                         append_str(out, "_{");
//                         latex_emit(tree, node->right, out);
//                         append_char(out, '}');
//                     }
//                     append_char(out, '{');
//                     latex_emit(tree, node->left, out);
//                     append_char(out, '}');
//                     return;
//                 case SQRT:
//                     append_str(out, "\\sqrt{");
//                     latex_emit(tree, node->left, out);
//                     append_char(out, '}');
//                     return;
//                 default: {
//                     const char *func = latex_func_name(node->value.opr);
//                     if (func) {
//                         append_str(out, func);
//                         append_char(out, '{');
//                         bool wrap_arg = node->left && node->left->type == OPERATOR_T;
//                         if (wrap_arg) append_char(out, '(');
//                         latex_emit(tree, node->left, out);
//                         if (wrap_arg) append_char(out, ')');
//                         append_char(out, '}');
//                         return;
//                     }
//                 }
//             }
//             bool wrap_left = need_parens(node->value.opr, node->left, false);
//             bool wrap_right = need_parens(node->value.opr, node->right, true);
//             if (wrap_left) append_char(out, '(');
//             latex_emit(tree, node->left, out);
//             if (wrap_left) append_char(out, ')');
//             switch (node->value.opr) {
//                 case ADD: append_str(out, " + "); break;
//                 case SUB: append_str(out, " - "); break;
//                 case MUL: append_str(out, " \\cdot "); break;
//                 default: break;
//             }
//             if (wrap_right) append_char(out, '(');
//             latex_emit(tree, node->right, out);
//             if (wrap_right) append_char(out, ')');
//             break;
//         }
//         default:
//             break;
//     }
// }

// U need to free the returned string
// char *latex_dump(EQ_TREE_T *node) {
//     if (!node || !node->root) {
//         ERROR_MSG("latex_dump: node or node->root is nullptr");
//         return nullptr;
//     }
//     size_t len = latex_size(node, node->root);
//     char *buf = TYPED_CALLOC(len + 1, char);
//     if (!buf) return nullptr;
//     char *cursor = buf;
//     latex_emit(node, node->root, &cursor);
//     if (cursor) *cursor = '\0';
//     return buf;
// }

// const char *get_random_str_to_dif_op(OPERATOR::OPERATOR op) {
//     int is_op_string = randint(0, 2); // Если 1 то выдает строку оператора, иначе глобальную
//     if (is_op_string == 0)
//     switch (op) {
// #define CASE_(OPR) case OPR: return OPR##_FUNC_STR[randint(0, ARRAY_COUNT(OPR##_FUNC_STR))]
//             CASE_(ADD);
//             CASE_(SUB);
//             CASE_(MUL);
//             CASE_(DIV);
//             CASE_(POW);
//             CASE_(LOG);
//             CASE_(LN);
//             CASE_(SIN);
//             CASE_(COS);
//             CASE_(TAN);
//             CASE_(CTG);
//             CASE_(ASIN);
//             CASE_(ACOS);
//             CASE_(ATAN);
//             CASE_(ACTG);
//             CASE_(SQRT);
//             CASE_(SINH);
//             CASE_(COSH);
//             CASE_(TANH);
//             CASE_(CTH);
//             default:
//                 return STEPS_STR[randint(0, ARRAY_COUNT(STEPS_STR))];
// #undef CASE_
//         }
//     else if (is_op_string == 1) {
//         return STEPS_STR[randint(0, ARRAY_COUNT(STEPS_STR))];
//     }
//     else /*if (is_op_string == 2)*/ {
//         return COMPLEX_FUNC_STR[randint(0, ARRAY_COUNT(COMPLEX_FUNC_STR))];
//     }
// }