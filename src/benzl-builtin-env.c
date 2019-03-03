// This file implements built-in functions for getting and setting variables,
// and setting properties on custom types
//
// Part of benzl - https://github.com/pokeb/benzl

#include <stddef.h>

#include "benzl-builtins.h"
#include "benzl-lval.h"
#include "benzl-lval-eval.h"
#include "benzl-lenv.h"

// Type representing an action for setting a variable
typedef enum {
    var_action_define = 0, // Define a new variable and set value
    var_action_set = 1 // Set the value of a pre-existing variable
} var_action;

// Human-readable name of a variable set operation (used in errors)
static inline const char* var_action_to_string(var_action t)
{
    if (t >= var_action_define && t <= var_action_set) {
        const char *names[2] = {"def", "set"};
        return names[t];
    }
    return "<Unknown set operation>";
}

lval* builtin_var(lenv *e, const lval *a, var_action action) {

    lval *syms = child(a, 0);

    for (size_t i=0; i < count(syms); i++) {
        if (child(syms, i)->type != LVAL_SYM &&
            child(syms, i)->type != LVAL_KEY_VALUE_PAIR) {
            return lval_err_for_val(
                a, "%s cannot define non-symbol'",
                var_action_to_string(action)
            );
        }
    }
    if (count(syms) != count(a)-1) {
        return lval_err_for_val(
            a, "%s cannot define incorrect number of values to symbols",
            var_action_to_string(action)
        );
    }

    lval *err = NULL;
    for (size_t i=0; i<count(syms); i++) {
        lval *name = child(syms, i);
        lval *val = child(a, i+1);

        if (action == var_action_define) {
            if (name->type == LVAL_KEY_VALUE_PAIR) {
                lval *type = type_from_pair(e, name);
                if (type->type == LVAL_ERR) {
                    char *s = lval_to_string(name->val.vkvpair.value);
                    lval *err = lval_err_for_val(
                        a, "Variable '%s': Invalid type '%s'",
                        name->val.vkvpair.key->val.vsym.name, s
                    );
                    free(s);
                    lval_release(type);
                    return err;
                }

                lval *cast_val = NULL;
                if (!value_matches_type(e, val, type, &cast_val)) {
                    char *s = type_mismatch_description(&type->val.vtype,
                                                        val);
                    lval *err = lval_err_for_val(a, "Variable '%s': %s",
                                                 name->val.vsym.name, s);
                    free(s);
                    return err;
                }

                name = lval_retain(name->val.vkvpair.key);
                if (cast_val != NULL) {
                    err = lenv_def_with_type(e, name, cast_val, type);
                    lval_release(cast_val);
                } else {
                    err = lenv_def_with_type(e, name, val, type);
                }
                lval_release(name);
                lval_release(type);
            } else {
                err = lenv_def(e, name, val);
            }
        } else if (action == var_action_set) {
            lenv *env = e;
            lval_entry *entry = NULL;
            while (env != NULL) {
                entry = lval_table_get_entry(env->items, name);
                if (entry == NULL) {
                    env = env->parent;
                } else {
                    break;
                }
            }
            lval *cast_val = NULL;
            if (entry != NULL && entry->type != NULL &&
                !value_matches_type(e, val, entry->type, &cast_val)) {
                char *s = type_mismatch_description(&entry->type->val.vtype,
                                                    val);
                lval *err = lval_err_for_val(a, "Variable '%s': %s",
                                             name->val.vsym.name, s);
                free(s);
                return err;
            }
            if (cast_val != NULL) {
                err = lenv_set(e, name, cast_val);
                lval_release(cast_val);
            } else {
                err = lenv_set(e, name, val);

            }
        }
    }
    if (err != NULL) {
        return err;
    }
    return lval_sexpr();
}

lval* builtin_def(lenv *e, const lval *a) {
    return builtin_var(e, a, var_action_define);
}

lval* builtin_set(lenv *e, const lval *a) {
    return builtin_var(e, a, var_action_set);
}

lval* builtin_set_prop(lenv *e, const lval *a) {

    lval *syms = child(a, 0);
    if (count(syms) != 2 || count(a) != 2) {
        return lval_err_for_val(a, "set-prop takes arguments in the form "
                                   "(set-prop {obj prop} value)'");
    }
    for (size_t i=1; i < count(syms); i++) {
        if (child(syms, i)->type != LVAL_SYM) {
            return lval_err_for_val(a, "set-prop cannot define non-symbol'");
        }
    }

    lval *obj = lval_eval(e, child(syms, 0));

    if (obj->type == LVAL_ERR) {
        return obj;
    } else if (obj->type == LVAL_DICT) {
        lval *prop_name = child(syms, 1);
        lval_table_insert(obj->val.vdict, prop_name, child(a, 1));
        return obj;

    } else if (obj->type != LVAL_CUSTOM_TYPE_INSTANCE) {
        char *v = lval_to_string(obj);
        lval *err = lval_err_for_val(a, "Cannot call set-prop on '%s'", v);
        free(v);
        return err;
    }

    lval *prop_name = child(syms, 1);

    lval *type_props = obj->val.vinst.type->val.vtype.props;
    bool found = false;
    for (size_t i=0; i<count(type_props); i++) {
        lval *prop = child(type_props, i);
        if (prop->type == LVAL_KEY_VALUE_PAIR) {
            prop = prop->val.vkvpair.key;
        }
        if (equal_symbols(prop, prop_name)) {
            found = true;
            break;
        }
    }
    if (!found) {
        lval *err = lval_err_for_val(a, "set-prop: %s has no property %s'",
                                    obj->val.vinst.type->val.vtype.name,
                                    prop_name->val.vsym);
        lval_release(obj);
        return err;
    }
    lval_table_insert(obj->val.vinst.props, prop_name, child(a, 1));
    return obj;
}

static void lenv_add_builtin(lenv *e, char *name, lbuiltin func) {
    lval *sym = lval_sym(name);
    lval *fun = lval_fun(func);
    lenv_def(e, sym, fun);
    lval_release(sym);
    lval_release(fun);
}

void lenv_add_builtins(lenv *e) {

    // Variable functions
    lenv_add_builtin(e, "def", builtin_def);
    lenv_add_builtin(e, "set", builtin_set);
    lenv_add_builtin(e, "set-prop", builtin_set_prop);

    // User defined functions
    lenv_add_builtin(e, "lambda", builtin_lambda);
    lenv_add_builtin(e, "fun", builtin_fun);

    // List / String functions
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "len", builtin_len);
    lenv_add_builtin(e, "drop", builtin_drop);
    lenv_add_builtin(e, "take", builtin_take);
    lenv_add_builtin(e, "first", builtin_first);
    lenv_add_builtin(e, "second", builtin_second);
    lenv_add_builtin(e, "last", builtin_last);
    lenv_add_builtin(e, "nth", builtin_nth);


    // Mathematical functions
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_subtract);
    lenv_add_builtin(e, "*", builtin_multiply);
    lenv_add_builtin(e, "/", builtin_divide);
    lenv_add_builtin(e, "%", builtin_modulo);
    lenv_add_builtin(e, ">>", builtin_right_shift);
    lenv_add_builtin(e, "<<", builtin_left_shift);
    lenv_add_builtin(e, "&", builtin_bitwise_and);
    lenv_add_builtin(e, "|", builtin_bitwise_or);
    lenv_add_builtin(e, "^", builtin_bitwise_xor);

    lenv_add_builtin(e, "min", builtin_min);
    lenv_add_builtin(e, "max", builtin_max);

    // Comparison functions
    lenv_add_builtin(e, "if", builtin_if);
    lenv_add_builtin(e, ">", builtin_greater_than);
    lenv_add_builtin(e, "<", builtin_less_than);
    lenv_add_builtin(e, ">=", builtin_greater_than_or_equal);
    lenv_add_builtin(e, "<=", builtin_less_than_or_equal);
    lenv_add_builtin(e, "==", builtin_equal);
    lenv_add_builtin(e, "!=", builtin_not_equal);

    // Errors
    lenv_add_builtin(e, "error", builtin_error);
    lenv_add_builtin(e, "try", builtin_try);

    // Type conversion functions
    lenv_add_builtin(e, "floor", builtin_floor);
    lenv_add_builtin(e, "ceil", builtin_ceil);

    // Logical functions
    lenv_add_builtin(e, "or", builtin_logical_or);
    lenv_add_builtin(e, "and", builtin_logical_and);
    lenv_add_builtin(e, "not", builtin_logical_not);

    // Buffers
    lenv_add_builtin(e, "create-buffer", builtin_create_buffer);
    lenv_add_builtin(e, "buffer-with-bytes", builtin_buffer_with_bytes);
    lenv_add_builtin(e, "buffer-map", builtin_buffer_map);

    lenv_add_builtin(e, "put-byte", builtin_put_byte);
    lenv_add_builtin(e, "get-byte", builtin_get_byte);
    lenv_add_builtin(e, "put-unsigned-char", builtin_put_unsigned_char);
    lenv_add_builtin(e, "get-unsigned-char", builtin_get_unsigned_char);
    lenv_add_builtin(e, "put-signed-char", builtin_put_signed_char);
    lenv_add_builtin(e, "get-signed-char", builtin_get_signed_char);
    lenv_add_builtin(e, "put-unsigned-short", builtin_put_unsigned_short);
    lenv_add_builtin(e, "get-unsigned-short", builtin_get_unsigned_short);
    lenv_add_builtin(e, "put-signed-short", builtin_put_signed_short);
    lenv_add_builtin(e, "get-signed-short", builtin_get_signed_short);
    lenv_add_builtin(e, "put-unsigned-integer", builtin_put_unsigned_integer);
    lenv_add_builtin(e, "get-unsigned-integer", builtin_get_unsigned_integer);
    lenv_add_builtin(e, "put-signed-integer", builtin_put_signed_integer);
    lenv_add_builtin(e, "get-signed-integer", builtin_get_signed_integer);
    lenv_add_builtin(e, "get-unsigned-long", builtin_get_unsigned_long);
    lenv_add_builtin(e, "put-unsigned-long", builtin_put_unsigned_long);
    lenv_add_builtin(e, "get-signed-long", builtin_get_signed_long);
    lenv_add_builtin(e, "put-signed-long", builtin_put_signed_long);
    lenv_add_builtin(e, "put-string", builtin_put_string);
    lenv_add_builtin(e, "get-string", builtin_get_string);
    lenv_add_builtin(e, "put-bytes", builtin_put_bytes);
    lenv_add_builtin(e, "get-bytes", builtin_get_bytes);


    // String format
    lenv_add_builtin(e, "print", builtin_print);
    lenv_add_builtin(e, "format", builtin_format);
    lenv_add_builtin(e, "printf", builtin_printf);

    // Evaluation
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "eval-string", builtin_eval_string);
    lenv_add_builtin(e, "load", builtin_load);

    // Type functions
    lenv_add_builtin(e, "type-of", builtin_type_of);
    lenv_add_builtin(e, "def-type", builtin_def_type);
    lenv_add_builtin(e, "to-string", builtin_to_string);
    lenv_add_builtin(e, "to-number", builtin_to_number);

    // Dictionary functions
    lenv_add_builtin(e, "dict", builtin_dictionary);

    // File functions
    lenv_add_builtin(e, "read-file", builtin_read_file);
    lenv_add_builtin(e, "write-file", builtin_write_file);

    // Time
    lenv_add_builtin(e, "cpu-time-since", builtin_cpu_time_since);

    // Misc
    lenv_add_builtin(e, "console-size", builtin_console_size);
    lenv_add_builtin(e, "version", builtin_version);
    lenv_add_builtin(e, "print-env", builtin_print_env);
    lenv_add_builtin(e, "exit", builtin_exit);
}

// Returns the name of the passed function for debug printing
char* builtin_func_string(lbuiltin func) {
    if (func == builtin_head) {
        return "head";
    } else if (func == builtin_tail) {
        return "tail";
    } else if (func == builtin_list) {
        return "list";
    } else if (func == builtin_drop) {
        return "drop";
    } else if (func == builtin_take) {
        return "take";
    } else if (func == builtin_first) {
        return "first";
    } else if (func == builtin_second) {
        return "second";
    } else if (func == builtin_last) {
        return "last";
    } else if (func == builtin_eval) {
        return "eval";
    } else if (func == builtin_eval_string) {
        return "eval-string";
    } else if (func == builtin_join) {
        return "join";
    } else if (func == builtin_add) {
        return "+";
    } else if (func == builtin_subtract) {
        return "-";
    } else if (func == builtin_multiply) {
        return "*";
    } else if (func == builtin_divide) {
        return "/";
    } else if (func == builtin_modulo) {
        return "%";
    } else if (func == builtin_min) {
        return "min";
    } else if (func == builtin_max) {
        return "max";
    } else if (func == builtin_def) {
        return "def";
    } else if (func == builtin_set) {
        return "set";
    } else if (func == builtin_set_prop) {
        return "set-prop";
    } else if (func == builtin_lambda) {
        return "lambda";
    } else if (func == builtin_if) {
        return "if";
    } else if (func == builtin_fun) {
        return "fun";
    } else if (func == builtin_try) {
        return "try";
    } else if (func == builtin_len) {
        return "len";
    } else if (func == builtin_greater_than) {
        return "greater-than";
    } else if (func == builtin_less_than) {
        return "less_than";
    } else if (func == builtin_greater_than_or_equal) {
        return "greater_than_or_equal";
    } else if (func == builtin_less_than_or_equal) {
        return "less_than_or_equal";
    } else if (func == builtin_equal) {
        return "equal";
    } else if (func == builtin_not_equal) {
        return "not_equal";
    } else if (func == builtin_floor) {
        return "floor";
    } else if (func == builtin_ceil) {
        return "ceil";
    } else if (func == builtin_logical_or) {
        return "or";
    } else if (func == builtin_logical_and) {
        return "and";
    } else if (func == builtin_logical_not) {
        return "not";
    } else if (func == builtin_load) {
        return "load";
    } else if (func == builtin_print) {
        return "print";
    } else if (func == builtin_format) {
        return "format";
    } else if (func == builtin_printf) {
        return "printf";
    } else if (func == builtin_type_of) {
        return "type-of";
    } else if (func == builtin_def_type) {
        return "def-type";
    } else if (func == builtin_error) {
        return "error";
    } else if (func == builtin_exit) {
        return "exit";
    } else if (func == builtin_read_file) {
        return "read-file";
    } else if (func == builtin_write_file) {
        return "write-file";
    } else if (func == builtin_print_env) {
        return "print-env";
    } else if (func == builtin_cpu_time_since) {
        return "cpu-time-since";
    } else if (func == builtin_console_size) {
        return "console-size";
    } else if (func == builtin_version) {
        return "version";
    } else if (func == builtin_create_buffer) {
        return "create-buffer";
    } else if (func == builtin_buffer_with_bytes) {
        return "buffer-with-bytes";
    } else if (func == builtin_buffer_map) {
        return "buffer-map";
    } else if (func == builtin_dictionary) {
        return "dict";
    }
    return "unknown_function";
}
