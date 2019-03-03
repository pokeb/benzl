// Part of benzl - https://github.com/pokeb/benzl

#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "benzl-parse.h"
#include "benzl-sprintf.h"

lval* string_to_number(char *string) {

    size_t len = strlen(string);
    if (len == 0) {
        return NULL;
    }

    // Check if this symbol is an integer in hex
    if (len > 3 && len < 11 && string[0] == '0' && string[1] == 'x') {
        long x = strtol(string+2, NULL, 16);
        if (errno == ERANGE) {
            return lval_err("Invalid number '%s'", string);
        } else if (x < 256) {
            return lval_byte((uint8_t)x);
        } else {
            return lval_int(x);
        }

        // Check if this symbol is a float or integer
    } else if (strchr("-0123456789", string[0])) {

        bool is_number = true;
        bool is_float = false;
        if (string[0] == '-' && strlen(string) == 1) {
            is_number = false;
        } else {
            for (size_t i=1; i< strlen(string); i++) {
                is_number = true;
                if (string[i] == '.') {
                    is_float = true;
                } else if (!strchr("0123456789", string[i])) {
                    is_number = false;
                    break;
                }
            }
        }
        if (is_number) {
            errno = 0;
            if (is_float) {
                double x = strtod(string, NULL);
                if (errno == ERANGE) {
                    return lval_err("Invalid float '%s'", string);
                } else {
                    return lval_float(x);
                }
            } else {
                long x = strtol(string, NULL, 10);
                if (errno == ERANGE) {
                    return lval_err("Invalid integer '%s'", string);
                } else {
                    return lval_int(x);
                }
            }
        }
    }
    return NULL;
}

size_t read_sym(lval *v, char *s, size_t i, code_pos *pos) {

    lval *n = NULL;
    char *part = calloc(1,1);
    while (strchr("abcdefghijklmnopqrstuvwxyz"
                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                  "0123456789._+-*\\/=<>!&%^|", s[i]) && s[i] != '\0') {
        part = realloc(part, strlen(part)+2);
        part[strlen(part)+1] = '\0';
        part[strlen(part)+0] = s[i];
        i++;
    }

    // Convert to a number if possible
    n = string_to_number(part);

    // Check if this symbol is a built-in type
    if (n == NULL) {
        for (lval_type i=0; i<15; i++) {
            if (strcmp(part, ltype_name(i)) == 0) {
                n = lval_primitive_type(i);
                break;
            }
        }
    }
    if (n == NULL) {
        n = lval_sym(part);
    }
    n->source_position = code_pos_retain(*pos);
    lval_add(v, n);
    lval_release(n);

    free(part);
    return i;
}


size_t read_str(lval *v, char *s, size_t i, char end, code_pos *pos) {

    char *part = calloc(1,1);

    while (s[i] != end) {
        char c = s[i];
        if (c == '\0') {
            lval *err = lval_err("Unexpected end of input in string literal");
            lval_add(v, err);
            lval_release(err);
            free(part);
            return strlen(s);
        }
        if (c == '\\') {
            i++;
            if (strchr(lval_str_unescapable, s[i])) {
                c = lval_str_unescape(s[i]);
            } else {
                i--;
            }
        }

        part = realloc(part, strlen(part)+2);
        part[strlen(part)+1] = '\0';
        part[strlen(part)+0] = c;
        i++;
    }

    lval *n = lval_str(part);
    n->source_position = code_pos_retain(*pos);
    lval_add(v, n);
    lval_release(n);
    free(part);
    return i+1;
}

size_t read_expr(lval *v, char *s, size_t i, char end, size_t len, code_pos *pos) {

    while (i < len && s[i] != end) {

        if (s[i] == '\n') {
            pos->row++;
            pos->col = 0;
            i++;
            continue;
        }
        pos->col++;

        // Whitespace
        if (strchr(" \t\v\r", s[i])) {
            i++;
            continue;
        }

        // Comment
        if (s[i] == ';') {
            while (s[i] != '\n' && s[i] != '\0') {
                i++;
            }
            continue;
        }

        // S-Expression
        if (s[i] == '(') {
            lval *x = lval_sexpr();
            x->source_position = code_pos_retain(*pos);
            lval_add(v, x);
            lval_release(x);
            i = read_expr(x, s, i+1, ')', len, pos);
            continue;
        }

        // Q-Expression
        if (s[i] == '{') {
            lval *x = lval_qexpr();
            x->source_position = code_pos_retain(*pos);
            lval_add(v, x);
            lval_release(x);
            i = read_expr(x, s, i+1, '}', len, pos);
            continue;
        }

        // Key-Value Separator
        if (s[i] == ':') {
            lval *tmp = lval_qexpr();
            tmp->source_position = code_pos_retain(*pos);

            i = read_expr(tmp, s, i+1, end, len, pos);
            i--;
            // Grab the key that proceeded the colon
            lval *key = lval_retain(child(v, count(v)-1));
            // Remove the key from the parent as we want it part of the pair instead
            lval_pop(v, count(v)-1);
            lval *val = child(tmp, 0);

            // Make sure the key is a symbol
            if (key->type != LVAL_SYM) {
                char *ks = lval_to_string(key);
                char *vs = lval_to_string(val);
                lval *err = lval_err("Encountered unexpected key:value pair '%s:%s'", ks, vs);
                free(ks);
                free(vs);
                err->source_position = code_pos_retain(*pos);
                lval_add(v, err);
                lval_release(err);
                lval_release(key);
                lval_release(tmp);
                return strlen(s)+1;
            }

            lval *x = lval_kv_pair(key, val);
            lval_add(v, x);
            lval_release(x);
            lval_release(key);
            for (size_t i2=1; i2<count(tmp); i2++) {
                lval_add(v, child(tmp, i2));
            }
            lval_release(tmp);
            continue;
        }

        // Symbol
        if (strchr("abcdefghijklmnopqrstuvwxyz"
                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                   "0123456789._+-*\\/=<>!&%^|", s[i])) {
            i = read_sym(v, s, i, pos);
            continue;
        }

        //String
        if (strchr("\"", s[i]) || strchr("'", s[i])) {
            i = read_str(v, s, i+1, s[i], pos);
            continue;
        }

        // Shebang (we ignore this)
        if (i==0 && s[i] == '#' && s[i+1] == '!') {
            i+=2;
            while (s[i] != '\n') {
                i++;
            }
            continue;
        }

        // Something else
        lval *err = lval_err("Unknown character '%c'", s[i]);
        err->source_position = code_pos_retain(*pos);

        lval_add(v, err);
        lval_release(err);
        return strlen(s)+1;
    }
    // If we reach the end of input then it's a syntax error
    if (i == len && end != '\0') {
        lval *err = lval_err("Missing '%c' at end of input", end);
        err->source_position = code_pos_retain(*pos);

        lval_add(v, err);
        lval_release(err);
    }
    return i+1;
}

lval* lval_read_expr(char *s, size_t *i, char end, lval *source_file)
{
    lval *v = lval_sexpr();
    size_t len = strlen(s);
    code_pos pos = (code_pos){0};
    if (source_file != NULL) {
        pos.source_file = lval_retain(source_file);
    }
    v->source_position = pos;
    read_expr(v, s, *i, end, len, &pos);
    return v;
}
