// The lenv type describes an enviroment. Enviroments store a set of bound
// variables and functions.
// For example, evaluating (my-function 1 2 3) will look up the definition of
// 'my-function' in the environment to find the actual function to be called
// Environments store the list of bound variables and functions in a hash table
// If a value is not bound in the current environment, benzl will look in the
// parent environment until it is at the top level environment
//
// Part of benzl - https://github.com/pokeb/benzl

#pragma once

#include <stddef.h>
#include "benzl-hash-table.h"

// Forward declarations
typedef struct lval lval;
typedef struct lenv lenv;

// This type represents an environment
// A new environment is created when we call a function, for example
// Environments store bound variables and functions
struct lenv {
    lenv *parent;
    lval_table *items;
    char *script_path;
    lval_table *loaded_modules;
};

// Constructor
lenv* lenv_alloc(size_t bucket_count);

// Destructor
void lenv_free(lenv *e);

// Returns a copy of the environment
lenv* lenv_copy(const lenv *e);

// Get a value from the environment
lval* lenv_get(lenv *e, const lval *k);

// Put a value into the environment, assuming the name is not bound
lval* lenv_def(lenv *e, const lval *k, const lval *v);

lval* lenv_def_with_type(lenv *e, const lval *k, const lval *v, const lval *t);

// Put a value into the environment, assuming the name is already bound
lval* lenv_set(lenv *e, const lval *k, const lval *v);

// Put a value into the environment (only used internally)
lval* lenv_def_or_set(lenv *e, const lval *k, const lval *v);

// Used for recording which scripts have already been loaded
void record_module_loaded(lenv *e, char *module_path);

// Check if a script has already been loaded
bool is_module_already_loaded(lenv *e, char *module_path);
