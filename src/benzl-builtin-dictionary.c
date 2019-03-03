#include "benzl-builtins.h"
#include "benzl-lval.h"
#include "benzl-lval-eval.h"
#include "benzl-lenv.h"

lval* builtin_dictionary(lenv *e, const lval *a)
{
    for (size_t i=0; i<count(a); i++) {
        if (child(a, i)->type != LVAL_KEY_VALUE_PAIR) {
            return lval_err_for_val(
                a, "Initial entries for a dictionary must take the form "
                   "(dictionary key1:value1 key2:value2)"
            );
        }
    }
    lval *d = lval_dict(count(a)*2);
    for (size_t i=0; i<count(a); i++) {
        lval *v = child(a, i);
        lval *val = lval_eval(e, v->val.vkvpair.value);
        lval_table_insert(d->val.vdict, v->val.vkvpair.key, val);
        lval_release(val);
    }
    return d;
}
