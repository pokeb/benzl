// Part of benzl - https://github.com/pokeb/benzl

#include <stdio.h>

#include "benzl-call-count-debug.h"
#include "benzl-hash-table.h"
#include "benzl-config.h"

#if LOG_CALL_STATS

lval_table *call_counts = NULL;

void record_function_call(lval *f) {
    if (call_counts == NULL) {
        call_counts = lval_table_alloc(2048);
    }
    lval_entry *entry = lval_table_get_entry(call_counts, f->bound_name);
    if (entry != NULL) {
        entry->value->val.vint++;
    } else {
        lval *v = lval_int(1);
        lval_table_insert(call_counts, f->bound_name, v);
        lval_release(v);
    }
}

static int sort_entries(const void *v1, const void *v2)
{
    lval_entry *e1 = *((lval_entry **)v1);
    lval_entry *e2 = *((lval_entry **)v2);

    if (e1->value->val.vint < e2->value->val.vint) {
        return 1;
    } else if (e1->value->val.vint > e2->value->val.vint) {
        return -1;
    }
    return 0;
}

void print_call_count_stats(void) {
    printf("[CALL-STATS] Function call counts:-------------------\n");
    lval_entry **entries = NULL;
    size_t count = lval_table_entries(call_counts, &entries);
    qsort(entries, count, sizeof(lval_entry *), sort_entries);
    for (size_t i=0; i<count; i++) {
        char *val = lval_to_string(entries[i]->value);
        printf("%s: %s\n", entries[i]->key->val.vsym.name, val);
        free(val);
    }
    free(entries);
}

#else

void record_function_call(lval *f) {}
void print_call_count_stats(void) {}

#endif
