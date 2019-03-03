// This file implements built-in functions for creating errors
// and handling them with try/catch
//
// Part of benzl - https://github.com/pokeb/benzl

#include <string.h>

#include "benzl-builtins.h"
#include "benzl-lval.h"
#include "benzl-lval-eval.h"
#include "benzl-lenv.h"
#include "benzl-error-macros.h"

lval* builtin_error(lenv *e, const lval *a) {
    LASSERT_NUM_ARGS("error", a, 1);
    LASSERT_ARG_TYPE("error", a, 0, LVAL_STR);
    lval *err = lval_err(child(a, 0)->val.vstr);
    return err;
}

lval* builtin_try(lenv *e, const lval *a) {

    LASSERT_NUM_ARGS("try", a, 2);
    LASSERT_ARG_TYPE("try", a, 0, LVAL_QEXPR);
    LASSERT_ARG_TYPE("try", a, 0, LVAL_QEXPR);

    lval *condition = child(a, 0);
    if (condition->type != LVAL_QEXPR) {
        return lval_err_for_val(a, "try expects an expression for the condition");
    }

    lval *failure_exp = child(a, 1);
    LASSERT_NUM_ARGS("try", failure_exp, 3);

    lval *catch_sym = child(failure_exp, 0);
    if (catch_sym->type != LVAL_SYM || strcmp(catch_sym->val.vsym.name, "catch") != 0) {
        return lval_err_for_val(a, "Function 'try' missing catch");
    }

    lval *catch_err_sym = child(failure_exp, 1);
    if (catch_err_sym->type != LVAL_SYM) {
        return lval_err_for_val(a, "function 'catch' missing error argument");
    }

    lval *catch_body = child(failure_exp, 2);
    if (catch_body->type != LVAL_QEXPR) {
        return lval_err_for_val(a, "catch missing function body argument");
    }

    lval *r = lval_eval_sexpr(e, condition);
    if (r->type == LVAL_ERR) {
        r->type = LVAL_CAUGHT_ERR;
        lval *args1 = lval_qexpr_with_size(1);
        lval_add(args1, catch_err_sym);
        lval *catch_block = lval_lambda(args1, catch_body);
        lval_release(args1);
        lval *args2 = lval_qexpr_with_size(1);
        lval_add(args2, r);
        lval_release(r);
        r = lval_call(e, catch_block, args2);
        lval_release(catch_block);
        lval_release(args2);
    }
    return r;
}
