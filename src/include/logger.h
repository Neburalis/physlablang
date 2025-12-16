#ifndef DIFFERENTIATION_LOGGER_H
#define DIFFERENTIATION_LOGGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct Logger {
    char   dir[512];
    bool   dir_inited;
    FILE  *file;
    char   filepath[512];
} Logger;

int init_logger(const char *log_dirname);
int init_logger(const char *log_dirname, const char *first_head_line, ...);
void destruct_logger(void);

FILE *logger_get_file(void);
const char *logger_get_active_dir(void);

#endif // DIFFERENTIATION_LOGGER_H

