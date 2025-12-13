.section .text

// inicio de biblis/impressao.asm
// fn: [_escrever_int]
.align 2
_escrever_int:
    mov w1, w0 // w1 = numero
    ldr x0, = 5f // x0 = buffer
    mov x19, 0 // x19 = contador de caracteres
    
    cmp w1, 0
    b.ge 1f
    neg w1, w1 // torna positivo
    mov w2, '-'
    strb w2, [x0], 1 // escreve sinal
    mov x19, 1 // contador = 1
1:
    // escreve dígitos em ordem reversa
    mov x2, x0 // x2: aponta pra posição atual
2:
    mov w3, 10
    udiv w4, w1, w3 // w4 = quociente
    msub w5, w4, w3, w1 // w5 = resto
    add w5, w5, '0' // caractere
    strb w5, [x2], 1 // armazena
    add x19, x19, 1 // incrementa contador
    mov w1, w4
    cbnz w1, 2b
    // inverte a string de dígitos(a parte após o sinal, se existir)
    // x0: aponta pro início dos dígitos(pode ser buffer_int ou buffer_int+1)
    // x2-1: é o último dígito
    sub x2, x2, 1 // x2 aponta para o último dígito
    mov x3, x0 // x3 aponta para o primeiro dígito
3:
    cmp x3, x2
    b.ge 4f
    ldrb w4, [x3]
    ldrb w5, [x2]
    strb w5, [x3], 1
    strb w4, [x2], -1
    b 3b
4:
    ldr x1, = 5f
    mov x0, 1
    mov x2, x19 // x19: o número de caracteres
    mov x8, 64
    svc 0
    ret
.section .data
  .align 2
5: // buffer do inteiro
    .fill   32, 1, 0
// fim: [_escrever_int]
// fim de biblis/impressao.asm


// inicio de cvtint.fpb
// fn: [cvtint] (vars: 144, total: 320)
.align 2
cvtint:
  sub sp, sp, 320
  stp x29, x30, [sp, 304]
  add x29, sp, 304
  stp x19, x20, [x29, -16]
  stp x21, x22, [x29, -32]
  str x0, [x29, -48]  // param v
  str x1, [x29, -56]  // param tam
  ldr w0, [x29, -56]
  str w0, [x29, -80]
  ldr w0, [x29, -80]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
.B0:
  cmp w0, 0
  beq .B1
  mov w0, 0
  b 1f
  b .B2
.B1:
.B2:
  mov w0, 0
  str w0, [x29, -96]
  mov w0, 0
  strb w0, [x29, -112]
  mov w0, 0
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  strb w0, [x29, -128]
  ldrb w0, [x29, -128]
  str w0, [sp, -16]!
  mov w0, 45
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
.B3:
  cmp w0, 0
  beq .B4
  mov w0, 1
  strb w0, [x29, -112]
  mov w0, 1
  str w0, [x29, -96]
  b .B5
.B4:
  ldrb w0, [x29, -128]
  str w0, [sp, -16]!
  mov w0, 43
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
.B6:
  cmp w0, 0
  beq .B7
  mov w0, 1
  str w0, [x29, -96]
  b .B8
.B7:
.B8:
.B5:
  ldr w0, [x29, -80]
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  ldr w1, [sp], 16
  sub w0, w1, w0
  str w0, [x29, -144]
  ldr w0, [x29, -144]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
.B9:
  cmp w0, 0
  beq .B10
  mov w0, 0
  b 1f
  b .B11
.B10:
.B11:
  ldr w0, [x29, -144]
  str w0, [sp, -16]!
  mov w0, 4
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, le
.B12:
  cmp w0, 0
  beq .B13
  ldr w0, [x29, -144]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
.B14:
  cmp w0, 0
  beq .B15
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  str w0, [x29, 304]
  b .B16
.B15:
  ldr w0, [x29, -144]
  str w0, [sp, -16]!
  mov w0, 2
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
.B17:
  cmp w0, 0
  beq .B18
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 10
  ldr w1, [sp], 16
  mul w0, w1, w0
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [x29, 304]
  b .B19
.B18:
  ldr w0, [x29, -144]
  str w0, [sp, -16]!
  mov w0, 3
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
.B20:
  cmp w0, 0
  beq .B21
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 100
  ldr w1, [sp], 16
  mul w0, w1, w0
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 10
  ldr w1, [sp], 16
  mul w0, w1, w0
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 2
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [x29, 304]
  b .B22
.B21:
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 1000
  ldr w1, [sp], 16
  mul w0, w1, w0
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 100
  ldr w1, [sp], 16
  mul w0, w1, w0
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 2
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 10
  ldr w1, [sp], 16
  mul w0, w1, w0
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 3
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [x29, 304]
.B22:
.B19:
.B16:
  mov w0, 0
  str w0, [x29, 288]
  ldrb w0, [x29, -112]
.B23:
  cmp w0, 0
  beq .B24
  ldr w0, [x29, 304]
  neg w0, w0
  str w0, [x29, 288]
  b .B25
.B24:
  ldr w0, [x29, 304]
  str w0, [x29, 288]
.B25:
  ldr w0, [x29, 288]
  b 1f
  b .B26
.B13:
.B26:
  mov w0, 0
  str w0, [x29, -160]
.B28:
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  ldr w0, [x29, -80]
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
.B30:
  cmp w0, 0
  beq .B29
  ldr w0, [x29, -160]
  str w0, [sp, -16]!
  mov w0, 10
  ldr w1, [sp], 16
  mul w0, w1, w0
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  add w0, w0, 1
  str w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  str w0, [sp, -16]!
  mov w0, 48
  ldr w1, [sp], 16
  sub w0, w1, w0
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [x29, -160]
  b .B28
.B29:
  mov w0, 0
  str w0, [x29, -176]
  ldrb w0, [x29, -112]
.B31:
  cmp w0, 0
  beq .B32
  ldr w0, [x29, -160]
  neg w0, w0
  str w0, [x29, -176]
  b .B33
.B32:
  ldr w0, [x29, -160]
  str w0, [x29, -176]
.B33:
  ldr w0, [x29, -176]
  b 1f
  b 1f
// epilogo
1:
  ldp x19, x20, [x29, -16]
  ldp x21, x22, [x29, -32]
  ldp x29, x30, [sp, 304]
  add sp, sp, 320
  ret
// fim: [cvtint]

// fim de cvtint.fpb

.global inicio
// fn: [inicio] (vars: 16, total: 144)
.align 2
inicio:
  sub sp, sp, 144
  stp x29, x30, [sp, 128]
  add x29, sp, 128
  ldr x0, = .tex_0
  str w0, [sp, -16]!  // salva param 0 (int/bool/char/byte)
  mov w0, 3
  str w0, [sp, -16]!  // salva param 1 (int/bool/char/byte)
  ldr w1, [sp, 0]  // carrega param 1 (int/bool) em w1
  mov x1, x1  // estende pra 64 bits
  ldr w0, [sp, 16]  // carrega param 0 (int/bool) em w0
  mov x0, x0  // estende pra 64 bits
  add sp, sp, 32  // limpa temporarios
  bl cvtint
  str w0, [x29, -32]
  ldr w0, [x29, -32]
  add w0, w0, 1
  str w0, [x29, -32]
  ldr w0, [x29, -32]
  bl _escrever_int
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
.section .rodata
.align 2
.tex_0: .asciz "123"
.section .text


.section .rodata
.align 2
