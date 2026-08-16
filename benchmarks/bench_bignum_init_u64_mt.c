#define _POSIX_C_SOURCE 200809L

/**
 * @file    bench_bignum_init_u64_mt.c
 * @brief   Reproducible multi-thread benchmark for bignum_init_u64.
 */
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "bignum_init_u64.h"

#define DEFAULT_THREAD_COUNT        UINT32_C(2)
#define DEFAULT_TOTAL_ITERATIONS    UINT64_C(3200000000)
#define DEFAULT_ITERATIONS_PER_THREAD (DEFAULT_TOTAL_ITERATIONS / DEFAULT_THREAD_COUNT)
#define DEFAULT_WARMUP_ITERATIONS   UINT64_C(200000)
#define DEFAULT_DATA_COUNT          UINT32_C(8192)
#define DEFAULT_SEED                UINT64_C(0x9e3779b97f4a7c15)
#define DEFAULT_DATA_MODE           "all_nonzero"

typedef enum {
    DATA_MODE_ALL_ZERO,
    DATA_MODE_ALL_NONZERO,
    DATA_MODE_MIXED
} data_mode_t;

static const char *data_mode_name(data_mode_t mode)
{
    switch (mode) {
    case DATA_MODE_ALL_ZERO: return "all_zero";
    case DATA_MODE_ALL_NONZERO: return "all_nonzero";
    case DATA_MODE_MIXED: return "mixed";
    }
    return "invalid";
}

static int parse_data_mode(const char *text, data_mode_t *mode)
{
    if (strcmp(text, "all_zero") == 0) {
        *mode = DATA_MODE_ALL_ZERO;
    } else if (strcmp(text, "all_nonzero") == 0) {
        *mode = DATA_MODE_ALL_NONZERO;
    } else if (strcmp(text, "mixed") == 0) {
        *mode = DATA_MODE_MIXED;
    } else {
        fprintf(stderr, "data-mode must be all_zero, all_nonzero or mixed: %s\n", text);
        return EXIT_FAILURE;
    }
    return 0;
}

typedef struct {
    const uint64_t *data;
    uint32_t data_count;
    uint64_t iterations;
    uint32_t thread_index;
    uint64_t checksum;
    uint64_t successes;
    int status;
} worker_arg_t;

static uint64_t next_u64(uint64_t *state)
{
    *state ^= *state >> 12;
    *state ^= *state << 25;
    *state ^= *state >> 27;
    return *state * UINT64_C(0x2545f4914f6cdd1d);
}

static uint64_t fnv1a_update_u64(uint64_t hash, uint64_t value)
{
    for (unsigned byte = 0; byte < sizeof(value); ++byte) {
        hash ^= (uint8_t)(value >> (byte * CHAR_BIT));
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static uint64_t parse_u64(const char *text, const char *name)
{
    char *end = NULL;
    errno = 0;
    unsigned long long parsed = strtoull(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0') {
        fprintf(stderr, "invalid %s: %s\n", name, text);
        exit(EXIT_FAILURE);
    }
    return (uint64_t)parsed;
}

static double elapsed_seconds(const struct timespec *begin,
                              const struct timespec *end)
{
    return (double)(end->tv_sec - begin->tv_sec) +
           (double)(end->tv_nsec - begin->tv_nsec) / 1000000000.0;
}

static void print_usage(const char *program)
{
    fprintf(stderr,
            "usage: %s [--threads N] [--iterations N|--total-iterations N] "
            "[--warmup N] [--data-count N] [--seed N] "
            "[--data-mode all_zero|all_nonzero|mixed]\n", program);
}

static int parse_options(int argc, char **argv, uint32_t *threads,
                         uint64_t *iterations, uint64_t *total_iterations,
                         bool *total_iterations_specified, uint64_t *warmup,
                         uint32_t *data_count, uint64_t *seed,
                         data_mode_t *mode)
{
    for (int argument = 1; argument < argc; ++argument) {
        if (strcmp(argv[argument], "--threads") == 0 && argument + 1 < argc) {
            uint64_t parsed = parse_u64(argv[++argument], "threads");
            if (parsed == 0U || parsed > UINT32_MAX) {
                fprintf(stderr, "threads must be in 1..UINT32_MAX\n");
                return EXIT_FAILURE;
            }
            *threads = (uint32_t)parsed;
        } else if (strcmp(argv[argument], "--iterations") == 0 && argument + 1 < argc) {
            *iterations = parse_u64(argv[++argument], "iterations");
        } else if (strcmp(argv[argument], "--total-iterations") == 0 && argument + 1 < argc) {
            *total_iterations = parse_u64(argv[++argument], "total-iterations");
            *total_iterations_specified = true;
        } else if (strcmp(argv[argument], "--warmup") == 0 && argument + 1 < argc) {
            *warmup = parse_u64(argv[++argument], "warmup");
        } else if (strcmp(argv[argument], "--data-count") == 0 && argument + 1 < argc) {
            uint64_t parsed = parse_u64(argv[++argument], "data-count");
            if (parsed == 0U || parsed > UINT32_MAX || (parsed & (parsed - 1U)) != 0U) {
                fprintf(stderr, "data-count must be a non-zero power of two fitting uint32_t\n");
                return EXIT_FAILURE;
            }
            *data_count = (uint32_t)parsed;
        } else if (strcmp(argv[argument], "--seed") == 0 && argument + 1 < argc) {
            *seed = parse_u64(argv[++argument], "seed");
        } else if (strcmp(argv[argument], "--data-mode") == 0 && argument + 1 < argc) {
            if (parse_data_mode(argv[++argument], mode) != 0) {
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[argument], "--help") == 0) {
            print_usage(argv[0]);
            return 1;
        } else {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }
    return 0;
}

static void *worker(void *argument)
{
    worker_arg_t *worker_arg = argument;
    uint64_t checksum = 0U;
    uint64_t successes = 0U;

    for (uint64_t iteration = 0; iteration < worker_arg->iterations; ++iteration) {
        bignum_t value;
        uint64_t index = (iteration + worker_arg->thread_index) &
                         (worker_arg->data_count - 1U);
        uint64_t input = worker_arg->data[index];
        bignum_init_u64_status_t status = bignum_init_u64(&value, input);
        if (status != BIGNUM_INIT_U64_SUCCESS) {
            worker_arg->status = (int)status;
            return NULL;
        }
        ++successes;
        checksum += value.words[0] + value.len;
    }

    worker_arg->checksum = checksum;
    worker_arg->successes = successes;
    worker_arg->status = 0;
    return NULL;
}

int main(int argc, char **argv)
{
    uint32_t thread_count = DEFAULT_THREAD_COUNT;
    uint64_t iterations = DEFAULT_ITERATIONS_PER_THREAD;
    uint64_t total_iterations = DEFAULT_TOTAL_ITERATIONS;
    bool total_iterations_specified = false;
    uint64_t warmup_iterations = DEFAULT_WARMUP_ITERATIONS;
    uint32_t data_count = DEFAULT_DATA_COUNT;
    uint64_t seed = DEFAULT_SEED;
    data_mode_t mode = DATA_MODE_ALL_NONZERO;

    int option_result = parse_options(argc, argv, &thread_count, &iterations,
                                      &total_iterations, &total_iterations_specified,
                                      &warmup_iterations, &data_count, &seed, &mode);
    if (option_result != 0) {
        return option_result == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    if (total_iterations_specified) {
        if (total_iterations == 0U || total_iterations % thread_count != 0U) {
            fprintf(stderr, "total-iterations must be non-zero and divisible by threads\n");
            return EXIT_FAILURE;
        }
        iterations = total_iterations / thread_count;
    } else {
        if (iterations == 0U) {
            fprintf(stderr, "iterations must be non-zero\n");
            return EXIT_FAILURE;
        }
        total_iterations = iterations * (uint64_t)thread_count;
    }

    uint64_t *data = malloc((size_t)data_count * sizeof(*data));
    pthread_t *thread_handles = calloc(thread_count, sizeof(*thread_handles));
    worker_arg_t *arguments = calloc(thread_count, sizeof(*arguments));
    if (data == NULL || thread_handles == NULL || arguments == NULL) {
        perror("malloc benchmark state");
        free(data);
        free(thread_handles);
        free(arguments);
        return EXIT_FAILURE;
    }

    uint64_t generator_state = seed;
    uint64_t data_fingerprint = UINT64_C(1469598103934665603);
    for (uint32_t index = 0; index < data_count; ++index) {
        uint64_t generated = next_u64(&generator_state);
        if (mode == DATA_MODE_ALL_ZERO || (mode == DATA_MODE_MIXED && (index & 1U) != 0U)) {
            data[index] = 0U;
        } else {
            data[index] = generated == 0U ? UINT64_C(1) : generated;
        }
        data_fingerprint = fnv1a_update_u64(data_fingerprint, data[index]);
    }

    volatile uint64_t warmup_checksum = 0U;
    for (uint64_t iteration = 0; iteration < warmup_iterations; ++iteration) {
        bignum_t value;
        uint64_t input = data[iteration & (data_count - 1U)];
        bignum_init_u64_status_t status = bignum_init_u64(&value, input);
        if (status != BIGNUM_INIT_U64_SUCCESS ||
            value.len != (input == 0U ? 0U : 1U) || value.words[0] != input) {
            fprintf(stderr, "API contract failure during warm-up\n");
            free(data);
            free(thread_handles);
            free(arguments);
            return EXIT_FAILURE;
        }
        warmup_checksum += value.words[0] + value.len;
    }

    struct timespec begin;
    struct timespec end;
    if (clock_gettime(CLOCK_MONOTONIC, &begin) != 0) {
        perror("clock_gettime begin");
        free(data);
        free(thread_handles);
        free(arguments);
        return EXIT_FAILURE;
    }

    uint32_t created = 0U;
    for (uint32_t index = 0; index < thread_count; ++index) {
        arguments[index].data = data;
        arguments[index].data_count = data_count;
        arguments[index].iterations = iterations;
        arguments[index].thread_index = index;
        arguments[index].status = 0;
        if (pthread_create(&thread_handles[index], NULL, worker, &arguments[index]) != 0) {
            fprintf(stderr, "pthread_create failed for thread %" PRIu32 "\n", index);
            for (uint32_t joined = 0; joined < created; ++joined) {
                pthread_join(thread_handles[joined], NULL);
            }
            free(data);
            free(thread_handles);
            free(arguments);
            return EXIT_FAILURE;
        }
        ++created;
    }

    uint64_t checksum = 0U;
    uint64_t successes = 0U;
    int worker_status = 0;
    for (uint32_t index = 0; index < created; ++index) {
        pthread_join(thread_handles[index], NULL);
        checksum += arguments[index].checksum;
        successes += arguments[index].successes;
        if (arguments[index].status != 0) {
            worker_status = arguments[index].status;
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        perror("clock_gettime end");
        free(data);
        free(thread_handles);
        free(arguments);
        return EXIT_FAILURE;
    }
    if (worker_status != 0) {
        fprintf(stderr, "unexpected status during timed loop: %d\n", worker_status);
        free(data);
        free(thread_handles);
        free(arguments);
        return EXIT_FAILURE;
    }

    double seconds = elapsed_seconds(&begin, &end);
    if (seconds <= 0.0) {
        fprintf(stderr, "non-positive elapsed time\n");
        free(data);
        free(thread_handles);
        free(arguments);
        return EXIT_FAILURE;
    }

    printf("benchmark=bignum_init_u64_mt"
           " data_mode=%s"
           " seed=0x%016" PRIx64
           " data_count=%" PRIu32
           " data_fingerprint=0x%016" PRIx64
           " threads=%" PRIu32
           " iterations_per_thread=%" PRIu64
           " total_iterations=%" PRIu64
           " warmup_iterations=%" PRIu64
           " elapsed_seconds=%.9f"
           " ns_per_call=%.3f"
           " throughput_calls_per_second=%.3f"
           " successes=%" PRIu64
           " checksum=%" PRIu64
           " warmup_checksum=%" PRIu64 "\n",
           data_mode_name(mode), seed, data_count, data_fingerprint, thread_count, iterations,
           total_iterations, warmup_iterations, seconds,
           seconds * 1000000000.0 / (double)total_iterations,
           (double)total_iterations / seconds, successes, checksum,
           (uint64_t)warmup_checksum);

    free(data);
    free(thread_handles);
    free(arguments);
    return EXIT_SUCCESS;
}
