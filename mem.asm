.section .text

// inicio de biblis/mem.asm
// fn: [memcp]
// x0: array, x1: endereço da memoria, x2: tamanho
.align 2
memcp:
    ldrb w3, [x1], 1 // carrega byte e incrementa ponteiro
    strb w3, [x0], 1 // armazena byte e incrementa ponteiro
    subs x2, x2, 1 // decrementa contador
    b.gt memcp // continua se não terminou
    ret
// fim: [memcp]
// fim de biblis/mem.asm


// inicio de biblis/texs.asm
// fn: [textam]
// x0: texto, w0: retorno
.align 2
textam:
    mov x1, x0
1:
    ldrb w2, [x1], 1
    cbnz x2, 1b
    sub x0, x1, x0
    sub x0, x0, 1
    ret
// fim: [textam]
// fim de biblis/texs.asm

.global inicio
// fn: [inicio] (vars: 48, total: 176)
.align 2
inicio:
  sub sp, sp, 176
  stp x29, x30, [sp, 160]
  add x29, sp, 160
  mov w1, 101
  strb w1, [x29, -32]
  mov w1, 120
  strb w1, [x29, -31]
  mov w1, 101
  strb w1, [x29, -30]
  mov w1, 109
  strb w1, [x29, -29]
  mov w1, 112
  strb w1, [x29, -28]
  mov w1, 108
  strb w1, [x29, -27]
  mov w1, 111
  strb w1, [x29, -26]
  mov w1, 0
  strb w1, [x29, -25]
  ldr x0, = .tex_0
  str x0, [x29, -48]
  add x0, x29, -32
  str x0, [sp, -16]!
  ldr x0, [sp, 0]
  add sp, sp, 16
  bl textam
  str w0, [x29, -64]
  add x0, x29, -32
  str x0, [sp, -16]!
  ldr x0, [x29, -48]
  str x0, [sp, -16]!
  ldr w0, [x29, -64]
  str w0, [sp, -16]!
  ldr x0, [sp, 32]
  ldr x1, [sp, 16]
  ldr x2, [sp, 0]
  add sp, sp, 48
  bl memcp
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 160]
  add sp, sp, 176
  mov x0, 0
  mov x8, 93
  svc 0
  ret
// fim: [inicio]
.section .rodata
.align 2
.tex_0: .asciz "XxXemplo"
.section .text


.section .rodata
.align 2
