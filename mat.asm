.section .text
// fn: [ftanh] (vars: 64, total: 240)
.align 2
ftanh:
  sub sp, sp, 240
  stp x29, x30, [sp, 224]
  add x29, sp, 224
  stp x19, x20, [x29, -16]
  stp x21, x22, [x29, -32]
  str d0, [x29, -48]  // param x
  ldr s0, [x29, -48]
  fmov s19, s0
  ldr x0, = const_0
  ldr s0, [x0]
  fmov s1, s19
  fcmp s1, s0
  cset w0, lt
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B0
  ldr s0, [x29, -48]
  fneg s0, s0
  str s0, [sp, -16]!  // salva param 0 (float)
  ldr s0, [sp, 0]  // carrega param 0 (flu) em s0
  add sp, sp, 16  // limpa temporarios
  bl ftanh
  fmov s0, s0
  fneg s0, s0
  b 1f
  b .B1
.B0:
.B1:
  ldr s0, [x29, -48]
  fmov s19, s0
  ldr x0, = const_1
  ldr s0, [x0]
  fmov s1, s19
  fcmp s1, s0
  cset w0, gt
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B2
  ldr x0, = const_2
  ldr s0, [x0]
  b 1f
  b .B3
.B2:
.B3:
  ldr x0, = const_2
  ldr s0, [x0]
  fmov s19, s0
  str s0, [x29, -80]
  ldr x0, = const_2
  ldr s0, [x0]
  fmov s19, s0
  str s0, [x29, -96]
  ldr x0, = const_3
  ldr s0, [x0]
  fmov s19, s0  // salva em reg
  ldr s0, [x29, -48]
  fmov s1, s19  // restaura do reg
  fmul s0, s1, s0
  fmov s19, s0
  str s0, [x29, -112]
  mov w0, 1
  mov w19, w0
  str w0, [x29, 224]
.B5:
  ldr w0, [x29, 224]
  mov w19, w0
  mov w0, 16
  mov w1, w19
  cmp w1, w0
  cset w0, lt
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B6
  ldr s0, [x29, -96]
  fmov s19, s0  // salva em reg
  ldr s0, [x29, -112]
  str s0, [sp, -16]!
  ldr w0, [x29, 224]
  scvtf s0, w0
  ldr s1, [sp], 16
  fdiv s0, s1, s0
  fmov s1, s19  // restaura do reg
  fmul s0, s1, s0
  fmov s19, s0
  fmov s0, s19
  str s0, [x29, -96]
  ldr s0, [x29, -80]
  str s0, [sp, -16]!
  ldr s0, [x29, -96]
  ldr s1, [sp], 16
  fadd s0, s1, s0
  fmov s19, s0
  fmov s0, s19
  str s0, [x29, -80]
  ldr w0, [x29, 224]
  add w0, w0, 1
  str w0, [x29, 224]
  b .B5
.B6:
  ldr s0, [x29, -80]
  str s0, [sp, -16]!
  ldr x0, = const_2
  ldr s0, [x0]
  ldr s1, [sp], 16
  fsub s0, s1, s0
  fmov s19, s0  // salva em reg
  ldr s0, [x29, -80]
  str s0, [sp, -16]!
  ldr x0, = const_2
  ldr s0, [x0]
  ldr s1, [sp], 16
  fadd s0, s1, s0
  fmov s1, s19  // restaura do reg
  fdiv s0, s1, s0
  b 1f
  b 1f
// epilogo
1:
  ldp x19, x20, [x29, -16]
  ldp x21, x22, [x29, -32]
  ldp x29, x30, [sp, 224]
  add sp, sp, 240
  ret
// fim: [ftanh]

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

.global inicio
// fn: [inicio] (vars: 16, total: 144)
.align 2
inicio:
  sub sp, sp, 144
  stp x29, x30, [sp, 128]
  add x29, sp, 128
  ldr x0, = .tex_0
  bl _escrever_tex
  ldr x0, = const_4
  ldr s0, [x0]
  fmov s19, s0
  str s0, [x29, 128]
.B8:
  ldr s0, [x29, 128]
  fmov s19, s0
  ldr x0, = const_3
  ldr s0, [x0]
  fmov s1, s19
  fcmp s1, s0
  cset w0, le
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B9
  ldr s0, [x29, 128]
  bl _escrever_flu
  ldr x0, = .tex_1
  bl _escrever_tex
  ldr s0, [x29, 128]
  str s0, [sp, -16]!  // salva param 0 (float)
  ldr s0, [sp, 0]  // carrega param 0 (flu) em s0
  add sp, sp, 16  // limpa temporarios
  bl ftanh
  fmov s0, s0
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  ldr s0, [x29, 128]
  str s0, [sp, -16]!
  ldr x0, = const_5
  ldr s0, [x0]
  ldr s1, [sp], 16
  fadd s0, s1, s0
  fmov s19, s0
  fmov s0, s19
  str s0, [x29, 128]
  b .B8
.B9:
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
  .align 8
const_0:
  .float 0.000000
const_1:
  .float 4.000000
const_2:
  .float 1.000000
const_3:
  .float 2.000000
const_4:
  .float -2.000000
const_5:
  .float 0.500000
.section .rodata
.align 2
.tex_0: .asciz "ftanh(x) com flutuante:\n"
.tex_1: .asciz "\t\t"
.section .text


.section .rodata
.align 2
