// Part of benzl - https://github.com/pokeb/benzl

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "benzl-lval.h"
#include "benzl-lenv.h"
#include "benzl-sprintf.h"
#include "benzl-lval-pool.h"
#include "benzl-builtins.h"
#include "benzl-hash-table.h"
#include "benzl-stacktrace.h"

#pragma mark - Constructors

lval* lval_int(long x) {
    lval *v = lval_alloc();
    v->type = LVAL_INT;
    v->val.vint = x;
    return v;
}

lval* lval_float(double x) {
    lval *v = lval_alloc();
    v->type = LVAL_FLT;
    v->val.vflt = x;
    return v;
}

lval* lval_byte(uint8_t x) {
    lval *v = lval_alloc();
    v->type = LVAL_BYTE;
    v->val.vbyte = x;
    return v;
}

lval* lval_err(char *fmt, ...) {
    lval *v = lval_alloc();
    v->type = LVAL_ERR;

    va_list va;
    va_start(va, fmt);

    v->val.verr.message = malloc(256);
    vsnprintf(v->val.verr.message, 255, fmt, va);
    v->val.verr.message = realloc(v->val.verr.message, strlen(v->val.verr.message)+1);
    v->val.verr.stack_trace = NULL;
    va_end(va);
    return v;
}

lval* lval_err_for_val(const lval *v, char *fmt, ...) {

    va_list va;
    va_start(va, fmt);

    char *msg = malloc(512);
    vsnprintf(msg, 511, fmt, va);
    msg = realloc(msg, strlen(msg)+1);

    va_end(va);

    char *tmp = malloc(strlen(msg)+1+32);
    sprintf(tmp, "%s at line %d:%d", msg, v->source_position.row+1, v->source_position.col);
    lval *e = lval_err(tmp);
    e->val.verr.stack_trace = stack_trace(v);
    free(msg);
    free(tmp);
    return e;
}

lval* lval_sym(char *s) {
    lval *v = lval_alloc();
    v->type = LVAL_SYM;
    v->val.vsym.name = strdup(s);
    v->val.vsym.hash = lval_table_hash(s);
    return v;
}

lval* lval_str(char *s) {
    lval *v = lval_alloc();
    v->type = LVAL_STR;
    v->val.vstr = strdup(s);
    return v;
}

lval* lval_buf(size_t size) {
    lval *v = lval_alloc();
    v->type = LVAL_BUF;
    v->val.vbuf.size = size;
    v->val.vbuf.data = calloc(size, sizeof(uint8_t));
    return v;
}

lval* lval_dict(size_t bucket_count) {
    lval *v = lval_alloc();
    v->type = LVAL_DICT;
    v->val.vdict = lval_table_alloc(bucket_count);
    return v;
}

lval* lval_sexpr(void) {
    lval *v = lval_alloc();
    v->type = LVAL_SEXPR;
    v->val.vexp.count = 0;
    v->val.vexp.allocated_size = 0;
    v->val.vexp.cell = NULL;
    return v;
}

lval* lval_sexpr_with_size(size_t size) {
    lval *v = lval_alloc();
    v->type = LVAL_SEXPR;
    v->val.vexp.count = 0;
    v->val.vexp.allocated_size = size;
    v->val.vexp.cell = malloc(sizeof(lval*)*size);
    return v;
}

lval* lval_qexpr(void) {
    lval *v = lval_alloc();
    v->type = LVAL_QEXPR;
    v->val.vexp.count = 0;
    v->val.vexp.allocated_size = 0;
    v->val.vexp.cell = NULL;
    return v;
}

lval* lval_qexpr_with_size(size_t size) {
    lval *v = lval_alloc();
    v->type = LVAL_QEXPR;
    v->val.vexp.count = 0;
    v->val.vexp.allocated_size = size;
    v->val.vexp.cell = malloc(sizeof(lval*)*size);
    return v;
}

lval* lval_fun(lbuiltin func) {
    lval *v = lval_alloc();
    v->type = LVAL_FUN;
    v->val.vfunc.builtin = func;
    return v;
}

lval* lval_lambda(lval *args, lval *body)
{
    lval *v = lval_alloc();
    v->type = LVAL_FUN;
    v->val.vfunc.builtin = NULL;
    v->val.vfunc.args = lval_retain(args);
    v->val.vfunc.body = lval_retain(body);
    return v;
}

lval* lval_primitive_type(lval_type type)
{
    lval *v = lval_alloc();
    v->type = LVAL_TYPE;
    v->val.vtype.name = NULL;
    v->val.vtype.props = NULL;
    v->val.vtype.primitive = type;
    return v;
}

lval* lval_custom_type(lval *name, lval *props)
{
    lval *v = lval_alloc();
    v->type = LVAL_TYPE;
    v->val.vtype.name = lval_retain(name);
    v->val.vtype.props = lval_retain(props);
    return v;
}

lval* lval_kv_pair(lval *key, lval *value)
{
    lval *v = lval_alloc();
    v->type = LVAL_KEY_VALUE_PAIR;
    v->val.vkvpair.key = lval_retain(key);
    v->val.vkvpair.value = lval_retain(value);
    return v;
}

lval* lval_custom_type_instance(const lval *type, const lval *props)
{
    lval *v = lval_alloc();
    v->type = LVAL_CUSTOM_TYPE_INSTANCE;
    v->val.vinst.type = lval_retain(type);
    v->val.vinst.props = lval_table_alloc(count(props)*3);
    for (size_t i=0; i<count(props); i++) {
        lval *p = child(props, i);
        lval_table_insert(v->val.vinst.props, p->val.vkvpair.key,
                          p->val.vkvpair.value);
    }
    return v;
}

#pragma mark - Casting

lval* cast_to_buffer(const lval *v)
{
    if (v->type == LVAL_BUF) {
        return lval_copy(v);
    } else if (v->type == LVAL_STR) {
        size_t len = strlen(v->val.vstr)+1;
        lval *r = lval_buf(len);
        strcpy((char *)r->val.vbuf.data, v->val.vstr);
        return r;
    } else if (v->type == LVAL_BYTE) {
        lval *r = lval_buf(1);
        r->val.vbuf.data[0] = v->val.vbyte;
        return r;
    } else if (v->type == LVAL_INT) {
        lval *r = lval_buf(sizeof(int64_t));
        ((int64_t *)r->val.vbuf.data)[0] = v->val.vint;
        return r;
    } else if (v->type == LVAL_FLT) {
        lval *r = lval_buf(sizeof(double));
        ((double *)r->val.vbuf.data)[0] = v->val.vflt;
        return r;
    }
    return NULL;
}


lval* cast_to_string(const lval *v)
{
    if (v->type == LVAL_STR) {
        return lval_copy(v);
    } else if (v->type == LVAL_BUF) {
        size_t len = v->val.vbuf.size;
        lval *r = lval_alloc();
        r->type = LVAL_STR;
        r->val.vstr = malloc(len+1);
        memcpy(r->val.vstr, v->val.vbuf.data, len);
        r->val.vstr[len] = 0x00;
        r->val.vstr = realloc(r->val.vstr, strlen(r->val.vstr)+1);
        return r;

    } else if (lval_is_number(v)) {
        size_t offset = 0;
        size_t len = 22;
        char *tmp = malloc(sizeof(char)*len);
        lval_sprint(v, &tmp, &offset, &len, false);
        len = strlen(tmp);
        tmp = realloc(tmp, len+1);
        tmp[len] = 0x00;
        lval *r = lval_str(tmp);
        free(tmp);
        return r;
    }
    return NULL;
}

lval* cast_to_byte(const lval *v)
{
    if (v->type == LVAL_BYTE) {
        return lval_copy(v);
    } else if (v->type == LVAL_INT) {
        return lval_byte(v->val.vint);
    } else if (v->type == LVAL_FLT) {
        return lval_byte(v->val.vflt);
    }
    // Invalid cast
    return NULL;
}

lval* cast_to_int(const lval *v)
{
    if (v->type == LVAL_BYTE) {
        return lval_int(v->val.vbyte);
    } else if (v->type == LVAL_INT) {
        return lval_copy(v);
    } else if (v->type == LVAL_FLT) {
        return lval_int(v->val.vflt);
    }
    // Invalid cast
    return NULL;
}

lval* cast_to_float(const lval *v)
{
    if (v->type == LVAL_BYTE) {
        return lval_float(v->val.vbyte);
    } else if (v->type == LVAL_INT) {
        return lval_float(v->val.vint);
    } else if (v->type == LVAL_FLT) {
        return lval_copy(v);
    }
    // Invalid cast
    return NULL;
}

lval* cast_to(const lval *v, lval_type t) {
    if (t == LVAL_BYTE) {
        return cast_to_byte(v);
    } else if (t == LVAL_INT) {
        return cast_to_int(v);
    } else if (t == LVAL_FLT) {
        return cast_to_float(v);
    } else if (t == LVAL_STR) {
        return cast_to_string(v);
    } else if (t == LVAL_BUF) {
        return cast_to_buffer(v);
    }
    // Invalid cast
    return NULL;
}

lval *cast_list_to_type(const lval *l, lval_type type)
{
    lval *exp = lval_qexpr_with_size(count(l));
    for (size_t i = 0; i < count(l); i++) {
        lval *b = cast_to(child(l, i), type);
        assert(b != NULL);
        lval_add(exp, b);
        lval_release(b);
    }
    return exp;
}

#pragma mark - lval utility functions

lval* lval_add(lval *v, const lval *x) {
    assert(v->type == LVAL_SEXPR || v->type == LVAL_QEXPR);
    assert(v != x);
    if (count(v)+1 > v->val.vexp.allocated_size) {
        v->val.vexp.allocated_size = (count(v)+1)*4;
        v->val.vexp.cell = realloc(v->val.vexp.cell,
                               sizeof(lval *)*v->val.vexp.allocated_size);
    }
    v->val.vexp.cell[v->val.vexp.count] = lval_retain(x);
    v->val.vexp.count++;
    return v;
}

lval* lval_pop(lval *v, size_t i) {
    assert(v->type == LVAL_SEXPR || v->type == LVAL_QEXPR);
    assert(i <= count(v));

    lval *x = child(v, i);
    memmove(v->val.vexp.cell+i, v->val.vexp.cell+i+1,
            sizeof(lval*) * (v->val.vexp.count-i-1));
    v->val.vexp.count--;
    return lval_release(x);
}


lval* lval_copy(const lval *v) {

    // For types and symbols, let's just increase the ref_count
    // there should never be a reason to make an real copy
    if (v->type == LVAL_TYPE || v->type == LVAL_SYM) {
        return lval_retain(v);
    }

    lval *x = lval_alloc();
    x->type = v->type;
    x->source_position = code_pos_retain(v->source_position);

    if (v->bound_name != NULL) {
        x->bound_name = lval_retain(v->bound_name);
    }

    switch(v->type) {
        case LVAL_TYPE:
        case LVAL_SYM:
            break;
        case LVAL_INT:
            x->val.vint = v->val.vint;
            break;
        case LVAL_FLT:
            x->val.vflt = v->val.vflt;
            break;
        case LVAL_BYTE:
            x->val.vbyte = v->val.vbyte;
            break;
        case LVAL_FUN:
            if (v->val.vfunc.builtin) {
                x->val.vfunc.builtin = v->val.vfunc.builtin;
            } else {
                x->val.vfunc.builtin = NULL;
                x->val.vfunc.args = lval_copy(v->val.vfunc.args);
                x->val.vfunc.body = lval_copy(v->val.vfunc.body);
            }
            break;
        case LVAL_CAUGHT_ERR:
        case LVAL_ERR:
            x->val.verr.message = strdup(v->val.verr.message);
            if (v->val.verr.stack_trace != NULL) {
                x->val.verr.stack_trace = lval_retain(v->val.verr.stack_trace);
            } else {
                x->val.verr.stack_trace = NULL;
            }
            break;
        case LVAL_STR:
            x->val.vstr = strdup(v->val.vstr);
            break;
        case LVAL_BUF:
            x->val.vbuf.size = v->val.vbuf.size;
            x->val.vbuf.data = malloc(v->val.vbuf.size);
            memcpy(x->val.vbuf.data, v->val.vbuf.data, v->val.vbuf.size);
            break;
        case LVAL_DICT:
            x->val.vdict = lval_table_copy(v->val.vdict);
            break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            x->val.vexp.count = v->val.vexp.count;
            x->val.vexp.allocated_size = count(v);
            x->val.vexp.cell = malloc(sizeof(lval*) * x->val.vexp.count);
            for (size_t i = 0; i < count(x); i++) {
                x->val.vexp.cell[i] = lval_copy(child(v, i));
            }
            break;
        case LVAL_KEY_VALUE_PAIR:
            x->val.vkvpair.key = lval_copy(v->val.vkvpair.key);
            x->val.vkvpair.value = lval_copy(v->val.vkvpair.value);
            break;
        case LVAL_CUSTOM_TYPE_INSTANCE:
            x->val.vinst.type = lval_retain(v->val.vinst.type);
            x->val.vinst.props = lval_table_copy(v->val.vinst.props);
            break;
    }
    return x;
}

#pragma mark - Type checking

bool lval_is_number(const lval *v) {
    return v->type == LVAL_BYTE || v->type == LVAL_INT || v->type == LVAL_FLT;
}

lval* type_from_pair(lenv *e, const lval *v) {
    if (v->val.vkvpair.value->type == LVAL_TYPE) {
        return lval_retain(v->val.vkvpair.value);
    } else if (v->val.vkvpair.value->type == LVAL_SYM) {
        return lenv_get(e, v->val.vkvpair.value);
    }
    return NULL;
}

bool equal_types(const vtype *t1, const vtype *t2) {
    if (t1 == t2) {
        return true;
    } else if (t1->name && t2->name && !equal_symbols(t1->name, t2->name)) {
        return false;
    } else if (t1->props != NULL && t2->props == NULL) {
        return false;
    } else if (t1->props == NULL && t2->props != NULL) {
        return false;
    } else if (t1->props == NULL && t2->props == NULL) {
        return t1->primitive == t2->primitive;
    } else if (count(t1->props) != count(t2->props)) {
        return false;
    }
    for (size_t i=0; i<count(t1->props); i++) {
        if (!lval_eq(child(t1->props, i), child(t2->props, i))) {
            return false;
        }
    }
    return true;
}

bool value_matches_type(lenv *e, const lval *v, const lval *type, lval **cast_value)
{
    // Do we want a primitive type?
    if (type->val.vtype.props == NULL) {
        // Is the value of the same type?
        if (v->type == type->val.vtype.primitive) {
            return true;

        // Technically we can cast numbers to strings but
        // it probably isn't want we wanted for a type-specifier
        } else if (type->val.vtype.primitive == LVAL_STR) {
            return false;
        // Can the value be cast to the correct type?
        }
        lval *c = cast_to(v, type->val.vtype.primitive);
        if (c == NULL) {
            return false;
        }
        *cast_value = c;
        return true;

        // Did we get a primitive type when we didn't want one?
    } else if (v->type != LVAL_CUSTOM_TYPE_INSTANCE) {
        return false;
    }
    return lval_eq(v->val.vinst.type, type);
}

char* type_mismatch_description(vtype *wanted, const lval *v)
{
    char *wanted_name = NULL;
    if (wanted->props == NULL) {
        wanted_name = ltype_name(wanted->primitive);
    } else {
        wanted_name = wanted->name->val.vsym.name;
    }
    char *got_name = NULL;
    if (v->type != LVAL_CUSTOM_TYPE_INSTANCE) {
        got_name = ltype_name(v->type);
    } else {
        got_name = v->val.vinst.type->val.vtype.name->val.vsym.name;
    }
    size_t len = 30+strlen(wanted_name)+strlen(got_name);
    char *s = malloc(len);
    sprintf(s, "Type mismatch (Got %s, wanted %s)", got_name, wanted_name);
    s[len-1] = 0x00;
    return s;
}


// Internal function for performing equality checks between two lvals
int lval_eq(const lval *x, const lval *y) {

    if (x == y) {
        return true;
    }

    lval *x1 = NULL;
    lval *y1 = NULL;

    // Upgrade bytes to ints when comparing against an int
    if (x->type == LVAL_BYTE && y->type == LVAL_INT) {
        x1 = cast_to(x, LVAL_INT);
        x = x1;

    } else if (x->type == LVAL_INT && y->type == LVAL_BYTE) {
        y1 = cast_to(y, LVAL_INT);
        y = y1;

    // Upgrade bytes to float when comparing against a float
    } else if (x->type == LVAL_BYTE && y->type == LVAL_FLT) {
        x1 = cast_to(x, LVAL_FLT);
        x = x1;

    } else if (x->type == LVAL_FLT && y->type == LVAL_BYTE) {
        y1 = cast_to(y, LVAL_FLT);
        y = y1;

    // Upgrade ints to floats when comparing against a float
    } else if (x->type == LVAL_INT && y->type == LVAL_FLT) {
        x1 = cast_to(x, LVAL_FLT);
        x = x1;

    } else if (x->type == LVAL_FLT && y->type == LVAL_INT) {
        y1 = cast_to(y, LVAL_FLT);
        y = y1;

    } else {
        // Allow matching errors and caught errors
        if (x->type == LVAL_ERR && y->type != LVAL_ERR && y->type != LVAL_CAUGHT_ERR) {
            return false; // Safe to return here, x1/y1 are still NULL

            // For other types, a mismatch between types means they are not equal
        } else if (x->type != y->type) {
             return false; // Safe to return here, x1/y1 are still NULL
        }
    }

    bool r = true;

    switch (x->type) {
        case LVAL_INT:
            r = (x->val.vint == y->val.vint);
            break;
        case LVAL_FLT:
            r = (x->val.vflt == y->val.vflt);
            break;
        case LVAL_BYTE:
            r = (x->val.vbyte == y->val.vbyte);
            break;
        case LVAL_CAUGHT_ERR:
        case LVAL_ERR:
            r = (lval_eq(x->val.verr.stack_trace, y->val.verr.stack_trace) &&
                 strcmp(x->val.verr.message, y->val.verr.message) == 0);
            break;
        case LVAL_SYM:
            r = x->val.vsym.hash == y->val.vsym.hash &&
                (strcmp(x->val.vsym.name, y->val.vsym.name) == 0);
            break;
        case LVAL_STR:
            r = (strcmp(x->val.vstr, y->val.vstr) == 0);
            break;
        case LVAL_BUF:
            if (x->val.vbuf.size != y->val.vbuf.size) {
                r = false;
            } else {
                for (size_t i=0; i<x->val.vbuf.size; i++) {
                    if (x->val.vbuf.data[i] != y->val.vbuf.data[i]) {
                        r = false;
                        break;
                    }
                }
            }
            break;
        case LVAL_DICT:
            r = lval_tables_equal(x->val.vdict, y->val.vdict);
            break;
        case LVAL_FUN:
            if (x->val.vfunc.builtin || y->val.vfunc.builtin) {
                r = x->val.vfunc.builtin == y->val.vfunc.builtin;
            } else {
                r = lval_eq(x->val.vfunc.args, y->val.vfunc.args)
                && lval_eq(x->val.vfunc.body, y->val.vfunc.body);
            }
            break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            if (count(x) != count(y)) {
                r = false;
            } else {
                for (size_t i = 0; i < count(x); i++) {
                    if (!lval_eq(child(x, i), child(y, i))) {
                        r = false;
                        break;
                    }
                }
            }
            break;
        case LVAL_TYPE:
            r = equal_types(&x->val.vtype, &y->val.vtype);
            break;
        case LVAL_KEY_VALUE_PAIR:
            if (!equal_symbols(x->val.vkvpair.key, y->val.vkvpair.key)) {
                r = false;
            } else {
                r = lval_eq(x->val.vkvpair.value, y->val.vkvpair.value);
            }
            break;
        case LVAL_CUSTOM_TYPE_INSTANCE:
            if (x->val.vinst.type != y->val.vinst.type) {
                r = lval_tables_equal(x->val.vinst.props, y->val.vinst.props);
            }
    }
    if (x1) {
        lval_release(x1);
    }
    if (y1) {
        lval_release(y1);
    }
    return r;
}


#pragma mark - Evaluation



#pragma mark - Debugging helpers

// Returns a string representation of the passed lval
char* lval_to_string(const lval *v) {
    size_t len = 2;
    size_t offset = 0;
    char *s = malloc(len*sizeof(char));
    lval_sprint(v, &s, &offset, &len, true);
    s[offset] = 0x00;
    return s;
}

// Prints a string value
void lval_print_str(const lval *v) {
    putchar('"');
    for (size_t i=0; i<strlen(v->val.vstr); i++) {
        if (strchr(lval_str_escapable, v->val.vstr[i])) {
            printf("%s", lval_str_escape(v->val.vstr[i]));
        } else {
            putchar(v->val.vstr[i]);
        }
    }
    putchar('"');
}

// Prints an expression
void lval_expr_print(const lval *v, char open, char close) {
    if (count(v) == 0) {
        return;
    }
    putchar(open);
    for (size_t i = 0; i < count(v); i++) {
        lval_print(child(v, i));
        if (i != (count(v)-1)) {
            putchar (' ');
        }
    }
    putchar(close);
}

// Prints an lval
void lval_print(const lval *v) {
    switch (v->type) {
        case LVAL_INT:
            printf("%li", v->val.vint);
            return;
        case LVAL_FLT:
        {
            static char temp[22];
            sprintf(temp, "%f", v->val.vflt);
            size_t len = strlen(temp);
            while (temp[len-1] == '0') {
                len--;
            }
            if (len > 0 && temp[len-1] == '.') {
                len--;
            }
            temp[len] = 0x00;
            printf("%s", temp);
            return;
        }
        case LVAL_BYTE:
            printf("0x%02X", v->val.vbyte);
            return;
        case LVAL_SYM:
            printf("%s", v->val.vsym.name);
            return;
        case LVAL_FUN:
            if (v->val.vfunc.builtin) {
                printf("%s",builtin_func_string(v->val.vfunc.builtin));
            } else {
                printf("(\\ ");
                lval_print(v->val.vfunc.args);
                putchar(' ');
                lval_print(v->val.vfunc.body);
                putchar(')');
            }
            return;
        case LVAL_STR:
            lval_print_str(v);
            return;
        case LVAL_BUF:
            putchar('<');
            for (size_t i = 0; i<v->val.vbuf.size; i++) {
                printf("0x%02X", v->val.vbuf.data[i]);
                if (i < v->val.vbuf.size-1) {
                    putchar(' ');
                }
            }
            putchar('>');
            return;
        case LVAL_SEXPR:
            lval_expr_print(v, '(', ')');
            return;
        case LVAL_QEXPR:
            lval_expr_print(v, '{', '}');
            return;
        case LVAL_CAUGHT_ERR:
        case LVAL_ERR:
            printf("Error: %s", v->val.verr.message);
            return;
        case LVAL_DICT: {
            printf("(dict ");
            lval_entry **props = NULL;
            size_t prop_count = lval_table_entries(v->val.vdict, &props);
            for (size_t i=0; i<prop_count; i++) {
                lval_entry *e = props[i];
                char *key = lval_to_string(e->key);
                char *val = lval_to_string(e->value);
                printf("%s:%s", key, val);
                free(key);
                free(val);
                if (i<prop_count-1) {
                    putchar(' ');
                }
            }
            putchar(')');
            free(props);
            return;
        }
        case LVAL_TYPE: {
            if (v->val.vtype.props) {
                printf("(%s ", v->val.vtype.name->val.vsym.name);
                size_t max = count(v->val.vtype.props);
                for (size_t i=0; i<max; i++) {
                    lval *p = child(v->val.vtype.props, i);
                    char *v = lval_to_string(p->val.vkvpair.value);
                    printf("%s:%s", p->val.vkvpair.key->val.vsym.name, v);
                    free(v);
                    if (i<max-1) {
                        putchar(' ');
                    }
                }
                putchar(')');
            } else {
                printf("<%s>", ltype_name(v->val.vtype.primitive));
            }
            return;
        }

        case LVAL_KEY_VALUE_PAIR:
            printf("%s:", v->val.vkvpair.key->val.vsym.name);
            char *val = lval_to_string(v->val.vkvpair.value);
            printf("%s", val);
            free(val);
            return;
        case LVAL_CUSTOM_TYPE_INSTANCE: {
            printf("<%s ", v->val.vinst.type->val.vtype.name->val.vsym.name);
            lval_entry **props = NULL;
            size_t prop_count = lval_table_entries(v->val.vinst.props, &props);
            for (size_t i=0; i<prop_count; i++) {
                char *val = lval_to_string(props[i]->value);
                printf("%s:%s", props[i]->key->val.vsym.name, val);
                free(val);
                if (i<prop_count-1) {
                    putchar(' ');
                }
            }
            putchar('>');
            free(props);
            break;
        }

    }
}

// Prints an lval with a line break
void lval_println(const lval *v) {
    lval_print(v);
    if (v->type != LVAL_SEXPR || count(v) > 0) {
        putchar('\n');
    }
}

#pragma mark - Allocating and freeing lvals in the shared global pool

lval* lval_alloc(void) {
    lval *v = pool_lval_alloc(global_pool());
    return v;
}

void lval_free(lval *v) {

    if (v->bound_name != NULL) {
        lval_release(v->bound_name);
    }
    code_pos_release(v->source_position);

    switch (v->type) {
        case LVAL_INT:
        case LVAL_FLT:
        case LVAL_BYTE:
            break;
        case LVAL_FUN:
            if (!v->val.vfunc.builtin) {
                lval_release(v->val.vfunc.args);
                lval_release(v->val.vfunc.body);
            }
            break;
        case LVAL_CAUGHT_ERR:
        case LVAL_ERR:
            free(v->val.verr.message);
            if (v->val.verr.stack_trace != NULL) {
                lval_release(v->val.verr.stack_trace);
            }
            break;
        case LVAL_SYM:
            free(v->val.vsym.name);
            break;
        case LVAL_STR:
            free(v->val.vstr);
            break;
        case LVAL_BUF:
            free(v->val.vbuf.data);
            break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for (size_t i=0; i<count(v); i++) {
                lval_release(child(v, i));
            }
            free(v->val.vexp.cell);
            break;
        case LVAL_TYPE:
            if (v->val.vtype.name != NULL) {
                lval_release(v->val.vtype.name);
            }
            if (v->val.vtype.props != NULL) {
                lval_release(v->val.vtype.props);
            }
            break;
        case LVAL_DICT:
            lval_table_free(v->val.vdict);
            break;
        case LVAL_KEY_VALUE_PAIR:
            lval_release(v->val.vkvpair.key);
            lval_release(v->val.vkvpair.value);
            break;
        case LVAL_CUSTOM_TYPE_INSTANCE:
            if (v->val.vinst.props != NULL) {
                lval_table_free(v->val.vinst.props);
            }
            lval_release(v->val.vinst.type);
            break;
    }
    pool_lval_free(global_pool(), v);
}

