// Defines functions for evaluating a lval
//
// Part of benzl - https://github.com/pokeb/benzl

#pragma once

// Forward declarations
typedef struct lval lval;
typedef struct lenv lenv;

// Evaluates the passed lval
lval* lval_eval(lenv *e, const lval *v);

// Evalulates the passed s-expression lval
// Can also be used to evaluate q-expressions as if they were s-expressions
lval* lval_eval_sexpr(lenv *e, const lval *v);

// Call the function f with argument list a
lval* lval_call(lenv *e, const lval *f, const lval *a);

// Prints stats on the number of times each named function has been called
// Does nothing if LOG_CALL_STATS is 0
void print_call_count_stats(void);
