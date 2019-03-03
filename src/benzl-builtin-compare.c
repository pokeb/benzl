// This file implements built-in functions for comparing two lvals
//
// Part of benzl - https://github.com/pokeb/benzl

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "benzl-builtins.h"
#include "benzl-lval-pool.h"
#include "benzl-lenv.h"
#include "benzl-error-macros.h"
#include "benzl-sprintf.h"

// Type representing a comparison operation
typedef enum {
    builtin_ord_op_less_than = 0,
    builtin_ord_op_greater_than = 1,
    builtin_ord_op_less_than_or_equal = 2,
    builtin_ord_op_greater_than_or_equal = 3
} builtin_ord_op;

// Human-readable name of a compare operation (used in errors)
static inline char* builtin_ord_op_to_string(builtin_ord_op op) {
    if (op >= builtin_ord_op_less_than &&
        op <= builtin_ord_op_greater_than_or_equal) {
        static char *names[4] = {"<", ">", "<=", ">="};
        return names[op];
    }
    return "<Unknown order operation>";
}

lval* builtin_integer_ord(lenv *e, const lval *a, builtin_ord_op op)
{
    lval *v1 = child(a, 0);
    lval *v2 = child(a, 1);

    bool r;
    if (op == builtin_ord_op_less_than) {
        r = (v1->val.vint < v2->val.vint);
    } else if (op == builtin_ord_op_greater_than) {
        r = (v1->val.vint > v2->val.vint);
    } else if (op == builtin_ord_op_less_than_or_equal) {
        r = (v1->val.vint <= v2->val.vint);
    } else if (op == builtin_ord_op_greater_than_or_equal) {
        r = (v1->val.vint >= v2->val.vint);
    } else {
        return lval_err_for_val(a, "Unhandled operator for integer: '%s'",
                                builtin_ord_op_to_string(op));
    }
    return lval_int(r);
}

lval* builtin_float_ord(lenv *e, const lval *a, builtin_ord_op op)
{
    lval *v1 = child(a, 0);
    lval *v2 = child(a, 1);

    bool r;
    if (op == builtin_ord_op_less_than) {
        r = (v1->val.vflt < v2->val.vflt);
    } else if (op == builtin_ord_op_greater_than) {
        r = (v1->val.vflt > v2->val.vflt);
    } else if (op == builtin_ord_op_less_than_or_equal) {
        r = (v1->val.vflt <= v2->val.vflt);
    } else if (op == builtin_ord_op_greater_than_or_equal) {
        r = (v1->val.vflt >= v2->val.vflt);
    } else {
        return lval_err_for_val(a, "Unhandled operator for float: '%s'",
                                   builtin_ord_op_to_string(op));
    }
    return lval_int(r);
}

lval* builtin_byte_ord(lenv *e, const lval *a, builtin_ord_op op)
{
    lval *v1 = child(a, 0);
    lval *v2 = child(a, 1);

    bool r;
    if (op == builtin_ord_op_less_than) {
        r = (v1->val.vbyte < v2->val.vbyte);
    } else if (op == builtin_ord_op_greater_than) {
        r = (v1->val.vbyte > v2->val.vbyte);
    } else if (op == builtin_ord_op_less_than_or_equal) {
        r = (v1->val.vbyte <= v2->val.vbyte);
    } else if (op == builtin_ord_op_greater_than_or_equal) {
        r = (v1->val.vbyte >= v2->val.vbyte);
    } else {
        return lval_err_for_val(a, "Unhandled operator for byte: '%s'",
                                   builtin_ord_op_to_string(op));
    }
    return lval_int(r);
}

lval* builtin_string_ord(lenv *e, lval *a, builtin_ord_op op)
{
    lval *v1 = child(a, 0);
    lval *v2 = child(a, 1);

    int cmp = strcmp(v1->val.vstr, v2->val.vstr);
    bool r;
    if (op == builtin_ord_op_less_than) {
        r = cmp < 0;
    } else if (op == builtin_ord_op_greater_than) {
        r = cmp > 0;
    } else if (op == builtin_ord_op_less_than_or_equal) {
        r = (cmp <= 0);
    } else if (op == builtin_ord_op_greater_than_or_equal) {
        r = (cmp >= 0);
    } else {
        return lval_err_for_val(a, "Unhandled operator for string: '%s'",
                                   builtin_ord_op_to_string(op));
    }
    return lval_int(r);
}

lval* builtin_ord(lenv *e, const lval *a, builtin_ord_op op)
{
    // If we got a single list argument, use its values as arguments
    if (count(a) == 1 && child(a, 0)->type == LVAL_QEXPR) {
        return builtin_ord(e, child(a, 0), op);
    }

    char *op_name = builtin_ord_op_to_string(op);

    LASSERT_NUM_ARGS(op_name, a, 2);

    lval_type type = LVAL_BYTE;
    lval *v1 = child(a, 0);
    lval *v2 = child(a, 1);

    if (v1->type == LVAL_STR) {
        type = LVAL_STR;
    } else if (v1->type == LVAL_FLT) {
        type = LVAL_FLT;
    } else if (v1->type == LVAL_INT) {
        type = LVAL_INT;
    } else if (v1->type != LVAL_BYTE) {
        return lval_err_for_val(
            a, "Unexpected type for arg 0 of '%s' comparison (Got: '%s')",
            op_name, ltype_name(v1->type)
        );
    }
    if (v2->type == LVAL_STR) {
        type = LVAL_STR;
    } else if (v2->type == LVAL_FLT) {
        if (type != LVAL_STR) {
            type = LVAL_FLT;
        }
    } else if (v2->type == LVAL_INT) {
        if (type != LVAL_STR && type != LVAL_FLT) {
            type = LVAL_INT;
        }
    } else if (v2->type != LVAL_BYTE) {
        return lval_err_for_val(
            a, "Unexpected type for arg 1 of '%s' comparison (Got: '%s')",
            op_name, ltype_name(v2->type)
        );
    }
    if (type == LVAL_BYTE) {
        return builtin_byte_ord(e, a, op);
    } else if (type == LVAL_INT) {
        lval *exp = cast_list_to_type(a, LVAL_INT);
        lval *r = builtin_integer_ord(e, exp, op);
        lval_release(exp);
        return r;
    } else if (type == LVAL_FLT) {
        lval *exp = cast_list_to_type(a, LVAL_FLT);
        lval *r = builtin_float_ord(e, exp, op);
        lval_release(exp);
        return r;
    }

    lval *exp = cast_list_to_type(a, LVAL_STR);
    lval *r = builtin_string_ord(e, exp, op);
    lval_release(exp);
    return r;
}

lval* builtin_less_than(lenv *e, const lval *a) {
    return builtin_ord(e, a, builtin_ord_op_less_than);
}

lval* builtin_greater_than(lenv *e, const lval *a) {
    return builtin_ord(e, a, builtin_ord_op_greater_than);
}

lval* builtin_less_than_or_equal(lenv *e, const lval *a) {
    return builtin_ord(e, a, builtin_ord_op_less_than_or_equal);
}

lval* builtin_greater_than_or_equal(lenv *e, const lval *a) {
    return builtin_ord(e, a, builtin_ord_op_greater_than_or_equal);
}

lval* builtin_equal(lenv *e, const lval *a) {
    LASSERT_NUM_ARGS("==", a, 2);
    bool r = lval_eq(child(a, 0), child(a, 1));
    return lval_int(r);
}

lval* builtin_not_equal(lenv *e, const lval *a) {
    LASSERT_NUM_ARGS("!=", a, 2);
    bool r = !lval_eq(child(a, 0), child(a, 1));
    return lval_int(r);
}
