// Parser for benzl code
// Turns a string into a set of benzl expressions
//
// Part of benzl - https://github.com/pokeb/benzl

#include <stddef.h>

#include "benzl-lval.h"

// Takes a string and returns an S-Expression representing benzl code that can
// be evaluated
lval* lval_read_expr(char *s, size_t *i, char end, lval *source_file);

// Attempts to convert a string to a number
// Returns NULL if the string doesn't seem to be a number
// or an error if it does but the conversion failed
lval* string_to_number(char *string);
