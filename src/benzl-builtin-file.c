// This file implements built-in functions for reading and writing binary data
// from a file
//
// Part of benzl - https://github.com/pokeb/benzl

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "benzl-builtins.h"
#include "benzl-lval.h"
#include "benzl-lenv.h"
#include "benzl-error-macros.h"

lval *builtin_read_file(lenv *e, const lval *a)
{
    LASSERT_NUM_ARGS("read-file", a, 1);
    LASSERT_ARG_TYPE("read-file", a, 0, LVAL_STR);

    lval *path = child(a, 0);
    char *path_str = path->val.vstr;

    FILE *file = fopen(path_str, "r");
    if (file == NULL) {
        return lval_err_for_val(a, "Unable to read the file at '%s'", path_str);
    }

    fseek(file, 0, SEEK_END);
    long pos = ftell(file);
    if (pos == -1) {
        return lval_err_for_val(a,"Unable to read the file at '%s' (Error: %d)",
                                path_str, errno);
    }
    size_t len = (size_t)pos;
    fseek(file, 0, SEEK_SET);

    uint8_t *buf = malloc(len + 1);
    size_t unused __attribute__((unused)) = fread(buf, len, 1, file);
    fclose(file);

    lval *r = lval_buf(len);
    memcpy(r->val.vbuf.data, buf, len);
    free(buf);
    return r;
}

static lval *write_lval(FILE *f, const lval *a) {
    switch (a->type) {
        case LVAL_BUF:
            fwrite(a->val.vbuf.data, a->val.vbuf.size, 1, f);
            break;
        case LVAL_INT:
            fwrite(&a->val.vint, sizeof(long), 1, f);
            break;
        case LVAL_FLT:
            fwrite(&a->val.vflt, sizeof(double), 1, f);
            break;
        case LVAL_BYTE:
            fwrite(&a->val.vbyte, sizeof(uint8_t), 1, f);
            break;
        case LVAL_STR:
            fwrite(a->val.vstr, sizeof(uint8_t), strlen(a->val.vstr), f);
            break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for (size_t i=0; i<count(a); i++) {
                write_lval(f, child(a, i));
            }
            break;
        default:
            return lval_err_for_val(
                a, "Writing is not supported for objects of type '%s'",
                ltype_name(a->type)
            );
    }
    return NULL;
}

lval *builtin_write_file(lenv *e, const lval *a) {
    LASSERT_NUM_ARGS("write-file", a, 2);
    LASSERT_ARG_TYPE("write-file", a, 0, LVAL_STR);

    lval *path = child(a, 0);
    char *path_str = path->val.vstr;

    FILE *file = fopen(path_str, "w");
    if (file == NULL) {
        return lval_err_for_val(a, "Unable to open '%s' for writing", path_str);
    }

    lval *contents = child(a, 1);
    lval *err = write_lval(file, contents);

    fclose(file);
    if (err != NULL) {
        return err;
    }
    return lval_sexpr();
}
