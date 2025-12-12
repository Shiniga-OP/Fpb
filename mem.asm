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
// fn: [_escrever_bool]
.align 2
_escrever_bool:
    cmp w0, 0
    b.eq 1f
    adr x1, 3f
    mov x2, 7
    b 2f
1:
    adr x1, 4f
    mov x2, 5
2:
    mov x0, 1
    mov x8, 64
    svc 0
    ret
// buffers do booleano
3:
    .asciz "verdade"
4:
    .asciz "falso"
// fim: [_escrever_bool]
// fim de biblis/impressao.asm

.global inicio
// fn: [inicio] (vars: 48, total: 176)
.align 2
inicio:
  sub sp, sp, 176
  stp x29, x30, [sp, 160]
  add x29, sp, 160
  mov w0, 0
  strb w0, [x29, -32]
  mov w0, 0
  strb w0, [x29, -48]
  mov w0, 0
  strb w0, [x29, -64]
.B1:
  ldrb w0, [x29, -32]
  cmp w0, 0
  cset w0, eq
  str w0, [sp, -16]!
  ldrb w0, [x29, -48]
  cmp w0, 0
  cset w0, eq
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  str w0, [sp, -16]!
  ldrb w0, [x29, -64]
  cmp w0, 0
  cset w0, eq
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
.B3:
  cmp w0, 0
  beq .B2
  ldr x0, = .tex_0
  bl _escrever_tex
  ldrb w0, [x29, -32]
  bl _escrever_bool
  ldr x0, = .tex_1
  bl _escrever_tex
  mov w0, 1
  strb w0, [x29, -32]
  mov w0, 1
  strb w0, [x29, -48]
  mov w0, 1
  strb w0, [x29, -64]
  b .B1
.B2:
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 160]
  add sp, sp, 176
  mov x0, 0
  mov x8, 93
  svc 0
  ret
// fim: [inicio]
.section .rodata
.align 2
.tex_0: .asciz "Teste, X = "
.tex_1: .asciz "\n"
.section .text


.section .rodata
.align 2
