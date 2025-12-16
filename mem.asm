.section .text

// inicio de biblis/texs.fpb

// inicio de biblis/texs.asm
// fn: [textam]
// x0: texto, w0: retorno
.align 2
textam:
    mov x1, x0
1:
    ldrb w2, [x1], 1
    cbnz w2, 1b
    sub x0, x1, x0
    sub x0, x0, 1
    ret
// fim: [textam]
// fim de biblis/texs.asm


// fim de biblis/texs.fpb

.global inicio
// fn: [testeMatrizes] (vars: 32, total: 160)
.align 2
testeMatrizes:
  sub sp, sp, 160
  stp x29, x30, [sp, 144]
  add x29, sp, 144
  mov w0, 0
  str w0, [x29, -32]
  mov w0, 1
  str w0, [x29, -28]
  ldr x0, = const_0
  ldr s0, [x0]
  str s0, [x29, -48]
  ldr x0, = const_1
  ldr s0, [x0]
  str s0, [x29, -44]
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 144]
  add sp, sp, 160
  ret
// fim: [testeMatrizes]
// fn: [testeAsm] (vars: 16, total: 144)
.align 2
testeAsm:
  sub sp, sp, 144
  stp x29, x30, [sp, 128]
  add x29, sp, 128
  ldr x0, = .tex_0
  str x0, [x29, -32]
  ldr x0, [x29, -32]
  str x0, [sp, -16]!  // salva param 0 (ponteiro/longo)
  ldr x0, [sp, 0]  // carrega param 0 (ptr/longo) em x0
  add sp, sp, 16  // limpa temporarios
  bl textam
  mov w19, w0
  mov w0, w19
  ldr x1, = global_tam
  str w0, [x1]
// inicio assembly manual
   mov x0, 1
   ldr x1, [x29, -32]
   ldr x2, global_tam
   mov x8, 64
   svc 0

// fim assembly manual
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 128]
  add sp, sp, 144
  ret
// fim: [testeAsm]
// fn: [inicio] (vars: 0, total: 128)
.align 2
inicio:
  sub sp, sp, 128
  stp x29, x30, [sp, 112]
  add x29, sp, 112
  add sp, sp, 0  // limpa temporarios
  bl testeMatrizes
  add sp, sp, 0  // limpa temporarios
  bl testeAsm
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
  .section .rodata
  .align 8
const_0:
  .float 3.000000
const_1:
  .float 5.000000
.section .rodata
.align 2
.tex_0: .asciz "mensagem\n"
.section .text


.section .data
.align 3
global_tam:
  .word 0

.section .rodata
.align 2
