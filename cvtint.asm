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
// fn: [_escrever_flu]
.align 2
_escrever_flu:
    // s0 contem o valor flutuante
    adr x1, 8f // buffer de saida
    mov x6, x1 // salva inicio do buffer
    // verifica se é negativo
    fcmp s0, 0.0
    b.ge 1f
    mov w2, '-' // sinal negativo
    strb w2, [x1], 1
    fneg s0, s0 // torna positivo pra conversão
    b 1f
1:
    // parte inteira
    fcvtzs  w2, s0 // converte flutuante para inteiro(trunca)
    mov w3, 10
    mov w4, 0 // contador de dígitos
    mov x5, sp // usa pilha para empilhar digitos
    // se parte inteira for zero
    cbz w2, 3f
    // converte parte inteira
2:
    udiv w7, w2, w3
    msub w8, w7, w3, w2
    add w8, w8, '0'
    strb w8, [x5, -1]! // empilha digitos
    add w4, w4, 1
    mov w2, w7
    cbnz w2, 2b
    b 4f
3:
    mov w7, '0'
    strb w7, [x1], 1
    b 5f
    // desempilha digitos inteiros
4:
    ldrb w7, [x5], 1
    strb w7, [x1], 1
    subs w4, w4, 1
    b.ne 4b
5:
    // ponto decimal
    mov w2, '.'
    strb w2, [x1], 1
    // parte fracionária(2 casas)
    fcvtzs w2, s0 // parte inteira
    scvtf s1, w2 // converte inteiro de volta pra flutuante
    fsub s0, s0, s1 // s0 = parte fracionaria
    // primeiro dígito decimal
    fmov s2, 10.0
    fmul s0, s0, s2 // *10
    fcvtzs w2, s0 // primeiro dígito
    add w2, w2, '0'
    strb w2, [x1], 1
    // segundo digito decimal
    fcvtzs w2, s0 // parte inteira
    scvtf s1, w2 // converte para flutuante
    fsub s0, s0, s1 // remove parte inteira
    fmul s0, s0, s2 // *10
    fcvtzs w2, s0 // segundo digito
    add w2, w2, '0'
    strb w2, [x1], 1
    // finaliza texto
    mov w2, 0
    strb w2, [x1]
    mov x0, 1 // saida de impressão
    mov x1, x6 // inicio do buffer
    // calcula tamanho: x1 aponta para inicio, x1+... para final
    adr x2, 8f // buffer
    sub x3, x1, x2 // deslocamento atual
    add x2, x2, x3 // x2 = posição atual
    sub x2, x1, x6
    // conta ate encontrar o nulo
    mov x2, 0 // contador
    mov x3, x6 // ponteiro
6:
    ldrb w4, [x3], 1
    cbz w4, 7f
    add x2, x2, 1
    b 6b
7:
    mov x1, x6 // texto do flutuante
    mov x8, 64
    svc 0
    ret
.section .data
  .align 2
.align 2
8: // buffer do flutuante
    .fill   32, 1, 0
// fim: [_escrever_flu]
// fn: [_escrever_longo]
.align 2
_escrever_longo:
    mov x1, x0 // x1 = numero(64 bits)
    ldr x0, =5f // x0 = buffer
    mov x19, 0 // x19 = contador de caracteres
    
    cmp x1, 0 // compara 64 bits
    b.ge 1f
    neg x1, x1 // torna positivo(64 bits)
    mov w2, '-'
    strb w2, [x0], 1 // escreve sinal(w2 é 32 bits)
    mov x19, 1 // contador = 1
1:
    // escreve digitos em ordem reversa
    mov x2, x0 // x2: aponta pra posição atual
2:
    mov x3, 10
    udiv x4, x1, x3 // x4 = quociente(64 bits)
    msub x5, x4, x3, x1 // x5 = resto(64 bits)
    add w5, w5, '0' // converte resto pra caractere (w5)
    strb w5, [x2], 1 // armazena o byte(w5)
    add x19, x19, 1 // incrementa contador
    mov x1, x4
    cbnz x1, 2b // continua se x1 != 0
    // inverte o texto de digitos
    sub x2, x2, 1 // x2 aponta para o ultimo digito
    mov x3, x0 // x3 aponta para o primeiro digito
3:
    cmp x3, x2
    b.ge 4f
    ldrb w4, [x3] // carrega byte(w4)
    ldrb w5, [x2] // carrega byte(w5)
    strb w5, [x3], 1 // armazena byte(w5)
    strb w4, [x2], -1 // armazena byte(w4)
    b 3b
4:
    ldr     x1, = 5f
    mov     x0, 1
    mov     x2, x19 // x19: o numero de caracteres
    mov x8, 64
    svc 0
    ret
.section .data
  .align 2
5: // buffer do inteiro
    .fill   32, 1, 0
// fim: [_escrever_longo]
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
// fn: [cvt] (vars: 144, total: 320)
.align 2
cvt:
  sub sp, sp, 320
  stp x29, x30, [sp, 304]
  add x29, sp, 304
  stp x19, x20, [x29, -16]
  stp x21, x22, [x29, -32]
  str x0, [x29, -48]  // param s
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
  beq .B1
  mov w0, 0
  b 1f
  b .B2
.B1:
.B2:
  mov w0, 0
  str w0, [x29, -96]
  mov w0, 0
  cmp w0, 0
  cset w0, ne
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
  beq .B4
  mov w0, 1
  cmp w0, 0
  cset w0, ne
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
  cmp w0, 0
  beq .B13
  ldr w0, [x29, -144]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
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
  cmp w0, 0
  beq .B29
  ldr w0, [x29, -160]
  str w0, [sp, -16]!
  mov w0, 10
  ldr w1, [sp], 16
  mul w0, w1, w0
  str w0, [sp, -16]!
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
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [x29, -160]
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [x29, -96]
  b .B28
.B29:
  mov w0, 0
  str w0, [x29, -176]
  ldrb w0, [x29, -112]
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
// fim: [cvt]
// fn: [cvt_flutuante_simples] (vars: 144, total: 320)
.align 2
cvt_flutuante_simples:
  sub sp, sp, 320
  stp x29, x30, [sp, 304]
  add x29, sp, 304
  stp x19, x20, [x29, -16]
  stp x21, x22, [x29, -32]
  str x0, [x29, -48]  // param s
  str x1, [x29, -56]  // param tam
  ldr x0, = const_0
  ldr s0, [x0]
  str s0, [x29, -80]
  mov w0, 0
  str w0, [x29, -96]
  mov w0, 0
  cmp w0, 0
  cset w0, ne
  strb w0, [x29, -112]
  mov w0, 0
  cmp w0, 0
  cset w0, ne
  strb w0, [x29, -128]
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  ldr w0, [x29, -56]
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B35
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
  mov w0, 45
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B37
  mov w0, 1
  cmp w0, 0
  cset w0, ne
  strb w0, [x29, -128]
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [x29, -96]
  mov w0, 1
  cmp w0, 0
  cset w0, ne
  strb w0, [x29, -112]
  b .B38
.B37:
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
  mov w0, 43
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B40
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [x29, -96]
  mov w0, 1
  cmp w0, 0
  cset w0, ne
  strb w0, [x29, -112]
  b .B41
.B40:
.B41:
.B38:
  b .B42
.B35:
.B42:
  mov w0, 0
  cmp w0, 0
  cset w0, ne
  strb w0, [x29, -144]
  mov w0, 0
  str w0, [x29, -160]
.B44:
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  ldr w0, [x29, -56]
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B45
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
  strb w0, [x29, 304]
  ldrb w0, [x29, 304]
  str w0, [sp, -16]!
  mov w0, 46
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B48
  mov w0, 1
  cmp w0, 0
  cset w0, ne
  strb w0, [x29, -144]
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [x29, -96]
