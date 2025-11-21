.section .text
.global _start
.align 2
_start:
  bl inicio
  mov x0, 0
  mov x8, 93
  svc 0
// fn: [teste] (vars: 32, total: 192)
.align 2
teste:
  sub sp, sp, 192
  stp x29, x30, [sp, 176]
  add x29, sp, 176
  stp x19, x20, [x29, -16]
  stp x21, x22, [x29, -32]
  mov w0, 10
  str w0, [sp, -16]!
  mov w0, 20
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [x29, -64]
  ldr w0, [x29, -64]
  str w0, [sp, -16]!
  mov w0, 2
  ldr w1, [sp], 16
  mul w0, w1, w0
  str w0, [x29, -80]
  ldr w0, [x29, -80]
  b .epilogo_0
  b .epilogo_0
.epilogo_0:
  ldp x19, x20, [x29, -16]
  ldp x21, x22, [x29, -32]
  ldp x29, x30, [sp, 176]
  add sp, sp, 192
  ret
// fim: [teste]
// fn: [inicio] (vars: 0, total: 128)
.align 2
inicio:
  sub sp, sp, 128
  stp x29, x30, [sp, 112]
  add x29, sp, 112
  bl teste
  b .epilogo_1
.epilogo_1:
  ldp x29, x30, [sp, 112]
  add sp, sp, 128
  ret
// fim: [inicio]
