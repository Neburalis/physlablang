#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base.h"
#include "io_utils.h"
#include "frontend.h"
#include "stringNthong.h"
#include "utf8.h"
#include "enhanced_string.h"

typedef struct {
    const char *text;
    size_t      len;
    NODE_TYPE   type;
    int         value;
    int         casefold;
    int         word_boundary;
} fixed_token_t;

#define FIXED(text, t, val, fold, bound) { text, sizeof(text) - 1, t, val, fold, bound }
#define y 1
#define n 0

static const fixed_token_t g_fixed[] = {
    FIXED("ТЕОРЕТИЧЕСКИЕ СВЕДЕНИЯ",   KEYWORD_T,  KEYWORD::THEORETICAL,      y, y),
    FIXED("ОБСУЖДЕНИЕ РЕЗУЛЬТАТОВ",   KEYWORD_T,  KEYWORD::RESULTS,          y, y),
    FIXED("ЛАБОРАТОРНАЯ РАБОТА",      KEYWORD_T,  KEYWORD::LAB,              y, y),
    FIXED("КОНЕЦ РЕЗУЛЬТАТОВ",        KEYWORD_T,  KEYWORD::END_RESULTS,      y, y),
    FIXED("РАССЧИТЫВАЕТСЯ ИЗ",        KEYWORD_T,  KEYWORD::FUNC_CALL,        y, y),
    FIXED("КОНЕЦ АННОТАЦИИ",          KEYWORD_T,  KEYWORD::END_ANNOTATION,   y, y),
    FIXED("КОНЕЦ ВЫВОДОВ",            KEYWORD_T,  KEYWORD::END_CONCLUSION,   y, y),
    FIXED("КОНЕЦ ФОРМУЛЫ",            KEYWORD_T,  KEYWORD::END_FORMULA,      y, y),
    FIXED("КОНЕЦ ТЕОРИИ",             KEYWORD_T,  KEYWORD::END_THEORETICAL,  y, y),
    FIXED("КОНЕЦ РАБОТЫ",             KEYWORD_T,  KEYWORD::END_EXPERIMENTAL, y, y),
    FIXED("КОНЕЦ ВЫВОДА",             KEYWORD_T,  KEYWORD::END_CONCLUSION,   y, y),
    FIXED("ХОД РАБОТЫ",               KEYWORD_T,  KEYWORD::EXPERIMENTAL,     y, y),
    FIXED("ВОЗВРАТИТЬ",               KEYWORD_T,  KEYWORD::RETURN,           y, y),
    FIXED("АННОТАЦИЯ",                KEYWORD_T,  KEYWORD::ANNOTATION,       y, y),
    FIXED("ПОВТОРЯЕМ",                KEYWORD_T,  KEYWORD::WHILE,            y, y),
    FIXED("ПРИМЕНЯЕМ",                KEYWORD_T,  KEYWORD::FUNC_CALL,        y, y),
    FIXED("ВЫЧИСЛЯЕМ",                KEYWORD_T,  KEYWORD::FUNC_CALL,        y, y),
    FIXED("ВЕЛИЧИНА",                 KEYWORD_T,  KEYWORD::VAR_DECLARATION,  y, y),
    FIXED("ПОКАЗАТЬ",                 KEYWORD_T,  OPERATOR::OUT,             y, y),
    FIXED("ИЗМЕРИТЬ",                 KEYWORD_T,  OPERATOR::IN,              y, y),
    FIXED("ФОРМУЛА",                  KEYWORD_T,  KEYWORD::FORMULA,          y, y),
    FIXED("ВЫРАЗИМ",                  KEYWORD_T,  KEYWORD::LET_ASSIGNMENT,   y, y),
    FIXED("ВЫВЕСТИ",                  KEYWORD_T,  OPERATOR::OUT,             y, y),
    FIXED("ARCCTAN",                  OPERATOR_T, OPERATOR::ACTG,            y, y),
    FIXED("ВЫВОДЫ",                   KEYWORD_T,  KEYWORD::CONCLUSION,       y, y),
    FIXED("ARCSIN",                   OPERATOR_T, OPERATOR::ASIN,            y, y),
    FIXED("ARCCOS",                   OPERATOR_T, OPERATOR::ACOS,            y, y),
    FIXED("ARCTAN",                   OPERATOR_T, OPERATOR::ATAN,            y, y),
    FIXED("ARCCTG",                   OPERATOR_T, OPERATOR::ACTG,            y, y),
    FIXED("ПУСТЬ",                    KEYWORD_T,  KEYWORD::LET_ASSIGNMENT,   y, y),
    FIXED("БУДЕТ",                    KEYWORD_T,  KEYWORD::LET_ASSIGNMENT,   y, y),
    FIXED("ИНАЧЕ",                    KEYWORD_T,  KEYWORD::ELSE,             y, y),
    FIXED("ARCTG",                    OPERATOR_T, OPERATOR::ATAN,            y, y),
    FIXED("ЦЕЛЬ",                     KEYWORD_T,  KEYWORD::GOAL_LITERAL,     y, y),
    FIXED("ПОКА",                     KEYWORD_T,  KEYWORD::WHILE_CONDITION,  y, y),
    FIXED("СТОП",                     KEYWORD_T,  KEYWORD::END_WHILE,        y, y),
    FIXED("ЕСЛИ",                     KEYWORD_T,  KEYWORD::IF,               y, y),
    FIXED("CTAN",                     OPERATOR_T, OPERATOR::CTG,             y, y),
    FIXED("ASIN",                     OPERATOR_T, OPERATOR::ASIN,            y, y),
    FIXED("ACOS",                     OPERATOR_T, OPERATOR::ACOS,            y, y),
    FIXED("ATAN",                     OPERATOR_T, OPERATOR::ATAN,            y, y),
    FIXED("ACTG",                     OPERATOR_T, OPERATOR::ACTG,            y, y),
    FIXED("SQRT",                     OPERATOR_T, OPERATOR::SQRT,            y, y),
    FIXED("COS",                      OPERATOR_T, OPERATOR::COS,             y, y),
    FIXED("SIN",                      OPERATOR_T, OPERATOR::SIN,             y, y),
    FIXED("POW",                      OPERATOR_T, OPERATOR::POW,             n, n),
    FIXED("TAN",                      OPERATOR_T, OPERATOR::TAN,             y, y),
    FIXED("CTG",                      OPERATOR_T, OPERATOR::CTG,             y, y),
    FIXED("AND",                      OPERATOR_T, OPERATOR::AND,             y, y),
    FIXED("NOT",                      OPERATOR_T, OPERATOR::NOT,             y, y),
    FIXED("ИЛИ",                      OPERATOR_T, OPERATOR::OR,              y, y),
    FIXED("ТО",                       KEYWORD_T,  KEYWORD::THEN,             y, y),
    FIXED("LN",                       OPERATOR_T, OPERATOR::LN,              y, y),
    FIXED("TG",                       OPERATOR_T, OPERATOR::TAN,             y, y),
    FIXED("OR",                       OPERATOR_T, OPERATOR::OR,              y, y),
    FIXED("НЕ",                       OPERATOR_T, OPERATOR::NOT,             y, y),
    FIXED("==",                       OPERATOR_T, OPERATOR::EQ,              n, n),
    FIXED("!=",                       OPERATOR_T, OPERATOR::NEQ,             n, n),
    FIXED("<=",                       OPERATOR_T, OPERATOR::BELOW_EQ,        n, n),
    FIXED(">=",                       OPERATOR_T, OPERATOR::ABOVE_EQ,        n, n),
    FIXED("И",                        OPERATOR_T, OPERATOR::AND,             y, y),
    FIXED("=",                        OPERATOR_T, OPERATOR::ASSIGNMENT,      n, n),
    FIXED("<",                        OPERATOR_T, OPERATOR::BELOW,           n, n),
    FIXED(">",                        OPERATOR_T, OPERATOR::ABOVE,           n, n),
    FIXED("+",                        OPERATOR_T, OPERATOR::ADD,             n, n),
    FIXED("-",                        OPERATOR_T, OPERATOR::SUB,             n, n),
    FIXED("*",                        OPERATOR_T, OPERATOR::MUL,             n, n),
    FIXED("/",                        OPERATOR_T, OPERATOR::DIV,             n, n),
    FIXED("%",                        OPERATOR_T, OPERATOR::MOD,             n, n),
    FIXED("(",                        DELIMITER_T,DELIMITER::PAR_OPEN,       n, n),
    FIXED(")",                        DELIMITER_T,DELIMITER::PAR_CLOSE,      n, n),
    FIXED(",",                        DELIMITER_T,DELIMITER::COMA,           n, n),
    FIXED(":",                        DELIMITER_T,DELIMITER::COLON,          n, n)
};

#undef FIXED
#undef y
#undef n

/**
 * @brief Гарантирует наличие VarList в контексте.
 * @param ctx[in,out]   контекст компилятора.
 * @return 0 если готово, -1 при ошибке.
 */
static int ensure_varlist(FRONT_COMPL_T *ctx);

/**
 * @brief Создает токен на основе записи из таблицы g_fixed.
 * @param ctx[in,out]   контекст компилятора.
 * @param ft[in]        описание лексемы.
 * @param idx[in]       смещение в буфере.
 * @param line[in]      номер строки.
 * @param pos[in]       позиция в строке.
 * @return 0 при успехе, -1 при ошибке.
 */
static int emit_fixed(FRONT_COMPL_T *ctx,
    const fixed_token_t *ft, size_t idx, int32_t line, int32_t pos
);

/**
 * @brief Сохраняет подстроку в VarList и возвращает индекс.
 * @param ctx[in]       контекст компилятора.
 * @param text[in]      подстрока.
 * @param len[in]       длина в байтах.
 * @return индекс в VarList или varlist::NPOS.
 */
static size_t store_span(FRONT_COMPL_T *ctx, const char *text, size_t len);

/**
 * @brief Обновляет счетчики строки/позиции для диапазона UTF-8.
 * @param buf[in]       исходный текст.
 * @param start[in]     начало диапазона.
 * @param end[in]       конец диапазона.
 * @param line[in,out]  счетчик строк.
 * @param pos[in,out]   позиция в строке.
 */
static void advance_pos(const char *buf,
    size_t start, size_t end, int32_t *line, int32_t *pos
);

/**
 * @brief Ищет совпадение с таблицей фиксированных лексем.
 * @param buf[in]       исходный текст.
 * @param len[in]       длина буфера.
 * @param idx[in]       текущая позиция.
 * @param tmp[in,out]   временный буфер для casefold.
 * @return описание лексемы или nullptr.
 */
static const fixed_token_t *match_fixed(const char *buf, size_t len,
    size_t idx, char *tmp
);

/**
 * @brief Обходит буфер и формирует все токены.
 * @param ctx[in,out]   контекст компилятора.
 * @return 0 при успехе, -1 при ошибке.
 */
static int lex_buffer(FRONT_COMPL_T *ctx);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * @brief Загружает UTF-8 буфер и выполняет разбор.
 * @param ctx[in]       контекст компилятора.
 * @param name[in]      имя источника или nullptr.
 * @param text[in]      входной текст.
 * @param bytes[in]     длина в байтах.
 * @return 0 при успехе, -1 иначе.
 */
int lexer_from_buffer(
    FRONT_COMPL_T *ctx,
    const char *name, const char *text, size_t bytes
) {
    if (!ctx || !text) return -1;
    lexer_reset(ctx);
    char *copy = strndup(text, bytes);
    if (!copy) return -1;
    ctx->buf = copy;
    ctx->buf_len = bytes;
    ctx->owns_buf = true;
    if (name) {
        char *n = strdup(name);
        if (!n) {
            lexer_reset(ctx);
            return -1;
        }
        ctx->name = n;
        ctx->owns_name = true;
    } else {
        ctx->name = nullptr;
        ctx->owns_name = false;
    }
    int res = lex_buffer(ctx);
    if (res) lexer_reset(ctx);
    return res;
}

/**
 * @brief Загружает содержимое файла и передает в лексер.
 * @param ctx[in]       контекст компилятора.
 * @param filename[in]  путь к файлу.
 * @return 0 при успехе, -1 иначе.
 */
int lexer_load_file(FRONT_COMPL_T *ctx, const char *filename) {
    if (!ctx || !filename) return -1;
    size_t buf_len = 0;
    char *buf = read_file_to_buf(filename, &buf_len);
    if (!buf) return -1;
    size_t bytes = buf_len;
    if (bytes && buf[bytes - 1] == '\0') --bytes;
    int res = lexer_from_buffer(ctx, filename, buf, bytes);
    free(buf);
    return res;
}

/**
 * @brief Освобождает буферы, токены и VarList.
 * @param ctx[in,out]   контекст компилятора.
 */
void lexer_reset(FRONT_COMPL_T *ctx) {
    if (!ctx) return;
    if (ctx->owns_buf && ctx->buf) free(ctx->buf);
    ctx->buf = NULL;
    ctx->buf_len = 0;
    ctx->owns_buf = false;
    if (ctx->tokens) free(ctx->tokens);
    ctx->tokens = NULL;
    ctx->token_count = 0;
    ctx->token_capacity = 0;
    if (ctx->owns_name && ctx->name) free((void *)ctx->name);
    ctx->name = NULL;
    ctx->owns_name = false;
    ctx->root = NULL;
    if (ctx->owns_vars && ctx->vars) {
        varlist::destruct(ctx->vars);
        free(ctx->vars);
        ctx->vars = NULL;
        ctx->owns_vars = false;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * @brief Гарантирует наличие VarList в контексте.
 * @param ctx[in,out] контекст компилятора.
 * @return 0 если готово, -1 при ошибке.
 */
static int ensure_varlist(FRONT_COMPL_T *ctx) {
    if (ctx->vars) return 0;
    varlist::VarList *list = TYPED_MALLOC(varlist::VarList);
    if (!list) return -1;
    varlist::init(list);
    ctx->vars = list;
    ctx->owns_vars = true;
    return 0;
}

/**
 * @brief Расширяет динамический массив токенов при необходимости.
 * @param ctx[in] контекст компилятора.
 * @param need[in] требуемый размер.
 * @return 0 при успехе, -1 при нехватке памяти.
 */
int ensure_token_cap(FRONT_COMPL_T *ctx, size_t need) {
    if (ctx->token_capacity >= need) return 0;
    size_t cap = ctx->token_capacity ? ctx->token_capacity : 32;
    while (cap < need) cap <<= 1;
    TOKEN_T *tmp = TYPED_REALLOC(ctx->tokens, cap, TOKEN_T);
    if (!tmp) return -1;
    ctx->tokens = tmp;
    ctx->token_capacity = cap;
    return 0;
}

/**
 * @brief Создает запись о токене в массиве.
 * @param ctx[in,out] контекст компилятора.
 * @param type[in] тип узла.
 * @param value[in] значение узла.
 * @param text[in] указатель на исходную подстроку.
 * @param len[in] длина подстроки.
 * @param line[in] номер строки.
 * @param pos[in] позиция в строке.
 * @return 0 при успехе, -1 при ошибке.
 */
int add_token(
    FRONT_COMPL_T *ctx,
    NODE_TYPE type, NODE_VALUE_T value,
    const char *text, size_t len, int32_t line, int32_t pos
) {
    if (ensure_token_cap(ctx, ctx->token_count + 1)) return -1;
    TOKEN_T *tok = &ctx->tokens[ctx->token_count++];
    tok->node.signature = signature;
    tok->node.type = type;
    tok->node.value = value;
    tok->node.elements = 0;
    tok->node.left = NULL;
    tok->node.right = NULL;
    tok->node.parent = NULL;
    tok->text = text;
    tok->length = len;
    tok->line = line;
    tok->pos = pos;
    tok->filename = ctx->name;
    return 0;
}

/**
 * @brief Сохраняет подстроку в VarList и возвращает индекс.
 * @param ctx[in]  контекст компилятора.
 * @param text[in] подстрока.
 * @param len[in]  длина в байтах.
 * @return индекс в VarList или varlist::NPOS.
 */
static size_t store_span(FRONT_COMPL_T *ctx, const char *text, size_t len) {
    if (ensure_varlist(ctx)) return varlist::NPOS;
    char *tmp = strndup(text, len);
    if (!tmp) return varlist::NPOS;
    mystr::mystr_t str = mystr::construct(tmp);
    size_t id = varlist::add(ctx->vars, &str);
    free(tmp);
    return id;
}

/**
 * @brief Обновляет счетчики строки/позиции для диапазона UTF-8.
 * @param buf[in]       исходный текст.
 * @param start[in]     начало диапазона.
 * @param end[in]       конец диапазона.
 * @param line[in,out]  счетчик строк.
 * @param pos[in,out]   позиция в строке.
 */
static void advance_pos(
    const char *buf,
    size_t start, size_t end,
    int32_t *line, int32_t *pos
) {
    size_t i = start;
    while (i < end) {
        unsigned char c = (unsigned char) buf[i];
        if (c == '\n') {
            ++(*line);
            *pos = 1;
            ++i;
            continue;
        }
        uint8_t step = utf8_char_length(c);
        if (!step) step = 1;
        i += step;
        ++(*pos);
    }
}

/**
 * @brief Ищет совпадение с таблицей фиксированных лексем.
 * @param buf[in]       исходный текст.
 * @param len[in]       длина буфера.
 * @param idx[in]       текущая позиция.
 * @param tmp[in,out]   временный буфер для casefold.
 * @return описание лексемы или nullptr.
 */
static const fixed_token_t *match_fixed(
    const char *buf, size_t len,
    size_t idx, char *tmp
) {
    size_t i;
    for (i = 0; i < ARRAY_COUNT(g_fixed); ++i) {
        const fixed_token_t *ft = &g_fixed[i];
        if (idx + ft->len > len) continue;
        if (ft->casefold) {
            copy_upper(tmp, buf + idx, ft->len);
            if (strncmp(tmp, ft->text, ft->len) != 0) continue;
        } else {
            if (strncmp(buf + idx, ft->text, ft->len) != 0) continue;
        }
        if (ft->word_boundary) {
            if (idx > 0) {
                unsigned char prev = (unsigned char) buf[idx - 1];
                if (ascii_word(prev)) continue;
            }
            if (idx + ft->len < len) {
                unsigned char next = (unsigned char) buf[idx + ft->len];
                if (ascii_word(next)) continue;
            }
        }
        return ft;
    }
    return nullptr;
}

/**
 * @brief Эмитирует токен на основе записи из таблицы g_fixed.
 * @param ctx[in,out] контекст компилятора.
 * @param ft[in]      описание лексемы.
 * @param idx[in]     смещение в буфере.
 * @param line[in]    номер строки.
 * @param pos[in]     позиция в строке.
 * @return 0 при успехе, -1 при ошибке.
 */
static int emit_fixed(
    FRONT_COMPL_T *ctx,
    const fixed_token_t *ft, size_t idx, int32_t line, int32_t pos
) {
    NODE_VALUE_T val;
    if (ft->type == KEYWORD_T)
        val.keyword = (KEYWORD::KEYWORD) ft->value;
    else if (ft->type == OPERATOR_T)
        val.opr = (OPERATOR::OPERATOR) ft->value;
    else
        val.delimiter = (DELIMITER::DELIMITER) ft->value;
    return add_token(ctx,
        ft->type, val,
        ctx->buf + idx, ft->len, line, pos
    );
}

/**
 * @brief Обходит буфер и формирует все токены.
 * @param ctx[in,out]   контекст компилятора.
 * @return 0 при успехе, -1 при ошибке.
 */
static int lex_buffer(FRONT_COMPL_T *ctx) {
    if (!ctx || !ctx->buf) return -1;
    const char *buf = ctx->buf;
    size_t len = ctx->buf_len;
    size_t idx = 0;
    int32_t line = 1;
    int32_t pos = 1;
    char tmp[64];

    while (idx < len) {
        if (buf[idx] == '\n') {
            ++line;
            pos = 1;
            ++idx;
            continue;
        }

        while (idx < len && isspace((unsigned char) buf[idx])) {
            if (buf[idx] == '\n') {
                ++line;
                pos = 1;
            } else
                ++pos;
            ++idx;
        }
        if (idx >= len) break;

        const fixed_token_t *ft = match_fixed(buf, len, idx, tmp);
        if (ft) {
            if (emit_fixed(ctx, ft, idx, line, pos)) return -1;
            advance_pos(buf, idx, idx + ft->len, &line, &pos);
            idx += ft->len;
            continue;
        }

        if (buf[idx] == '"') {
            size_t start = idx;
            size_t end = idx + 1;
            while (end < len && buf[end] != '"') {
                if (buf[end] == '\n') return -1;
                ++end;
            }
            if (end >= len) return -1;
            size_t content_start = start + 1;
            size_t content_len = end - content_start;
            size_t id = store_span(ctx, buf + content_start, content_len);
            if (id == varlist::NPOS) return -1;
            NODE_VALUE_T val;
            val.delimiter = DELIMITER::QUOTE;
            if (add_token(ctx, DELIMITER_T, val, buf + start, 1, line, pos + 1)) return -1;
            val.id = id;
            if (add_token(ctx, LITERAL_T, val, buf + content_start, content_len, line, pos + 1)) return -1;
            val.delimiter = DELIMITER::QUOTE;
            if (add_token(ctx, DELIMITER_T, val, buf + end, 1, line, pos + 1)) return -1;
            advance_pos(buf, start, end + 1, &line, &pos);
            idx = end + 1;
            continue;
        }

        if (buf[idx] == '/' && idx + 1 < len && buf[idx + 1] == '/') {
            idx += 2;
            pos += 2;
            while (idx < len && buf[idx] != '\n') {
                ++idx;
                ++pos;
            }
            continue;
        }

        unsigned char c = (unsigned char) buf[idx];
        if (isdigit(c)) {
            size_t start = idx;
            while (idx < len && isdigit((unsigned char)buf[idx])) ++idx;
            if (idx < len && buf[idx] == '.') {
                ++idx;
                while (idx < len && isdigit((unsigned char)buf[idx])) ++idx;
            }
            size_t num_len = idx - start;
            char *num = (char *)malloc(num_len + 1);
            if (!num) return -1;
            memcpy(num, buf + start, num_len);
            num[num_len] = '\0';
            NODE_VALUE_T val;
            val.num = strtod(num, NULL);
            free(num);
            if (add_token(ctx, NUMBER_T, val, buf + start, num_len, line, pos)) return -1;
            advance_pos(buf, start, idx, &line, &pos);
            continue;
        }

        if (isascii(c) && (isalpha(c) || c == '_')) {
            size_t start = idx;
            ++idx;
            while (idx < len) {
                unsigned char cc = (unsigned char) buf[idx];
                if (!ascii_word(cc)) break;
                ++idx;
            }
            size_t span = idx - start;
            size_t id = store_span(ctx, buf + start, span);
            if (id == varlist::NPOS) return -1;
            NODE_VALUE_T val;
            val.id = id;
            if (add_token(ctx, IDENTIFIER_T, val, buf + start, span, line, pos)) return -1;
            advance_pos(buf, start, idx, &line, &pos);
            continue;
        }

        {
            size_t start = idx;
            while (idx < len && buf[idx] != '\n') ++idx;
            size_t span = idx - start;
            if (span == 0) continue;
            size_t id = store_span(ctx, buf + start, span);
            if (id == varlist::NPOS) return -1;
            NODE_VALUE_T val;
            val.id = id;
            if (add_token(ctx, LITERAL_T, val, buf + start, span, line, pos)) return -1;
            advance_pos(buf, start, idx, &line, &pos);
            continue;
        }

        uint8_t step = utf8_char_length(c);
        if (!step) step = 1;
        advance_pos(buf, idx, idx + step, &line, &pos);
        idx += step;
    }
    return 0;
}