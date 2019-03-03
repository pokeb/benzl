// Part of benzl - https://github.com/pokeb/benzl

#include <stdio.h>

#include "benzl-hash-table.h"
#include "benzl-lval.h"
#include "benzl-config.h"

lval_table* lval_table_alloc(size_t size)
{
    size = MAX(2, size);
    lval_table *table = malloc(sizeof(lval_table));
    table->bucket_count = size;
    table->min_buckets = MAX(2, size);
    table->count = 0;
    table->collision_count = 0;
    table->worst_case_lookup_count = 0;
    table->items = calloc(table->bucket_count, sizeof(lval_entry*));
    return table;
}

void lval_table_reset(lval_table *table, bool should_clear)
{
    if (table->count == 0) {
        return;
    }
    for (size_t i=0; i<table->bucket_count; i++) {
        lval_entry *entry = table->items[i];
        while (entry != NULL) {
            lval_entry *next = entry->next;
            lval_entry_free(entry);
            entry = next;
            table->count--;
            if (table->count == 0) {
                goto end;
            }
        }

    }
end:
    if (should_clear) {
        memset(table->items, 0, table->bucket_count*sizeof(lval_entry*));
    }
}

void lval_table_free(lval_table *table)
{
    lval_table_reset(table, false);
    free(table->items);
    free(table);
    table = NULL;
}

lval_table* lval_table_copy(const lval_table *table)
{
    lval_table *new_table = lval_table_alloc(table->bucket_count);
    for (size_t i=0; i<table->bucket_count; i++) {
        lval_entry *entry = table->items[i];
        while (entry != NULL) {
            lval_table_insert(new_table, entry->key, entry->value);
            entry = entry->next;
        }
    }
    return new_table;
}

lval_entry* lval_entry_alloc(const lval *key, const lval *value)
{
    lval_entry *entry = malloc(sizeof(lval_entry));
    entry->key = lval_retain(key);
    entry->value = lval_retain(value);
    entry->next = NULL;
    entry->type = NULL;
    return entry;
}

void lval_entry_free(lval_entry *entry)
{
    lval_release(entry->key);
    lval_release(entry->value);
    if (entry->type) {
        lval_release(entry->type);
    }
    free(entry);
}

// Simple hash function
// Add up the values of each byte in the key
// then get the remainder when divided by the number of buckets
// to provide an index between 0 and bucket_count-1
size_t lval_table_hash(const char *key)
{
    const size_t len = strlen(key);
    size_t hash = 0;
    for (size_t i=0; i<len; i++) {
        hash += key[i];
    }
    return hash;
}

static inline size_t bucket_for_key(lval_table *table, const lval *key)
{
    return key->val.vsym.hash % table->bucket_count;
}

static inline size_t bucket_for_entry(lval_table *table,
                                      const lval_entry *entry)
{
    return bucket_for_key(table, entry->key);
}

// Internal function for inserting an entry
// This is used for normal inserts, and during resizing
void lval_table_insert_entry(lval_table *table, lval_entry *entry)
{
    // Hash to get the index inside our bucket array
    size_t index = bucket_for_entry(table, entry);

    // Is there already an entry in this bucket?
    lval_entry *existing_entry = table->items[index];
    if (existing_entry != NULL) {

        // Yes - store the entry at the end of the linked list
        lval_entry *previous_entry = NULL;
        do {
            // Does this entry have the same key?
            if (equal_symbols(existing_entry->key, entry->key)) {

                // Take the type from the entry so future sets
                // can be type-checked
                entry->type = existing_entry->type;
                existing_entry->type = NULL;

                // replace the entry
                entry->next = existing_entry->next;
                if (previous_entry != NULL) {
                    previous_entry->next = entry;
                } else {
                    table->items[index] = entry;
                }
                lval_entry_free(existing_entry);
                existing_entry = NULL;
            } else if (existing_entry->next != NULL) {
                previous_entry = existing_entry;
                existing_entry = existing_entry->next;
            } else {
                existing_entry->next = entry;
                existing_entry = NULL;
                table->count++;
            }
        } while (existing_entry != NULL);
    } else {
        // No - just put the entry in the bucket
        table->items[index] = entry;
        table->count++;
    }
}

lval_entry* lval_table_insert(lval_table *table, const lval *key,
                              const lval *value)
{
    assert(key->type == LVAL_SYM);

    lval_entry *entry = lval_entry_alloc(key, value);
    lval_table_insert_entry(table, entry);

    // Decide if it's worth resizing the table
    lval_table_resize_if_needed(table);

    return entry;
}

void lval_table_remove(lval_table *table, const lval *key)
{
    assert(key->type == LVAL_SYM);

    // Get the bucket this value is stored in
    size_t index = bucket_for_key(table, key);

    // Get the first entry in the bucket
    lval_entry *entry = table->items[index];

    if (entry == NULL) {
        // This key does not exist in the hash table
        return;
    } else if (equal_symbols(entry->key, key)) {
        // First entry in the bucket is the one we want
        table->items[index] = NULL;
    } else {
        // First entry in the bucket is not the one we want
        // Traverse the linked list to find the right one
        lval_entry *previous_entry = NULL;
        do {
            previous_entry = entry;
            entry = entry->next;
            assert(entry != NULL);
        } while (!equal_symbols(entry->key, key));
        previous_entry->next = entry->next;
    }
    // Dispose of the entry as we no longer need it
    lval_entry_free(entry);
    table->count--;

    // Decide if it's worth resizing the table
    lval_table_resize_if_needed(table);
}

#if LOG_HASH_TABLE_STATS
static unsigned long global_lookup_collisions = 0;
static unsigned long global_lookup_count = 0;
#endif

lval_entry* lval_table_get_entry(lval_table *table, const lval *key)
{
    assert(key->type == LVAL_SYM);

    if (table->count == 0) {
        return NULL;
    }
    lval_entry *entry = table->items[bucket_for_key(table, key)];

#if LOG_HASH_TABLE_STATS
    size_t lookup_count = 0;
    while (entry != NULL && !equal_symbols(entry->key, key)) {
        entry = entry->next;
        lookup_count++;
    }
    
    table->worst_case_lookup_count = MAX(table->worst_case_lookup_count, lookup_count);
    global_lookup_count++;
    global_lookup_collisions+=lookup_count;
    table->collision_count+=lookup_count;
#else
    while (entry != NULL && !equal_symbols(entry->key, key)) {
        entry = entry->next;
    }
#endif
    return entry;
}

lval* lval_table_get(lval_table *table, const lval *key)
{
    assert(key->type == LVAL_SYM);
    lval_entry *entry = lval_table_get_entry(table, key);
    if (entry == NULL) {
        return NULL;
    }
    return lval_retain(entry->value);
}


void lval_table_resize(lval_table *table, size_t new_bucket_count)
{
    // Make a new temporary table with the new item
    lval_table *new_table = lval_table_alloc(new_bucket_count);

    // Add all the items in our current table to our new one
    for (size_t i=0; i<table->bucket_count; i++) {
        lval_entry *entry = table->items[i];
        while (entry != NULL) {
            lval_entry *next = entry->next;
            entry->next = NULL;
            lval_table_insert_entry(new_table, entry);
            entry = next;
        }
    }
    // Update table to use the storage from new_table
    free(table->items);
    table->items = new_table->items;
    table->bucket_count = new_table->bucket_count;

    // Cleanup the temporary table
    free(new_table);
}

void lval_table_resize_if_needed(lval_table *table)
{
    // Table too small - let's make it bigger
    if (table->count >= table->bucket_count/2) {
        lval_table_resize(table, table->count*4);

    // Table too big - let's make it smaller
    } else if (MAX(table->min_buckets, table->count) < table->bucket_count/2) {
        lval_table_resize(table, table->count*4);
    }
}

void lval_table_print(const lval_table *table)
{
    for (size_t i=0; i<table->bucket_count; i++) {
        lval_entry *entry = table->items[i];
        while (entry != NULL) {
            char *v = lval_to_string(entry->value);
            printf("%s: %s\n", entry->key->val.vsym.name, v);
            free(v);
            entry = entry->next;
        }
    }
}

size_t lval_table_entries(const lval_table *table, lval_entry ***entries)
{
    lval_entry **entry_list = malloc(table->count*sizeof(lval_entry *));
    size_t count = 0;
    for (size_t i=0; i<table->bucket_count; i++) {
        lval_entry *entry = table->items[i];
        while (entry != NULL) {
            entry_list[count] = entry;
            count++;
            entry = entry->next;
        }
    }
    *entries = entry_list;
    return count;
}

bool lval_tables_equal(lval_table *t1,  lval_table *t2)
{
    if (t1->count != t2->count) {
        return false;
    }
    for (size_t i=0; i<t1->bucket_count; i++) {
        lval_entry *t1_entry = t1->items[i];
        if (t1_entry != NULL) {
            lval_entry *t2_entry = lval_table_get_entry(t2, t1_entry->key);
            if (t2_entry == NULL || !equal_symbols(t1_entry->key, t2_entry->key) ||
                !lval_eq(t1_entry->value, t2_entry->value)) {
                return false;
            }
        }
    }
    return true;
}

void print_lval_table_stats(void) {
#if LOG_HASH_TABLE_STATS
    printf("[TABLE-STATS] Did %lu lookups, collisions %lu, (%f%%)\n",
           global_lookup_count, global_lookup_collisions,
           (global_lookup_collisions/(double)global_lookup_count)*100);
#endif
}
