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
// fn: [inicio] (vars: 16, total: 144)
.align 2
inicio:
  sub sp, sp, 144
  stp x29, x30, [sp, 128]
  add x29, sp, 128
  mov w1, 111
  strb w1, [x29, -32]
  mov w1, 108
  strb w1, [x29, -31]
  mov w1, 195
  strb w1, [x29, -30]
  mov w1, 161
  strb w1, [x29, -29]
  mov w1, 10
  strb w1, [x29, -28]
  mov w1, 0
  strb w1, [x29, -27]
  add x0, x29, -32
  bl _escrever_tex
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 128]
  add sp, sp, 144
  mov x0, 0
  mov x8, 93
  svc 0
  ret
// fim: [inicio]

.section .rodata
.align 2
