    .section .text
    .align 2

_imprime_int:
    stp     x29, x30, [sp, #-32]!
    mov     x29, sp
    adr     x1, .Lint_buffer
    mov     x2, 0
    cmp     w0, #0
    b.ge    .Lpos
    neg     w0, w0
    mov     w3, '-'
    strb    w3, [x1], #1
    mov     x2, 1

.Lpos:
    .align 2
    mov     x3, x1

.Lloop:
    .align 2
    mov     w4, 10
    udiv    w5, w0, w4
    msub    w6, w5, w4, w0
    add     w6, w6, #'0'
    strb    w6, [x3], #1
    add     x2, x2, #1
    mov     w0, w5
    cbnz    w0, .Lloop

    sub     x3, x3, #1

.Lrev:
    .align 2
    cmp     x1, x3
    b.ge    .Lfim
    ldrb    w5, [x1]
    ldrb    w6, [x3]
    strb    w5, [x3], #-1
    strb    w6, [x1], #1
    b       .Lrev

.Lfim:
    .align 2
    adr     x1, .Lint_buffer
    mov     x0, 1
    mov     x8, 64
    svc     0
    ldp     x29, x30, [sp], #32
    ret

    .section .data
    .align 2
.Lint_buffer:
    .fill   32, 1, 0

    .section .text
    .align 2
_imprime_float:
    stp     x29, x30, [sp, #-32]!
    mov     x29, sp
    adr     x1, .Lfloat_buffer
    mov     x0, 1
    mov     x2, 12
    mov     x8, 64
    svc     0
    ldp     x29, x30, [sp], #32
    ret

    .section .data
    .align 2
.Lfloat_buffer:
    .asciz  "[float]"

    .section .text
    .align 2
_imprime_double:
    stp     x29, x30, [sp, #-32]!
    mov     x29, sp
    adr     x1, .Ldouble_buffer
    mov     x0, 1
    mov     x2, 12
    mov     x8, 64
    svc     0
    ldp     x29, x30, [sp], #32
    ret

    .section .data
    .align 2
.Ldouble_buffer:
    .asciz  "[double]"

    .section .text
    .align 2
_imprime_char:
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp
    strb    w0, [sp, #-1]!
    mov     x0, 1
    mov     x1, sp
    mov     x2, 1
    mov     x8, 64
    svc     0
    add     sp, sp, #1
    ldp     x29, x30, [sp], #16
    ret

    .section .text
    .align 2
_imprime_bool:
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp
    cmp     w0, #0
    adr     x1, .Lfalso
    adr     x2, .Lverdade
    csel    x1, x1, x2, eq
    mov     x0, 1
    mov     x2, 7
    cbnz    w0, .Limpr
    mov     x2, 5

.Limpr:
    .align 2
    mov     x8, 64
    svc     0
    ldp     x29, x30, [sp], #16
    ret

    .section .data
    .align 2
.Lverdade:
    .asciz  "verdade"
    .align 2
.Lfalso:
    .asciz  "falso"