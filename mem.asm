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
// fn: [_escrever_flu] (vars: 176, total: 320)
.align 2
_escrever_flu:
  // Prologo otimizado
  stp x29, x30, [sp, -48]!
  mov x29, sp
  str d0, [sp, 16]      // param valor (guardar para usar depois)
  
  // Converter para centavos/inteiro diretamente
  adr x0, 1f
  ldr s0, [x0]
  ldr s1, [sp, 16]
  fmul s0, s1, s0
  fcvtzs w8, s0        // w8 = valor * 100 como inteiro
  
  // Inicializar índice do buffer
  mov w9, 0            // w9 = índice no buffer
  add x10, sp, 24      // x10 = buffer (48-24=24 bytes disponíveis)
  
  // Verificar se é negativo
  cmp w8, 0
  b.ge .positivo
  mov w0, '-'
  strb w0, [x10], 1    // armazenar '-' e incrementar ponteiro
  add w9, w9, 1
  neg w8, w8           // tornar positivo
  
.positivo:
  // Separar parte inteira e decimal
  mov w0, 100
  sdiv w11, w8, w0     // w11 = parte inteira
  msub w12, w11, w0, w8 // w12 = parte decimal (0-99)
  
  // Escrever parte inteira
  cmp w11, 0
  b.ne .tem_inteiro
  
  // Caso especial: inteiro é zero
  mov w0, '0'
  strb w0, [x10], 1
  add w9, w9, 1
  b .decimal
  
.tem_inteiro:
  // Converter inteiro para string (reverso)
  add x13, sp, 40      // buffer temporário (8 bytes)
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
  
  // Copiar na ordem correta
  sub x14, x14, 1
  
.loop_copiar:
  ldrb w0, [x14], -1
  strb w0, [x10], 1
  add w9, w9, 1
  cmp x14, x13
  b.ge .loop_copiar
  
.decimal:
  // Ponto decimal
  mov w0, '.'
  strb w0, [x10], 1
  add w9, w9, 1
  
  // Parte decimal (sempre 2 dígitos)
  mov w0, 10
  sdiv w1, w12, w0      // dezenas
  msub w2, w1, w0, w12  // unidades
  
  add w1, w1, '0'
  add w2, w2, '0'
  
  strb w1, [x10], 1
  strb w2, [x10], 1
  add w9, w9, 2
  
  // Terminar string com null
  mov w0, 0
  strb w0, [x10]
  
  // Chamar _escrever_tex
  add x0, sp, 24
  mov x1, x0
  mov x0, 1
  mov w2, w9
  mov x8, 64
  svc 0
  
  // Epilogo
  ldp x29, x30, [sp], 48
  ret
1:
    .float 100.0
// fim: [_escrever_flu]
// fim de biblis/impressao.asm

.global inicio
// fn: [inicio] (vars: 0, total: 128)
.align 2
inicio:
  sub sp, sp, 128
  stp x29, x30, [sp, 112]
  add x29, sp, 112
  ldr x0, = .tex_comb_0
  bl _escrever_tex
  ldr x0, = const_0
  ldr s0, [x0]
  bl _escrever_flu
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
  .float 123.449997
.section .rodata
.align 2
.section .text


.section .rodata
.align 2
.tex_comb_0: .asciz "=== medidor de tempo em Milissegundos ===\n\n[FPB] Inicio: "
