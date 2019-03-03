// lval_table is a simple hash table with lvals for keys and values
// It is used by lenv for storing bound functions and variables,
// and also for lvals representing instances of custom types, storing the bound
// values for the type's properties
// This hash table uses a linked list for storing values with colliding keys
// (Separate Chaining rather than Open Addressing)
//
// Part of benzl - https://github.com/pokeb/benzl

#pragma once

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct lval lval;
typedef struct lval_entry lval_entry;

struct lval_entry {
    lval *key;
    lval *value;
    // May contain the type values for this entry are supposed to have
    // or NULL if the value is un-typed
    lval *type;
    // Next entry in case of collision, or NULL
    lval_entry *next;
};

typedef struct {
    size_t count; // Entries in the table
    size_t min_buckets; // Minimum number of buckets
    size_t bucket_count; // Number of buckets
    size_t collision_count; // Number of lookup collisions (for stats)
    size_t worst_case_lookup_count; // Worst case collision count (for stats)
    lval_entry **items; // Buckets
} lval_table;

// Create a hash table for storing lvals
lval_table* lval_table_alloc(size_t size);

// Free the hash table and its contents
void lval_table_free(lval_table *table);

// Empties the hash table. Pass true for should_clear to NULL the entries
void lval_table_reset(lval_table *table, bool should_clear);

// Copies a hash table
lval_table* lval_table_copy(const lval_table *table);

// Create an entry for a hash table
lval_entry* lval_entry_alloc(const lval *key, const lval *value);

// Free the entry
void lval_entry_free(lval_entry *entry);

// Add a value to the hash table
lval_entry* lval_table_insert(lval_table *table, const lval *key, const lval *value);

// Remove a value from the hash table
void lval_table_remove(lval_table *table, const lval *key);

// Get a value from the hash table
lval* lval_table_get(lval_table *table, const lval *key);

// Get a reference to an entry in the table
lval_entry* lval_table_get_entry(lval_table *table, const lval *key);

// Resize the hash table to use the passed number of buckets
void lval_table_resize(lval_table *table, size_t new_bucket_count);

// Automatically make the hash table bigger or smaller depending on
// how many items it stores
void lval_table_resize_if_needed(lval_table *table);

// Print the contents of the table
void lval_table_print(const lval_table *table);

// Returns an array of all the entries in the table
size_t lval_table_entries(const lval_table *table, lval_entry ***entries);

// Returns true if both tables contain the same keys, and equal values for those keys
bool lval_tables_equal(lval_table *t1, lval_table *t2);

// For testing - prints statistics on total hash table lookups and collisions
void print_lval_table_stats(void);

// Hash function used for transforming symbols into an integer
// The hash table does (hash % bucket_count) to get the index of the entry
size_t lval_table_hash(const char *key);
