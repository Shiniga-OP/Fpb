.section .text
.global _start
.align 2
_start:
  bl inicio // usada [inicio]
  mov x0, 0
  mov x8, 93
  svc 0

// inicio de biblis/impressao.asm
.align 2
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
// fim de biblis/impressao.asm

// fn: [inicio]
.align 2
inicio:
  sub sp, sp, 160
  stp x29, x30, [sp]
  mov x29, sp
  ldr x0, = const_0
  ldr s0, [x0]
  bl _escrever_flu
  b .epilogo_0
.epilogo_0:
  mov sp, x29
  ldp x29, x30, [sp]
  add sp, sp, 160
  ret
// fim: [inicio]
  .section .rodata
  .align 8
const_0:
  .float 0.100000
