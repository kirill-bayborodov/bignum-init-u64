# bignum_init_u64 standard benchmark profile

This companion document defines the `bignum_init_u64_standard.json` matrix. The adapter accepts only `init_u64` and `mixed` operation kinds, uses deterministic seed transport, and consumes framework tools from `libs/benchmark-framework/dist/tools`.

The matrix is a smoke/reproducibility contract, not a performance baseline. Run it with `make bench_matrix CONFIG=release BENCH_MATRIX_PROFILE=benchmarks/profiles/bignum_init_u64_standard.json`.
