// This file defines functions used for printing lvals to a string buffer
//
// Part of benzl - https://github.com/pokeb/benzl

#include <stddef.h>
#include <stdbool.h>

#include "benzl-lval.h"

#pragma mark - Buffer operations

// Helper function that resizes the passed buffer if smaller than wanted_length
void resize_buffer_if_needed(char **buf, size_t *max_len,
                             size_t wanted_length);

#pragma mark - Escaping / Unescaping strings

// Possible unescapable characters
extern char* lval_str_unescapable;

// List of possible escapable characters
extern char* lval_str_escapable;

// Function to unescape characters
static inline char lval_str_unescape(char x) {
    switch (x) {
        case 'a':  return '\a';
        case 'b':  return '\b';
        case 'f':  return '\f';
        case 'n':  return '\n';
        case 'r':  return '\r';
        case 't':  return '\t';
        case 'v':  return '\v';
        case '\\': return '\\';
        case '\'': return '\'';
        case '\"': return '\"';
    }
    return '\0';
}

/* Function to escape characters */
static inline char* lval_str_escape(char x) {
    switch (x) {
        case '\a': return "\\a";
        case '\b': return "\\b";
        case '\f': return "\\f";
        case '\n': return "\\n";
        case '\r': return "\\r";
        case '\t': return "\\t";
        case '\v': return "\\v";
        case '\\': return "\\\\";
        case '\'': return "\\\'";
        case '\"': return "\\\"";
    }
    return "";
}

// Prints an lval to the passed buffer, resizing the buffer if needed
void lval_sprint(const lval *v, char **buf, size_t *offset,
                 size_t *max_len, bool quote_strings);
