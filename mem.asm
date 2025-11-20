.section .text
.global _start
.align 2
_start:
  bl inicio
  mov x0, 0
  mov x8, 93
  svc 0
// fn: [teste]
.align 2
teste:
  sub sp, sp, 160
  stp x29, x30, [sp]
  mov x29, sp
  b .epilogo_0
.epilogo_0:
  mov sp, x29
  ldp x29, x30, [sp]
  add sp, sp, 160
  ret
// fim: [teste]
// fn: [inicio]
.align 2
inicio:
  sub sp, sp, 160
  stp x29, x30, [sp]
  mov x29, sp
  bl teste
  b .epilogo_1
.epilogo_1:
  mov sp, x29
  ldp x29, x30, [sp]
  add sp, sp, 160
  ret
// fim: [inicio]
