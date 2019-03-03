// Part of benzl - https://github.com/pokeb/benzl

#include "benzl-lval-eval.h"
#include "benzl-lval.h"
#include "benzl-lenv.h"
#include "benzl-builtins.h"
#include "benzl-call-count-debug.h"
#include "benzl-stacktrace.h"

lval* lval_eval(lenv *e, const lval *v) {
    lval *r = NULL;
    if (v->type == LVAL_SYM) {
        r = lenv_get(e, v);
    } else if (v->type == LVAL_SEXPR) {
        r = lval_eval_sexpr(e, v);
    } else {
        r = lval_retain(v);
    }
    assert(r != NULL);
    return r;
}

lval* lval_create_custom_type_instance(lenv *e, const lval *t, const lval *v)
{
    // Check we are supplying all properties of the custom type
    if (count(v) != count(t->val.vtype.props)) {
        char *supplied = lval_to_string(v);
        char *expected = lval_to_string(t->val.vtype.props);

        lval *err = lval_err_for_val(v, "Incorrect number of arguments to create %s (got %s expected %s)",
                                     t->val.vtype.name, supplied, expected);
        free(supplied);
        free(expected);
        return err;
    }

    lval *args = lval_qexpr_with_size(count(t->val.vtype.props));

    // Loop through the required properties in the type
    for (size_t i = 0; i < count(t->val.vtype.props); i++) {
        bool found_arg = false;
        lval *v1 = child(t->val.vtype.props, i);

        lval *prop = NULL;
        if (v1->type == LVAL_KEY_VALUE_PAIR) {
            prop = v1->val.vkvpair.key;
        } else {
            prop = v1;
        }
        // Loop through the supplied values
        for (size_t i2 = 0; i2< count(v); i2++) {

            if (equal_symbols(prop, child(v, i2)->val.vkvpair.key)) {

                lval *v2 = lval_retain(child(v, i2));
                found_arg = true;

                // If this property is typed, check we have the right type
                if (v1->type == LVAL_KEY_VALUE_PAIR) {

                    if (v2->val.vkvpair.value->type == LVAL_SEXPR) {
                        lval *r = lval_eval(e, v2->val.vkvpair.value);
                        if (r->type == LVAL_ERR) {
                            lval_release(v2);
                            return r;
                        } else if (r != v2->val.vkvpair.value) {
                            lval_release(v2);
                            v2 = lval_kv_pair(v2->val.vkvpair.key, r);
                            lval_release(r);
                        }
                    } else if (v2->val.vkvpair.value->type == LVAL_SYM) {
                        lval *r = lenv_get(e, v2->val.vkvpair.value);
                        if (r->type == LVAL_ERR) {
                            lval_release(v2);
                            return r;
                        } else if (r != v2->val.vkvpair.value) {
                            lval_release(v2);
                            v2 = lval_kv_pair(v2->val.vkvpair.key, r);
                        }
                        lval_release(r);
                    }

                    lval *type = type_from_pair(e, v1);
                    lval *cast_val = NULL;
                    if (type->type == LVAL_ERR) {
                        char *s = lval_to_string(v1->val.vkvpair.value);
                        lval *err = lval_err_for_val(
                            v, "Parameter '%s': Invalid type '%s'",
                            prop, s
                        );
                        free(s);
                        lval_release(type);
                        lval_release(args);
                        lval_release(v2);
                        return err;
                    } else if (!value_matches_type(e, v2->val.vkvpair.value, type, &cast_val)) {
                        char *s = type_mismatch_description(&type->val.vtype, v2->val.vkvpair.value);


                        lval *err = lval_err_for_val(v, "Property '%s' for '%s': %s",
                                                     prop,
                                                     bound_name_for_lval(t), s);
                        free(s);
                        lval_release(type);
                        lval_release(args);
                        lval_release(v2);
                        return err;
                    }
                    lval_release(type);

                    if (cast_val != NULL) {
                        lval *pair = lval_kv_pair(v2->val.vkvpair.key, cast_val);
                        lval_add(args, pair);
                        lval_release(cast_val);
                        lval_release(pair);
                    } else {
                        lval_add(args, v2);
                    }
                } else {
                    lval_add(args, v2);
                }
                lval_release(v2);
                break;
            }
        }
        // If didn't get a value for this property, throw an error
        if (!found_arg) {
            lval_release(args);
            return lval_err_for_val(v, "Missing argument '%s' to create %s",
                                    prop,
                                    t->val.vtype.name->val.vsym.name);
        }
    }
    lval *r = lval_custom_type_instance(t, args);
    lval_release(args);
    return r;
}

lval* lval_eval_sexpr(lenv *e, const lval *v) {

    stack_push_frame(v);
//    lval_print(v);
//    printf("\n");
    // This will store a temporary environment created for processing type property access
    // eg (mypoint x) ; where x is a value declared in the type of mypoint
    lenv *temp_env = NULL;

    lval *nv = lval_sexpr_with_size(count(v));
    nv->source_position = code_pos_retain(v->source_position);

    // Evaluate children
    for (size_t i=0; i<count(v); i++) {
        lval *input = child(v, i);
        lval *output = lval_eval(e, input);
        lval_add(nv, output);
        lval_release(output);

        if (i==0) {
            // If the first item is a custom instance,
            // create a temporary environment with its properties available
            if (output->type == LVAL_CUSTOM_TYPE_INSTANCE) {
                temp_env = malloc(sizeof(lenv));
                temp_env->parent = e;
                temp_env->items = output->val.vinst.props;
                e = temp_env;
            // Same thing for dictionaries
            } else if (output->type == LVAL_DICT) {
                temp_env = malloc(sizeof(lenv));
                temp_env->parent = e;
                temp_env->items = output->val.vdict;
                e = temp_env;
            }
        }

    }

    if (temp_env != NULL) {
        e = temp_env->parent;
        free(temp_env);
    }

    // Error checking
    for (size_t i=0; i<count(nv); i++) {
        if (child(nv, i)->type == LVAL_ERR) {
            lval *err = lval_retain(child(nv, i));
            lval_release(nv);
            stack_pop_frame();
            return err;
        }
    }
    // Empty expression
    if (count(nv) == 0) {
        stack_pop_frame();
        return nv;
    }

    lval *f = lval_retain(child(nv, 0));
    lval_pop(nv, 0);

    // Single expression
    if (count(nv) == 0 && f->type != LVAL_FUN) {
        lval_release(nv);
        stack_pop_frame();
        return f;
    }

    // If this is a type, assume we are creating an instance of that type
    if (f->type == LVAL_TYPE) {
        lval *r = lval_create_custom_type_instance(e, f, nv);
        lval_release(f);
        lval_release(nv);
        stack_pop_frame();
        return r;

    // If this is a custom instance or dictionary,
    // assume we are attempting to read a property from that object
    } else if (f->type == LVAL_CUSTOM_TYPE_INSTANCE || f->type == LVAL_DICT) {
        lval *r = lval_eval(e, nv);
        lval_release(f);
        lval_release(nv);
        stack_pop_frame();
        return r;
    }

    // Ensure first element is symbol
    if (f->type != LVAL_FUN) {
        lval *err = lval_err_for_val(nv, "Expression starts with incorrect type (got %s expected %s)",
                                    ltype_name(f->type),
                                    ltype_name(LVAL_FUN));
        lval_release(f);
        lval_release(nv);
        stack_pop_frame();
        return err;
    }


    if (f->bound_name != NULL) {
        record_function_call(f);
    }


    lval *r = lval_call(e, f, nv);

    lval_release(f);
    lval_release(nv);
    stack_pop_frame();
    return r;
}

#pragma mark - Calling functions

static lval* lval_subexp(const lval *a, size_t start) {
    lval *v = lval_qexpr_with_size(count(a));
    for (size_t i=start; i<count(a); i++) {
        lval_add(v, child(a, i));
    }
    return v;
}

lval* lval_call(lenv *e, const lval *f, const lval *a)
{
    if (f->val.vfunc.builtin) {
        return f->val.vfunc.builtin(e, a);
    }

    size_t needed_args_count = count(f->val.vfunc.args);
    size_t used_args = 0;

    lenv *env = lenv_alloc(count(f->val.vfunc.args));
    env->parent = e;

    for (size_t i=0; i<count(a); i++) {

        if (i >= count(f->val.vfunc.args)) {
            char *vs = lval_to_string(a);
            lval *err = lval_err_for_val(a, "Function '%s' expects %d arguments (Got: %s)",
                                         bound_name_for_lval(f),
                                         needed_args_count,
                                         vs);
            free(vs);
            lenv_free(env);
            return err;
        }

        lval *sym = child(f->val.vfunc.args, i);
        lval *cast_val = NULL;

        // Is this a typed-parameter?
        if (sym->type == LVAL_KEY_VALUE_PAIR) {

            lval *type = type_from_pair(e, sym);

            if (type->type == LVAL_ERR) {
                char *s = lval_to_string(sym->val.vkvpair.value);
                lval *err = lval_err_for_val(
                    a, "Parameter '%s': Invalid type '%s'",
                    sym->val.vkvpair.key->val.vsym.name, s
                );
                free(s);
                lval_release(type);
                lenv_free(env);
                return err;
            } else if (!value_matches_type(e, child(a, i), type, &cast_val)) {
                char *s = type_mismatch_description(&type->val.vtype,
                                                    child(a, i));

                lval *err = lval_err_for_val(a, "Parameter '%s' for function '%s': %s",
                                             sym->val.vkvpair.key->val.vsym.name,
                                             bound_name_for_lval(f), s);
                free(s);
                lval_release(type);
                lenv_free(env);
                return err;
            }
            lval_release(type);

            sym = sym->val.vkvpair.key;
        }

        if (strcmp(sym->val.vsym.name, "&") == 0) {
            if (i != needed_args_count-2) {
                lenv_free(env);
                return lval_err_for_val(a, "Function format for '%s': Symbol '&' not followed by single symbol.",
                                        bound_name_for_lval(f));
            }
            needed_args_count -=1;
            used_args++;
            lval *exp = lval_subexp(a, i);
            lval *lst = builtin_list(e, exp);
            lenv_def_or_set(env, child(f->val.vfunc.args, i+1), lst);
            lval_release(exp);
            lval_release(lst);
            break;
        }

        if (cast_val != NULL) {
            lenv_def_or_set(env, sym, cast_val);
            lval_release(cast_val);
        } else {
            lenv_def_or_set(env, sym, child(a, i));

        }
        used_args++;
    }

    // If we've bound values for all arguments
    if (used_args == needed_args_count) {
        lval *expr = lval_sexpr_with_size(1);
        lval_add(expr, f->val.vfunc.body);
        lval *r = builtin_eval(env, expr);
        lval_release(expr);
        lenv_free(env);
        return r;
    }
    // Otherwise, return an error
    char *vs = lval_to_string(a);
    lval *err = lval_err_for_val(a, "Function '%s' expects %d arguments (Got: %s)",
                                 bound_name_for_lval(f),
                                 needed_args_count,
                                 vs);
    free(vs);
    lenv_free(env);
    return err;
}
