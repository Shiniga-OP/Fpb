.section .text
.global _start
.align 2
_start:
  bl inicio
  mov x0, 0
  mov x8, 93
  svc 0

// inicio de tmp/texint.asm
// fn: [_escrever_tex]
.align 2
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
// fim: [_escrever_tex]
// fim de tmp/texint.asm

// fn: [inicio]
.align 2
inicio:
  sub sp, sp, 160
  stp x29, x30, [sp]
  mov x29, sp
  ldr x0, = .tex_comb_0
  bl _escrever_tex
  ldr x0, = .tex_comb_1
  bl _escrever_tex
  ldr x0, = .tex_4
  bl _escrever_tex
  b .epilogo_0
.epilogo_0:
  mov sp, x29
  ldp x29, x30, [sp]
  add sp, sp, 160
  ret
// fim: [inicio]
.section .rodata
.align 2
.tex_4: .asciz "texto 5\n"
.section .text


.section .rodata
.align 2
.tex_comb_0: .asciz "texto 1\ntexto 2\n"
.tex_comb_1: .asciz "texto 3\ntexto 4\n"
