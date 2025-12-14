.section .text

// inicio de biblis/impressao.asm
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
// fim de biblis/impressao.asm

.global inicio
// fn: [inicio] (vars: 32, total: 160)
.align 2
inicio:
  sub sp, sp, 160
  stp x29, x30, [sp, 144]
  add x29, sp, 144
  mov w0, 3
  str w0, [x29, -32]
  ldr x0, = const_0
  ldr s0, [x0]
  str s0, [x29, -48]
  ldr s0, [x29, -48]
  fmov s8, s0  // salva em reg
  ldr w0, [x29, -32]
  scvtf s0, w0
  fmov s1, s8  // restaura do reg
  fmul s0, s1, s0
  bl _escrever_flu
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 144]
  add sp, sp, 160
  mov x0, 0
  mov x8, 93
  svc 0
  ret
// fim: [inicio]
  .section .rodata
  .align 8
const_0:
  .float 0.500000

.section .rodata
.align 2
