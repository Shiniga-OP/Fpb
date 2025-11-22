.section .text
.global _start
.align 2
_start:
  bl inicio
  mov x0, 0
  mov x8, 93
  svc 0

// inicio de biblis/impressao.asm
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

// fn: [inicio] (vars: 16, total: 144)
.align 2
inicio:
  sub sp, sp, 144
  stp x29, x30, [sp, 128]
  add x29, sp, 128
  mov w0, 65
  and w0, w0, 0xFF
  strb w0, [x29, -32]
  ldrb w0, [x29, -32]
  bl _escrever_car
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 128]
  add sp, sp, 144
  ret
// fim: [inicio]

.section .rodata
.align 2
