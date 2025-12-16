#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "frontend.h"
#include "logger.h"

static int ensure_dir(const char *path) {
    if (!path || !*path)
        return -1;
    struct stat st = {};
    char buf[PATH_MAX] = "";
    size_t len = strlen(path);
    if (len >= sizeof(buf))
        return -1;
    memcpy(buf, path, len + 1);
    for (size_t i = 1; i <= len; ++i) {
        if (buf[i] == '/' || buf[i] == '\0') {
            char saved = buf[i];
            buf[i] = '\0';
            if (*buf) {
                if (stat(buf, &st) != 0) {
#if defined(_WIN32)
                    if (mkdir(buf) != 0 && errno != EEXIST)
                        return -1;
#else
                    if (mkdir(buf, 0755) != 0 && errno != EEXIST)
                        return -1;
#endif
                } else if (!S_ISDIR(st.st_mode)) {
                    return -1;
                }
            }
            buf[i] = saved;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    char exe_path[PATH_MAX] = ".";
    if (realpath(argv[0], exe_path) == NULL)
        strncpy(exe_path, ".", sizeof(exe_path));

    char base_dir[PATH_MAX] = ".";
    size_t total = strlen(exe_path);
    while (total > 0 && exe_path[total - 1] != '/')
        --total;
    if (total > 0) {
        if (total >= sizeof(base_dir))
            total = sizeof(base_dir) - 1;
        memcpy(base_dir, exe_path, total);
        base_dir[total] = '\0';
    }

    char default_input[PATH_MAX] = "";
    snprintf(default_input, sizeof(default_input), "%s/examples/factorial.phylab", base_dir);
    char default_log[PATH_MAX] = "";
    snprintf(default_log, sizeof(default_log), "%s/log", base_dir);

    const char *input = default_input;
    const char *logdir = default_log;
    if (argc > 1 && argv[1] && argv[1][0])
        input = argv[1];
    if (argc > 2 && argv[2] && argv[2][0])
        logdir = argv[2];

    if (ensure_dir(logdir) != 0) {
        fprintf(stderr, "cannot create log directory \"%s\"\n", logdir);
        return 1;
    }

    if (init_logger(logdir) != 0) {
        fprintf(stderr, "cannot init logger at \"%s\"\n", logdir);
        return 1;
    }

    FRONT_COMPL_T ctx = {};
    if (lexer_load_file(&ctx, input) != 0) {
        fprintf(stderr, "lexer failed on \"%s\"\n", input);
        destruct_logger();
        return 1;
    }

    dump_lexer_tokens(&ctx, input);

    lexer_reset(&ctx);
    destruct_logger();
    return 0;
}
