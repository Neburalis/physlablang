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
#include "io_utils.h"
#include "base.h"

function void usage(const char *prog) {
    fprintf(stderr, "usage: %s <input.physlab> [output.ast]\n", prog ? prog : "frontend");
}

int main(int argc, char **argv) {
    const char *input = nullptr;
    const char *output = nullptr;

    if (argc < 2) {
        usage(argc ? argv[0] : "frontend");
        return 1;
    }
    input = argv[1];
    if (!input || !*input) {
        usage(argc ? argv[0] : "frontend");
        return 1;
    }
    if (argc > 2 && argv[2] && argv[2][0])
        output = argv[2];

    char exe_path[PATH_MAX] = ".";
    if (realpath(argv[0], exe_path) == nullptr)
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

    char logdir[PATH_MAX] = "";
    snprintf(logdir, sizeof(logdir), "%s/log", base_dir);

    if (create_folder_if_not_exists(logdir) != 0) {
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

    if (parse_tokens(&ctx) != 0) {
        fprintf(stderr, "parser failed on \"%s\"\n", input);
        lexer_reset(&ctx);
        destruct_logger();
        return 1;
    }

    full_dump(&ctx);
    simple_dump(&ctx);

    output = (output && output[0]) ? output : "out.ast";

    if (save_ast_to_file(&ctx, output)) {
        fprintf(stderr, "save failed");
        lexer_reset(&ctx);
        destruct_logger();
        return 1;
    }

    printf("MEOW\n");

    lexer_reset(&ctx);
    destruct_logger();
    return 0;
}
