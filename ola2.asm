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


// início de biblis/teste.asm
.section .data
  .align 2
.align 8
  .align 2
msg: .asciz "wok1kwmsksksmm teste\n"
tam = . - msg
.section .text
teste:
  mov x0, 1
  adrp x1, msg
  add  x1, x1, :lo12:msg
  mov x2, tam
  mov x8, 64
  svc 0
  ret// fim de biblis/teste.asm


// início de biblis/mem.asm
.section .text
// x0: array, x1: endereço da memoria, x2: tamanho
.align 2
memcp:
    ldrb w3, [x1], 1 // carrega byte e incrementa ponteiro
    strb w3, [x0], 1 // armazena byte e incrementa ponteiro
    subs x2, x2, 1 // decrementa contador
    b.gt memcp // continua se não terminou
    ret// fim de biblis/mem.asm

.align 2
somar:
  // [prologo]
  stp x29, x30, [sp, -64]!
  mov x29, sp
  stp x19, x20, [x29, 16]
  stp x21, x22, [x29, 32]
  str x0, [x29, 48]  // salvar param a
  str x1, [x29, 56]  // salvar param b
  ldr x0, =.tex_0
  bl _escrever_tex
  ldr w0, [x29, 48]
  bl _escrever_int
  ldr x0, =.tex_1
  bl _escrever_tex
  ldr x0, =.tex_2
  bl _escrever_tex
  ldr w0, [x29, 56]
  bl _escrever_int
  ldr x0, =.tex_1
  bl _escrever_tex
  ldr w0, [x29, 48]
  mov w1, w0
  ldr w0, [x29, 56]
  add w0, w1, w0
  // [epilogo]
  ldp x19, x20, [x29, 16]
  ldp x21, x22, [x29, 32]
  mov sp, x29
  ldp x29, x30, [sp], 64
  ret
.align 2
inicio:
  // [prologo]
  stp x29, x30, [sp, -48]!
  mov x29, sp
  ldr x0, =.tex_3
  bl _escrever_tex
  mov w0, 65
  strb w0, [x29, -48]
  ldr x0, =.tex_4
  bl _escrever_tex
  ldrb w0, [x29, -48]
  bl _escrever_car
  mov w0, 42
  str w0, [x29, -64]
  ldr x0, =.tex_5
  bl _escrever_tex
  ldr w0, [x29, -64]
  bl _escrever_int
  ldr x0, = const_0
  ldr s0, [x0]
  str s0, [x29, -80]
  ldr x0, =.tex_6
  bl _escrever_tex
  ldr s0, [x29, -80]
  bl _escrever_flu
  mov w0, 1
  strb w0, [x29, -96]
  ldr x0, =.tex_7
  bl _escrever_tex
  ldrb w0, [x29, -96]
  bl _escrever_bool
  ldr x0, =.tex_8
  bl _escrever_tex
  mov w0, 5
  str w0, [sp, -16]!
  mov w0, 7
  str w0, [sp, -16]!
  ldr x0, [sp, 16]
  ldr x1, [sp, 0]
  add sp, sp, 32
  bl somar
  str w0, [x29, -112]
  ldr x0, =.tex_9
  bl _escrever_tex
  ldr w0, [x29, -112]
  bl _escrever_int
  ldr w0, [x29, -112]
  str w0, [sp, -16]!
  ldr w0, [x29, -64]
  str w0, [sp, -16]!
  ldrb w0, [x29, -48]
  str w0, [sp, -16]!
  ldrb w0, [x29, -96]
  str w0, [sp, -16]!
  ldr x0, [sp, 48]
  ldr x1, [sp, 32]
  ldr x2, [sp, 16]
  ldr x3, [sp, 0]
  add sp, sp, 64
  bl testeAlteracoes
  // [epilogo]
  mov sp, x29
  ldp x29, x30, [sp], 48
  ret
.align 2
testeAlteracoes:
  // [prologo]
  stp x29, x30, [sp, -48]!
  mov x29, sp
  str x0, [x29, 16]  // salvar param s
  str x1, [x29, 24]  // salvar param numero
  str x2, [x29, 32]  // salvar param letra
  str x3, [x29, 40]  // salvar param flag
  mov w0, 5
  mov w1, w0
  mov w0, 7
  add w0, w1, w0
  str w0, [x29, 16]
  ldr x0, =.tex_10
  bl _escrever_tex
  ldr w0, [x29, 16]
  bl _escrever_int
  ldr x0, =.tex_11
  bl _escrever_tex
  mov w0, 100
  str w0, [x29, 24]
  mov w0, 90
  strb w0, [x29, 32]
  mov w0, 0
  strb w0, [x29, 40]
  ldr x0, =.tex_12
  bl _escrever_tex
  ldr w0, [x29, 24]
  bl _escrever_int
  ldr x0, =.tex_13
  bl _escrever_tex
  ldrb w0, [x29, 32]
  bl _escrever_car
  ldr x0, =.tex_14
  bl _escrever_tex
  ldrb w0, [x29, 40]
  bl _escrever_bool
  ldr x0, =.tex_1
  bl _escrever_tex
  bl testeOperacoes
  bl testeComparacoes
  bl testeLoops
  bl testeMemoria
  ldr x0, =.tex_15
  bl _escrever_tex
  bl teste
  // [epilogo]
  mov sp, x29
  ldp x29, x30, [sp], 48
  ret
.align 2
testeOperacoes:
  // [prologo]
  stp x29, x30, [sp, -16]!
  mov x29, sp
  ldr x0, =.tex_16
  bl _escrever_tex
  ldr x0, =.tex_17
  bl _escrever_tex
  mov w0, 5
  mov w1, w0
  mov w0, 5
  mov w1, w0
  mov w0, 5
  mul w0, w1, w0
  add w0, w1, w0
  bl _escrever_int
  ldr x0, =.tex_1
  bl _escrever_tex
  ldr x0, =.tex_18
  bl _escrever_tex
  mov w0, 5
  mov w1, w0
  mov w0, 5
  add w0, w1, w0
  mov w1, w0
  mov w0, 5
  mul w0, w1, w0
  bl _escrever_int
  ldr x0, =.tex_1
  bl _escrever_tex
  mov w0, 5
  mov w1, w0
  mov w0, 5
  add w0, w1, w0
  bl _escrever_int
  ldr x0, =.tex_1
  bl _escrever_tex
  mov w0, 5
  str w0, [sp, -16]!
  mov w0, 5
  str w0, [sp, -16]!
  ldr x0, [sp, 16]
  ldr x1, [sp, 0]
  add sp, sp, 32
  bl somar
  bl _escrever_int
  ldr x0, =.tex_1
  bl _escrever_tex
  // [epilogo]
  mov sp, x29
  ldp x29, x30, [sp], 16
  ret
.align 2
testeComparacoes:
  // [prologo]
  stp x29, x30, [sp, -32]!
  mov x29, sp
  ldr x0, =.tex_19
  bl _escrever_tex
  mov w0, 4
  str w0, [x29, -32]
  ldr w0, [x29, -32]
  mov w1, w0
  mov w0, 5
  cmp w1, w0
  cset w0, gt
  cmp w0, 0
  beq .B0
  ldr x0, =.tex_20
  bl _escrever_tex
  b .B1
.B0:
  ldr w0, [x29, -32]
  mov w1, w0
  mov w0, 5
  cmp w1, w0
  cset w0, ge
  cmp w0, 0
  beq .B2
  ldr x0, =.tex_21
  bl _escrever_tex
  b .B3
.B2:
  ldr x0, =.tex_22
  bl _escrever_tex
.B3:
.B1:
  // [epilogo]
  mov sp, x29
  ldp x29, x30, [sp], 32
  ret
.align 2
testeMemoria:
  // [prologo]
  stp x29, x30, [sp, -32]!
  mov x29, sp
  ldr x0, =.tex_23
  bl _escrever_tex
  add x0, x29, -32
  mov w1, 116
  strb w1, [x0, 0]
  mov w1, 101
  strb w1, [x0, 1]
  mov w1, 120
  strb w1, [x0, 2]
  mov w1, 116
  strb w1, [x0, 3]
  mov w1, 111
  strb w1, [x0, 4]
  mov w1, 0
  strb w1, [x0, 5]
  ldr x0, =.tex_24
  bl _escrever_tex
  add x0, x29, -32
  bl _escrever_tex
  mov w0, 0
  mov w1, w0
  mov w0, 88
  add x2, x29, -32
  mov x3, 1
  mul w1, w1, w3
  add x2, x2, x1
  strb w0, [x2]
  ldr x0, =.tex_25
  bl _escrever_tex
  add x0, x29, -32
  bl _escrever_tex
  ldr x0, =.tex_26
  bl _escrever_tex
  ldr x0, = .tex_27
  mov x1, -48
  str x0, [x29, x1]
  ldr x0, =.tex_28
  bl _escrever_tex
  ldr x0, [x29, -48]
  bl _escrever_tex
  ldr x0, =.tex_29
  bl _escrever_tex
  ldr x0, [x29, -48]
  str x0, [sp, -16]!
  ldr x0, [sp, 0]
  add sp, sp, 16
  bl textam
  bl _escrever_int
  ldr x0, [x29, -48]
  str x0, [sp, -16]!
  mov w0, 116
  str w0, [sp, -16]!
  ldr x0, [sp, 16]
  ldr x1, [sp, 0]
  add sp, sp, 32
  bl texcar
  str w0, [x29, -64]
  ldr w0, [x29, -64]
  mov w1, w0
  mov w0, 0
  cmp w1, w0
  cset w0, ge
  cmp w0, 0
  beq .B4
  ldr x0, =.tex_30
  bl _escrever_tex
  ldr w0, [x29, -64]
  bl _escrever_int
  b .B5
.B4:
  ldr x0, =.tex_31
  bl _escrever_tex
.B5:
  ldr x0, =.tex_32
  bl _escrever_tex
  add x0, x29, -80
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
  mov w1, 0
  strb w1, [x0, 7]
  ldr x0, =.tex_33
  bl _escrever_tex
  add x0, x29, -80
  bl _escrever_tex
  ldr x0, = .tex_34
  mov x1, -96
  str x0, [x29, x1]
  add x0, x29, -80
  str x0, [sp, -16]!
  ldr x0, [x29, -96]
  str x0, [sp, -16]!
  ldr x0, [x29, -96]
  str x0, [sp, -16]!
  ldr x0, [sp, 0]
  add sp, sp, 16
  bl textam
  str w0, [sp, -16]!
  ldr x0, [sp, 32]
  ldr x1, [sp, 16]
  ldr x2, [sp, 0]
  add sp, sp, 48
  bl memcp
  ldr x0, =.tex_35
  bl _escrever_tex
  add x0, x29, -80
  bl _escrever_tex
  add x0, x29, -80
  str x0, [sp, -16]!
  mov w0, 88
  str w0, [sp, -16]!
  mov w0, 101
  str w0, [sp, -16]!
  ldr x0, [sp, 32]
  ldr x1, [sp, 16]
  ldr x2, [sp, 0]
  add sp, sp, 48
  bl subscar
  ldr x0, =.tex_36
  bl _escrever_tex
  add x0, x29, -80
  bl _escrever_tex
  ldr x0, =.tex_37
  bl _escrever_tex
  mov w0, 0
  mov w1, w0
  add x2, x29, -80
  mov x3, 1
  mul w1, w1, w3
  add x2, x2, x1
  ldrb w0, [x2]
  strb w0, [x29, -112]
  ldr x0, =.tex_38
  bl _escrever_tex
  ldrb w0, [x29, -112]
  bl _escrever_car
  // [epilogo]
  mov sp, x29
  ldp x29, x30, [sp], 32
  ret
.align 2
testeLoops:
  // [prologo]
  stp x29, x30, [sp, -32]!
  mov x29, sp
  ldr x0, =.tex_39
  bl _escrever_tex
  ldr x0, =.tex_40
  bl _escrever_tex
  mov w0, 0
  str w0, [x29, -32]
.B6:
  ldr w0, [x29, -32]
  mov w1, w0
  mov w0, 10
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B7
  ldr x0, =.tex_41
  bl _escrever_tex
  ldr w0, [x29, -32]
  bl _escrever_int
  ldr w0, [x29, -32]
  mov w1, w0
  mov w0, 1
  add w0, w1, w0
  str w0, [x29, -32]
  b .B6
.B7:
  // [epilogo]
  mov sp, x29
  ldp x29, x30, [sp], 32
  ret
  .section .rodata
  .align 8
const_0:
  .float 3.140000
  .section .text

.section .rodata
.align 2
.tex_0: .asciz "valor a: "
.tex_1: .asciz "\n"
.tex_2: .asciz "valor b: "
.tex_3: .asciz "Testando tipos básicos:\n"
.tex_4: .asciz "\ncaractere: "
.tex_5: .asciz "\ninteiro: "
.tex_6: .asciz "\nflutuante: "
.tex_7: .asciz "\nbooleano: "
.tex_8: .asciz "\n\nTestando função soma:\n"
.tex_9: .asciz "\nsoma com retorno 5 + 7 = esperando 12, veio: "
.tex_10: .asciz "\nsoma comum 5 + 7 = esperando 12, veio: "
.tex_11: .asciz "\n\nTestando atribuições:\n"
.tex_12: .asciz "\nnovo inteiro: "
.tex_13: .asciz "\nnovo caractere: "
.tex_14: .asciz "\nnovo booleano: "
.tex_15: .asciz "\n\nTeste de biblioteca:\n"
.tex_16: .asciz "\n\nTeste de operações matematicas:\n"
.tex_17: .asciz "operação 5 + 5 * 5, esperado: 30, veio: "
.tex_18: .asciz "operação (5 + 5) * 5, esperado: 50, veio: "
.tex_19: .asciz "\nTeste comparações:\n\n"
.tex_20: .asciz "x é maior que 5\n"
.tex_21: .asciz "x é maior ou igual a 5\n"
.tex_22: .asciz "x não é maior nem igual a 5\n"
.tex_23: .asciz "\nTeste de array:\n"
.tex_24: .asciz "\nvalor do array: "
.tex_25: .asciz "\narray mudado no indice 0: "
.tex_26: .asciz "\n\nTeste de ponteiro:\n"
.tex_27: .asciz "exemplo de ponteiro"
.tex_28: .asciz "\nponteiro texto, valor: "
.tex_29: .asciz "\ntamamho em bytes: "
.tex_30: .asciz "\no ponteiro tem t no indice: "
.tex_31: .asciz "\no ponteiro não tem t\n"
.tex_32: .asciz "\nTeste de manipulação da memoria:\n"
.tex_33: .asciz "\nArray padrão: "
.tex_34: .asciz "XxXmplo maior"
.tex_35: .asciz "\nArray copiado da memoria: "
.tex_36: .asciz "\nArray usando subscar(array, 'X', 'e'): "
.tex_37: .asciz "\nTeste de acesso a itens array:\n"
.tex_38: .asciz "item do indice 0 do array: "
.tex_39: .asciz "\n\nTeste de loops"
.tex_40: .asciz "\nEnquanto:"
.tex_41: .asciz "\nvalor de i: "
.section .text

