// This file implements built-in functions for working with lists
// These functions also work on strings and buffers
//
// Part of benzl - https://github.com/pokeb/benzl

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "benzl-builtins.h"
#include "benzl-lval.h"
#include "benzl-lval-eval.h"
#include "benzl-lenv.h"
#include "benzl-sprintf.h"
#include "benzl-error-macros.h"

lval* builtin_head(lenv *e, const lval *a)
{
    LASSERT_NUM_ARGS("head", a, 1);

    lval *v = child(a, 0);

    // If this is a list
    if (v->type == LVAL_QEXPR) {
        LASSERT_NOT_EMPTY("head", a, 0);
        lval *r = lval_qexpr_with_size(1);
        lval_add(r, child(v, 0));
        return r;

    // If this is a string
    } else if (v->type == LVAL_STR) {

        // Is the string empty already?
        if (strlen(v->val.vstr) == 0) {
            return lval_qexpr();
        }
        // Make a new string from just the first character
        char buf[2];
        buf[0] = v->val.vstr[0];
        buf[1] = 0x00;
        return lval_str(buf);

    // If this is a buffer
    } else if (v->type == LVAL_BUF) {

        // Is the buffer already 0 bytes long?
        if (v->val.vbuf.size == 0) {
            return lval_qexpr();
        }
        // Make a new buffer from just the first byte
        lval *r = lval_buf(1);
        r->val.vbuf.data[0] = v->val.vbuf.data[0];
        return r;
    }

    return lval_err_for_val(
        v, "head expects a single list, buffer or string argument (Got: %s)",
        ltype_name(v->type)
    );
}

lval *builtin_tail(lenv *e, const lval *a)
{
    LASSERT_NUM_ARGS("tail", a, 1);

    lval *v = child(a, 0);

    // If this is a list
    // Remove the first item
    if (v->type == LVAL_QEXPR) {
        LASSERT_NOT_EMPTY("tail", a, 0);
        lval *exp = lval_qexpr_with_size(count(v)-1);
        for (size_t i=1; i<count(v); i++) {
            lval_add(exp, child(v, i));
        }
        return exp;

    // If this is a string
    } else if (v->type == LVAL_STR) {

        // Is the string empty already?
        size_t len = strlen(v->val.vstr);
        if (len == 0) {
            return lval_qexpr();
        }

        // Make a new string with the first character removed
        char *buf = malloc(strlen(v->val.vstr));
        strcpy(buf, v->val.vstr+1);
        lval *r = lval_str(buf);
        free(buf);
        return r;

    // If this is a buffer
    } else if (v->type == LVAL_BUF) {

        // Is the buffer already 0 bytes long?
        if (v->val.vbuf.size == 0) {
            return lval_qexpr();
        }

        // Make a new buffer with the first byte removed
        lval *r = lval_buf(v->val.vbuf.size-1);
        memcpy(r->val.vbuf.data, v->val.vbuf.data+1, v->val.vbuf.size-1);
        return r;
    }

    lval *err = lval_err_for_val(
        v, "tail expects a single list, buffer or string argument (Got: %s)",
        ltype_name(v->type)
    );
    return err;
}

lval *builtin_drop(lenv *e, const lval *a)
{
    LASSERT_NUM_ARGS("drop", a, 2);
    LASSERT_ARG_TYPE("drop", a, 0, LVAL_INT);

    lval *v = child(a, 1);
    size_t num_to_drop = child(a, 0)->val.vint;

    // If this is a list
    // Remove the first item
    if (v->type == LVAL_QEXPR) {
        LASSERT_NOT_EMPTY("tail", a, 1);

        size_t len = count(v);

        if (num_to_drop > len) {
            return lval_err_for_val(
                v, "drop: out of range (List length is: %d, got: %d)",
                count(v), num_to_drop
            );
        }
        size_t num_to_keep = len-num_to_drop;

        lval *exp = lval_qexpr_with_size(num_to_keep);
        for (size_t i=num_to_drop; i<len; i++) {
            lval_add(exp, child(v, i));
        }
        return exp;

        // If this is a string
    } else if (v->type == LVAL_STR) {

        // Is the string empty already?
        size_t len = strlen(v->val.vstr);
        if (len == 0) {
            return lval_qexpr();
        }
        if (num_to_drop > len) {
            return lval_err_for_val(
                v, "drop: out of range (String length is: %d, got: %d)",
                len, num_to_drop
            );
        }
        size_t num_to_keep = len-num_to_drop;

        // Make a new string
        char *buf = malloc(num_to_keep+1);
        strcpy(buf, v->val.vstr+num_to_drop);
        lval *r = lval_str(buf);
        free(buf);
        return r;

        // If this is a buffer
    } else if (v->type == LVAL_BUF) {

        // Is the buffer already 0 bytes long?
        if (v->val.vbuf.size == 0) {
            return lval_qexpr();
        }

        if (num_to_drop > v->val.vbuf.size) {
            return lval_err_for_val(
                v, "drop: out of range (Buffer length is: %d, got: %d)",
                v->val.vbuf.size, num_to_drop
            );
        }

        size_t num_to_keep = v->val.vbuf.size-num_to_drop;

        // Make a new buffer with the first byte removed
        lval *r = lval_buf(num_to_keep);
        memcpy(r->val.vbuf.data, v->val.vbuf.data+num_to_drop, num_to_keep);
        return r;
    }

    return lval_err_for_val(
        v, "drop expects a single list, buffer or string argument (Got: %s)",
        ltype_name(v->type)
    );
}

lval *builtin_take(lenv *e, const lval *a)
{
    LASSERT_NUM_ARGS("take", a, 2);
    LASSERT_ARG_TYPE("take", a, 0, LVAL_INT);

    lval *v = child(a, 1);
    size_t num_to_take = child(a, 0)->val.vint;

    // If this is a list
    // Remove the first item
    if (v->type == LVAL_QEXPR) {
        LASSERT_NOT_EMPTY("tail", a, 1);

        size_t len = count(v);

        if (num_to_take > len) {
            return lval_err_for_val(
                v, "take: out of range (List length is: %d, got: %d)",
                count(v), num_to_take
            );
        }

        lval *exp = lval_qexpr_with_size(num_to_take);
        for (size_t i=0; i<num_to_take; i++) {
            lval_add(exp, child(v, i));
        }
        return exp;

        // If this is a string
    } else if (v->type == LVAL_STR) {

        // Is the string empty already?
        size_t len = strlen(v->val.vstr);
        if (len == 0) {
            return lval_qexpr();
        }
        if (num_to_take > len) {
            return lval_err_for_val(
                v, "take: out of range (String length is: %d, got: %d)",
                len, num_to_take
            );
        }

        // Make a new string
        char *buf = malloc(num_to_take+1);
        memcpy(buf, v->val.vstr, num_to_take);
        buf[num_to_take] = 0x00;
        lval *r = lval_str(buf);
        free(buf);
        return r;

        // If this is a buffer
    } else if (v->type == LVAL_BUF) {

        // Is the buffer already 0 bytes long?
        if (v->val.vbuf.size == 0) {
            return lval_qexpr();
        }

        if (num_to_take > v->val.vbuf.size) {
            return lval_err_for_val(
                v, "take: out of range (Buffer length is: %d, got: %d)",
                v->val.vbuf.size, num_to_take
            );
        }

        // Make a new buffer with the first byte removed
        lval *r = lval_buf(num_to_take);
        memcpy(r->val.vbuf.data, v->val.vbuf.data, num_to_take);
        return r;
    }

    return lval_err_for_val(
        v, "take expects a single list, buffer or string argument (Got: %s)",
        ltype_name(v->type)
    );
}

lval* get_element(lenv *e, char *func, const lval *a, long num)
{
    LASSERT_NUM_ARGS(func, a, 1);
    lval *v = child(a, 0);
    size_t len = 0;
    if (v->type == LVAL_QEXPR) {
        len = count(v);
    } else if (v->type == LVAL_STR) {
        len = strlen(v->val.vstr);
    } else if (v->type == LVAL_BUF) {
        len = v->val.vbuf.size;
    } else {
        return lval_err_for_val(
            v, "%s expects a list, buffer or string argument (Got: %s)",
            func, ltype_name(v->type)
        );
    }
    long index = num;
    if (index < 0) {
        index = len+index;
    }
    if (index >= len) {
        return lval_err_for_val(
            v, "%s: out of range (%s length is: %d)",
            func, ltype_name(v->type), len
        );
    }

    // If this is a list
    if (v->type == LVAL_QEXPR) {
        return lval_eval(e, child(v, index));

    // If this is a string
    } else if (v->type == LVAL_STR) {
        char buf[2];
        buf[0] = v->val.vstr[index];
        buf[1] = 0x00;
        return lval_str(buf);

    // If this is a buffer
    } else if (v->type == LVAL_BUF) {
        return lval_byte(v->val.vbuf.data[index]);
    }
    assert(false); // Shouldn't arrive here
    return NULL;
}

lval* builtin_last(lenv *e, const lval *a)
{
    return get_element(e, "last", a, -1);
}

lval* builtin_first(lenv *e, const lval *a)
{
    return get_element(e, "first", a, 0);
}

lval* builtin_second(lenv *e, const lval *a)
{
    return get_element(e, "second", a, 1);
}

lval* builtin_nth(lenv *e, const lval *a)
{
    lval *num = cast_to(child(a, 0), LVAL_INT);
    if (num == NULL) {
        return lval_err_for_val(
            a, "nth expects an number for the first argument (Got: %s)",
            ltype_name(child(a, 0)->type)
        );
    }
    lval *exp = lval_sexpr_with_size(1);
    lval_add(exp, child(a, 1));
    lval *r = get_element(e, "nth", exp, num->val.vint);
    lval_release(exp);
    lval_release(num);
    return r;
}

lval* builtin_list(lenv *e, const lval *a) {
    lval *r = lval_copy(a);
    r->type = LVAL_QEXPR;
    return r;
}

static inline lval *lval_join(lenv *e, lval *x, lval *y) {
    for (size_t i=0; i<count(y); i++) {
        x = lval_add(x, child(y, i));
    }
    return x;
}

lval* builtin_join(lenv *e, const lval *a)
{
    // The type we intend to use for the joined value
    lval_type type = LVAL_STR;

    // Check to see if we actually want to use a list or buffer instead
    for (size_t i = 0; i < count(a); i++) {
        lval *v = child(a, i);
        if (v->type == LVAL_BUF || v->type == LVAL_BYTE) {
            type = LVAL_BUF;
            break;
        } else if (v->type == LVAL_QEXPR || v->type == LVAL_SEXPR) {
            type = LVAL_QEXPR;
            break;
        }
    }
    lval *x = NULL;

    // If we are making a list
    if (type == LVAL_QEXPR) {
        x = lval_qexpr();
        for (size_t i=0; i<count(a); i++) {
            lval *y = child(a, i);
            // If the next item is another list, join them
            if (y->type == LVAL_QEXPR) {
                x = lval_join(e, x, y);
            // Otherwise, add the object as a child of the first list
            } else {
                lval_add(x, y);
            }
        }
    // If we are making a string
    } else if (type == LVAL_STR) {

        // Make a buffer for the new string
        size_t len = 32;
        size_t idx=0;
        char *buf = malloc(len);
        // sprintf all the items into the buffer
        for (size_t i=0; i<count(a); i++) {
            lval *v = child(a, i);
            if (v->type != LVAL_SEXPR || count(v) > 0) {
                lval_sprint(v, &buf, &idx, &len, false);
            }
        }
        buf[idx] = 0x00;
        x = lval_str(buf);
        free(buf);

    // If we are making a buffer
    } else if (type == LVAL_BUF) {

        // Make a new buffer
        x = lval_buf(0);

        // Convert all the items to buffers
        for (size_t i=0; i<count(a); i++) {
            lval *v = child(a, i);

            // If this item is a list
            if (v->type == LVAL_QEXPR) {

                lval *exp = lval_qexpr_with_size(count(v)+1);
                lval_add(exp, x);
                for (size_t i2=0; i2<count(v); i2++) {
                    lval_add(exp, child(v, i2));
                }
                lval *r = builtin_join(e, exp);
                lval_release(x);
                lval_release(exp);
                x = r;
                continue;
            }
            lval *b = cast_to(v, LVAL_BUF);
            if (b == NULL) {
                return lval_err_for_val(a, "Cannot perform join on type %s",
                                      ltype_name(v->type));
            }
            size_t new_len = x->val.vbuf.size+b->val.vbuf.size;
            x->val.vbuf.data = realloc(x->val.vbuf.data, new_len);
            memcpy(x->val.vbuf.data+x->val.vbuf.size,
                   b->val.vbuf.data, b->val.vbuf.size);
            x->val.vbuf.size = new_len;
            lval_release(b);
        }
    }
    return x;
}

lval* builtin_len(lenv *e, const lval *a)
{
    size_t r = 0;
    switch (a->type) {
        // if S-Expression: evaluate first
        case LVAL_SEXPR:
            assert(count(a) > 0);
            assert(child(a, 0) != a);
            lval *v = lval_eval(e, a);
            lval *l = builtin_len(e, v);
            lval_release(v);
            return l;
        // List: count the children
        case LVAL_QEXPR:
            r = count(a);
            break;
        // String: count the bytes
        case LVAL_STR:
            r = strlen(a->val.vstr);
            break;
        // Buffer: return the size
        case LVAL_BUF:
            r = a->val.vbuf.size;
            break;
        default:
            return lval_err_for_val(
                a, "len works on strings, lists and buffers (got %s)",
                ltype_name(a->type)
            );
    }
    return lval_int((long)r);
}


