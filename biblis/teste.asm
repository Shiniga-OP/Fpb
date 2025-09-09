.section .data
.align 8
msg: .asciz "wok1kwmsksksmm teste\n"
tam = . - msg
.section .text
teste:
  mov x0, 1
  adrp x1, msg
  add  x1, x1, :lo12:msg
  mov x2, tam
  mov x8, 64
  svc 0
  ret