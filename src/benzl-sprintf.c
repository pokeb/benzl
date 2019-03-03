// Part of benzl - https://github.com/pokeb/benzl

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "benzl-sprintf.h"
#include "benzl-lval.h"
#include "benzl-builtins.h"

/* Possible unescapable characters */
char* lval_str_unescapable = "abfnrtv\\\'\"";

/* List of possible escapable characters */
char* lval_str_escapable = "\a\b\f\n\r\t\v\\\'\"";

// Internal function for resizing the buffer used for sprintf operations
void resize_buffer_if_needed(char **buf, size_t *max_len, size_t wanted_length)
{
    if (wanted_length+1 > *max_len) {
        *max_len = (size_t)((wanted_length+1)*1.3);
        *buf = realloc(*buf, *max_len);
    }
}

static char* escape_string(char *s) {
    size_t len = strlen(s)+1;
    size_t offset = 0;
    char *buf = malloc(len);
    for (size_t i=0; i<strlen(s); i++) {
        if (strchr(lval_str_escapable, s[i])) {
            char *n = lval_str_escape(s[i]);
            resize_buffer_if_needed(&buf, &len, offset+strlen(n)+1);
            sprintf(buf, "%s", n);
        } else {
            resize_buffer_if_needed(&buf, &len, offset+2);
            sprintf(buf, "%c", s[i]);
        }
    }
    return buf;
}

// Prints a string to the passed buffer, resizing the buffer if needed
static void print_to_buffer(char **buf, size_t *offset, size_t *max_len, char *str)
{
    size_t len = strlen(str);
    resize_buffer_if_needed(buf, max_len, *offset+len);
    sprintf(*buf+*offset, "%s", str);
    *offset += len;
}

// Prints a char to the passed buffer, resizing the buffer if needed
void print_char_to_buffer(char **buf, size_t *offset, size_t *max_len, char c)
{
    resize_buffer_if_needed(buf, max_len, *offset+1);
    (*buf)[*offset] = c;
    *offset = *offset + 1;
}

// Prints a string lval to the passed buffer, resizing the buffer if needed
void lval_sprint_str(const lval *v, char **buf, size_t *offset, size_t *max_len) {
    char *escaped = escape_string(v->val.vstr);
    print_to_buffer(buf, offset, max_len, v->val.vstr);
    free(escaped);
}

// Fwd declaration
void lval_sprint(const lval *v, char **buf, size_t *offset, size_t *max_len, bool quote_strings);

// Prints an expression lval to the passed buffer, resizing the buffer if needed
void lval_expr_sprint(const lval *v, char open, char close, char **buf, size_t *offset, size_t *max_len) {
    if (count(v) == 0) {
        return;
    }
    print_char_to_buffer(buf, offset, max_len, open);
    for (size_t i = 0; i < count(v); i++) {
        lval_sprint(child(v, i), buf, offset, max_len, true);
        if (i != (count(v)-1)) {
            print_char_to_buffer(buf, offset, max_len, ' ');
        }
    }
    print_char_to_buffer(buf, offset, max_len, close);
}

// Prints an lval to the passed buffer, resizing the buffer if needed
void lval_sprint(const lval *v, char **buf, size_t *offset, size_t *max_len, bool quote_strings) {
    switch (v->type) {
        case LVAL_INT:
        {
            static char temp[22];
            sprintf(temp, "%li", v->val.vint);
            print_to_buffer(buf, offset, max_len, temp);
            return;
        }
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
            print_to_buffer(buf, offset, max_len, temp);
            return;
        }
        case LVAL_BYTE:
        {
            static char temp[5];
            sprintf(temp, "0x%02X", v->val.vbyte);
            print_to_buffer(buf, offset, max_len, temp);
            return;
        }
        case LVAL_SYM:
            print_to_buffer(buf, offset, max_len, v->val.vsym.name);
            return;

        case LVAL_FUN:
            if (v->val.vfunc.builtin) {
                print_to_buffer(buf, offset, max_len, builtin_func_string(v->val.vfunc.builtin));
            } else {
                print_to_buffer(buf, offset, max_len, "(\\ ");
                lval_sprint(v->val.vfunc.args, buf, offset, max_len, quote_strings);
                print_char_to_buffer(buf, offset, max_len, ' ');
                lval_sprint(v->val.vfunc.body, buf, offset, max_len, quote_strings);
                print_char_to_buffer(buf, offset, max_len, ')');
            }
            return;
        case LVAL_STR:
            if (quote_strings) {
                print_to_buffer(buf, offset, max_len, "\"");
            }
            lval_sprint_str(v, buf, offset, max_len);
            if (quote_strings) {
                print_to_buffer(buf, offset, max_len, "\"");
            }
            return;
        case LVAL_BUF:
            resize_buffer_if_needed(buf, max_len, (*offset)+2+(v->val.vbuf.size*5));
            (*buf)[*offset] = '<';
            (*offset)++;
            for (size_t i = 0; i<v->val.vbuf.size; i++) {
                sprintf((*buf)+(*offset), "0x%02X", v->val.vbuf.data[i]);
                (*offset) += 4;
                if (i < v->val.vbuf.size-1) {
                    (*buf)[*offset] = ' ';
                    (*offset)++;
                }
            }
            (*buf)[*offset] = '>';
            (*offset)++;
            return;
        case LVAL_SEXPR:
            lval_expr_sprint(v, '(', ')', buf, offset, max_len);
            return;
        case LVAL_QEXPR:
            lval_expr_sprint(v, '{', '}', buf, offset, max_len);
            return;
        case LVAL_CAUGHT_ERR:
        case LVAL_ERR:
        {
            char *tmp = malloc((strlen(v->val.verr.message)+10)*sizeof(char));
            sprintf(tmp, "<Error: %s>", v->val.verr.message);
            print_to_buffer(buf, offset, max_len, tmp);
            free(tmp);
            return;
        }
        case LVAL_TYPE:
            if (v->val.vtype.props) {
                print_char_to_buffer(buf, offset, max_len, '<');
                print_to_buffer(buf, offset, max_len, v->val.vtype.name->val.vsym.name);
                print_char_to_buffer(buf, offset, max_len, ' ');
                size_t max = count(v->val.vtype.props);
                for (size_t i=0; i<max; i++) {
                    lval *p =  child(v->val.vtype.props, i);
                    lval_sprint(p, buf, offset, max_len, true);
                    if (i<max-1) {
                        print_char_to_buffer(buf, offset, max_len, ' ');
                    }
                }
                print_char_to_buffer(buf, offset, max_len, '>');
                return;
            } else {
                print_char_to_buffer(buf, offset, max_len, '<');
                print_to_buffer(buf, offset, max_len, ltype_name(v->val.vtype.primitive));
                print_char_to_buffer(buf, offset, max_len, '>');
            }
            return;
        case LVAL_KEY_VALUE_PAIR:
            print_to_buffer(buf, offset, max_len, v->val.vkvpair.key->val.vsym.name);
            print_char_to_buffer(buf, offset, max_len, ':');
            char *val = lval_to_string(v->val.vkvpair.value);
            print_to_buffer(buf, offset, max_len, val);
            free(val);
            return;
        case LVAL_DICT: {
            print_to_buffer(buf, offset, max_len, "(dict ");
            lval_entry **props = NULL;
            size_t prop_count = lval_table_entries(v->val.vdict, &props);
            for (size_t i=0; i<prop_count; i++) {
                print_to_buffer(buf, offset, max_len, props[i]->key->val.vsym.name);
                print_char_to_buffer(buf, offset, max_len, ':');
                lval_sprint(props[i]->value, buf, offset, max_len, true);
                if (i<prop_count-1) {
                    print_char_to_buffer(buf, offset, max_len, ' ');
                }
            }
            print_char_to_buffer(buf, offset, max_len, ')');
            free(props);
            return;
        }

        case LVAL_CUSTOM_TYPE_INSTANCE: {
            print_char_to_buffer(buf, offset, max_len, '(');
            print_to_buffer(buf, offset, max_len, v->val.vinst.type->val.vtype.name->val.vsym.name);
            print_char_to_buffer(buf, offset, max_len, ' ');
            lval_entry **props = NULL;
            size_t prop_count = lval_table_entries(v->val.vinst.props, &props);
            for (size_t i=0; i<prop_count; i++) {
                print_to_buffer(buf, offset, max_len, props[i]->key->val.vsym.name);
                print_char_to_buffer(buf, offset, max_len, ':');
                lval_sprint(props[i]->value, buf, offset, max_len, true);
                if (i<prop_count-1) {
                    print_char_to_buffer(buf, offset, max_len, ' ');
                }
            }
            print_char_to_buffer(buf, offset, max_len, ')');
            free(props);
            return;
        }
    }
}
