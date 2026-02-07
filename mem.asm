.section .text

// inicio de biblis/impressao.asm
// fn: [_escrever_tex]
.align 2
_escrever_tex:
    mov x1, x0 // x1 = texto
    mov x2, 0 // x2 = contador
    // conta caracteres ate encontrar nulo
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
// fim: [_escrever_tex]
// fim de biblis/impressao.asm

.global inicio
// Espaço "Pessoa" definido: 32 bytes
// fn: [texcp] (vars: 16, total: 160)
.align 2
texcp:
  sub sp, sp, 160
  stp x29, x30, [sp, 144]
  add x29, sp, 144
  str x0, [x29, -16]  // param arr
  str x1, [x29, -24]  // param p
  mov w0, 0
  mov w19, w0
  str w0, [x29, -48]
.B1:
  mov w0, 1
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B2
  ldr w0, [x29, -48]
  mov w1, w0
  str w1, [sp, -16]!
  ldr w0, [x29, -48]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -24]
  add x2, x2, x0
  ldrb w0, [x2]
  ldr w1, [sp], 16
  ldr x2, [x29, -16]
  add x2, x2, x1
  strb w0, [x2]
  ldr w0, [x29, -48]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -24]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  mov w0, 0
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B3
  b .B2
  b .B4
.B3:
.B4:
  ldr w0, [x29, -48]
  add w0, w0, 1
  str w0, [x29, -48]
  b .B1
.B2:
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 144]
  add sp, sp, 160
  ret
// fim: [texcp]
// fn: [inicio] (vars: 0, total: 128)
.align 2
inicio:
  sub sp, sp, 128
  stp x29, x30, [sp, 112]
  add x29, sp, 112
  mov w0, 0
  str w0, [sp, -16]!
  mov w0, 65
  ldr w1, [sp], 16
  add x2, x29, -48
  add x2, x2, x1
  strb w0, [x2]
  mov w0, 1
  str w0, [sp, -16]!
  mov w0, 66
  ldr w1, [sp], 16
  add x2, x29, -48
  add x2, x2, x1
  strb w0, [x2]
  add x0, x29, -48
  bl _escrever_tex
  add x0, x29, -48
  str x0, [sp, -16]!  // salva param 0 (ponteiro/longo)
  ldr x0, = .tex_0
  str w0, [sp, -16]!  // salva param 1 (int/bool/char/byte)
  ldr w1, [sp, 0]  // carrega param 1 (int/bool) em w1
  mov x1, x1  // estende pra 64 bits
  ldr x0, [sp, 16]  // carrega param 0 (ptr/longo) em x0
  add sp, sp, 32  // limpa temporarios
  bl texcp
  add x0, x29, -48
  bl _escrever_tex
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
.align 2
.tex_0: .asciz "teste"
.section .text


.section .rodata
.align 2
