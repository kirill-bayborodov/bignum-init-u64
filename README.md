# bignum-init-u64

[![C/ASM CI](https://github.com/kirill-bayborodov/bignum-init-u64/actions/workflows/ci.yml/badge.svg)](https://github.com/kirill-bayborodov/bignum-init-u64/actions/workflows/ci.yml)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/kirill-bayborodov/bignum-init-u64?label=release)](https://github.com/kirill-bayborodov/bignum-init-u64/releases/latest)
`bignum-init-u64` is a high-performance standalone module for initializing an arbitrary-precision integer (`bignum_t`) from a 64-bit unsigned value. The production implementation is written in x86-64 YASM assembly and follows the System V AMD64 ABI.

The function completely reinitializes the destination object, stores the supplied `uint64_t` value in `words[0]`, sets the normalized length to zero or one, and returns a status code. A `NULL` destination is rejected without touching memory.

## Distribution

The module is designed as a standalone component of the `bignum-lib` family. It depends on `bignum-core`, included as a Git submodule at `libs/bignum-core`.

## Features

- **x86-64 assembly implementation:** the production path is implemented in YASM and is suitable for performance-critical code.
- **Status-returning API:** success and invalid-argument conditions are represented explicitly by `bignum_init_u64_status_t`.
- **Complete reinitialization:** every call clears the complete `bignum_t` object before storing the new value.
- **Deterministic tests:** the primary test suite covers the required API contract and boundary values.
- **Extended tests:** extra tests cover guard canaries, adjacent objects, state transitions, and 100,000 deterministic randomized cases.
- **Multithreaded validation:** independent objects are initialized concurrently to verify thread safety.
- **Reproducible benchmarks:** ST and MT benchmarks support deterministic fingerprints, checksums, warm-up iterations, and the `all_zero`, `all_nonzero`, and `mixed` data modes.
- **Perf integration:** the Makefile supports repeated `perf stat` runs, raw `perf.data` preservation, and separate ST/MT reports.
- **Static analysis:** C sources are checked with `cppcheck` through the `make lint` target.

## Dependencies

Build-time dependencies are:

| Dependency | Purpose |
|---|---|
| `make` | Build, test, lint, benchmark, and distribution targets |
| `gcc` | C compilation and linking |
| `yasm` | x86-64 assembly compilation |
| `cppcheck` | Static analysis |
| `perf` | Hardware/software performance counters and sampling profiles |
| `taskset` | CPU affinity control for reproducible ST/MT runs |
| `pthread` | Multithreaded tests and benchmark |

The project requires the `bignum-core` Git submodule. Clone the repository with submodules:

```bash
git clone --recurse-submodules <repository-url> bignum-init-u64
cd bignum-init-u64
```

If the repository was cloned without submodules, initialize them with:

```bash
git submodule update --init --recursive
```

## API

The public API is declared in `include/bignum_init_u64.h`:

```c
bignum_init_u64_status_t bignum_init_u64(bignum_t *restrict b, uint64_t val);
```

The function has the following contract:

| Input or result | Contract |
|---|---|
| `b == NULL` | Returns `BIGNUM_INIT_U64_ERROR_NULL_ARG` and does not access destination memory |
| `val == 0` | Clears the object and returns success with `len == 0` |
| `val != 0` | Clears the object, stores `val` in `words[0]`, and returns success with `len == 1` |
| Repeated invocation | The previous object contents are fully overwritten |
| `BIGNUM_INIT_U64_SUCCESS` | Numeric value `0`, indicating successful initialization |
| `BIGNUM_INIT_U64_ERROR_NULL_ARG` | Numeric value `-1`, indicating a `NULL` destination |

Example:

```c
#include <stdint.h>
#include "bignum_init_u64.h"

int initialize_value(bignum_t *value)
{
    bignum_init_u64_status_t status =
        bignum_init_u64(value, UINT64_C(42));

    return status == BIGNUM_INIT_U64_SUCCESS ? 0 : -1;
}
```

## Build, Test, Lint, and Distribution

The Makefile manages all project operations.

### Build

Build the release object and dependencies:

```bash
make build CONFIG=release
```

The main production object is generated at:

```text
build/bignum_init_u64.o
```

### Unit tests

Run the deterministic, extended, multithreaded, and runner test suites:

```bash
make test CONFIG=release
```

The expected result is:

```text
=== Summary: 0 / 4 failed ===
```

### Static analysis

Run `cppcheck` over the project C sources:

```bash
make lint
```

### Installation and distribution

Build the object-file distribution:

```bash
make install CONFIG=release
```

Build the single-header and static-library distribution:

```bash
make dist CONFIG=release
```

### Clean up

Remove generated build artifacts, binaries, distributions, and reports:

```bash
make clean
```

## Benchmarks

The benchmark suite contains single-thread and multithread variants:

```text
benchmarks/bench_bignum_init_u64.c
benchmarks/bench_bignum_init_u64_mt.c
```

Both programs generate deterministic input data and print a fingerprint, checksum, elapsed time, nanoseconds per call, throughput, and successful-call count. The data modes are:

| Mode | Purpose |
|---|---|
| `all_zero` | Measures the zero-value input path |
| `all_nonzero` | Measures the normal nonzero-value path |
| `mixed` | Alternates zero and nonzero inputs to exercise a mixed workload |

### Single-thread benchmark CLI

```text
bin/bench_bignum_init_u64 \
  [--iterations N] \
  [--warmup N] \
  [--data-count N] \
  [--seed N] \
  [--data-mode all_zero|all_nonzero|mixed]
```

Example:

```bash
./bin/bench_bignum_init_u64 \
  --iterations 1000000 \
  --warmup 10000 \
  --data-count 8192 \
  --seed 0x9e3779b97f4a7c15 \
  --data-mode mixed
```

### Multithread benchmark CLI

The MT benchmark supports both the legacy per-thread option and the reproducible total-work option:

```text
bin/bench_bignum_init_u64_mt \
  [--threads N] \
  [--iterations N|--total-iterations N] \
  [--warmup N] \
  [--data-count N] \
  [--seed N] \
  [--data-mode all_zero|all_nonzero|mixed]
```

For fair comparison between one and two threads, keep the total number of calls constant:

```bash
./bin/bench_bignum_init_u64_mt \
  --threads 1 \
  --total-iterations 3200000000 \
  --data-mode mixed

./bin/bench_bignum_init_u64_mt \
  --threads 2 \
  --total-iterations 3200000000 \
  --data-mode mixed
```

`--total-iterations` must be nonzero and divisible by `--threads`. The benchmark computes `iterations_per_thread` internally. The option `--iterations` remains available for compatibility and means iterations per thread.

## Perf benchmark workflow

The default environment provides two CPUs, so the default MT configuration is:

```make
MT_THREADS=2
MT_CPU_LIST=0-1
MT_TOTAL_ITERATIONS=3200000000
```

Run the full `perf record` and repeated `perf stat` workflow for all three input modes:

```bash
make bench_full CONFIG=release \
  REPORT_NAME=baseline \
  PERF_RUNS=7 \
  KEEP_PERF=1
```

The target executes `all_zero`, `all_nonzero`, and `mixed` in both ST and MT modes. It preserves raw perf files and writes reports under `benchmarks/reports/`.

Typical output files include:

```text
benchmarks/reports/baseline_all_zero_st.perf.data
benchmarks/reports/baseline_all_zero_st.txt
benchmarks/reports/baseline_all_zero_st_stat.csv
benchmarks/reports/baseline_all_zero_st_runtime.txt
benchmarks/reports/baseline_all_zero_mt.perf.data
benchmarks/reports/baseline_all_zero_mt.txt
benchmarks/reports/baseline_all_zero_mt_stat.csv
benchmarks/reports/baseline_all_zero_mt_runtime.txt
```

For a shorter targeted run:

```bash
make bench_stat_st CONFIG=release \
  REPORT_NAME=baseline_st_mixed \
  DATA_MODE=mixed \
  PERF_RUNS=7

make bench_stat_mt CONFIG=release \
  REPORT_NAME=baseline_mt2_mixed \
  DATA_MODE=mixed \
  MT_THREADS=2 \
  MT_CPU_LIST=0-1 \
  MT_TOTAL_ITERATIONS=3200000000 \
  PERF_RUNS=7
```

The Makefile validates that every repeated run prints the expected benchmark identifier, data mode, and elapsed-time field. An argument-parsing failure therefore cannot be mistaken for a valid performance profile.

## Recommended optimization cycle

Use the following procedure for an A/B comparison:

```bash
make clean
make test CONFIG=release
make bench_full CONFIG=release \
  REPORT_NAME=baseline \
  PERF_RUNS=7 \
  KEEP_PERF=1
```

Make the implementation change, then repeat:

```bash
make clean
make test CONFIG=release
make bench_full CONFIG=release \
  REPORT_NAME=opt_v1 \
  PERF_RUNS=7 \
  KEEP_PERF=1
```

Compare only matching data modes and matching ST/MT configurations:

```bash
diff -u \
  benchmarks/reports/baseline_all_nonzero_st_stat.csv \
  benchmarks/reports/opt_v1_all_nonzero_st_stat.csv

diff -u \
  benchmarks/reports/baseline_mixed_mt_stat.csv \
  benchmarks/reports/opt_v1_mixed_mt_stat.csv
```

For raw sample analysis:

```bash
perf report \
  -i benchmarks/reports/baseline_mixed_st.perf.data \
  --stdio
```

Use `cycles/call`, `ref-cycles/call`, `instructions/call`, cache misses, branch misses, throughput, and runtime spread together. A changed checksum or fingerprint under identical input parameters invalidates the performance comparison.

## Using the object file

The project produces `build/bignum_init_u64.o`, which can be linked into an application together with the `bignum-core` headers:

```bash
make build CONFIG=release

gcc your_app.c \
  build/bignum_init_u64.o \
  -I./include \
  -I./libs/bignum-core/include \
  -o your_app \
  -no-pie
```

The application must link against the same ABI and provide the `bignum_t` definition from `bignum-core`.

## Contributing

Contributions are welcome. Please fork the repository, create a feature branch, implement and test the change, run `make test CONFIG=release` and `make lint`, and submit a pull request with the relevant benchmark evidence when performance is affected.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
