// This file implements miscellaneous built-in functions that don't fit elsewhere
//
// Part of benzl - https://github.com/pokeb/benzl

#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "benzl-builtins.h"
#include "benzl-lval.h"
#include "benzl-lval-eval.h"
#include "benzl-lenv.h"
#include "benzl-error-macros.h"
#include "benzl-constants.h"

lval* builtin_console_size(lenv *e, const lval *a) {
    struct winsize ws;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
    lval *r = lval_qexpr_with_size(2);
    lval_add(r, lval_int(ws.ws_col));
    lval_add(r, lval_int(ws.ws_row));
    return r;
}

lval* builtin_cpu_time_since(lenv *e, const lval *a) {
    double v;
    if (a->type == LVAL_INT) {
        v = a->val.vint;
    } else if (a->type == LVAL_FLT) {
        v = a->val.vflt;
    } else if (a->type == LVAL_SEXPR) {
        lval *rv = lval_eval_sexpr(e, a);
        lval *r = builtin_cpu_time_since(e, rv);
        lval_release(rv);
        return r;
    } else {
        return lval_err_for_val(
            a, "cpu-time-since expects a single numeric argument - got '%s'",
            ltype_name(a->type)
        );
    }
    struct timespec time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time);
    double now = (time.tv_sec*1000.0)+time.tv_nsec/1000000.0;
    return lval_float(now-v);
}

lval* builtin_exit(lenv *e, const lval *a) {
    if (count(a) > 0) {
        lval *code = child(a, 0);
        if (code->type == LVAL_INT) {
            exit((int)code->val.vint);
        } else {
            exit(1);
        }
    } else {
        exit(0);
    }
}

lval* builtin_version(lenv *e, const lval *a) {
    printf("--\nbenzl v%s\n%s\n%s\n--\n", version_number, credits, url);
    return lval_sexpr();
}

lval* builtin_print_env(lenv *e, const lval *a) {
    printf("Env:\n");
    lval_table_print(e->items);
    if (e->parent != NULL) {
        printf("Parent:\n");
        builtin_print_env(e->parent, NULL);
    }
    return lval_sexpr();
}
