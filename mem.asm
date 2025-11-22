.section .text
.global _start
.align 2
_start:
  bl inicio
  mov x0, 0
  mov x8, 93
  svc 0
// fn: [inicio] (vars: 32, total: 160)
.align 2
inicio:
  sub sp, sp, 160
  stp x29, x30, [sp, 144]
  add x29, sp, 144
  mov w0, 10
  str w0, [x29, -32]
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 144]
  add sp, sp, 160
  ret
// fim: [inicio]
