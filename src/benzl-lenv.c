// Part of benzl - https://github.com/pokeb/benzl

#include <stdlib.h>
#include <stdlib.h>
#include <string.h>

#include "benzl-lenv.h"
#include "benzl-lval.h"

lenv* lenv_alloc(size_t bucket_count) {
    lenv *e = malloc(sizeof(lenv));
    e->parent = NULL;
    e->items = lval_table_alloc(bucket_count);
    e->script_path = NULL;
    e->loaded_modules = lval_table_alloc(4);
    return e;
}

void lenv_free(lenv *e) {
    if (e->script_path != NULL) {
        free(e->script_path);
    }
    lval_table_free(e->items);
    if (e->loaded_modules != NULL) {
        lval_table_free(e->loaded_modules);
    }
    free(e);
}

lenv* lenv_copy(const lenv *e) {
    lenv *n = malloc(sizeof(lenv));
    n->parent = e->parent;
    n->items = lval_table_copy(e->items);
    n->script_path = NULL;
    n->loaded_modules = NULL;
    return n;
}

lval* lenv_get(lenv *e, const lval *k) {

    while (e != NULL) {
        lval *item = lval_table_get(e->items, k);
        if (item != NULL) {

            // Retain first in case the existing bound name is the same
            lval *bn = lval_retain(k);
            if (item->bound_name != NULL) {
                lval_release(item->bound_name);
            }
            // No retain since we've done that already
            item->bound_name = bn;

            return item;
        }
        // Not found in this environment, check the parent environment
        e = e->parent;
    }
    return lval_err_for_val(k, "Unbound symbol '%s'", k->val.vsym);
}

lval* lenv_set(lenv *e, const lval *k, const lval *v) {

    while (e != NULL) {
        lval *item = lval_table_get(e->items, k);
        if (item != NULL) {
            lval_release(item);
            lval_table_insert(e->items, k, v);
            return NULL;
        }
        e = e->parent;
    }
    return lval_err_for_val(v, "'%s' must be defined before it can be set",
                            k->val.vsym);
}

lval* lenv_def(lenv *e, const lval *k, const lval *v) {

    lval *item = lval_table_get(e->items, k);
    if (item != NULL) {
        lval_release(item);
        return lval_err_for_val(v, "'%s' is already declared", k->val.vsym);
    }

    lval_table_insert(e->items, k, v);
    return NULL;
}

lval* lenv_def_with_type(lenv *e, const lval *k, const lval *v, const lval *t)
{
    lval *item = lval_table_get(e->items, k);
    if (item != NULL) {
        lval_release(item);
        return lval_err_for_val(v, "'%s' is already declared", k->val.vsym);
    }

    lval_entry *entry = lval_table_insert(e->items, k, v);
    entry->type = lval_retain(t);
    return NULL;
}

void record_module_loaded(lenv *e, char *module_path) {
    lval *ignore = lval_sexpr();
    lval *path = lval_sym(module_path);
    lval_table_insert(e->loaded_modules, path, ignore);
    lval_release(ignore);
    lval_release(path);
}

bool is_module_already_loaded(lenv *e, char *module_path) {
    lval *path = lval_sym(module_path);
    bool r = lval_table_get_entry(e->loaded_modules, path) != NULL;
    lval_release(path);
    return r;
}

lval* lenv_def_or_set(lenv *e, const lval *k, const lval *v)
{
    lval_table_insert(e->items, k, v);
    return NULL;
}
