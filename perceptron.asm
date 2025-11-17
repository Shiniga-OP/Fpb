.section .text
.global _start
.align 2
_start:
  bl inicio
  mov x0, 0
  mov x8, 93
  svc 0

// início de biblis/impressao.asm
.section .text
.align 2
// [TEXTO]
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
.align 2
// [INTEIRO]
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
//[FLUTUANTE]
.align 2
_escrever_flu:
    // s0 contem o valor flutuante
    adr x1, 9f // buffer de saida
    mov x6, x1 // salva inicio do buffer
    // verifica se é negativo
    fcmp s0, 0.0
    b.ge 1f
    mov w2, '-' // sinal negativo
    strb w2, [x1], 1
    fneg s0, s0 // torna positivo pra conversão
    b 2f
1:
    mov w2, ' ' // espaço pra positivo
    strb w2, [x1], 1
2:
    // parte inteira
    fcvtzs  w2, s0 // converte flutuante para inteiro(trunca)
    mov w3, 10
    mov w4, 0 // contador de dígitos
    mov x5, sp // usa pilha para empilhar digitos
    // se parte inteira for zero
    cbz w2, 4f
    // converte parte inteira
3:
    udiv w7, w2, w3
    msub w8, w7, w3, w2
    add w8, w8, '0'
    strb w8, [x5, -1]! // empilha digitos
    add w4, w4, 1
    mov w2, w7
    cbnz w2, 3b
    b 5f
4:
    mov w7, '0'
    strb w7, [x1], 1
    b 6f
    // desempilha digitos inteiros
5:
    ldrb w7, [x5], 1
    strb w7, [x1], 1
    subs w4, w4, 1
    b.ne 5b
6:
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
    adr x2, 9f // buffer
    sub x3, x1, x2 // deslocamento atual
    add x2, x2, x3 // x2 = posição atual
    sub x2, x1, x6
    // conta ate encontrar o nulo
    mov x2, 0 // contador
    mov x3, x6 // ponteiro
7:
    ldrb w4, [x3], 1
    cbz w4, 8f
    add x2, x2, 1
    b 7b
8:
    mov x1, x6 // texto do flutuante
    mov x8, 64
    svc 0
    ret
.section .data
  .align 2
.align 2
9: // buffer do flutuante
    .fill   32, 1, 0
// [CARACTERE]:
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
    .asciz "falso"// fim de biblis/impressao.asm

.align 2
degrau:
  sub sp, sp, 192
  stp x29, x30, [sp]
  mov x29, sp
  stp x19, x20, [x29, 16]
  stp x21, x22, [x29, 32]
  str d0, [x29, 48]  // salvar param x
  ldr s0, [x29, 48]
  str s0, [sp, -16]!
  ldr x0, = const_0
  ldr s0, [x0]
  ldr s1, [sp], 16
  fcmp s1, s0
  cset w0, gt
  cmp w0, 0
  beq .B1
  mov w0, 1
  b .epilogo_0
  b .B2
.B1:
.B2:
  mov w0, 0
  b .epilogo_0
  b .epilogo_0
.epilogo_0:
  ldp x19, x20, [x29, 16]
  ldp x21, x22, [x29, 32]
  mov sp, x29
  ldp x29, x30, [sp]
  add sp, sp, 192
  ret
.align 2
prever:
  sub sp, sp, 208
  stp x29, x30, [sp]
  mov x29, sp
  stp x19, x20, [x29, 16]
  stp x21, x22, [x29, 32]
  str x0, [x29, 48]  // salvar param entrada
  str x1, [x29, 56]  // salvar param pesos
  str d0, [x29, 64]  // salvar param bias
  ldr s0, [x29, 64]
  str s0, [x29, 32]
  mov w0, 0
  str w0, [x29, -192]
.B4:
  ldr w0, [x29, -192]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B5
  ldr s0, [x29, 32]
  str s0, [sp, -16]!
  ldr w0, [x29, -192]
  mov w1, w0
  ldr x2, [x29, 48]
  add x2, x2, x1, lsl 2
  ldr s0, [x2]
  str s0, [sp, -16]!
  ldr w0, [x29, -192]
  mov w1, w0
  ldr x2, [x29, 56]
  add x2, x2, x1, lsl 2
  ldr s0, [x2]
  ldr s1, [sp], 16
  fmul s0, s1, s0
  ldr s1, [sp], 16
  fadd s0, s1, s0
  str s0, [x29, 32]
  // incremento
  ldr w0, [x29, -192]
  add w0, w0, 1
  str w0, [x29, -192]
  b .B4
.B5:
  ldr s0, [x29, 32]
  str s0, [sp, -16]!
  ldr s0, [sp, 0]
  add sp, sp, 16
  bl degrau
  b .epilogo_1
  b .epilogo_1
.epilogo_1:
  ldp x19, x20, [x29, 16]
  ldp x21, x22, [x29, 32]
  mov sp, x29
  ldp x29, x30, [sp]
  add sp, sp, 208
  ret
.align 2
inicio:
  sub sp, sp, 160
  stp x29, x30, [sp]
  mov x29, sp
  ldr x0, = const_1
  ldr s0, [x0]
  str s0, [x29, 32]
  ldr x0, = const_2
  ldr s0, [x0]
  str s0, [x29, 36]
  ldr x0, = const_3
  ldr s0, [x0]
  str s0, [x29, 48]
  ldr x0, = const_4
  ldr s0, [x0]
  str s0, [x29, 64]
  ldr x0, = const_4
  ldr s0, [x0]
  str s0, [x29, 68]
  add x0, x29, 64
  str x0, [sp, -16]!
  add x0, x29, 32
  str x0, [sp, -16]!
  ldr s0, [x29, 48]
  str s0, [sp, -16]!
  ldr x0, [sp, 32]
  ldr x1, [sp, 16]
  ldr s0, [sp, 0]
  add sp, sp, 48
  bl prever
  bl _escrever_int
  b .epilogo_2
.epilogo_2:
  mov sp, x29
  ldp x29, x30, [sp]
  add sp, sp, 160
  ret
  .section .rodata
  .align 8
const_0:
  .float 0.000000
const_1:
  .float 0.400000
const_2:
  .float 0.500000
const_3:
  .float 0.800000
const_4:
  .float 1.000000
  .section .text

