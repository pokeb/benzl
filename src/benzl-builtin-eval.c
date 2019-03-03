// This file implements built-in functions for evaluating expressions
// including directly evaluating an S-Expression, as well as evalauting
// expressions in a string, and loading them from an external file
//
// Part of benzl - https://github.com/pokeb/benzl

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "benzl-builtins.h"
#include "benzl-lval.h"
#include "benzl-lval-eval.h"
#include "benzl-lenv.h"
#include "benzl-parse.h"
#include "benzl-error-macros.h"

lval* builtin_eval(lenv *e, const lval *a) {

    LASSERT_NUM_ARGS("eval", a, 1);

    lval *x = child(a, 0);

    if (x->type == LVAL_QEXPR) {
        return lval_eval_sexpr(e, x);
    }
    return lval_eval(e, x);
}

lval* builtin_eval_string(lenv *e, const lval *a) {

    LASSERT_NUM_ARGS("eval", a, 1);
    LASSERT_ARG_TYPE("eval", a, 0, LVAL_STR);

    lval *str = child(a, 0);
    size_t pos = 0;
    lval *expr = lval_read_expr(str->val.vstr, &pos, '\0',
                                a->source_position.source_file);

    if (count(expr) == 0) {
        lval_release(expr);
        return lval_err("Invalid expression: '%'",str->val.vstr);
    }

    lval *result = NULL;
    for (size_t i=0; i<count(expr); i++) {
        if (result != NULL) {
            lval_release(result);
        }
        result = lval_eval(e, child(expr, i));
        if (result->type == LVAL_ERR) {
            break;
        }
    }
    lval_release(expr);
    return result;
}

lval* builtin_load_str(lenv *e, char *input, lval *source_file) {
    size_t pos = 0;
    lval *expr = lval_read_expr(input, &pos, '\0', source_file);
    if (expr->type == LVAL_ERR) {
        lval_println(expr);
    } else {
        for (size_t i=0; i<count(expr); i++) {
            lval *x = lval_eval(e, child(expr, i));
            if (x->type == LVAL_ERR) {
                lval_release(expr);
                return x;
            }
            lval_release(x);
        }
    }
    lval_release(expr);
    return lval_sexpr();
}

lval* path_for_file(char *file, char *script_path)
{
    char *path = malloc(MAX(strlen(file)+1, 4096)*sizeof(char));
    if (file[0] == '/' || file[0] == '~') {
        strcpy(path, file);
    } else {
        if (getcwd(path, 4096) == NULL) {
            free(path);
            return lval_err("Could not load '%s': "
                            "Failed to determine the current path!", file);
        }
        size_t len = strlen(path);
        path = realloc(path, (len+strlen(file)+8)*sizeof(char));
        path[len] = '/';
        path[len+1] = 0x00;
        if (strcat(path, file) == NULL) {
            free(path);
            return lval_err("Could not load library '%s': "
                            "Failed to compute the path!", file);
        }
    }
    size_t len = strlen(path);
    if (strlen(path) > 6) {
        char *end = path+(len-6);
        if (strcmp(end, ".benzl") == 0) {
            goto end;
        }
    }
    strcpy(path+len, ".benzl");
    path[len+6] = 0x00;

    end:

    // If file does not exist
    if (access(path, F_OK ) == -1) {
        // Also try the same directory as the current script
        if (script_path != NULL) {
            char *fname = basename(path);
            char *p = malloc(strlen(script_path)+strlen(fname)+2);
            sprintf(p, "%s/%s", script_path, fname);
            free(path);
            path = p;
        }
    }

    lval *r = lval_str(path);
    free(path);
    return r;
}

lval* builtin_load(lenv *e, const lval *a) {
    LASSERT_NUM_ARGS("load", a, 1);
    LASSERT_ARG_TYPE("load", a, 0, LVAL_STR);

    lval *path = path_for_file(child(a, 0)->val.vstr, e->script_path);

    if (path->type == LVAL_ERR) {
        return path;

    // Don't load the script if we've loaded it already
    } else if (is_module_already_loaded(e, path->val.vstr)) {
        lval_release(path);
        return lval_sexpr();
    }

    FILE *file = fopen(path->val.vstr, "rb");
    if (file == NULL){
        lval *err = lval_err("Could not load library '%s'", path->val.vstr);
        return err;
    }

    // Store the directory that contains this script in the environment
    // this will help when scripts use require to load modules in the same directory
    e->script_path = dirname(path->val.vstr);

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *input = malloc(length+1);
    size_t unused __attribute__((unused)) = fread(input, 1, length, file);
    fclose(file);
    input[length] = 0x00;

    record_module_loaded(e, path->val.vstr);

    lval *r = builtin_load_str(e, input, path);
    lval_release(path);
    free(input);
    return r;
}

