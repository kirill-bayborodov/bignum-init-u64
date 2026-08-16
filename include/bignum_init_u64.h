/* ------------------------------------------------------------------ */
/**
 * @file    bignum_init_u64.h
 * @brief   Инициализация bignum_t из беззнакового 64-битного значения.
 */
/* ------------------------------------------------------------------ */
#pragma once
#ifndef BIGNUM_INIT_U64_H
#define BIGNUM_INIT_U64_H

#include <stdint.h>
#include "bignum.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Результаты выполнения bignum_init_u64. */
typedef enum {
    BIGNUM_INIT_U64_SUCCESS        = 0,
    BIGNUM_INIT_U64_ERROR_NULL_ARG = -1
} bignum_init_u64_status_t;

/**
 * @brief Инициализирует bignum_t из 64-битного числа.
 *
 * При NULL функция не обращается к памяти и возвращает
 * BIGNUM_INIT_U64_ERROR_NULL_ARG. Для нулевого значения результатом является
 * полностью очищенная структура с len == 0. Для ненулевого значения words[0]
 * получает val, len становится равным 1, а остальные слова обнуляются.
 *
 * @param[out] b   Структура, которую следует инициализировать.
 * @param[in]  val Значение для записи.
 * @return Статус операции.
 */
bignum_init_u64_status_t bignum_init_u64(bignum_t *restrict b, uint64_t val);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_INIT_U64_H */

/* SPDX-License-Identifier: MIT */
