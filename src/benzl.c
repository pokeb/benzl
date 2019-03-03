// This file includes the benzl's main entry point
//
// Part of benzl - https://github.com/pokeb/benzl

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <editline/readline.h>

#include "benzl-lval-pool.h"
#include "benzl-lval-eval.h"
#include "benzl-lenv.h"
#include "benzl-builtins.h"
#include "benzl-stdlib.h"
#include "benzl-parse.h"
#include "benzl-constants.h"
#include "benzl-call-count-debug.h"
#include "benzl-stacktrace.h"

// Returns the contents of the standard library as a null-terminated string
char* benzl_standard_library(void)
{
    char *stl = malloc(src_stdlib_benzl_len+1);
    memcpy(stl,src_stdlib_benzl,src_stdlib_benzl_len);
    stl[src_stdlib_benzl_len] = 0x00;
    return stl;
}

int main(int argc, char ** argv)
{
    // Create the top level enviroment (stores bound variables and functions)
    // 416 buckets provides enough space for the stdlib and tests to run without
    // the hash table resizing itself or storing more than 2 values per hash
    lenv *e = lenv_alloc(416);

    // Load built-in functions into the top level enviroment
    lenv_add_builtins(e);

    // Load standard library
    // stdlib.benzl is converted to a header file (benzl-stdlib.h) during make
    // so that its contents can be built directly into the benzl binary
    char *stlib = benzl_standard_library();
    lval *stdlib_label = lval_str("benzl-standard-library");
    lval *r = builtin_load_str(e, stlib, stdlib_label);
    lval_release(stdlib_label);
    free(stlib);
    if (r->type == LVAL_ERR) {
        printf("Error in standard library:\n");
        print_error_with_trace(r);
        lval_release(r);
        goto end;
    }
    lval_release(r);


    // If we got arguments, we'll assume we don't want to run the REPL
    if (argc >= 2) {

        // benzl will load the first argument as a .benzl program
        // If we got more arguments than that, add them to a list
        lval *launch_args = lval_qexpr_with_size(argc-2);
        for (int i=2; i<argc; i++) {
            lval *arg = lval_str(argv[i]);
            lval_add(launch_args, arg);
            lval_release(arg);
        }
        // Now set the launch-args variable in the root environment
        // to make them available to benzl programs
        lval *name = lval_sym("launch-args");
        lenv_def(e, name, launch_args);
        lval_release(launch_args);
        lval_release(name);

        // If we got a .benzl source file as the first argument, load and evaluate it
        lval *args = lval_sexpr_with_size(1);
        lval *file = lval_str(argv[1]);
        lval *r = builtin_load(e, lval_add(args, file));
        if (r->type == LVAL_ERR) {
            print_error_with_trace(r);
        }
        lval_release(args);
        lval_release(file);
        lval_release(r);
        goto end;
    }

    // If we got no arguments, start the REPL
    printf("--\nbenzl v%s\nType 'help' for examples of things to try, "
           "or 'quit' to exit\n--\n", version_number);

    while (1) {
        char *input = readline("benzl> ");
        add_history(input);

        size_t pos = 0;
        lval *expr = lval_read_expr(input, &pos, 0x00, NULL);
        lval *r = lval_eval(e, expr);
        if (r->type == LVAL_ERR) {
            print_error_with_trace(r);
        } else {
            lval_println(r);
        }
        lval_release(expr);
        lval_release(r);
        free(input);
    }

end:
    // Clean up the environment
    lenv_free(e);

    // Clean up the stack
    stack_cleanup();

    // Print counts for functions called
    print_call_count_stats();

    // Print stats about how the pool allocator was used
    pool_print_stats(global_pool());

    // Clean up the lval pool allocator
    pool_free(global_pool());

    // Print stats about how all hash tables were used
    print_lval_table_stats();

    return 0;
}

