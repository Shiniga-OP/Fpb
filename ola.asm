.section .text
.global _start
.align 2
_start:
  bl inicio
  mov x0, 0
  mov x8, 93
  svc 0

// início de biblis/texint.asm
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
    .fill   32, 1, 0// fim de biblis/texint.asm

.align 2
inicio:
  sub sp, sp, 160
  stp x29, x30, [sp]
  mov x29, sp
  ldr x0, =.tex_0
  bl _escrever_tex
  b .epilogo_0
.epilogo_0:
  mov sp, x29
  ldp x29, x30, [sp]
  add sp, sp, 160
  ret
.section .rodata
.align 2
.tex_0: .asciz "olá mundo\n"
.section .text

