// This file implements built-in functions for printing and formatting lvals
//
// Part of benzl - https://github.com/pokeb/benzl

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "benzl-builtins.h"
#include "benzl-lval-pool.h"
#include "benzl-lenv.h"
#include "benzl-sprintf.h"

lval* builtin_format(lenv *e, const lval *a) {
    if (count(a) < 1) {
        return lval_err_for_val(a, "Got no args for format!");
    }
    if (child(a, 0)->type == LVAL_QEXPR) {
        return builtin_format(e, child(a, 0));
    }
    if (child(a, 0)->type != LVAL_STR) {
        return lval_err_for_val(
            a, "First argument to format must be a string (got %s)",
            ltype_name(child(a, 0)->type)
        );
    }

    char *fmt = child(a, 0)->val.vstr;
    size_t len = strlen(fmt);

    size_t args_count = count(a)-1;
    if (args_count == 0) {
        return lval_str(child(a, 0)->val.vstr);
    }
    size_t arg_num = 0;
    size_t buf_len = (len*2)+1;
    char *buf = malloc(sizeof(char)*buf_len);
    size_t i2 = 0;

    bool escape = false;
    for (size_t i=0; i<len; i++) {
        if (fmt[i] == '\\') {
            if (!escape) {
                escape = true;
                continue;
            }
            escape = false;
        }
        if (!escape && fmt[i] == '%') {
            lval *arg = child(a, arg_num+1);
            lval_sprint(arg, &buf, &i2, &buf_len, false);
            arg_num++;
            // Have we run out of arguments to display?
            if (arg_num >= args_count) {
                if (i+1 < len) {
                    char *remainder = fmt+i+1;
                    resize_buffer_if_needed(&buf, &buf_len, i2+strlen(remainder));
                    sprintf(buf+i2, "%s", remainder);
                    i2+=strlen(remainder);
                }
                break;
            }

        } else {
            resize_buffer_if_needed(&buf, &buf_len, i2+1);
            buf[i2] = fmt[i];
            i2++;
        }
        escape = false;
    }
    buf[i2] = 0x00;
    lval *r = lval_str(buf);
    free(buf);
    return r;
}

lval* builtin_print(lenv *c, const lval *a) {
    for (size_t i=0; i<count(a); i++) {
        lval_print(child(a, i));
        putchar(' ');
    }
    putchar('\n');
    return lval_sexpr();
}

lval* builtin_printf(lenv *e, const lval *a)
{
    lval *s = builtin_format(e, a);
    switch (s->type) {
        case LVAL_STR:
            printf("%s\n", s->val.vstr);
            break;
        default:
            lval_print(s);
            break;
    }
    lval_release(s);
    return lval_sexpr();
}
