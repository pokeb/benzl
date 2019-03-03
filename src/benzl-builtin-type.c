// This file implements built-in functions for checking the type of an object
// and declaring custom types (structs)
//
// Part of benzl - https://github.com/pokeb/benzl

#include "benzl-builtins.h"
#include "benzl-error-macros.h"
#include "benzl-lval.h"
#include "benzl-lenv.h"
#include "benzl-parse.h"

lval* builtin_type_of(lenv *e, const lval *a) {
    LASSERT_NUM_ARGS("type-of", a, 1);
    lval *v = child(a, 0);
    if (v->type == LVAL_CUSTOM_TYPE_INSTANCE) {
        return lval_copy(v->val.vinst.type);
    }
    return lval_primitive_type(v->type);
}

lval* builtin_to_string(lenv *e, const lval *a) {
    LASSERT_NUM_ARGS("to-string", a, 1);
    lval *v = child(a, 0);
    lval *s = cast_to(v, LVAL_STR);
    if (s == NULL) {
        char *str = lval_to_string(v);
        s = lval_str(str);
        free(str);
    }
    return s;
}

lval* builtin_to_number(lenv *e, const lval *a) {
    LASSERT_NUM_ARGS("to-number", a, 1);
    lval *v = child(a, 0);
    if (v->type == LVAL_STR) {
        lval *r = string_to_number(v->val.vstr);
        if (r == NULL) {
            return lval_err_for_val(a, "Failed to convert string to number");
        }
        return r;
    } else if (lval_is_number(v)) {
        return lval_copy(v);
    }
    return lval_err_for_val(a, "Cannot convert %s to number", ltype_name(v->type));
}

lval* builtin_def_type(lenv *e, const lval *a)
{
    if (count(a) != 1 || child(a, 0)->type != LVAL_QEXPR ||
        child(a, 0)->val.vexp.count < 2) {
        return lval_err_for_val(a, "Arguments for def-type must be in the form "
                                   "(def-type {Name prop prop2}) or "
                                   "(def-type {Name prop:type prop2:type}");
    }
    lval *args = child(a, 0);
    lval *type_name = child(args, 0);
    if (type_name->type != LVAL_SYM) {
        if (type_name->type == LVAL_TYPE) {
            return lval_err_for_val(a, "Cannot redefine type '%s'",
                                    name_for_type(type_name->val.vtype));
        }
        return lval_err_for_val(a, "Arguments for def-type must be in the form "
                                   "(def-type {Name prop prop2}) or "
                                   "(def-type {Name prop:type prop2:type}");
    }

    for (size_t i=1; i<count(args); i++) {
        lval *arg = child(args, i);
        if (arg->type == LVAL_KEY_VALUE_PAIR) {
            if (arg->val.vkvpair.value->type == LVAL_SYM) {
                lval *type = lenv_get(e, arg->val.vkvpair.value);
                if (type->type == LVAL_ERR) {
                    lval_release(type);
                    return lval_err_for_val(
                        a, "def-type: invalid type '%s' for parameter '%s'",
                        arg->val.vkvpair.value->val.vsym.name,
                        arg->val.vkvpair.key->val.vsym.name
                    );
                }
                lval_release(type);
            }
        } else if (arg->type != LVAL_SYM) {
            return lval_err_for_val(a, "Arguments for def-type must be in the form "
                                       "(def-type {Name prop prop2}) or "
                                       "(def-type {Name prop:type prop2:type}");
        }
    }


    lval *props = lval_copy(args);
    lval *name = lval_retain(child(props, 0));
    lval_pop(props, 0); // Remove the type name
    lval *v = lval_custom_type(name, props);
    lenv_def(e, name, v);
    lval_release(props);
    lval_release(v);
    lval_release(name);
    return lval_sexpr();
}
