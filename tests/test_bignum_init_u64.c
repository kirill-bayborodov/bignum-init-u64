/* ------------------------------------------------------------------ */
/**
 * @file    test_bignum_init_u64.c
 * @brief   Deterministic unit tests for bignum_init_u64.
 */
/* ------------------------------------------------------------------ */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "bignum_init_u64.h"

static void assert_zero_tail(const bignum_t *b, size_t first_zero)
{
    for (size_t i = first_zero; i < BIGNUM_CAPACITY; ++i) {
        assert(b->words[i] == 0U);
    }
}

static void test_zero(void)
{
    bignum_t b;
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        b.words[i] = UINT64_MAX;
    }
    b.len = BIGNUM_CAPACITY;

    assert(bignum_init_u64(&b, 0U) == BIGNUM_INIT_U64_SUCCESS);
    assert(b.len == 0U);
    assert_zero_tail(&b, 0U);
    puts("test_zero: PASSED");
}

static void test_one(void)
{
    bignum_t b;

    assert(bignum_init_u64(&b, 1U) == BIGNUM_INIT_U64_SUCCESS);
    assert(b.len == 1U);
    assert(b.words[0] == 1U);
    assert_zero_tail(&b, 1U);
    puts("test_one: PASSED");
}

static void test_uint64_max(void)
{
    bignum_t b;

    assert(bignum_init_u64(&b, UINT64_MAX) == BIGNUM_INIT_U64_SUCCESS);
    assert(b.len == 1U);
    assert(b.words[0] == UINT64_MAX);
    assert_zero_tail(&b, 1U);
    puts("test_uint64_max: PASSED");
}

static void test_null(void)
{
    assert(bignum_init_u64(NULL, 0U) == BIGNUM_INIT_U64_ERROR_NULL_ARG);
    assert(bignum_init_u64(NULL, 42U) == BIGNUM_INIT_U64_ERROR_NULL_ARG);
    assert(bignum_init_u64(NULL, UINT64_MAX) == BIGNUM_INIT_U64_ERROR_NULL_ARG);
    puts("test_null: PASSED");
}

static void test_repeat(void)
{
    bignum_t b;

    assert(bignum_init_u64(&b, UINT64_MAX) == BIGNUM_INIT_U64_SUCCESS);
    assert(bignum_init_u64(&b, 7U) == BIGNUM_INIT_U64_SUCCESS);
    assert(b.len == 1U);
    assert(b.words[0] == 7U);
    assert_zero_tail(&b, 1U);

    assert(bignum_init_u64(&b, 0U) == BIGNUM_INIT_U64_SUCCESS);
    assert(b.len == 0U);
    assert_zero_tail(&b, 0U);
    puts("test_repeat: PASSED");
}

int main(void)
{
    puts("--- Starting deterministic bignum_init_u64 tests ---");
    test_zero();
    test_one();
    test_uint64_max();
    test_null();
    test_repeat();
    puts("--- All deterministic bignum_init_u64 tests passed ---");
    return 0;
}
