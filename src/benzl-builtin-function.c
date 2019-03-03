// This file implements built-in functions for defining functions and lambdas
//
// Part of benzl - https://github.com/pokeb/benzl

#include "benzl-builtins.h"
#include "benzl-lval.h"
#include "benzl-lenv.h"
#include "benzl-error-macros.h"

lval *builtin_fun(lenv *e, const lval *a) {

    if (a->type != LVAL_SEXPR || count(a) != 2) {
        return lval_err_for_val(a, "Functions must be defined in the form "
                                   "(fun {name arg1 arg2} {body}) or "
                                   "(fun {name arg1:type arg2:type} {body})");
    }

    lval *args = child(a, 0);
    lval *fbody = child(a, 1);

    if (args->type != LVAL_QEXPR || count(args) < 1) {
        return lval_err_for_val(a, "Bad function name or arguments: "
                                   "Functions must be defined in the form "
                                   "(fun {name arg1 arg2} {body}) or "
                                   "(fun {name arg1:type arg2:type} {body})");
    } else if (fbody->type != LVAL_QEXPR) {
        return lval_err_for_val(a, "Bad function body: "
                                   "Functions must be defined in the form "
                                   "(fun {name arg1 arg2} {body}) or "
                                   "(fun {name arg1:type arg2:type} {body})");
    }

    // Type check args
    for (size_t i=1; i<count(args); i++) {
        lval *arg = child(args, i);
        if (arg->type == LVAL_KEY_VALUE_PAIR) {
            if (arg->val.vkvpair.value->type == LVAL_SYM) {
                lval *type = lenv_get(e, arg->val.vkvpair.value);
                if (type->type == LVAL_ERR) {
                    lval_release(type);
                    return lval_err_for_val(
                        a, "Invalid type '%s' for function parameter '%s'",
                        arg->val.vkvpair.value->val.vsym.name,
                        arg->val.vkvpair.key->val.vsym.name
                    );
                }
                lval_release(type);
            }
        }
    }

    args = child(a, 0);
    fbody = child(a, 1);

    lval *fname = child(args, 0);

    lval *fargs = lval_qexpr_with_size(count(args)-1);
    for (size_t i=1; i<count(args); i++) {
        lval_add(fargs, child(args, i));
    }
    lval *fun = lval_lambda(fargs, fbody);
    lval_release(fargs);

    lval *err = lenv_def(e, fname, fun);
    lval_release(fun);
    if (err != NULL) {
        return err;
    }
    return lval_sexpr();
}

lval *builtin_lambda(lenv *e, const lval *a) {
    if (a->type != LVAL_SEXPR || count(a) != 2) {
        return lval_err_for_val(a, "Lambdas must be defined in the form "
                                   "(\\ {name arg1 arg2} {body}) or "
                                   "(\\ {name arg1:type arg2:type} {body})");
    }
    lval *args = child(a, 0);
    for (size_t i=0; i<count(args); i++) {
        lval *arg = child(args, i);
        if (arg->type != LVAL_SYM && arg->type != LVAL_KEY_VALUE_PAIR) {
            return lval_err_for_val(a, "Bad function arguments: "
                                       "Lambdas must be defined in the form "
                                       "(\\ {name arg1 arg2} {body}) or "
                                       "(\\ {name arg1:type arg2:type} {body})");
        }
    }
    lval *fbody = child(a, 1);
    if (fbody->type != LVAL_QEXPR || count(fbody) < 1) {
        return lval_err_for_val(a, "Bad function body: "
                                   "Lambdas must be defined in the form "
                                   "(\\ {name arg1 arg2} {body}) or "
                                   "(\\ {name arg1:type arg2:type} {body})");
    }

    for (size_t i=0; i<count(args); i++) {
        lval *arg = child(args, i);
        if (arg->type == LVAL_KEY_VALUE_PAIR) {
            if (arg->val.vkvpair.value->type == LVAL_SYM) {
                lval *type = lenv_get(e, arg->val.vkvpair.value);
                if (type->type == LVAL_ERR) {
                    lval_release(type);
                    return lval_err_for_val(
                        a, "Invalid type '%s' for lambda parameter '%s'",
                        arg->val.vkvpair.value->val.vsym.name,
                        arg->val.vkvpair.key->val.vsym.name
                    );
                }
                lval_release(type);
            }
        }
    }

    args = child(a, 0);
    fbody = child(a, 1);
    return lval_lambda(args, fbody);
}
