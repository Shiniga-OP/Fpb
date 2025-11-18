.section .text
.global _start
.align 2
_start:
  bl inicio
  mov x0, 0
  mov x8, 93
  svc 0

// início de biblis/impressao.asm
.section .text
.align 2
// [TEXTO]
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
.align 2
// [INTEIRO]
.section .data
  .align 2
5: // buffer do inteiro
    .fill   32, 1, 0
// [FLUTUANTE]
.align 2
.section .data
  .align 2
.align 2
8: // buffer do flutuante
    .fill   32, 1, 0
// [LONGO]
.align 2
.section .data
  .align 2
5: // buffer do inteiro
    .fill   32, 1, 0
inicio:
  sub sp, sp, 160
  stp x29, x30, [sp]
  mov x29, sp
  mov w0, 1
  str w0, [x29, 32]
  mov w0, 1
  str w0, [x29, 36]
  mov w0, 1
  str w0, [x29, 40]
  mov w0, 1
  str w0, [x29, 44]
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
.tex_0: .asciz "teste"
.section .text

