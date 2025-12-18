#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "base.h"
#include "io_utils.h"
#include "backend.h"

function void usage(const char *prog) {
    fprintf(stderr, "usage: %s <input.ast> [output.asm]\n", prog ? prog : "backend");
}

/**
 * @brief CLI: backend <input.ast> [output.asm].
 */
int main(int argc, char **argv) {
    const char *input = nullptr;
    const char *output = nullptr;

    if (argc < 2) {
        usage(argc ? argv[0] : "backend");
        return 1;
    }
    input = argv[1];
    if (!input || !*input) {
        usage(argc ? argv[0] : "backend");
        return 1;
    }
    if (argc > 2 && argv[2] && argv[2][0])
        output = argv[2];

    NODE_T *root = nullptr;
    varlist::VarList vars = {};
    if (load_ast_from_file(input, &root, &vars)) {
        fprintf(stderr, "failed to load AST from %s\n", input);
        return 1;
    }

    FILE *fp = fopen(output, "w");
    if (!fp) {
        fprintf(stderr, "cannot open %s for writing\n", output);
        destroy_ast(root, &vars);
        return 1;
    }

    int rc = emit_program(root, &vars, fp);
    fclose(fp);
    destroy_ast(root, &vars);

    return rc ? 1 : 0;
}
