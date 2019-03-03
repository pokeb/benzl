// This file implements built-in functions for working with Buffers
//
// Part of benzl - https://github.com/pokeb/benzl

#include "benzl-builtins.h"
#include "benzl-lval.h"
#include "benzl-lval-eval.h"
#include "benzl-error-macros.h"

lval* builtin_create_buffer(lenv *e, const lval *a)
{
    if (count(a) != 1 || child(a, 0)->type != LVAL_INT) {
        return lval_err_for_val(
            a, "create-buffer takes a single integer argument for the length"
        );
    }
    return lval_buf(child(a, 0)->val.vint);
}

lval* builtin_buffer_with_bytes(lenv *e, const lval *a)
{
    lval *r = lval_buf(count(a));
    for (size_t i=0; i<count(a); i++) {
        lval *b = cast_to(child(a, i), LVAL_BYTE);
        if (b == NULL) {
            return lval_err_for_val(
                a, "buffer-with-bytes expects only bytes "
                   "(got: %s for argument %d)",
                   ltype_name(child(a, i)->type), i
            );
        }
        r->val.vbuf.data[i] = b->val.vbyte;
        lval_release(b);
    }
    return r;
}

lval* builtin_buffer_map(lenv *e, const lval *a)
{
    if (count(a) < 3 || child(a, 0)->type != LVAL_BUF ||
        child(a, 1)->type != LVAL_INT || child(a, 2)->type != LVAL_FUN) {
        return lval_err_for_val(
            a, "buffer-map expects 3 arguments in the form"
               "(buffer-map buffer:Buffer componentSize:Integer func:function)"
        );
    }
    lval *buffer = child(a, 0);
    size_t size = child(a, 1)->val.vint;
    lval *fun = child(a, 2);
    lval *new_buffer = lval_buf(buffer->val.vbuf.size);

    lval *data = lval_buf(size);
    lval *offset = lval_int(0);

    lval *args = lval_qexpr_with_size(2);
    lval_add(args, data);
    lval_add(args, offset);

    for (size_t i=0; i<buffer->val.vbuf.size; i+=size) {

        // Update the args
        memcpy(data->val.vbuf.data, buffer->val.vbuf.data+i, size);
        offset->val.vint = i/size;

        lval *r = lval_call(e, fun, args);
        if (r->type == LVAL_ERR) {
            lval_release(args);
            lval_release(data);
            lval_release(offset);
            return r;
        }
        memset(new_buffer->val.vbuf.data+i, 0, size);
        if (r->type == LVAL_BYTE) {
            new_buffer->val.vbuf.data[i] = r->val.vbyte;
        } else if (r->type == LVAL_INT) {
            memcpy(new_buffer->val.vbuf.data+i, &r->val.vint, MIN(sizeof(long), size));
        } else if (r->type == LVAL_FLT) {
            memcpy(new_buffer->val.vbuf.data+i, &r->val.vflt, MIN(sizeof(double), size));
        } else if (r->type == LVAL_BUF) {
            memcpy(new_buffer->val.vbuf.data+i, &r->val.vbuf.data, MIN(r->val.vbuf.size, size));
        }

        lval_release(r);
    }
    lval_release(args);
    lval_release(data);
    lval_release(offset);
    return new_buffer;
}

static inline lval* bad_args(const lval *a, const char *func_name)
{
    return lval_err_for_val(
        a, "%s expects arguments in the form "
        "(%s buffer:Buffer offset:Integer value:Integer)", func_name, func_name
    );
}

static inline lval* out_of_range(const lval *a, char *func_name, const lval* buffer,
                                 const lval *offset, const size_t size)
{
    return lval_err_for_val(
        a, "%s: offset %d out of range to set %d bytes (Buffer size: %d bytes)",
        func_name, offset->val.vint,size, buffer->val.vbuf.size
    );
}

// Eeew.
#define PUT_VALUE(__a, __name, __type) { \
    if (count(__a) != 3 || child(__a, 0)->type != LVAL_BUF) { \
        return bad_args(__a, __name); \
    } \
    lval *__buffer = child(__a, 0); \
    lval *__offset = cast_to(child(__a, 1), LVAL_INT); \
    if (__offset == NULL) { \
        return bad_args(__a, __name); \
    } \
    lval *__value = cast_to(child(__a, 2), LVAL_INT); \
    if (__value == NULL) { \
        lval_release(__offset); \
        return bad_args(__a, __name); \
    } \
    if (__buffer->val.vbuf.size < __offset->val.vint+sizeof(__type)) { \
        lval *err = out_of_range(__a, __name, __buffer, __offset, sizeof(__type)); \
        lval_release(__offset); lval_release(__value); \
        return err; \
    } \
    __type __v = (__type)__value->val.vint; \
    lval *__newbuf = lval_copy(__buffer); \
    memcpy(__newbuf->val.vbuf.data+__offset->val.vint, &__v, sizeof(__type)); \
    lval_release(__offset); lval_release(__value); \
    return __newbuf; \
}

// Eeew, again?
#define GET_VALUE(__a, __name, __type, __lval_func) { \
    if (count(__a) != 2 || child(__a, 0)->type != LVAL_BUF) { \
        return bad_args(__a, __name); \
    } \
    lval *__buffer = child(__a, 0); \
    lval *__offset = cast_to(child(__a, 1), LVAL_INT); \
    if (__offset == NULL) { \
        return bad_args(__a, __name); \
    } \
    if (__buffer->val.vbuf.size < __offset->val.vint+sizeof(__type)) { \
        lval *err = out_of_range(__a, __name, __buffer, __offset, sizeof(__type)); \
        lval_release(__offset); \
        return err; \
    } \
    __type __v = 0; \
    memcpy(&__v, __buffer->val.vbuf.data+__offset->val.vint, sizeof(__type)); \
    lval_release(__offset); \
    return __lval_func(__v); \
}

// uint8 functions
lval* builtin_get_byte(lenv *e, const lval *a) {
    GET_VALUE(a, "get-byte", uint8_t, lval_byte);
}

lval* builtin_put_byte(lenv *e, const lval *a) {
    PUT_VALUE(a, "put-byte", uint8_t);
}

// unsigned char functions
// (these return ints rather than bytes)
lval* builtin_get_unsigned_char(lenv *e, const lval *a) {
    GET_VALUE(a, "get-unsigned-char", uint8_t, lval_int);
}

lval* builtin_put_unsigned_char(lenv *e, const lval *a) {
    PUT_VALUE(a, "put-unsigned-char", uint8_t);
}

// signed char functions
// (these return ints rather than bytes)
lval* builtin_get_signed_char(lenv *e, const lval *a) {
    GET_VALUE(a, "get-signed-char", int8_t, lval_int);
}

lval* builtin_put_signed_char(lenv *e, const lval *a) {
    PUT_VALUE(a, "put-signed-char", int8_t);
}

// unsigned short functions
// (these return ints)
lval* builtin_get_unsigned_short(lenv *e, const lval *a) {
    GET_VALUE(a, "get-unsigned-short", uint16_t, lval_int);
}

lval* builtin_put_unsigned_short(lenv *e, const lval *a) {
    PUT_VALUE(a, "put-unsigned-short", uint16_t);
}

// signed short functions
// (these return ints)
lval* builtin_get_signed_short(lenv *e, const lval *a) {
    GET_VALUE(a, "get-signed-short", int16_t, lval_int);
}

lval* builtin_put_signed_short(lenv *e, const lval *a) {
    PUT_VALUE(a, "put-signed-short", int16_t);
}

// unsigned int functions
lval* builtin_get_unsigned_integer(lenv *e, const lval *a) {
    GET_VALUE(a, "get-unsigned-integer", uint32_t, lval_int);
}

lval* builtin_put_unsigned_integer(lenv *e, const lval *a) {
    PUT_VALUE(a, "put-unsigned-integer", uint32_t);
}

// signed int functions
lval* builtin_get_signed_integer(lenv *e, const lval *a) {
    GET_VALUE(a, "get-signed-integer", int32_t, lval_int);
}

lval* builtin_put_signed_integer(lenv *e, const lval *a) {
    PUT_VALUE(a, "put-signed-integer", int32_t);
}

// unsigned long functions
lval* builtin_get_unsigned_long(lenv *e, const lval *a) {
    GET_VALUE(a, "get-unsigned-long", uint64_t, lval_int);
}

lval* builtin_put_unsigned_long(lenv *e, const lval *a) {
    PUT_VALUE(a, "put-unsigned-long", uint64_t);
}

// signed int functions
lval* builtin_get_signed_long(lenv *e, const lval *a) {
    GET_VALUE(a, "get-signed-long", uint64_t, lval_int);
}

lval* builtin_put_signed_long(lenv *e, const lval *a) {
    PUT_VALUE(a, "put-signed-long", uint64_t);
}

// string functions
lval *builtin_get_string(lenv *e, const lval *a)
{
    if (count(a) != 2 || child(a, 0)->type != LVAL_BUF) {
        return lval_err_for_val(
            a, "get-string expects arguments in the form "
               "(get-string buffer:Buffer offset:Integer)"
        );
    }
    lval *buffer = child(a, 0);
    lval *offset = cast_to(child(a, 1), LVAL_INT);
    if (offset == NULL) {
        return lval_err_for_val(
            a, "get-string expects arguments in the form "
               "(get-string buffer:Buffer offset:Integer)"
        );
    }

    if (buffer->val.vbuf.size < offset->val.vint+1) {
        lval *err =  lval_err_for_val(
            a, "get-string: offset %d out of range (Buffer size: %d bytes)",
            offset->val.vint, buffer->val.vbuf.size
        );
        lval_release(offset);
        return err;
    }
    char *buf = malloc(buffer->val.vbuf.size+1);
    strcpy(buf, (char *)buffer->val.vbuf.data+offset->val.vint);
    buf = realloc(buf, strlen(buf)+1);
    lval *r = lval_str(buf);
    free(buf);
    lval_release(offset);
    return r;
}


lval *builtin_put_string(lenv *e, const lval *a)
{
    if (count(a) != 3 || child(a, 0)->type != LVAL_BUF ||
        child(a, 2)->type != LVAL_STR) {
        return lval_err_for_val(
            a, "put-string expects arguments in the form "
            "(put-string buffer:Buffer offset:Integer string:String)"
        );
    }
    lval *buffer = child(a, 0);
    lval *value = child(a, 2);
    lval *offset = cast_to(child(a, 1), LVAL_INT);
    if (offset == NULL) {
        return lval_err_for_val(
            a, "put-string expects arguments in the form "
            "(put-string buffer:Buffer offset:Integer string:String)"
        );
    }
    size_t len = strlen(value->val.vstr);
    if (buffer->val.vbuf.size < offset->val.vint+len+1) {
        lval *err =  lval_err_for_val(
            a, "put-string: offset %d out of range to set %d bytes (Buffer size: %d bytes)",
            offset->val.vint, len+1, buffer->val.vbuf.size
        );
        lval_release(offset);
        return err;
    }
    buffer = lval_buf(buffer->val.vbuf.size);
    strcpy((char *)buffer->val.vbuf.data+offset->val.vint, value->val.vstr);
    lval_release(offset);
    return buffer;
}

// buffer functions
lval *builtin_get_bytes(lenv *e, const lval *a)
{
    if (count(a) != 3 || child(a, 0)->type != LVAL_BUF) {
        return lval_err_for_val(
            a, "get-bytes expects arguments in the form "
               "(get-bytes source:Buffer offset:Integer length:Integer)"
        );
    }
    lval *buffer = child(a, 0);
    lval *offset = cast_to(child(a, 1), LVAL_INT);
    if (offset == NULL) {
        return lval_err_for_val(
            a, "get-bytes expects arguments in the form "
            "(get-bytes source:Buffer offset:Integer length:Integer)"
        );
    }
    lval *length = cast_to(child(a, 2), LVAL_INT);
    if (length == NULL) {
        lval_release(offset);
        return lval_err_for_val(
            a, "get-bytes expects arguments in the form "
            "(get-bytes source:Buffer offset:Integer length:Integer)"
        );
    }

    if (buffer->val.vbuf.size < offset->val.vint+length->val.vint) {
        lval *err = lval_err_for_val(
            a, "get-bytes: offset %d out of range to get %d bytes (Buffer size: %d bytes)",
            offset->val.vint, length->val.vint, buffer->val.vbuf.size
        );
        lval_release(offset);
        lval_release(length);
        return err;
    }
    lval *r = lval_buf(length->val.vint);
    memcpy(r->val.vbuf.data, (char *)buffer->val.vbuf.data+offset->val.vint, length->val.vint);
    lval_release(offset);
    lval_release(length);
    return r;
}

lval *builtin_put_bytes(lenv *e, const lval *a)
{
    if (count(a) != 3 || child(a, 0)->type != LVAL_BUF ||
        child(a, 2)->type != LVAL_BUF) {
        return lval_err_for_val(
            a, "put-bytes expects arguments in the form "
               "(put-bytes target:Buffer offset:Integer source:Buffer)"
        );
    }
    lval *buffer = child(a, 0);
    lval *value = child(a, 2);
    lval *offset = cast_to(child(a, 1), LVAL_INT);
    if (offset == NULL) {
        return lval_err_for_val(
            a, "put-bytes expects arguments in the form "
               "(put-bytes target:Buffer offset:Integer source:Buffer)"
        );
    }
    if (buffer->val.vbuf.size < offset->val.vint+value->val.vbuf.size) {
        lval *err = lval_err_for_val(
            a, "put-bytes: offset %d out of range to set %d bytes (Buffer size: %d bytes)",
            offset->val.vint, value->val.vbuf.size, buffer->val.vbuf.size
        );
        lval_release(offset);
        return err;
    }
    buffer = lval_copy(buffer);
    memcpy(buffer->val.vbuf.data+offset->val.vint, value->val.vbuf.data, value->val.vbuf.size);
    lval_release(offset);
    return buffer;
}
