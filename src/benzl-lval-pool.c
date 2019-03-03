// Part of benzl - https://github.com/pokeb/benzl

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "benzl-lval-pool.h"
#include "benzl-lval.h"
#include "benzl-config.h"

static size_t block_element_count = 32;

#pragma mark - Creating and destroying pool allocators

lval_pool* pool_alloc(void) {
    lval_pool *pool = malloc(sizeof(lval_pool));
    pool->total_allocated = 0;
    pool->total_used = 0;
    pool->current_block = 0;
    pool->used_in_current_block = 0;
    pool->next_available = NULL;
    pool->blocks_allocated = 1;
    pool->blocks = malloc(sizeof(lval *));
    pool->blocks[0] = calloc(block_element_count, sizeof(lval));
    return pool;
}

void pool_free(lval_pool *pool) {
    for (size_t i = 0; i < pool->blocks_allocated; i++) {
        if (pool->blocks[i] == NULL) {
            break;
        } else {
            free(pool->blocks[i]);
        }
    }
    free(pool->blocks);
    free(pool);
    pool = NULL;
}

#pragma mark - Getting lvals from the pool and releasing them

// Nils out shared members of an lval for reuse
// (other properties are set in the lval_* constructor functions)
static inline lval* reset_lval(lval *v)
{
    v->source_position = (code_pos){0};
    v->bound_name = NULL;
    v->ref_count = 1;
    return v;
}

lval *pool_lval_alloc(lval_pool *pool) {

#if LOG_ALLOCATION_POOL_STATS
    pool->total_allocated++;
    pool->total_used++;
#endif
    
#if DISABLE_POOL_ALLOCATION
    return reset_lval(malloc(sizeof(lval)));
#else

    // Is there a previously freed lval we can use?
    if (pool->next_available != NULL) {
        lval *recycle = (lval *)pool->next_available;

        // Use the lval we freed before that next (or NULL if there isn't one)
        pool->next_available = pool->next_available->next;

        return reset_lval(recycle);
    }

    // Is our current block is full?
    if (pool->used_in_current_block == block_element_count) {

        // Yes: Use the next block
        pool->current_block++;
        pool->used_in_current_block = 0;

        // Have we run out of blocks?
        if (pool->current_block == pool->blocks_allocated) {

            // Yes, allocate more pointers for blocks
            size_t new_blocks_used = MAX(1, pool->blocks_allocated*2);
            pool->blocks = realloc(pool->blocks, sizeof(lval*)*new_blocks_used);
            memset(pool->blocks+pool->blocks_allocated, 0x00,
                   (new_blocks_used-pool->blocks_allocated)*sizeof(lval*));
            pool->blocks_allocated = new_blocks_used;
        }

        // If the current block hasn't been allocated yet, do that
        if (pool->blocks[pool->current_block] == NULL) {
            pool->blocks[pool->current_block] = calloc(block_element_count,
                                                       sizeof(lval));
        }
    }

    // Return the the next unused lval slot in the current block
    lval *r = reset_lval(
        (lval *)(pool->blocks[pool->current_block] + pool->used_in_current_block)
    );
    pool->used_in_current_block++;
    return r;
#endif
}

void pool_lval_free(lval_pool *pool, lval *v) {

#if LOG_ALLOCATION_POOL_STATS
    assert(pool->total_used > 0);
    pool->total_used--;
#endif

#if DISABLE_POOL_ALLOCATION
    free(v);
    return;
#else
    lval_ll *f = pool->next_available;
    pool->next_available = (lval_ll *)v;

    // Double free - this must be a bug!
    assert(v != (lval *)f);

    pool->next_available->next = f;
#endif
}

static lval_pool *shared_pool = NULL;

lval_pool* global_pool(void) {
    if (shared_pool == NULL) {
        shared_pool = pool_alloc();
    }
    return shared_pool;
}

void pool_print_stats(lval_pool *pool)
{
#if LOG_ALLOCATION_POOL_STATS
    printf("[POOL-STATS] %lu lvals / %lu blocks allocated, %lu still in use\n",
           pool->total_allocated, pool->blocks_allocated, pool->total_used);
#endif
}
