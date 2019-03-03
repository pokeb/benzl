// lval-pool is a pool allocator for lvals
// It speeds up allocating and freeing lvals by pre-allocating storage for them.
// When lval_alloc() is called, the global pool will return a pointer to the
// first available lval-sized block of memory in the pool
// When lval_release() is called, the lval's storage becomes the first node in a
// linked list of previously freed lvals. These are reused until there are no
// more previously freed lvals, at which point the pool will provide a pointer
// from its pre-allocated blocks of memory again (more blocks mat be allocated
// as needed)
//
// Part of benzl - https://github.com/pokeb/benzl

#pragma once

#include <stddef.h>

#include "benzl-lval.h"

#pragma mark - Type definitions

// Linked list that will store a ptr to the next available lval in the pool
// that was previously freed (or NULL if there isn't one)
typedef struct lval_ll {
    struct lval_ll *next;
} lval_ll;

// Pool comprised of blocks of block_element_count lvals
typedef struct {
    // Incremented every time an lval is allocated from the pool
    size_t total_allocated;
    // Stores the total number of lvals the pool has still allocated
    size_t total_used;
    // Current block for new allocations
    size_t current_block;
    // Number elememts used in the current block
    size_t used_in_current_block;
    // Number of blocks used
    size_t blocks_allocated;
    // Stores a reference to the last freed element (which will be the next used)
    lval_ll *next_available;
    // Array of pointers to the allocated blocks
    lval **blocks;
} lval_pool;

#pragma mark - Creating and destroying pool allocators

// Allocates a pool
lval_pool* pool_alloc(void);

// Frees a pool
void pool_free(lval_pool *pool);

#pragma mark - Getting lvals from the pool and releasing them

// Gets an lval from the pool
lval *pool_lval_alloc(lval_pool *pool);

// Release an lval back to the pool
void pool_lval_free(lval_pool *pool, lval *v);

// Get a reference to the global shared pool
lval_pool* global_pool(void);

// Get statistics on how the pool has been used (for debugging benzl)
void pool_print_stats(lval_pool *pool);
