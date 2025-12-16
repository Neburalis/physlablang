#include <stdarg.h>
#include <string.h>

#include "base.h"
#include "logger.h"

global Logger GLOBAL_LOGGER = {};

function Logger *get_global_logger() {
    return &GLOBAL_LOGGER;
}

int init_logger(const char *log_dirname, const char *html_head_fmt, ...) {
    Logger *logger = get_global_logger();

    if (log_dirname == nullptr || log_dirname[0] == '\0') return -1;

    if (logger->file != nullptr) {
        fclose(logger->file);
        logger->file = nullptr;
    }

    memset(logger->dir, 0, sizeof(logger->dir));
    memset(logger->filepath, 0, sizeof(logger->filepath));
    logger->dir_inited = false;

    size_t len = strlen(log_dirname);
    if (len >= sizeof(logger->dir)) return -1;
    strncpy(logger->dir, log_dirname, sizeof(logger->dir));

    int written = 0;
    if (logger->dir[len - 1] == '/')
        written = snprintf(logger->filepath, sizeof(logger->filepath), "%slog.html", logger->dir);
    else
        written = snprintf(logger->filepath, sizeof(logger->filepath), "%s/log.html", logger->dir);

    if (written <= 0 || (size_t) written >= sizeof(logger->filepath)) {
        logger->dir[0] = '\0';
        return -1;
    }

    logger->file = fopen(logger->filepath, "w");
    if (logger->file == nullptr) {
        logger->dir[0] = '\0';
        logger->filepath[0] = '\0';
        return -1;
    }

    FILE *file = logger->file;
    fprintf(file, "<html>\n<head><meta charset=\"utf-8\"><title>Logger</title>");

    if (html_head_fmt != nullptr) {
        va_list args;
        va_start(args, html_head_fmt);
        vfprintf(file, html_head_fmt, args);
        va_end(args);
    }

    fprintf(file, "\n</head>\n<body>\n<pre>\n");
    fflush(logger->file);

    logger->dir_inited = true;
    return 0;
}

int init_logger(const char *log_dirname) {
    return init_logger(log_dirname, nullptr);
}

void destruct_logger(void) {
    Logger *logger = get_global_logger();

    if (logger->file != nullptr) {
        fprintf(logger->file, "</pre>\n</body>\n</html>\n");
        fclose(logger->file);
        logger->file = nullptr;
    }

    logger->dir[0] = '\0';
    logger->filepath[0] = '\0';
    logger->dir_inited = false;
}

FILE *logger_get_file(void) {
    Logger *logger = get_global_logger();
    return logger->file;
}

const char *logger_get_active_dir(void) {
    Logger *logger = get_global_logger();
    return logger->dir_inited ? logger->dir : ".";
}
