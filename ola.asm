.section .data
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
    // conta caracteres até encontrar null
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
    mov w1, w0 // w1 = número
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
    // s0 contém o valor flutuante
    adr x1, 8f // buffer de saída
    mov x6, x1 // salva início do buffer
    // verifica se é negativo
    fcmp s0, 0.0
    b.ge 1f
    mov w2, '-' // sinal negativo
    strb w2, [x1], 1
    fneg s0, s0 // torna positivo para conversão
    b 2f
    
1:
    mov     w2, ' ' // espaço para positivo
    strb    w2, [x1], 1
    
2:
    // parte inteira
    fcvtzs  w2, s0 // converte flutuante para inteiro(trunca)
    mov w3, 10
    mov w4, 0 // contador de dígitos
    mov x5, sp // usa pilha para empilhar dígitos
    // se parte inteira for zero
    cbz w2, 4f
    // converte parte inteira
3:
    udiv w7, w2, w3
    msub w8, w7, w3, w2
    add w8, w8, '0'
    strb w8, [x5, -1]! // empilha dígitos
    add w4, w4, 1
    mov w2, w7
    cbnz w2, 3b
    b 5f
4:
    mov w7, '0'
    strb w7, [x1], 1
    // desempilha dígitos inteiros
5:
    ldrb w7, [x5], 1
    strb w7, [x1], 1
    subs w4, w4, 1
    b.ne 5b
    // ponto decimal
    mov w2, '.'
    strb w2, [x1], 1
    // parte fracionária(2 casas)
    fcvtzs w2, s0 // parte inteira
    scvtf s1, w2 // converte inteiro de volta para flutuante
    fsub s0, s0, s1 // s0 = parte fracionária
    // primeiro dígito decimal
    fmov s2, 10.0
    fmul s0, s0, s2 // *10
    fcvtzs w2, s0 // primeiro dígito
    add w2, w2, '0'
    strb w2, [x1], 1
    // segundo dígito decimal
    fcvtzs w2, s0 // parte inteira
    scvtf s1, w2 // converte para flutuante
    fsub s0, s0, s1 // remove parte inteira
    fmul s0, s0, s2 // *10
    fcvtzs w2, s0 // segundo dígito
    add w2, w2, '0'
    strb w2, [x1], 1
    // finaliza string
    mov w2, 0
    strb w2, [x1]
    mov x0, 1 // saida de impressão
    mov x1, x6 // início do buffer
    // calcula tamanho: x1 aponta para início, x1+... para final
    adr x2, 8f
    sub x3, x1, x2 // deslocamento atual
    add x2, x2, x3 // x2 = posição atual
    sub x2, x1, x6 // ERRADO - vamos fazer diferente
    // vamos contar até encontrar o null
    mov x2, 0 // contador
    mov x3, x6 // ponteiro
6:
    ldrb w4, [x3], 1
    cbz w4, 7f
    add x2, x2, 1
    b 6b
7:
    mov x1, x6 // string do flutuante
    mov x8, 64
    svc 0
    ret
.section .data
  .align 2
.align 2
8: // buffer do flutuante
    .fill   32, 1, 0
.align 2
_escrever_double:
    adr x1, .Ldouble_buffer
    mov x0, 1
    mov x2, 12
    mov x8, 64
    svc 0
    ret

.Ldouble_buffer:
    .asciz  "[double]"
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


// início de biblis/texs.asm
.section .text
// x0: texto, w0: retorno
.align 2
textam:
    mov x1, x0
1:
    ldrb w2, [x1], 1
    cbnz x2, 1b
    sub x0, x1, x0
    sub x0, x0, 1
    ret
// x0: texto, w1: car a substituir, w2: novo car
.align 2
subscar:
    mov x3, x0 // x3 = ponteiro para iterar
1: // loop
    ldrb w4, [x3] // carrega caractere atual
    cbz w4, 3f // se zero, fim da string
    cmp w4, w1 // e o caractere alvo?
    b.ne 2f
    
    strb w2, [x3] // substitui pelo novo caractere
2: // proximo
    add x3, x3, 1 // avança pra o próximo caractere
    b 1b
3: // retorna
    ret
// x0: ponteiro, w1: caractere
.align 2
texcar:
    mov x2, x0 // x2 = ponteiro para iterar
    mov w3, w1 // w3 = caractere procurado
1: // loop
    ldrb w4, [x2] // carrega byte atual
    cbz w4, 3f // se zero, fim da string
    cmp w4, w3 // compara com caractere buscado
    b.eq 2f // se igual, encontrou
    add x2, x2, 1 // próximo caractere
    b 1b
2: // achou
    sub x0, x2, x0 // calcula posição(endereço atual - início)
    b 4f
3: // não achou
    mov x0, -1 // retorna -1 se não encontrou
4: // retorna
    ret// fim de biblis/texs.asm

.align 2
olhar:
  // [prologo]
  stp x29, x30, [sp, -32]!
  mov x29, sp
  str x0, [x29, 16]  // salvar param arr
  ldr x0, =.tex_0
  bl _escrever_tex
  ldr x0, [x29, 16]
  bl _escrever_tex
  b .epilogo_2
.epilogo_2:
  mov sp, x29
  ldp x29, x30, [sp], 32
  ret
.align 2
inicio:
  // [prologo]
  stp x29, x30, [sp, -16]!
  mov x29, sp
  add x0, x29, -32
  mov w1, 101
  strb w1, [x0, 0]
  mov w1, 120
  strb w1, [x0, 1]
  mov w1, 101
  strb w1, [x0, 2]
  mov w1, 109
  strb w1, [x0, 3]
  mov w1, 112
  strb w1, [x0, 4]
  mov w1, 108
  strb w1, [x0, 5]
  mov w1, 111
  strb w1, [x0, 6]
  mov w1, 32
  strb w1, [x0, 7]
  mov w1, 97
  strb w1, [x0, 8]
  mov w1, 114
  strb w1, [x0, 9]
  mov w1, 114
  strb w1, [x0, 10]
  mov w1, 97
  strb w1, [x0, 11]
  mov w1, 121
  strb w1, [x0, 12]
  mov w1, 0
  strb w1, [x0, 13]
  add x0, x29, -48
  mov w1, 32
  strb w1, [x0, 0]
  mov w1, 109
  strb w1, [x0, 1]
  mov w1, 97
  strb w1, [x0, 2]
  mov w1, 105
  strb w1, [x0, 3]
  mov w1, 111
  strb w1, [x0, 4]
  mov w1, 114
  strb w1, [x0, 5]
  mov w1, 32
  strb w1, [x0, 6]
  mov w1, 116
  strb w1, [x0, 7]
  mov w1, 101
  strb w1, [x0, 8]
  mov w1, 115
  strb w1, [x0, 9]
  mov w1, 116
  strb w1, [x0, 10]
  mov w1, 101
  strb w1, [x0, 11]
  mov w1, 0
  strb w1, [x0, 12]
  ldr x0, =.tex_1
  bl _escrever_tex
  add x0, x29, -32
  bl _escrever_tex
  mov w0, 0
  mov w1, w0
  add x2, x29, -32
  mov x3, 1
  mul w1, w1, w3
  add x2, x2, x1
  ldrb w0, [x2]
  mov w1, w0
  mov w0, 101
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B0
  b .epilogo_3
  b .B1
.B0:
.B1:
  add x0, x29, -32
  str x0, [sp, -16]!
  ldr x0, [sp, 0]
  add sp, sp, 16
  bl olhar
  ldr x0, =.tex_2
  bl _escrever_tex
  add x0, x29, -48
  bl _escrever_tex
  b .epilogo_3
.epilogo_3:
  mov sp, x29
  ldp x29, x30, [sp], 16
  ret
.section .rodata
.align 2
.tex_0: .asciz "\nesse foi o array recebido: "
.tex_1: .asciz "\nArray: "
.tex_2: .asciz "\nArray2: "
.section .text

