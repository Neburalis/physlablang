#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "rev-front.h"
#include "base.h"

function void usage(const char *prog) {
    fprintf(stderr, "usage: %s <input.ast> [output.physlab]\n", prog ? prog : "reversed-frontend");
}

int main(int argc, char **argv) {
    const char *input = nullptr;
    const char *dima_v_oute = nullptr;

    if (argc < 2) {
        usage(argv ? argv[0] : "reversed-frontend");
        return 1;
    }
    input = argv[1];
    if (!input || !*input) {
        usage(argv ? argv[0] : "reversed-frontend");
        return 1;
    }
    if (argc > 2 && argv[2] && argv[2][0])
        dima_v_oute = argv[2];

    NODE_T *root = nullptr;
    varlist::VarList vars = {};
    if (load_ast_from_file(input, &root, &vars) != 0) {
        fprintf(stderr, "cannot load AST from %s\n", input);
        destroy_ast(root, &vars);
        return 1;
    }

    FILE *out = stdout;
    if (dima_v_oute) {
        out = fopen(dima_v_oute, "w");
        if (!out) {
            fprintf(stderr, "cannot open %s for write\n", dima_v_oute);
            destroy_ast(root, &vars);
            return 1;
        }
    }

    int rc = reverse_program(root, &vars, out);
    if (out && out != stdout)
        fclose(out);

    destroy_ast(root, &vars);
    if (rc != 0) {
        fprintf(stderr, "emission failed\n");
        return 1;
    }
    return 0;
}
