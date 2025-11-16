.section .text
.global _start
.align 2
_start:
  bl inicio
  mov x0, 0
  mov x8, 93
  svc 0

// início de tmp/texint.asm
.section .text
.align 2
// [TEXTO]
_escrever_tex:
    mov x1, x0 // x1 = texto
    mov x2, 0 // x2 = contador
    // conta caracteres até encontrar null
1:
    ldrb w3, [x1, x2]
    cbz w3, 2f
    add x2, x2, 1
    b 1b
    
2:
    mov x0, 1
    mov x8, 64
    svc 0
    ret
.align 2
// [INTEIRO]
_escrever_int:
    mov w1, w0 // w1 = número
    ldr x0, = 5f // x0 = buffer
    mov x19, 0 // x19 = contador de caracteres
    
    cmp w1, 0
    b.ge 1f
    neg w1, w1 // torna positivo
    mov w2, '-'
    strb w2, [x0], 1 // escreve sinal
    mov x19, 1 // contador = 1

1:
    // escreve dígitos em ordem reversa
    mov x2, x0 // x2: aponta pra posição atual
2:
    mov w3, 10
    udiv w4, w1, w3 // w4 = quociente
    msub w5, w4, w3, w1 // w5 = resto
    add w5, w5, '0' // caractere
    strb w5, [x2], 1 // armazena
    add x19, x19, 1 // incrementa contador
    mov w1, w4
    cbnz w1, 2b
    // inverte a string de dígitos(a parte após o sinal, se existir)
    // x0: aponta pro início dos dígitos(pode ser buffer_int ou buffer_int+1)
    // x2-1: é o último dígito
    sub x2, x2, 1 // x2 aponta para o último dígito
    mov x3, x0 // x3 aponta para o primeiro dígito
3:
    cmp x3, x2
    b.ge 4f
    ldrb w4, [x3]
    ldrb w5, [x2]
    strb w5, [x3], 1
    strb w4, [x2], -1
    b 3b

4:
    ldr x1, = 5f
    mov x0, 1
    mov x2, x19 // x19: o número de caracteres
    mov x8, 64
    svc 0
    ret

.section .data
  .align 2
5: // buffer do inteiro
    .fill   32, 1, 0// fim de tmp/texint.asm

.align 2
fib:
  sub sp, sp, 192
  stp x29, x30, [sp]
  mov x29, sp
  stp x19, x20, [x29, 16]
  stp x21, x22, [x29, 32]
  str x0, [x29, 48]  // salvar param n
  ldr w0, [x29, 48]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, le
  cmp w0, 0
  beq .B1
  ldr w0, [x29, 48]
  b .epilogo_0
  b .B2
.B1:
.B2:
  ldr w0, [x29, 48]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  sub w0, w1, w0
  str w0, [sp, -16]!
  ldr x0, [sp, 0]
  add sp, sp, 16
  bl fib
  str w0, [sp, -16]!
  ldr w0, [x29, 48]
  str w0, [sp, -16]!
  mov w0, 2
  ldr w1, [sp], 16
  sub w0, w1, w0
  str w0, [sp, -16]!
  ldr x0, [sp, 0]
  add sp, sp, 16
  bl fib
  ldr w1, [sp], 16
  add w0, w1, w0
  b .epilogo_0
  b .epilogo_0
.epilogo_0:
  ldp x19, x20, [x29, 16]
  ldp x21, x22, [x29, 32]
  mov sp, x29
  ldp x29, x30, [sp]
  add sp, sp, 192
  ret
.align 2
teste_fibonacci:
  sub sp, sp, 160
  stp x29, x30, [sp]
  mov x29, sp
  ldr x0, =.tex_0
  bl _escrever_tex
  mov w0, 15
  str w0, [sp, -16]!
  ldr x0, [sp, 0]
  add sp, sp, 16
  bl fib
  str w0, [x29, 32]
  ldr x0, =.tex_1
  bl _escrever_tex
  ldr w0, [x29, 32]
  bl _escrever_int
  ldr x0, =.tex_2
  bl _escrever_tex
  b .epilogo_1
.epilogo_1:
  mov sp, x29
  ldp x29, x30, [sp]
  add sp, sp, 160
  ret
.align 2
teste_loops:
  sub sp, sp, 160
  stp x29, x30, [sp]
  mov x29, sp
  ldr x0, =.tex_3
  bl _escrever_tex
  mov w0, 0
  str w0, [x29, 32]
  mov w0, 0
  str w0, [x29, 48]
  mov w0, 0
  str w0, [x29, 64]
.B3:
  ldr w0, [x29, 48]
  str w0, [sp, -16]!
  mov w0, 500
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B4
  mov w0, 0
  str w0, [x29, 64]
.B5:
  ldr w0, [x29, 64]
  str w0, [sp, -16]!
  mov w0, 500
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B6
  ldr w0, [x29, 32]
  str w0, [sp, -16]!
  ldr w0, [x29, 48]
  str w0, [sp, -16]!
  ldr w0, [x29, 64]
  ldr w1, [sp], 16
  mul w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 97
  ldr w1, [sp], 16
  sdiv w2, w1, w0
  msub w0, w2, w0, w1
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [x29, 32]
  ldr w0, [x29, 64]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [x29, 64]
  b .B5
.B6:
  ldr w0, [x29, 48]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [x29, 48]
  b .B3
.B4:
  ldr x0, =.tex_4
  bl _escrever_tex
  ldr w0, [x29, 32]
  bl _escrever_int
  ldr x0, =.tex_2
  bl _escrever_tex
  b .epilogo_2
.epilogo_2:
  mov sp, x29
  ldp x29, x30, [sp]
  add sp, sp, 160
  ret
.align 2
teste_loops_por:
  sub sp, sp, 160
  stp x29, x30, [sp]
  mov x29, sp
  ldr x0, =.tex_5
  bl _escrever_tex
  mov w0, 0
  str w0, [x29, 32]
  mov w0, 0
  str w0, [x29, -144]
.B8:
  ldr w0, [x29, -144]
  str w0, [sp, -16]!
  mov w0, 500
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B9
  mov w0, 0
  str w0, [x29, -128]
.B11:
  ldr w0, [x29, -128]
  str w0, [sp, -16]!
  mov w0, 500
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B12
  ldr w0, [x29, 32]
  str w0, [sp, -16]!
  ldr w0, [x29, -144]
  str w0, [sp, -16]!
  ldr w0, [x29, -128]
  ldr w1, [sp], 16
  mul w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 97
  ldr w1, [sp], 16
  sdiv w2, w1, w0
  msub w0, w2, w0, w1
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [x29, 32]
  // incremento
  ldr w0, [x29, -128]
  add w0, w0, 1
  str w0, [x29, -128]
  b .B11
.B12:
  // incremento
  ldr w0, [x29, -144]
  add w0, w0, 1
  str w0, [x29, -144]
  b .B8
.B9:
  ldr x0, =.tex_4
  bl _escrever_tex
  ldr w0, [x29, 32]
  bl _escrever_int
  ldr x0, =.tex_2
  bl _escrever_tex
  b .epilogo_3
.epilogo_3:
  mov sp, x29
  ldp x29, x30, [sp]
  add sp, sp, 160
  ret
.align 2
teste_operacoes:
  sub sp, sp, 160
  stp x29, x30, [sp]
  mov x29, sp
  ldr x0, =.tex_6
  bl _escrever_tex
  mov w0, 0
  str w0, [x29, 32]
  mov w0, 0
  str w0, [x29, 48]
  mov w0, 0
  str w0, [x29, -144]
.B14:
  ldr w0, [x29, -144]
  str w0, [sp, -16]!
  mov w0, 10000
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B15
  ldr w0, [x29, -144]
  str w0, [sp, -16]!
  mov w0, 3
  ldr w1, [sp], 16
  sdiv w2, w1, w0
  msub w0, w2, w0, w1
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B17
  ldr w0, [x29, -144]
  str w0, [sp, -16]!
  mov w0, 100
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, gt
  cmp w0, 0
  beq .B17
  mov w0, 1
  b .B18
.B17:
  mov w0, 0
.B18:
  cmp w0, 0
  beq .B19
  ldr w0, [x29, 48]
  str w0, [sp, -16]!
  ldr w0, [x29, -144]
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [x29, 48]
  b .B20
.B19:
.B20:
  // incremento
  ldr w0, [x29, -144]
  add w0, w0, 1
  str w0, [x29, -144]
  b .B14
.B15:
  ldr x0, =.tex_7
  bl _escrever_tex
  ldr w0, [x29, 48]
  bl _escrever_int
  ldr x0, =.tex_2
  bl _escrever_tex
  b .epilogo_4
.epilogo_4:
  mov sp, x29
  ldp x29, x30, [sp]
  add sp, sp, 160
  ret
.align 2
inicio:
  sub sp, sp, 160
  stp x29, x30, [sp]
  mov x29, sp
  ldr x0, =.tex_8
  bl _escrever_tex
  ldr x0, =.tex_9
  bl _escrever_tex
  bl teste_fibonacci
  bl teste_loops_por
  bl teste_loops
  bl teste_operacoes
  ldr x0, =.tex_9
  bl _escrever_tex
  ldr x0, =.tex_10
  bl _escrever_tex
  b .epilogo_5
.epilogo_5:
  mov sp, x29
  ldp x29, x30, [sp]
  add sp, sp, 160
  ret
.section .rodata
.align 2
.tex_0: .asciz "=== TESTE FIBONACCI ===\n"
.tex_1: .asciz "Fibonacci(15) = "
.tex_2: .asciz "\n"
.tex_3: .asciz "=== TESTE LOOPS ANINHADOS ===\n"
.tex_4: .asciz "Soma loops: "
.tex_5: .asciz "=== TESTE LOOPS COM POR ===\n"
.tex_6: .asciz "=== TESTE OPERAÇÕES ===\n"
.tex_7: .asciz "Total: "
.tex_8: .asciz "TESTE DE PERFORMACE\n"
.tex_9: .asciz "=======================\n"
.tex_10: .asciz "FIM\n"
.section .text

