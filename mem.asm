.section .text
.global _start
.align 2
_start:
  bl inicio
  mov x0, 0
  mov x8, 93
  svc 0

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

// fn: [inicio] (vars: 16, total: 144)
.align 2
inicio:
  sub sp, sp, 144
  stp x29, x30, [sp, 128]
  add x29, sp, 128
  ldr x0, = .tex_0
  str x0, [x29, -32]
  ldr x0, [x29, -32]
  bl _escrever_tex
  b .epilogo_0
.epilogo_0:
  ldp x29, x30, [sp, 128]
  add sp, sp, 144
  ret
// fim: [inicio]
.section .rodata
.align 2
.tex_0: .asciz "texto"
.section .text

