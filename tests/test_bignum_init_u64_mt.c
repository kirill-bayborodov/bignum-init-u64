/* ------------------------------------------------------------------ */
/**
 * @file    test_bignum_init_u64_mt.c
 * @brief   Multithreaded correctness tests for bignum_init_u64.
 */
/* ------------------------------------------------------------------ */
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

#include "bignum_init_u64.h"

#define THREAD_COUNT 8U
#define ITERATIONS   10000U

typedef struct {
    bignum_t value;
    uint64_t seed;
} thread_data_t;

static void *worker(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    for (unsigned i = 0; i < ITERATIONS; ++i) {
        uint64_t value = data->seed + i;
        bignum_init_u64(&data->value, value);
        if (value == 0U) {
            assert(data->value.len == 0U);
        } else {
            assert(data->value.len == 1U);
            assert(data->value.words[0] == value);
        }
        for (size_t w = 1; w < BIGNUM_CAPACITY; ++w) {
            assert(data->value.words[w] == 0U);
        }
    }
    return NULL;
}

int main(void)
{
    pthread_t threads[THREAD_COUNT];
    thread_data_t data[THREAD_COUNT];

    for (unsigned i = 0; i < THREAD_COUNT; ++i) {
        data[i].seed = UINT64_C(0x100000000) * (i + 1U);
        assert(pthread_create(&threads[i], NULL, worker, &data[i]) == 0);
    }
    for (unsigned i = 0; i < THREAD_COUNT; ++i) {
        assert(pthread_join(threads[i], NULL) == 0);
    }

    puts("--- Multithreaded bignum_init_u64 test passed ---");
    return 0;
}
