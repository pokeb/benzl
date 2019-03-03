// Internal functions for generating stack traces
// (When an unhandled error occurs, these are printed out)
//
// Part of benzl - https://github.com/pokeb/benzl

#include "benzl-lval.h"

#pragma once

// Record that we pushed an expression onto the stack
void stack_push_frame(const lval *v);

// Record that we popped the last expression from the stack
void stack_pop_frame(void);

// Returns a stack trace
lval* stack_trace(const lval *a);

// Prints out an error, including the stack trace if one is available
void print_error_with_trace(const lval *err);

// Cleans up the stack
void stack_cleanup(void);
