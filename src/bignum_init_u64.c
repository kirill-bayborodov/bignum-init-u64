/* ------------------------------------------------------------------ */
/**
 * @file    bignum_init_u64.c
 * @brief   Эталонная переносимая C-реализация bignum_init_u64.
 */
/* ------------------------------------------------------------------ */
#include <stddef.h>
#include <stdint.h>

#include "bignum_init_u64.h"

bignum_init_u64_status_t bignum_init_u64(bignum_t *restrict b, uint64_t val)
{
    if (b == NULL) {
        return BIGNUM_INIT_U64_ERROR_NULL_ARG;
    }

    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        b->words[i] = 0U;
    }
    b->len = 0U;

    if (val != 0U) {
        b->words[0] = val;
        b->len = 1U;
    }

    return BIGNUM_INIT_U64_SUCCESS;
}
