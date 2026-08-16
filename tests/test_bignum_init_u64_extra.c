/* ------------------------------------------------------------------ */
/**
 * @file    test_bignum_init_u64_extra.c
 * @brief   Extended and fuzz-style tests for bignum_init_u64.
 */
/* ------------------------------------------------------------------ */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "bignum_init_u64.h"

typedef struct {
    uint64_t before;
    bignum_t value;
    uint64_t after;
} guarded_bignum_t;

static uint64_t next_value(uint64_t *state)
{
    *state = *state * UINT64_C(6364136223846793005) + UINT64_C(1442695040888963407);
    return *state;
}

static void poison(bignum_t *b, uint64_t seed)
{
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        b->words[i] = seed ^ (UINT64_C(0x9e3779b97f4a7c15) * (i + 1U));
    }
    b->len = SIZE_MAX;
}

static void assert_initialized(const bignum_t *b, uint64_t value)
{
    assert(b->len == (value == 0U ? 0U : 1U));
    assert(b->words[0] == value);
    for (size_t i = 1; i < BIGNUM_CAPACITY; ++i) {
        assert(b->words[i] == 0U);
    }
}

static void assert_canaries(const guarded_bignum_t *guarded)
{
    assert(guarded->before == UINT64_C(0x1122334455667788));
    assert(guarded->after == UINT64_C(0x8877665544332211));
}

static void test_guard_canaries_and_boundaries(void)
{
    static const uint64_t values[] = {
        UINT64_C(0),
        UINT64_C(1),
        UINT64_C(2),
        UINT64_C(0x7fffffffffffffff),
        UINT64_C(0x8000000000000000),
        UINT64_C(0xfffffffffffffffe),
        UINT64_MAX
    };

    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        guarded_bignum_t guarded = {
            UINT64_C(0x1122334455667788),
            { { 0U }, 0U },
            UINT64_C(0x8877665544332211)
        };
        poison(&guarded.value, UINT64_C(0xa5a5a5a5a5a5a5a5));
        assert(bignum_init_u64(&guarded.value, values[i]) == BIGNUM_INIT_U64_SUCCESS);
        assert_initialized(&guarded.value, values[i]);
        assert_canaries(&guarded);
    }
    puts("test_guard_canaries_and_boundaries: PASSED");
}

static void test_state_transition_matrix(void)
{
    static const uint64_t values[] = {
        UINT64_MAX, 0U, 1U, 0U, UINT64_C(0x8000000000000000),
        UINT64_C(0x0123456789abcdef), 0U
    };
    guarded_bignum_t guarded = {
        UINT64_C(0x1122334455667788),
        { { 0U }, 0U },
        UINT64_C(0x8877665544332211)
    };

    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        assert(bignum_init_u64(&guarded.value, values[i]) == BIGNUM_INIT_U64_SUCCESS);
        assert_initialized(&guarded.value, values[i]);
        assert_canaries(&guarded);
    }
    puts("test_state_transition_matrix: PASSED");
}

static void test_fuzz_reference_equivalence(void)
{
    uint64_t state = UINT64_C(0x123456789abcdef0);
    guarded_bignum_t guarded = {
        UINT64_C(0x1122334455667788),
        { { 0U }, 0U },
        UINT64_C(0x8877665544332211)
    };

    for (unsigned i = 0; i < 100000U; ++i) {
        uint64_t value = next_value(&state);
        poison(&guarded.value, next_value(&state));
        assert(bignum_init_u64(&guarded.value, value) == BIGNUM_INIT_U64_SUCCESS);
        assert_initialized(&guarded.value, value);
        assert_canaries(&guarded);
    }
    puts("test_fuzz_reference_equivalence: PASSED (100000 cases)");
}

static void test_adjacent_objects(void)
{
    guarded_bignum_t objects[4];

    for (size_t i = 0; i < 4U; ++i) {
        objects[i].before = UINT64_C(0x1122334455667788);
        objects[i].after = UINT64_C(0x8877665544332211);
        poison(&objects[i].value, (uint64_t)i);
    }

    for (size_t i = 0; i < 4U; ++i) {
        uint64_t value = i == 0U ? 0U : next_value(&objects[i].value.words[0]);
        assert(bignum_init_u64(&objects[i].value, value) == BIGNUM_INIT_U64_SUCCESS);
        assert_initialized(&objects[i].value, value);
        assert_canaries(&objects[i]);
        if (i > 0U) {
            assert_canaries(&objects[i - 1U]);
        }
    }
    puts("test_adjacent_objects: PASSED");
}

int main(void)
{
    puts("--- Starting extended bignum_init_u64 tests ---");
    test_guard_canaries_and_boundaries();
    test_state_transition_matrix();
    test_fuzz_reference_equivalence();
    test_adjacent_objects();
    puts("--- All extended bignum_init_u64 tests passed ---");
    return 0;
}
