// This file implements built-in functions for conditional logic
//
// Part of benzl - https://github.com/pokeb/benzl

#include <stdbool.h>
#include <stddef.h>

#include "benzl-builtins.h"
#include "benzl-lval.h"
#include "benzl-lval-eval.h"
#include "benzl-lenv.h"
#include "benzl-error-macros.h"

static bool lval_is_true(lval *a) {
    if (a->type == LVAL_INT) {
        if (a->val.vint != 0) {
            return true;
        }
    } else if (a->type == LVAL_FLT) {
        if (a->val.vflt != 0) {
            return true;
        }
    } else if (a->type == LVAL_BYTE) {
        if (a->val.vbyte != 0) {
            return true;
        }
    } else if (a->type == LVAL_QEXPR) {
        if (count(a) > 0) {
            return true;
        }
    } else {
        return true;
    }
    return false;
}

lval* builtin_if(lenv *e, const lval *a) {

    lval *v = child(a, 0);
    if (!lval_is_number(v) && v->type != LVAL_STR && a->type != LVAL_QEXPR) {
        return lval_err_for_val(
            a, "Function if expects a value for the condition"
        );
    }
    LASSERT_NUM_ARGS("if", a, 3);
    LASSERT_ARG_TYPE("if", a, 1, LVAL_QEXPR);
    LASSERT_ARG_TYPE("if", a, 2, LVAL_QEXPR);


    lval *x = NULL;
    if (lval_is_true(v)) {
        x = lval_eval_sexpr(e, child(a, 1));
    } else {
        x = lval_eval_sexpr(e, child(a, 2));
    }

    return x;
}

lval* builtin_logical_or(lenv *e, const lval *a) {
    bool r = 0;
    for (size_t i=0; i<count(a); i++) {
        if (lval_is_true(child(a, i))) {
            r = 1;
            break;
        }
    }
    return lval_int(r);
}

lval* builtin_logical_and(lenv *e, const lval *a) {
    bool r = 1;
    for (size_t i=0; i<count(a); i++) {
        if (!lval_is_true(child(a, i))) {
            r = 0;
            break;
        }
    }
    return lval_int(r);
}

lval* builtin_logical_not(lenv *e, const lval *a) {
    bool r = 1;
    for (size_t i=0; i<count(a); i++) {
        if (lval_is_true(child(a, i))) {
            r = 0;
            break;
        }
    }
    return lval_int(r);
}
