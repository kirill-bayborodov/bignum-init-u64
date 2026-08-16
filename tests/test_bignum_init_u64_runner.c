/**
 * @file    test_bignum_init_u64_runner.c
 * @brief   Интеграционный runner для bignum_init_u64.
 */
#include "bignum_init_u64.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    printf("Running test: test_bignum_init_u64_runner... ");

    bignum_t num;
    num.len = BIGNUM_CAPACITY;
    memset(num.words, 0xFF, sizeof(num.words));

    bignum_init_u64_status_t rc = bignum_init_u64(&num, 0U);
    assert(rc == BIGNUM_INIT_U64_SUCCESS);
    assert(num.len == 0U);
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        assert(num.words[i] == 0U);
    }

    rc = bignum_init_u64(&num, UINT64_MAX);
    assert(rc == BIGNUM_INIT_U64_SUCCESS);
    assert(num.len == 1U);
    assert(num.words[0] == UINT64_MAX);
    for (size_t i = 1; i < BIGNUM_CAPACITY; ++i) {
        assert(num.words[i] == 0U);
    }

    rc = bignum_init_u64(NULL, UINT64_MAX);
    assert(rc == BIGNUM_INIT_U64_ERROR_NULL_ARG);

    printf("PASSED\n");
    return 0;
}
