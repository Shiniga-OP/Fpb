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
// fn: [_escrever_flu] (vars: 176, total: 320)
.align 2
_escrever_flu:
  // prologo
  stp x29, x30, [sp, -48]!
  mov x29, sp
  str d0, [sp, 16] // param valor
  
  // converte pra centavos/inteiro diretamente
  adr x0, 1f
  ldr s0, [x0]
  ldr s1, [sp, 16]
  fmul s0, s1, s0
  fcvtzs w8, s0 // w8 = valor * 100 como inteiro
  
  // inicia indice do buffer
  mov w9, 0 // w9 = indice no buffer
  add x10, sp, 24 // x10 = buffer(48-24=24 bytes disponiveis)
  // verifica se é negativo
  cmp w8, 0
  b.ge .positivo
  mov w0, '-'
  strb w0, [x10], 1 // armazena '-' e incrementar ponteiro
  add w9, w9, 1
  neg w8, w8 // tornar positivo
.positivo:
  // separa parte inteira e decimal
  mov w0, 100
  sdiv w11, w8, w0 // w11 = parte inteira
  msub w12, w11, w0, w8 // w12 = parte decimal(0-99)
  
  // escreve parte inteira
  cmp w11, 0
  b.ne .tem_inteiro
  
  // caso especial: inteiro é zero
  mov w0, '0'
  strb w0, [x10], 1
  add w9, w9, 1
  b .decimal
.tem_inteiro:
  // converte inteiro pra texto(reverso)
  add x13, sp, 40 // buffer temporario(8 bytes)
  mov x14, x13
.loop_inteiro:
  mov w0, 10
  sdiv w1, w11, w0
  msub w0, w1, w0, w11
  add w0, w0, '0'
  strb w0, [x14], 1
  mov w11, w1
  cmp w11, 0
  b.ne .loop_inteiro
  
  // copia na ordem correta
  sub x14, x14, 1
.loop_copiar:
  ldrb w0, [x14], -1
  strb w0, [x10], 1
  add w9, w9, 1
  cmp x14, x13
  b.ge .loop_copiar
.decimal:
  // ponto decimal
  mov w0, '.'
  strb w0, [x10], 1
  add w9, w9, 1
  
  // parte decimal(sempre 2 digitos)
  mov w0, 10
  sdiv w1, w12, w0 // dezenas
  msub w2, w1, w0, w12 // unidades
  
  add w1, w1, '0'
  add w2, w2, '0'
  
  strb w1, [x10], 1
  strb w2, [x10], 1
  add w9, w9, 2
  
  // termina com '\0'
  mov w0, 0
  strb w0, [x10]
  
  // imprime:
  add x0, sp, 24
  mov x1, x0
  mov x0, 1
  mov w2, w9
  mov x8, 64
  svc 0
  
  // epilogo
  ldp x29, x30, [sp], 48
  ret
1:
    .float 100.0
// fim: [_escrever_flu]
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


// inicio de biblis/cvts.fpb
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
  cmp w0, 0
  beq .B0
  mov w0, 0
  b 1f
  b .B1
.B0:
.B1:
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
  cmp w0, 0
  beq .B2
  mov w0, 1
  strb w0, [x29, -112]
  mov w0, 1
  str w0, [x29, -96]
  b .B3
.B2:
  ldrb w0, [x29, -128]
  str w0, [sp, -16]!
  mov w0, 43
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B4
  mov w0, 1
  str w0, [x29, -96]
  b .B5
.B4:
.B5:
.B3:
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
  cmp w0, 0
  beq .B6
  mov w0, 0
  b 1f
  b .B7
.B6:
.B7:
  ldr w0, [x29, -144]
  str w0, [sp, -16]!
  mov w0, 4
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, le
  cmp w0, 0
  beq .B8
  ldr w0, [x29, -144]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B9
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
  b .B10
.B9:
  ldr w0, [x29, -144]
  str w0, [sp, -16]!
  mov w0, 2
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B11
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
  b .B12
.B11:
  ldr w0, [x29, -144]
  str w0, [sp, -16]!
  mov w0, 3
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B13
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
  b .B14
.B13:
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
.B14:
.B12:
.B10:
  mov w0, 0
  str w0, [x29, 288]
  ldrb w0, [x29, -112]
  cmp w0, 0
  beq .B15
  ldr w0, [x29, 304]
  neg w0, w0
  str w0, [x29, 288]
  b .B16
.B15:
  ldr w0, [x29, 304]
  str w0, [x29, 288]
.B16:
  ldr w0, [x29, 288]
  b 1f
  b .B17
.B8:
.B17:
  mov w0, 0
  str w0, [x29, -160]
.B19:
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  ldr w0, [x29, -80]
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B20
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
  b .B19
.B20:
  mov w0, 0
  str w0, [x29, -176]
  ldrb w0, [x29, -112]
  cmp w0, 0
  beq .B21
  ldr w0, [x29, -160]
  neg w0, w0
  str w0, [x29, -176]
  b .B22
.B21:
  ldr w0, [x29, -160]
  str w0, [x29, -176]
.B22:
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
// fn: [cvtflu] (vars: 64, total: 240)
.align 2
cvtflu:
  sub sp, sp, 240
  stp x29, x30, [sp, 224]
  add x29, sp, 224
  stp x19, x20, [x29, -16]
  stp x21, x22, [x29, -32]
  str x0, [x29, -48]  // param v
  str x1, [x29, -56]  // param tam
  ldr w0, [x29, -56]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B23
  ldr x0, = const_0
  ldr s0, [x0]
  b 1f
  b .B24
.B23:
.B24:
  mov w0, 0
  str w0, [x29, -80]
  mov w0, 0
  strb w0, [x29, -96]
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
  str w0, [sp, -16]!
  mov w0, 45
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B25
  mov w0, 1
  strb w0, [x29, -96]
  mov w0, 1
  str w0, [x29, -80]
  b .B26
.B25:
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
  str w0, [sp, -16]!
  mov w0, 43
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B27
  mov w0, 1
  str w0, [x29, -80]
  b .B28
.B27:
.B28:
.B26:
  ldr x0, = const_0
  ldr s0, [x0]
  str s0, [x29, -112]
.B30:
  ldr w0, [x29, -80]
  str w0, [sp, -16]!
  ldr w0, [x29, -56]
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  str w0, [sp, -16]!
  ldr w0, [x29, -80]
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
  mov w0, 46
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, ne
  str w0, [sp, -16]!
  ldr w0, [x29, -80]
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
  mov w0, 101
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, ne
  str w0, [sp, -16]!
  ldr w0, [x29, -80]
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
  mov w0, 69
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, ne
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  cmp w0, 0
  beq .B31
  ldr w0, [x29, -80]
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
  cmp w1, w0
  cset w0, ge
  str w0, [sp, -16]!
  ldr w0, [x29, -80]
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
  mov w0, 57
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, le
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  cmp w0, 0
  beq .B32
  ldr s0, [x29, -112]
  str s0, [sp, -16]!
  ldr x0, = const_1
  ldr s0, [x0]
  ldr s1, [sp], 16
  fmul s0, s1, s0
  str s0, [sp, -16]!
  ldr w0, [x29, -80]
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
  sxtb w0, w0
  scvtf s0, w0
  ldr s1, [sp], 16
  fadd s0, s1, s0
  str s0, [x29, -112]
  b .B33
.B32:
.B33:
  ldr w0, [x29, -80]
  add w0, w0, 1
  str w0, [x29, -80]
  b .B30
.B31:
  ldr w0, [x29, -80]
  str w0, [sp, -16]!
  ldr w0, [x29, -56]
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  str w0, [sp, -16]!
  ldr w0, [x29, -80]
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
  mov w0, 46
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  cmp w0, 0
  beq .B34
  ldr w0, [x29, -80]
  add w0, w0, 1
  str w0, [x29, -80]
  ldr x0, = const_2
  ldr s0, [x0]
  str s0, [x29, 224]
.B36:
  ldr w0, [x29, -80]
  str w0, [sp, -16]!
  ldr w0, [x29, -56]
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  str w0, [sp, -16]!
  ldr w0, [x29, -80]
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
  cmp w1, w0
  cset w0, ge
  str w0, [sp, -16]!
  ldr w0, [x29, -80]
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
  mov w0, 57
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, le
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  cmp w0, 0
  beq .B37
  ldr s0, [x29, -112]
  str s0, [sp, -16]!
  ldr w0, [x29, -80]
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
  sxtb w0, w0
  scvtf s0, w0
  str s0, [sp, -16]!
  ldr s0, [x29, 224]
  ldr s1, [sp], 16
  fmul s0, s1, s0
  ldr s1, [sp], 16
  fadd s0, s1, s0
  str s0, [x29, -112]
  ldr s0, [x29, 224]
  str s0, [sp, -16]!
  ldr x0, = const_2
  ldr s0, [x0]
  ldr s1, [sp], 16
  fmul s0, s1, s0
  str s0, [x29, 224]
  ldr w0, [x29, -80]
  add w0, w0, 1
  str w0, [x29, -80]
  b .B36
.B37:
  b .B38
.B34:
.B38:
  ldrb w0, [x29, -96]
  cmp w0, 0
  beq .B39
  ldr s0, [x29, -112]
  fneg s0, s0
  str s0, [x29, -112]
  b .B40
.B39:
.B40:
  ldr s0, [x29, -112]
  b 1f
  b 1f
// epilogo
1:
  ldp x19, x20, [x29, -16]
  ldp x21, x22, [x29, -32]
  ldp x29, x30, [sp, 224]
  add sp, sp, 240
  ret
// fim: [cvtflu]

// fim de biblis/cvts.fpb

.global inicio
// fn: [inicio] (vars: 0, total: 128)
.align 2
inicio:
  sub sp, sp, 128
  stp x29, x30, [sp, 112]
  add x29, sp, 112
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
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  add w0, w1, w0
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_1
  str w0, [sp, -16]!  // salva param 0 (int/bool/char/byte)
  mov w0, 5
  str w0, [sp, -16]!  // salva param 1 (int/bool/char/byte)
  ldr w1, [sp, 0]  // carrega param 1 (int/bool) em w1
  mov x1, x1  // estende pra 64 bits
  ldr w0, [sp, 16]  // carrega param 0 (int/bool) em w0
  mov x0, x0  // estende pra 64 bits
  add sp, sp, 32  // limpa temporarios
  bl cvtflu
  fmov s0, s0
  str s0, [sp, -16]!
  ldr x0, = const_3
  ldr s0, [x0]
  ldr s1, [sp], 16
  fadd s0, s1, s0
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 112]
  add sp, sp, 128
  mov x0, 0
  mov x8, 93
  svc 0
  ret
// fim: [inicio]
  .section .rodata
  .align 8
const_0:
  .float 0.000000
const_1:
  .float 10.000000
const_2:
  .float 0.100000
const_3:
  .float 0.500000
.section .rodata
.align 2
.tex_0: .asciz "123"
.tex_1: .asciz "12.50"
.section .text


.section .rodata
.align 2
