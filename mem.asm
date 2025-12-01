.section .text

// inicio de biblis/impressao.asm
// fn: [_escrever_int]
.align 2
_escrever_int:
    mov w1, w0 // w1 = numero
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
    .fill   32, 1, 0
// fim: [_escrever_int]
// fim de biblis/impressao.asm

.global inicio
// fn: [inicio] (vars: 0, total: 128)
.align 2
inicio:
  sub sp, sp, 128
  stp x29, x30, [sp, 112]
  add x29, sp, 112
  mov w0, 5
  ldr x1, = global_teste
  str w0, [x1]
  ldr x0, = global_teste
  ldr w0, [x0]
  bl _escrever_int
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 112]
  add sp, sp, 128
  mov x0, 0
  mov x8, 93
  svc 0
  ret
// fim: [inicio]

.section .data
.align 3
global_teste:
  .word 1

.section .text

.section .rodata
.align 2
