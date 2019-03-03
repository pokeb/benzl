// Configuration options
//
// Part of benzl - https://github.com/pokeb/benzl

#pragma once

// Set to 1, and all lval allocations via pool_lval_alloc will go through malloc
// individually instead.
// This is slower than using the pool, but it can help finding leaks
// or overflows (eg with address-sanitizer)
// Should be 0 unless debugging the benzl language
#define DISABLE_POOL_ALLOCATION 0

// Set to 1, and benzl will count the number of times named functions are called,
// and print them out at the end of execution
// (this is helpful for finding optimisation opportunities)
// Should be 0 unless debugging the benzl language
#define LOG_CALL_STATS 0

// Set to 1, and benzl will measure how the global lval pool is used
// and print out stats at the end
// This is helpful for finding leaks
// Should be 0 unless debugging the benzl language
#define LOG_ALLOCATION_POOL_STATS 0

// Set to 1, and benzl will record the number of lookups and collisions
// across all hash tables, and print out stats at the end
// Should be 0 unless debugging the benzl language
#define LOG_HASH_TABLE_STATS 0
