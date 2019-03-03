// Macros used for validating parameters passed to built-in functions
//
// Part of benzl - https://github.com/pokeb/benzl

#include "benzl-lval.h"

// Internal function for creating an s-expression in the form (func params)
// Used for debug printing
static inline lval* _debug_func_exp(char *func_name, const lval *a) {
    lval *sexp = lval_sexpr_with_size(2);
    lval *fname = lval_sym(func_name);
    lval_add(sexp, fname);
    lval_add(sexp, a);
    lval_release(fname);
    return sexp;
}

// Returns an error if the passed condition fails
// Including information about the passed lval to explain the problem
#define LASSERTV(_a, _func_name, _cond, _fmt, ...) \
if (!(_cond)) { \
    lval *__tmp = _debug_func_exp(_func_name, _a); \
    lval *__err_lval = lval_err_for_val(__tmp, _fmt, ##__VA_ARGS__); \
    lval_release(__tmp); \
    return __err_lval; \
}

// Returns an error if the passed lval has the wrong number of arguments
#define LASSERT_NUM_ARGS(_func_name, _a, _num_required) {\
LASSERTV(_a, _func_name, count(_a) == _num_required, \
"Function '%s' passed wrong number of arguments (Got: %d Expected: %d)", \
_func_name, count(_a), _num_required); }

// Returns an error if the passed lval has the the wrong type for a given argument
#define LASSERT_ARG_TYPE(_func_name, _a, _index, _expected_type) {\
LASSERTV(_a, _func_name, child(_a, _index)->type == _expected_type, \
"Function '%s' passed incorrect type for arg %d (Got: %s Expected: %s)", \
_func_name, _index, ltype_name(child(_a, _index)->type), ltype_name(_expected_type)); }

// Returns an error if the passed lval has an empty expression for a given argument
#define LASSERT_NOT_EMPTY(_func_name, _a, _index) {\
LASSERTV(_a, _func_name, child(_a, _index)->val.vexp.count != 0, \
"Function '%s' passed {} for argument %d", \
_func_name, _index); }
