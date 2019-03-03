// lvals are the basic building blocks of benzl. Both benzl code and data
// are described using lvals - code is parsed into a set of lvals, and each is
// evaluated to produce the result.
// Each lval has a type to indicate what kind of value it represents.
// These include numbers (Integer, Float, Byte), arrays of bytes (Buffer, String),
// lists and S-Expressions, dictionaries, functions, errors, type references
// and instances of custom types.
// lvals with a symbol type represent the name of a bound variable or function:
// during evaluation, benzl will lookup the value that that symbol represents
// in the environment, or raise an error if that symbol doesn't have a value
// bound to it

#pragma once

#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <stddef.h>

#include "benzl-hash-table.h"

#pragma mark - Type definitions

// Forward declarations
typedef struct lval lval;
typedef struct lenv lenv;

// Type representing a kind of value an lval can store
typedef enum {
    LVAL_INT = 0, // Integer
    LVAL_FLT = 1, // Float
    LVAL_BYTE = 2, // Single byte
    LVAL_SYM = 3, // Symbol
    LVAL_STR = 4, // String
    LVAL_BUF = 5, // Buffer
    LVAL_DICT = 6, // Dictionary
    LVAL_FUN = 7, // Function
    LVAL_SEXPR = 8, // S-Expression
    LVAL_QEXPR = 9, // Q-Expression/List (Not automatically evalulated)
    LVAL_ERR = 10, // Error (returning one of these will stop evalution)
    LVAL_CAUGHT_ERR = 11, // Caught error (Used for try/catch)
    LVAL_TYPE = 12, // Reference to a type eg Integer, MyCustomType
    LVAL_CUSTOM_TYPE_INSTANCE = 13, // Instance of a custom type (struct)
    LVAL_KEY_VALUE_PAIR = 14, // In the form 'key:value' (Used internally only)
} lval_type;

// Human-readable name of an lval type (Used in errors)
static inline char* ltype_name(lval_type t) {
    if (t >= LVAL_INT && t <= LVAL_KEY_VALUE_PAIR) {
        static char *names[15] = {
            "Integer", "Float", "Byte", "Symbol", "String", "Buffer",
            "Dictionary", "Function", "S-Expression", "List", "UnhandledError",
            "Error", "Type", "CustomTypeInstance", "KeyValuePair"
        };
        return names[t];
    }
    return "<Unknown type>";
}

// Function pointer type for built-in functions
typedef struct lval*(*lbuiltin)(struct lenv*, const struct lval*);

// Properties stored in a lval for a buffer
typedef struct {
    size_t size;
    uint8_t *data;
} vbuf;

// Properties stored in an lval for a function
typedef struct {
    // For built-in functions
    lbuiltin builtin; // Pointer to the c function

    // For user defined functions
    lval *args; // List of arguments to the function
    lval *body; // Body of the function
} vfunc;

// Properties stored in an lval for an expression
// (S-Expressions and Q-Expressions)
typedef struct {
    size_t count; // Number of items in the list/expression
    size_t allocated_size; // Number of items we have space for without realloc
    struct lval ** cell; // Items in the list/expression
} vexp;

// Properties stored in an lval representing a type
typedef struct {
    // For primitive types (where props == NULL)
    lval_type primitive; // Built-in type this type reference refers to

    // For user-defined types (structs)
    lval *name; // Name of the custom type
    lval *props; // List of properties of the custom type
} vtype;

// Properties stored in an lval representing a symbol
// We pre-compute the hash (used for looking up the value
// in the environment's hash table) when we create a symbol
typedef struct {
    char *name;
    size_t hash;
} vsym;

// Properties stored in an lval representing a key-value pair
// These are only used internally in benzl for representing things like
// 'parameter1:type parameter2:type' after parsing
typedef struct {
    lval *key;
    lval *value;
} vkvpair;

// Properties stored in an lval representing an instance of a custom type
typedef struct {
    lval *type; // Type of this instance
    lval_table *props; // Values defined in this instance for the type's keys
} vcustom_type_instance;

// Properties stored in a lvl representing an error (or caught error)
typedef struct {
    // Error message
    char *message;
    // NULL or a string lval containing a stack trace
    lval *stack_trace;
} verr;

// Union type for storing properties of lvals unique to each type
typedef union {
    long vint; // Integer value
    double vflt; // Float value
    uint8_t vbyte; // Single byte value
    char *vstr; // String value
    vsym vsym; // Symbol value
    verr verr; // Error value
    vbuf vbuf; // Buffer value
    lval_table *vdict; // Dictionary value
    vfunc vfunc; // Function value
    vexp vexp; // S/Q-Expression value
    vtype vtype; // Type definition value
    vkvpair vkvpair; // Property with type
    vcustom_type_instance vinst; // Instance of custom type
} vval;

// A reference to location of this value in the source file
typedef struct {
    int row;
    int col;
    lval *source_file;
} code_pos;

// Represents a type of value we can use in our programs
struct lval {
    lval_type type; // Type of value
    code_pos source_position; // Line / Col number in source code
    int ref_count; // Reference count
    lval *bound_name; // Name bound to this value, if applicable
    vval val; // Actual value (stores different things depending on type)
};

// Helper function to return the name of a type
static inline char* name_for_type(const vtype type) {
    if (type.props == NULL) {
        return ltype_name(type.primitive);
    } else {
        return type.name->val.vsym.name;
    }
}

// Helper function to determine if two symbols are equal
// Since symbols always have a hash, we can look at that first
static inline bool equal_symbols(const lval *k1, const lval *k2)
{
    assert(k1->type == LVAL_SYM && k2->type == LVAL_SYM);
    return k1->val.vsym.hash == k2->val.vsym.hash &&
    strcmp(k1->val.vsym.name, k2->val.vsym.name) == 0;
}

// Helper function to return the name this value was bound to in the environment
// (Used in errors)
static inline char* bound_name_for_lval(const lval *v)
{
    if (v->bound_name != NULL) {
        return v->bound_name->val.vsym.name;
    }
    return "<Unnamed>";
}

#pragma mark - Constructors

// Create a new lval representing an integer
lval* lval_int(long x);

// Create a new lval representing a float
lval* lval_float(double x);

// Create a new lval representing a byte
lval* lval_byte(uint8_t x);

// Create a new lval representing an error
lval* lval_err(char *fmt, ...);

// Create a new error lval
// with an error message referring to a problem value
// that includes line/column number
lval* lval_err_for_val(const lval *v, char *fmt, ...);

// Create a new lval representing a symbol
lval* lval_sym(char *s);

// Create a new lval representing a string
lval* lval_str(char *s);

// Create a new lval representing a buffer
lval* lval_buf(size_t size);

// Create a new lval representing a dictionary (hash table)
lval* lval_dict(size_t bucket_count);

// Create a new lval representing an s-expression
lval* lval_sexpr(void);

// Same as above, but with size slots preallocated for children
lval* lval_sexpr_with_size(size_t size);

// Create a new lval representing an q-expression (unevaluated list)
lval* lval_qexpr(void);

// Same as above, but with size slots preallocated for children
lval* lval_qexpr_with_size(size_t size);

// Create a new lval representing a built-in function
lval* lval_fun(lbuiltin func);

// Create a new lval representing a user-defined function / lambda
lval* lval_lambda(lval *params, lval *body);

// Create a new lval representing a primitive type
lval* lval_primitive_type(lval_type type);

// Create a new lval representing a custom type
lval* lval_custom_type(lval *name, lval *props);

lval* lval_kv_pair(lval *key, lval *value);

// Create a new lval representing a typed custom property
// eg: (Point 1 2 3)
lval* lval_custom_type_instance(const lval *type, const lval *props);

#pragma mark - Casting

// Change the type of the passed val, converting its current value
// Returns NULL if the conversion was not possible
lval* cast_to(const lval *v, lval_type t);

lval *cast_list_to_type(const lval *l, lval_type type);

#pragma mark - Type checking

// Returns true for Integers, Floats and Bytes
bool lval_is_number(const lval *v);

// Creates a type reference from the value of a KVPair
lval* type_from_pair(lenv *e, const lval *v);

// Returns true if the passed types are equal
// bool equal_types(vtype *t1, vtype *t2);

// Returns true if the passed value has the passed type
// (or can be cast to that type)
bool value_matches_type(lenv *e, const lval *v, const lval *type, lval **cast_value);

// Returns a string for use in errors describing a type mismatch
char* type_mismatch_description(vtype *wanted, const lval *v);

#pragma mark - lval utility functions

// Add a child to this lval
lval* lval_add(lval *v, const lval *x);

// Removes a child from this lval and return it
lval* lval_pop(lval *v, size_t i);

// Returns a deep copy of this lval
lval* lval_copy(const lval *v);

// Returns 1 if two passed lvals are equal to each other, 0 otherwise
int lval_eq(const lval *x, const lval *y);

// Number of children in a S-Expression or Q-Expressions
// This inline function provides additional santity checks
// when assertions are enabled
static inline size_t count(const lval *v)
{
    assert(v->type == LVAL_SEXPR || v->type == LVAL_QEXPR);
    return v->val.vexp.count;
}

// Fetch a child from an S-Expression or Q-Expression
// This inline function provides additional santity checks
// when assertions are enabled
static inline lval* child(const lval *v, const size_t i)
{
    assert(v->type == LVAL_SEXPR || v->type == LVAL_QEXPR);
    assert(i < count(v));
    return v->val.vexp.cell[i];
}

#pragma mark - Debugging helpers

// Get a string representation of the lval
char* lval_to_string(const lval *v);

// Print the value to the console
void lval_print(const lval *v);

// Print a value to the console followed by a line break
void lval_println(const lval *v);

#pragma mark - Allocating and freeing lvals in the shared global pool

// Get an lval from the shared pool
lval* lval_alloc(void);

// Return an lval to the shared pool
void lval_free(lval *v);

#pragma mark - Reference counting

// Increments ref_count for an lval
// Note: we treat lval_retain() as a special case:
// v is const because we often have const lvals from builtin functions
// but this mutation is safe
static inline lval* lval_retain(const lval *v) {
    assert(v != NULL);
    lval *v2 = (lval *)v;
    v2->ref_count++;
    return v2;
}

// Decrements ref_count for an lval
// When ref_count is zero, the value is freed back to the pool
static inline lval* lval_release(lval *v) {
    assert(v != NULL);
    assert(v->ref_count > 0);
    v->ref_count--;
    if (v->ref_count == 0) {
        lval_free(v);
    }
    return v;
}

// Helper for copying a code_pos
static inline code_pos code_pos_retain(code_pos pos)
{
    return (code_pos){
        .row = pos.row,
        .col = pos.col,
        .source_file = (pos.source_file != NULL) ?
        lval_retain(pos.source_file) :
        NULL
    };
}

// Helper for releasing a code_pos
static inline void code_pos_release(code_pos pos)
{
    if (pos.source_file != NULL) {
        lval_release(pos.source_file);
    }
}


#define MAX(x,y) (x > y ? x : y)
#define MIN(x,y) (x < y ? x : y)
