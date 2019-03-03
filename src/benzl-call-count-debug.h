// This file implements functions used for printing out stats on how often
// named functions are called
//
// Part of benzl - https://github.com/pokeb/benzl

#pragma once
#include "benzl-lval.h"

// Record that a named function has been called
// Does nothing when LOG_CALL_STATS is zero
void record_function_call(lval *f);

// Print stats on how often each named function was called
// Does nothing when LOG_CALL_STATS is zero
void print_call_count_stats(void);
