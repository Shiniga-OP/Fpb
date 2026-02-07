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
// Espaço "Pessoa" definido: 16 bytes
// fn: [tex] (vars: 0, total: 144)
.align 2
tex:
  sub sp, sp, 144
  stp x29, x30, [sp, 128]
  add x29, sp, 128
  str x0, [x29, -16]  // param p
  mov w0, 0
  mov w1, w0
  str w1, [sp, -16]!
  mov w0, 65
  ldr w1, [sp], 16
  ldr x2, [x29, -16]
  add x2, x2, x1
  strb w0, [x2]
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 128]
  add sp, sp, 144
  ret
// fim: [tex]
// fn: [inicio] (vars: 0, total: 128)
.align 2
inicio:
  sub sp, sp, 128
  stp x29, x30, [sp, 112]
  add x29, sp, 112
  add x0, x29, -32
  str w0, [sp, -16]!  // salva param 0 (int/bool/char/byte)
  ldr w0, [sp, 0]  // carrega param 0 (int/bool) em w0
  mov x0, x0  // estende pra 64 bits
  add sp, sp, 16  // limpa temporarios
  bl tex
  mov w0, 1
  str w0, [sp, -16]!
  mov w0, 10
  ldr w1, [sp], 16
  add x2, x29, -32
  add x2, x2, x1
  strb w0, [x2]
  add x0, x29, -32
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
