; -----------------------------------------------------------------------------
; @file    bignum_init_u64.asm
; @brief   Инициализация bignum_t из uint64_t на x86-64 YASM.
; @details System V ABI: rdi = bignum_t *, rsi = uint64_t.
; -----------------------------------------------------------------------------
; SPDX-License-Identifier: MIT
; -----------------------------------------------------------------------------

default rel
section .text
    align 16
    global bignum_init_u64

BIGNUM_CAPACITY             equ 32
BIGNUM_WORD_SIZE            equ 8
BIGNUM_OFFSET_WORDS         equ 0
BIGNUM_OFFSET_LEN           equ BIGNUM_CAPACITY * BIGNUM_WORD_SIZE
BUF_QWORDS                  equ BIGNUM_CAPACITY + 1
BIGNUM_INIT_U64_SUCCESS     equ 0
BIGNUM_INIT_U64_ERROR_NULL_ARG equ -1

; bignum_init_u64_status_t bignum_init_u64(bignum_t *b, uint64_t val)
bignum_init_u64:
    test    rdi, rdi
    jz      .ret_error

    mov     r8, rdi

    ; Обнуляем words[0..31] и len.
    xor     eax, eax
    mov     ecx, BUF_QWORDS
    rep     stosq

    ; Записываем значение после очистки всей структуры.
    mov     [r8 + BIGNUM_OFFSET_WORDS], rsi

    ; len = (val != 0) ? 1 : 0.
    xor     eax, eax
    test    rsi, rsi
    setnz   al
    mov     [r8 + BIGNUM_OFFSET_LEN], rax

    mov     eax, BIGNUM_INIT_U64_SUCCESS
    ret

.ret_error:
    mov     eax, BIGNUM_INIT_U64_ERROR_NULL_ARG
    ret
