// This file implements built-in functions for basic mathematical operations:
// Adding, subtracting, multiplying, dividing, modulo, min, max, ceil, floor and
// bitwise operations
//
// Part of benzl - https://github.com/pokeb/benzl

#include <math.h>

#include "benzl-builtins.h"
#include "benzl-lval.h"
#include "benzl-lval-eval.h"

// Type representing a mathematical operation
typedef enum {
    builtin_op_add = 0,
    builtin_op_subtract = 1,
    builtin_op_multiply = 2,
    builtin_op_divide = 3,
    builtin_op_modulo = 4,
    builtin_op_shift_right = 5,
    builtin_op_shift_left = 6,
    builtin_op_bitwise_and = 7,
    builtin_op_bitwise_or = 8,
    builtin_op_bitwise_xor = 9
} builtin_math_op;

// Human-readable name of a mathematical operation (used in errors)
static inline char* builtin_op_to_string(builtin_math_op op) {
    if (op >= builtin_op_add && op <= builtin_op_bitwise_or) {
        static char *names[10] = {
            "+", "-", "*", "/", "%%", ">>", "<<", "&", "|", "^"
        };
        return names[op];
    }
    return "<Unknown mathematical operation>";
}

// Internal function to make an s-expression with the passed params as children
static lval* op_error_lval(builtin_math_op op, const lval *arg1, const lval *arg2) {
    lval *d = lval_str(builtin_op_to_string(op));
    lval *r = lval_sexpr_with_size(3);
    lval_add(r, d);
    lval_add(r, arg1);
    lval_add(r, arg2);
    lval_release(d);
    return r;
}

// Internal function for performing basic mathematical operations
// on integer items in a list
// Used by builtin_op
lval *integer_op(lenv *e, const lval *a, builtin_math_op op) {

    // Handle negating a single value eg (- 4)
    lval *x = child(a, 0);
    if (op == builtin_op_subtract && count(a) == 1) {
        return lval_int(-x->val.vint);
    }

    x = lval_copy(x);

    for (size_t i=1; i<count(a); i++) {

        lval *y = child(a, i);

        if (op == builtin_op_add) {
            x->val.vint += y->val.vint;
        } else if (op == builtin_op_subtract) {
            x->val.vint -= y->val.vint;
        } else if (op == builtin_op_multiply) {
            x->val.vint *= y->val.vint;
        } else if (op == builtin_op_divide) {
            if (y->val.vint == 0) {
                lval *op_desc = op_error_lval(op, x, y);
                lval *err = lval_err_for_val(op_desc, "Division by zero!");
                lval_release(op_desc);
                lval_release(x);
                return err;
            }
            x->val.vint /= y->val.vint;
        } else if (op == builtin_op_modulo) {
            if (y->val.vint == 0) {
                lval *op_desc = op_error_lval(op, x, y);
                lval *err = lval_err_for_val(op_desc, "Modulo by zero");
                lval_release(op_desc);
                lval_release(x);
                return err;
            }
            x->val.vint %= y->val.vint;
        } else if (op == builtin_op_shift_right) {
            x->val.vint >>= y->val.vint;
        } else if (op == builtin_op_shift_left) {
            x->val.vint <<= y->val.vint;
        } else if (op == builtin_op_bitwise_and) {
            x->val.vint &= y->val.vint;
        } else if (op == builtin_op_bitwise_or) {
            x->val.vint |= y->val.vint;
        } else if (op == builtin_op_bitwise_xor) {
            x->val.vint ^= y->val.vint;
        }
    }
    return x;
}

// Internal function for performing basic mathematical operations
// on float items in a list
// Used by builtin_op
lval *float_op(lenv *e, const lval *a, builtin_math_op op) {

    // Handle negating a single value eg (- 4.1)
    lval *x = child(a, 0);
    if (op == builtin_op_subtract && count(a) == 1) {
        return lval_float(-x->val.vflt);
    }

    x = lval_copy(x);

    for (size_t i=1; i<count(a); i++) {

        lval *y = child(a, i);

        if (op == builtin_op_add) {
            x->val.vflt += y->val.vflt;
        } else if (op == builtin_op_subtract) {
            x->val.vflt -= y->val.vflt;
        } else if (op == builtin_op_multiply) {
            x->val.vflt *= y->val.vflt;
        } else if (op == builtin_op_divide) {
            if (y->val.vflt == 0) {
                lval *op_desc = op_error_lval(op, x, y);
                lval *err = lval_err_for_val(op_desc, "Division by zero");
                lval_release(op_desc);
                lval_release(x);
                return err;
            }
            x->val.vflt /= y->val.vflt;
        } else if (op == builtin_op_modulo) {
            if (y->val.vint == 0) {
                lval *op_desc = op_error_lval(op, x, y);
                lval *err = lval_err_for_val(op_desc, "Modulo by zero");
                lval_release(op_desc);
                lval_release(x);
                return err;
            }
            x->val.vflt = fmod(x->val.vflt, y->val.vflt);
        } else {
            lval *op_desc = op_error_lval(op, x, y);
            lval *err = lval_err_for_val(
                op_desc, "Unsupported operation: %s on Float",
                builtin_op_to_string(op)
            );
            lval_release(op_desc);
            lval_release(x);
            return err;
        }
    }
    return x;
}

// Internal function for performing basic mathematical operations
// on integer items in a list
// Used by builtin_op
lval *byte_op(lenv *e, const lval *a, builtin_math_op op) {

    if (count(a) < 2) {
        lval *r = lval_sexpr_with_size(1);
        lval *d = lval_str(builtin_op_to_string(op));
        lval_add(r, d);
        lval_release(r);
        return lval_err_for_val(r, "%s requires at least 2 arguments!",
                                builtin_op_to_string(op));
    }

    lval *x = lval_copy(child(a, 0));

    for (size_t i=1; i<count(a); i++) {

        lval *y = child(a, i);

        if (op == builtin_op_add) {
            x->val.vbyte += y->val.vbyte;
        } else if (op == builtin_op_subtract) {
            x->val.vbyte -= y->val.vbyte;
        } else if (op == builtin_op_multiply) {
            x->val.vbyte *= y->val.vbyte;
        } else if (op == builtin_op_divide) {
            if (y->val.vbyte == 0) {
                lval *op_desc = op_error_lval(op, x, y);
                lval *err = lval_err_for_val(op_desc, "Division by zero");
                lval_release(op_desc);
                lval_release(x);
                return err;
            }
            x->val.vbyte /= y->val.vbyte;
        } else if (op == builtin_op_modulo) {
            if (y->val.vbyte == 0) {
                lval *op_desc = op_error_lval(op, x, y);
                lval *err = lval_err_for_val(op_desc, "Modulo by zero");
                lval_release(op_desc);
                lval_release(x);
                return err;
            }
            x->val.vbyte %= y->val.vbyte;
        } else if (op == builtin_op_shift_right) {
            x->val.vbyte >>= y->val.vbyte;
        } else if (op == builtin_op_shift_left) {
            x->val.vbyte <<= y->val.vbyte;
        } else if (op == builtin_op_bitwise_and) {
            x->val.vbyte &= y->val.vbyte;
        } else if (op == builtin_op_bitwise_or) {
            x->val.vbyte |= y->val.vbyte;
        } else if (op == builtin_op_bitwise_xor) {
            x->val.vbyte ^= y->val.vbyte;
        }
    }
    return x;
}


// Internal function for performing basic mathematical operations
// on items in a list
// (+ 1 2 3) => 6
// (- 10 2 3) => 5
// (* 4 2.2) => 8.8
// (/ 10 2) => 5
// (/ 10 0) => <Error>
// (% 10 3) => 1
// (% 4 0) => <Error>
// (<< 2 1) => 4
// (>> 9 2) => 2
// (& 8 2) => 2
// (| 8 2) => 8
lval *builtin_op(lenv *e, const lval *a, builtin_math_op op) {

    // If we got a single list argument, use the contents as arguments
    if (count(a) == 1 && child(a, 0)->type == LVAL_QEXPR) {
        return builtin_op(e, child(a, 0), op);
    }

    lval_type type = LVAL_BYTE;
    for (size_t i = 0; i < count(a); i++) {
        lval *arg = child(a, i);
        if (arg->type == LVAL_FLT) {
            type = LVAL_FLT;
        } else if (arg->type == LVAL_INT) {
            if (type != LVAL_FLT) {
                type = LVAL_INT;
            }
        } else if (arg->type != LVAL_BYTE) {
            return lval_err_for_val(
                a, "Cannot do operation '%s' on '%s'",
                builtin_op_to_string(op),
                ltype_name(child(a, i)->type)
            );
        }
    }

    if (type == LVAL_BYTE) {
        return byte_op(e, a, op);

    } else if (type == LVAL_INT) {
        lval *exp = cast_list_to_type(a, LVAL_INT);
        lval *r = integer_op(e, exp, op);
        lval_release(exp);
        return r;
    }

    lval *exp = cast_list_to_type(a, LVAL_FLT);
    lval *r = float_op(e, exp, op);
    lval_release(exp);
    return r;
}

lval *builtin_add(lenv *e, const lval *a) {
    // If we're trying to add non numbers, we may be able to join them
    for (size_t i=0; i<count(a); i++) {
        lval *arg = child(a, i);
        if (!lval_is_number(arg)) {
            return builtin_join(e, a);
        }
    }
    return builtin_op(e, a, builtin_op_add);
}

lval *builtin_subtract(lenv *e, const lval *a) {
    return builtin_op(e, a, builtin_op_subtract);
}

lval *builtin_multiply(lenv *e, const lval *a) {
    return builtin_op(e, a, builtin_op_multiply);
}

lval *builtin_divide(lenv *e, const lval *a) {
    return builtin_op(e, a, builtin_op_divide);
}

lval *builtin_modulo(lenv *e, const lval *a) {
    return builtin_op(e, a, builtin_op_modulo);
}

lval *builtin_right_shift(lenv *e, const lval *a) {
    return builtin_op(e, a, builtin_op_shift_right);
}

lval *builtin_left_shift(lenv *e, const lval *a) {
    return builtin_op(e, a, builtin_op_shift_left);
}

lval *builtin_bitwise_and(lenv *e, const lval *a) {
    return builtin_op(e, a, builtin_op_bitwise_and);
}

lval *builtin_bitwise_or(lenv *e, const lval *a) {
    return builtin_op(e, a, builtin_op_bitwise_or);
}

lval *builtin_bitwise_xor(lenv *e, const lval *a) {
    return builtin_op(e, a, builtin_op_bitwise_xor);
}

lval *builtin_bitwise_not(lenv *e, const lval *a) {
    return builtin_op(e, a, builtin_op_bitwise_xor);
}



// (min 3 5 2) => 2
lval *builtin_min(lenv *e, const lval *a) {


    // If we got a list, use the values from that
    if (count(a) == 1 && child(a, 0)->type == LVAL_QEXPR) {
        return builtin_min(e, child(a, 0));
    }
    if (count(a) < 2) {
        return lval_err_for_val(a, "min requires least two numeric arguments");
    }
    lval *first = child(a, 0);
    for (size_t i=1; i<count(a); i++) {
        lval *next = child(a, i);
        lval *exp = lval_qexpr_with_size(2);
        lval_add(exp, first);
        lval_add(exp, next);
        lval *v = builtin_less_than_or_equal(e, exp);
        lval_release(exp);
        if (v->type == LVAL_ERR) {
            return v;
        } else if (!v->val.vint) {
            first = next;
        }
        lval_release(v);
    }
    return lval_copy(first);
}

// (max 3 5 2) => 5
lval *builtin_max(lenv *e, const lval *a) {

    // If we got a list, use the values from that
    if (count(a) == 1 && child(a, 0)->type == LVAL_QEXPR) {
        return builtin_max(e, child(a, 0));
    }
    if (count(a) < 2) {
        return lval_err("max requires at least two numeric arguments");
    }
    lval *first = child(a, 0);
    for (size_t i=1; i<count(a); i++) {
        lval *next = child(a, i);
        lval *exp = lval_qexpr_with_size(2);
        lval_add(exp, first);
        lval_add(exp, next);
        lval *v = builtin_greater_than_or_equal(e, exp);
        lval_release(exp);
        if (v->type == LVAL_ERR) {
            return v;
        } else if (!v->val.vint) {
            first = next;
        }
        lval_release(v);
    }
    return lval_copy(first);
}

lval *builtin_floor(lenv *e, const lval *a) {
    if (a->type == LVAL_SEXPR) {
        lval *v = lval_eval(e, a);
        lval *r = builtin_floor(e, v);
        lval_release(v);
        return r;
    } else if (a->type == LVAL_INT || a->type == LVAL_BYTE) {
        return lval_copy(a);
    } else if (a->type == LVAL_FLT) {
        return lval_int((long)floor(a->val.vflt));
    }
    return lval_err_for_val(a, "floor only works on numbers");
}

lval *builtin_ceil(lenv *e, const lval *a) {
    if (a->type == LVAL_SEXPR) {
        lval *v = lval_eval(e, a);
        lval *r = builtin_ceil(e, v);
        lval_release(v);
        return r;
    } else if (a->type == LVAL_INT || a->type == LVAL_BYTE) {
        return lval_copy(a);
    } else if (a->type == LVAL_FLT) {
        return lval_int((long)ceil(a->val.vflt));
    }
    return lval_err_for_val(a, "ceil only works on numbers");
}

