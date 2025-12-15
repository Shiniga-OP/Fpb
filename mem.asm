.section .text

// inicio de biblis/impressao.asm
// fn: [_escrever_longo]
.align 2
_escrever_longo:
    mov x1, x0 // x1 = numero(64 bits)
    ldr x0, =5f // x0 = buffer
    mov x19, 0 // x19 = contador de caracteres
    
    cmp x1, 0 // compara 64 bits
    b.ge 1f
    neg x1, x1 // torna positivo(64 bits)
    mov w2, '-'
    strb w2, [x0], 1 // escreve sinal(w2 é 32 bits)
    mov x19, 1 // contador = 1
1:
    // escreve digitos em ordem reversa
    mov x2, x0 // x2: aponta pra posição atual
2:
    mov x3, 10
    udiv x4, x1, x3 // x4 = quociente(64 bits)
    msub x5, x4, x3, x1 // x5 = resto(64 bits)
    add w5, w5, '0' // converte resto pra caractere (w5)
    strb w5, [x2], 1 // armazena o byte(w5)
    add x19, x19, 1 // incrementa contador
    mov x1, x4
    cbnz x1, 2b // continua se x1 != 0
    // inverte o texto de digitos
    sub x2, x2, 1 // x2 aponta para o ultimo digito
    mov x3, x0 // x3 aponta para o primeiro digito
3:
    cmp x3, x2
    b.ge 4f
    ldrb w4, [x3] // carrega byte(w4)
    ldrb w5, [x2] // carrega byte(w5)
    strb w5, [x3], 1 // armazena byte(w5)
    strb w4, [x2], -1 // armazena byte(w4)
    b 3b
4:
    ldr     x1, = 5f
    mov     x0, 1
    mov     x2, x19 // x19: o numero de caracteres
    mov x8, 64
    svc 0
    ret
.section .data
  .align 2
5: // buffer do inteiro
    .fill   32, 1, 0
// fim: [_escrever_longo]
// fn: [_escrever_car]
.align 2
_escrever_car:
    strb w0, [sp, -1]!
    mov x0, 1
    mov x1, sp
    mov x2, 1
    mov x8, 64
    svc 0
    add sp, sp, 1
    ret
// fim: [_escrever_car]
// fim de biblis/impressao.asm

.global inicio
// fn: [inicio] (vars: 32, total: 160)
.align 2
inicio:
  sub sp, sp, 160
  stp x29, x30, [sp, 144]
  add x29, sp, 144
  mov w0, 0
  mov w19, w0
  str x0, [x29, -32]
  mov w0, 0
  mov w19, w0
  str x0, [x29, 144]
.B1:
  ldr x0, [x29, 144]
  mov x19, x0
  movz w0, 61568
  movk w0, 762, lsl 16
  mov x1, x19
  cmp x1, x0
  cset x0, lt
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B2
  ldr x0, [x29, -32]
  str x0, [sp, -16]!
  ldr x0, [x29, 144]
  ldr x1, [sp], 16
  add x0, x1, x0
  mov x19, x0
  mov x0, x19
  str x0, [x29, -32]
  ldr x0, [x29, 144]
  add x0, x0, 1
  str x0, [x29, 144]
  b .B1
.B2:
  ldr x0, [x29, -32]
  bl _escrever_longo
  mov w0, 10
  bl _escrever_car
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 144]
  add sp, sp, 160
  mov x0, 0
  mov x8, 93
  svc 0
  ret
// fim: [inicio]

.section .rodata
.align 2
