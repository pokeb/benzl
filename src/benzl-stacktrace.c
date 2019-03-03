// Part of benzl - https://github.com/pokeb/benzl

#include <stdio.h>

#include "benzl-builtins.h"
#include "benzl-lval.h"
#include "benzl-sprintf.h"

lval *shared_stack = NULL;

void stack_cleanup(void)
{
    if (shared_stack != NULL) {
        assert(count(shared_stack) == 0);
        lval_release(shared_stack);
        shared_stack = NULL;
    }
}

void stack_push_frame(const lval *v)
{
    if (shared_stack == NULL) {
        shared_stack = lval_qexpr_with_size(64);
    }
    lval_add(shared_stack, v);
}

void stack_pop_frame(void) {
    lval_pop(shared_stack, count(shared_stack)-1);
}



lval* stack_trace(const lval *a)
{
    if (shared_stack == NULL) {
        return NULL;
    }
    size_t len = 0;
    size_t buf_len = 0;
    char *buf = malloc(1);

    for (size_t i= count(shared_stack)-1; i != -1; i--) {
        lval *frame = child(shared_stack, i);
        char *exp = lval_to_string(frame);
        char *source_file = "";
        char *divider = "";
        if (frame->source_position.source_file != NULL) {
            source_file = frame->source_position.source_file->val.vstr;
            divider = ":";
        }
        resize_buffer_if_needed(&buf, &buf_len, len+strlen(exp)+strlen(source_file)+32);
        sprintf(buf+len, "at %s %s%s%d:%d\n", exp, source_file, divider,
                frame->source_position.row+1, frame->source_position.col);
        len = strlen(buf);
        free(exp);
    }

    buf = realloc(buf, strlen(buf)+1);
    lval *r = lval_str(buf);
    free(buf);
    return r;
}

void print_error_with_trace(const lval *err)
{
    if (err->val.verr.stack_trace != NULL) {
        printf("%s\n%s\n", err->val.verr.message,
               err->val.verr.stack_trace->val.vstr);
    } else {
        char *source_file = "";
        char *divider = "";
        if (err->source_position.source_file != NULL) {
            source_file = err->source_position.source_file->val.vstr;
            divider = ":";
        }
        printf("%s at %s%s%i:%i\n", err->val.verr.message, source_file, divider,
               err->source_position.row+1,
               err->source_position.col);
    }

}
