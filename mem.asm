.section .text
.global inicio
// fn: [inicio] (vars: 16, total: 144)
.align 2
inicio:
  sub sp, sp, 144
  stp x29, x30, [sp, 128]
  add x29, sp, 128
  ldr x0, = global_cor
  ldrb w0, [x0]
  mov w19, w0
  strb w0, [x29, -32]
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

.section .data
.align 3
global_cor:
  .word -16711936

.section .rodata
.align 2
