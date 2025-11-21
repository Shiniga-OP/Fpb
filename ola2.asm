.section .text
.global _start
.align 2
_start:
  bl inicio
  mov x0, 0
  mov x8, 93
  svc 0

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


// inicio de biblis/texs.asm
// fn: [textam]
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
// fim: [textam]
// fn: [subscar]
// x0: texto, w1: car a substituir, w2: novo car
.align 2
subscar:
    mov x3, x0 // x3 = ponteiro para iterar
1: // loop
    ldrb w4, [x3] // carrega caractere atual
    cbz w4, 3f // se zero, fim do texto
    cmp w4, w1 // e o caractere alvo?
    b.ne 2f
    
    strb w2, [x3] // substitui pelo novo caractere
2: // proximo
    add x3, x3, 1 // avança pra o próximo caractere
    b 1b
3: // retorna
    ret
// fim: [subscar]
// fn: [texcar]
// x0: ponteiro, w1: caractere
.align 2
texcar:
    mov x2, x0 // x2 = ponteiro para iterar
    mov w3, w1 // w3 = caractere procurado
1: // loop
    ldrb w4, [x2] // carrega byte atual
    cbz w4, 3f // se zero, fim do texto
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
    ret
// fim: [texcar]
// fn: [texcmp]
// x0: ponteiro para o texto 1
// x1: ponteiro para o texto 2
// w0: retorno(1 se verdadeiro, 0 se falso)
.align 2
texcmp:
    // x2 e x3 são usados para carregar os bytes
    
1:  // inicio do loop de comparação
    ldrb w2, [x0] // carrega o byte atual do texto 1(t1)
    ldrb w3, [x1] // carrega o byte atual do texto 2(t2)

    cmp w2, w3 // compara os dois bytes
    b.ne 3f // se forem diferentes salta pra FALSO(3f)
    // se os bytes são iguais, verifica se é o fim do texto
    // se w2(que é igual a w3) for zero, ambos os textos terminaram
    // ao mesmo tempo, logo são iguais
    cbz w2, 2f// Se w2 for zero, salta para VERDADEIRO(2f)
    // se os bytes são iguais E não são zero, continua o loop
    add x0, x0, 1 // avança o ponteiro t1
    add x1, x1, 1 // avança o ponteiro t2
    b 1b // volta ao inicio do loop
2:  // VERDADEIRO: os textos são iguais
    mov w0, 1 // define o retorno w0 = 1
    b 4f // salta para o fim da função
    
3:  // FALSO: os bytes foram diferentes em algum ponto
    mov w0, 0 // define o retorno w0 = 0
    
4:
    ret
// fim: [texcmp]
// fim de biblis/texs.asm


// inicio de biblis/mem.asm
// fn: [memcp]
// x0: array, x1: endereço da memoria, x2: tamanho
.align 2
memcp:
    ldrb w3, [x1], 1 // carrega byte e incrementa ponteiro
    strb w3, [x0], 1 // armazena byte e incrementa ponteiro
    subs x2, x2, 1 // decrementa contador
    b.gt memcp // continua se não terminou
    ret
// fim: [memcp]
// fim de biblis/mem.asm


// inicio de biblis/sistema.asm
// fn: [obter_tempo_milis]
// retorna o total de milissegundos desde a epoca(1970)
// O resultado(64 bits) em x0
.align 2
obter_tempo_milis:
    // prepara a Pilha
    // precisa de 16 bytes pra a struct timespec(2x 64-bit: tv_sec, tv_nsec)
    // alocamo 32 bytes(pra manter alinhamento de 16) e salvar registradores
    stp x29, x30, [sp, -32]!  // salva frame pointer(fp) e link register(lr)
    mov x29, sp // define novo frame pointer
    // x0 = clock_id(o tempo "real")
    // x1 = ponteiro pra a struct timespec(usaremos o espaço na pilha)
    // x8 = numero da chamada de sistema(113 = obter o tempo)
    
    mov x0, 0 // x0 = tempo
    add x1, sp, 16 // x1 = aponta pra o buffer de 16 bytes na pilha
    mov x8, 113 // função
    svc 0
    
    // converte o resultado(struct) pra milissegundos
    // a chamada de sistema preencheu o buffer na pilha:
    // [sp+16] = tv_sec(segundos, 64 bits)
    // [sp+24] = tv_nsec(nanossegundos, 64 bits)
    
    // formula: total_ms = (segundos*1000)+(nanossegundos/1_000_000)
    ldr x2, [sp, 16] // x2 = segundos(tv_sec)
    ldr x3, [sp, 24] // x3 = nanossegundos(tv_nsec)
    
    // calcula(segundos * 1000)
    ldr x4, = 1000
    mul x2, x2, x4 // x2 = total de milissegundos dos segundos
    
    // calcula (nanossegundos / 1_000_000)
    ldr x4, = 1000000
    udiv x3, x3, x4 // x3 = total de milissegundos dos nanossegundos
    
    // soma os dois e coloca em x0(retorno)
    add x0, x2, x3 // x0 = (segundos*1000)+(nanos/1M)
    
    // restaura a pilha e retorna
    ldp x29, x30, [sp], 32  // restaura fp, lr e libera a pilha
    ret
// fim: [obter_tempo_milis]
// fim de biblis/sistema.asm

// fn: [somar] (vars: 0, total: 176)
.align 2
somar:
  sub sp, sp, 176
  stp x29, x30, [sp, 160]
  add x29, sp, 160
  stp x19, x20, [x29, -16]
  stp x21, x22, [x29, -32]
  str x0, [x29, -48]  // param a
  str x1, [x29, -56]  // param b
  ldr x0, = .tex_0
  bl _escrever_tex
  ldr w0, [x29, -48]
  bl _escrever_int
  ldr x0, = .tex_1
  bl _escrever_tex
  ldr x0, = .tex_2
  bl _escrever_tex
  ldr w0, [x29, -56]
  bl _escrever_int
  ldr x0, = .tex_1
  bl _escrever_tex
  ldr w0, [x29, -48]
  str w0, [sp, -16]!
  ldr w0, [x29, -56]
  ldr w1, [sp], 16
  add w0, w1, w0
  b .epilogo_12
  b .epilogo_12
.epilogo_12:
  ldp x19, x20, [x29, -16]
  ldp x21, x22, [x29, -32]
  ldp x29, x30, [sp, 160]
  add sp, sp, 176
  ret
// fim: [somar]
// fn: [inicio] (vars: 112, total: 240)
.align 2
inicio:
  sub sp, sp, 240
  stp x29, x30, [sp, 224]
  add x29, sp, 224
  ldr x0, = .tex_3
  bl _escrever_tex
  mov w0, 65
  strb w0, [x29, -32]
  ldr x0, = .tex_4
  bl _escrever_tex
  ldrb w0, [x29, -32]
  bl _escrever_car
  mov w0, 42
  str w0, [x29, -48]
  ldr x0, = .tex_5
  bl _escrever_tex
  ldr w0, [x29, -48]
  bl _escrever_int
  ldr x0, = const_0
  ldr s0, [x0]
  str s0, [x29, -64]
  ldr x0, = .tex_6
  bl _escrever_tex
  ldr s0, [x29, -64]
  bl _escrever_flu
  mov w0, 1
  strb w0, [x29, -80]
  ldr x0, = .tex_7
  bl _escrever_tex
  ldrb w0, [x29, -80]
  bl _escrever_bool
  bl obter_tempo_milis
  str x0, [x29, -96]
  ldr x0, = .tex_8
  bl _escrever_tex
  ldr x0, [x29, -96]
  bl _escrever_longo
  ldr x0, = .tex_9
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
  ldr x0, = .tex_10
  bl _escrever_tex
  ldr w0, [x29, -112]
  bl _escrever_int
  mov w0, 1
  str w0, [x29, -128]
  ldr x0, = .tex_11
  bl _escrever_tex
  ldr w0, [x29, -128]
  bl _escrever_int
  ldr x0, = .tex_1
  bl _escrever_tex
  ldr w0, [x29, -112]
  str w0, [sp, -16]!
  ldr w0, [x29, -48]
  str w0, [sp, -16]!
  ldrb w0, [x29, -32]
  str w0, [sp, -16]!
  ldrb w0, [x29, -80]
  str w0, [sp, -16]!
  ldr x0, [x29, -96]
  str x0, [sp, -16]!
  ldr x0, [sp, 64]
  ldr x1, [sp, 48]
  ldr x2, [sp, 32]
  ldr x3, [sp, 16]
  ldr x4, [sp, 0]
  add sp, sp, 80
  bl testeAlteracoes
  b .epilogo_13
.epilogo_13:
  ldp x29, x30, [sp, 224]
  add sp, sp, 240
  ret
// fim: [inicio]
// fn: [testeAlteracoes] (vars: 32, total: 208)
.align 2
testeAlteracoes:
  sub sp, sp, 208
  stp x29, x30, [sp, 192]
  add x29, sp, 192
  str x0, [x29, -16]  // param s
  str x1, [x29, -24]  // param numero
  str x2, [x29, -32]  // param letra
  str x3, [x29, -40]  // param marca
  str x4, [x29, -48]  // param numLongo
  mov w0, 5
  str w0, [sp, -16]!
  mov w0, 7
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [x29, -16]
  ldr x0, = .tex_12
  bl _escrever_tex
  ldr w0, [x29, -16]
  bl _escrever_int
  ldr x0, = .tex_13
  bl _escrever_tex
  mov w0, 100
  str w0, [x29, -24]
  mov w0, 90
  strb w0, [x29, -32]
  mov w0, 0
  strb w0, [x29, -40]
  ldr x0, = const_1
  ldr x0, [x0]
  str x0, [x29, -48]
  ldr x0, = .tex_14
  bl _escrever_tex
  ldr w0, [x29, -24]
  bl _escrever_int
  ldr x0, = .tex_15
  bl _escrever_tex
  ldrb w0, [x29, -32]
  bl _escrever_car
  ldr x0, = .tex_16
  bl _escrever_tex
  ldrb w0, [x29, -40]
  bl _escrever_bool
  ldr x0, = .tex_17
  bl _escrever_tex
  ldr x0, [x29, -48]
  bl _escrever_longo
  ldr x0, = .tex_1
  bl _escrever_tex
  ldr x0, = .tex_18
  bl _escrever_tex
  mov w0, 1
  str w0, [x29, -80]
  ldr x0, = .tex_19
  bl _escrever_tex
  ldr w0, [x29, -80]
  bl _escrever_int
  ldr x0, = .tex_1
  bl _escrever_tex
  mov w0, -1
  str w0, [x29, -80]
  ldr x0, = .tex_20
  bl _escrever_tex
  ldr w0, [x29, -80]
  bl _escrever_int
  ldr x0, = .tex_1
  bl _escrever_tex
  ldr x0, = const_2
  ldr s0, [x0]
  str s0, [x29, -96]
  ldr x0, = .tex_21
  bl _escrever_tex
  ldr s0, [x29, -96]
  bl _escrever_flu
  ldr x0, = .tex_1
  bl _escrever_tex
  ldr x0, = const_3
  ldr s0, [x0]
  str s0, [x29, -96]
  ldr x0, = .tex_22
  bl _escrever_tex
  ldr s0, [x29, -96]
  bl _escrever_flu
  ldr x0, = .tex_1
  bl _escrever_tex
  bl testeOperacoes
  bl testeComparacoes
  bl testeLoops
  bl testeMemoria
  bl testeMatrizes
  b .epilogo_14
.epilogo_14:
  ldp x29, x30, [sp, 192]
  add sp, sp, 208
  ret
// fim: [testeAlteracoes]
// fn: [testeOperacoes] (vars: 32, total: 160)
.align 2
testeOperacoes:
  sub sp, sp, 160
  stp x29, x30, [sp, 144]
  add x29, sp, 144
  ldr x0, = .tex_23
  bl _escrever_tex
  ldr x0, = .tex_24
  bl _escrever_tex
  mov w0, 5
  str w0, [sp, -16]!
  mov w0, 5
  str w0, [sp, -16]!
  mov w0, 5
  ldr w1, [sp], 16
  mul w0, w1, w0
  ldr w1, [sp], 16
  add w0, w1, w0
  bl _escrever_int
  ldr x0, = .tex_1
  bl _escrever_tex
  ldr x0, = .tex_25
  bl _escrever_tex
  mov w0, 5
  str w0, [sp, -16]!
  mov w0, 5
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [sp, -16]!
  mov w0, 5
  ldr w1, [sp], 16
  mul w0, w1, w0
  bl _escrever_int
  ldr x0, = .tex_1
  bl _escrever_tex
  mov w0, 5
  str w0, [sp, -16]!
  mov w0, 5
  ldr w1, [sp], 16
  add w0, w1, w0
  bl _escrever_int
  ldr x0, = .tex_1
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
  ldr x0, = .tex_1
  bl _escrever_tex
  ldr x0, = .tex_26
  bl _escrever_tex
  mov w0, 10
  str w0, [sp, -16]!
  mov w0, 3
  ldr w1, [sp], 16
  sdiv w2, w1, w0
  msub w0, w2, w0, w1
  bl _escrever_int
  ldr x0, = .tex_1
  bl _escrever_tex
  ldr x0, = .tex_27
  bl _escrever_tex
  mov w0, 3
  str w0, [x29, -32]
  ldr x0, = const_4
  ldr s0, [x0]
  str s0, [x29, -48]
  ldr x0, = .tex_28
  bl _escrever_tex
  ldr s0, [x29, -48]
  bl _escrever_flu
  ldr x0, = .tex_29
  bl _escrever_tex
  ldr w0, [x29, -32]
  bl _escrever_int
  ldr x0, = .tex_30
  bl _escrever_tex
  ldr s0, [x29, -48]
  str s0, [sp, -16]!
  ldr w0, [x29, -32]
  ldr s1, [sp], 16
  scvtf s0, w0
  fmul s0, s1, s0
  bl _escrever_flu
  ldr x0, = .tex_1
  bl _escrever_tex
  ldr x0, = .tex_31
  bl _escrever_tex
  mov w0, 10
  str w0, [sp, -16]!
  mov w0, 2
  ldr w1, [sp], 16
  lsl w0, w1, w0
  bl _escrever_int
  ldr x0, = .tex_1
  bl _escrever_tex
  b .epilogo_15
.epilogo_15:
  ldp x29, x30, [sp, 144]
  add sp, sp, 160
  ret
// fim: [testeOperacoes]
// fn: [testeComparacoes] (vars: 64, total: 192)
.align 2
testeComparacoes:
  sub sp, sp, 192
  stp x29, x30, [sp, 176]
  add x29, sp, 176
  ldr x0, = .tex_32
  bl _escrever_tex
  mov w0, 4
  str w0, [x29, -32]
  ldr w0, [x29, -32]
  str w0, [sp, -16]!
  mov w0, 5
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, gt
  cmp w0, 0
  beq .B1
  ldr x0, = .tex_33
  bl _escrever_tex
  b .B2
.B1:
  ldr w0, [x29, -32]
  str w0, [sp, -16]!
  mov w0, 5
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, ge
  cmp w0, 0
  beq .B4
  ldr x0, = .tex_34
  bl _escrever_tex
  b .B5
.B4:
  ldr x0, = .tex_35
  bl _escrever_tex
.B5:
.B2:
  mov w0, 5
  str w0, [x29, -48]
  ldr w0, [x29, -48]
  str w0, [sp, -16]!
  mov w0, 4
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, ge
  cmp w0, 0
  beq .B7
  ldr w0, [x29, -32]
  str w0, [sp, -16]!
  mov w0, 4
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, gt
  cmp w0, 0
  beq .B7
  mov w0, 1
  b .B8
.B7:
  mov w0, 0
.B8:
  cmp w0, 0
  beq .B9
  ldr x0, = .tex_36
  bl _escrever_tex
  b .B10
.B9:
  ldr x0, = .tex_37
  bl _escrever_tex
.B10:
  ldr w0, [x29, -48]
  str w0, [sp, -16]!
  ldr w0, [x29, -32]
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 1
  beq .B13
  ldr w0, [x29, -32]
  str w0, [sp, -16]!
  mov w0, 3
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, gt
  cmp w0, 1
  beq .B13
.B12:
  mov w0, 0
  b .B13
  mov w0, 1
.B13:
  cmp w0, 0
  beq .B14
  ldr x0, = .tex_38
  bl _escrever_tex
  b .B15
.B14:
  ldr x0, = .tex_39
  bl _escrever_tex
.B15:
  ldr x0, = .tex_40
  bl _escrever_tex
  ldr x0, = .tex_41
  str x0, [x29, -64]
  ldr x0, = .tex_42
  str x0, [x29, -80]
  ldr x0, [x29, -64]
  bl _escrever_tex
  ldr x0, = .tex_1
  bl _escrever_tex
  ldr x0, [x29, -80]
  bl _escrever_tex
  ldr x0, = .tex_1
  bl _escrever_tex
  ldr x0, [x29, -64]
  str x0, [sp, -16]!
  ldr x0, [x29, -80]
  str x0, [sp, -16]!
  ldr x0, [sp, 16]
  ldr x1, [sp, 0]
  add sp, sp, 32
  bl texcmp
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B17
  ldr x0, = .tex_43
  bl _escrever_tex
  b .B18
.B17:
.B18:
  ldr x0, [x29, -64]
  str x0, [sp, -16]!
  ldr x0, [x29, -80]
  str x0, [sp, -16]!
  ldr x0, [sp, 16]
  ldr x1, [sp, 0]
  add sp, sp, 32
  bl texcmp
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B20
  ldr x0, = .tex_44
  bl _escrever_tex
  b .B21
.B20:
.B21:
  ldr x0, [x29, -64]
  str x0, [sp, -16]!
  ldr x0, [x29, -64]
  str x0, [sp, -16]!
  ldr x0, [sp, 16]
  ldr x1, [sp, 0]
  add sp, sp, 32
  bl texcmp
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B23
  ldr x0, = .tex_45
  bl _escrever_tex
  b .B24
.B23:
  ldr x0, = .tex_46
  bl _escrever_tex
.B24:
  b .epilogo_16
.epilogo_16:
  ldp x29, x30, [sp, 176]
  add sp, sp, 192
  ret
// fim: [testeComparacoes]
// fn: [testeMemoria] (vars: 192, total: 320)
.align 2
testeMemoria:
  sub sp, sp, 320
  stp x29, x30, [sp, 304]
  add x29, sp, 304
  ldr x0, = .tex_47
  bl _escrever_tex
  mov w1, 116
  strb w1, [x29, -32]
  mov w1, 101
  strb w1, [x29, -31]
  mov w1, 120
  strb w1, [x29, -30]
  mov w1, 116
  strb w1, [x29, -29]
  mov w1, 111
  strb w1, [x29, -28]
  mov w1, 0
  strb w1, [x29, -27]
  ldr x0, = .tex_48
  bl _escrever_tex
  add x0, x29, -32
  bl _escrever_tex
  mov w0, 0
  mov w1, w0
  str w1, [sp, -16]!
  mov w0, 88
  ldr w1, [sp], 16
  add x2, x29, -32
  add x2, x2, x1
  strb w0, [x2]
  ldr x0, = .tex_49
  bl _escrever_tex
  add x0, x29, -32
  bl _escrever_tex
  ldr x0, = .tex_50
  bl _escrever_tex
  mov w0, 10
  str w0, [x29, -48]
  ldr x0, = .tex_51
  bl _escrever_tex
  add x0, x29, -48
  str x0, [x29, -64]
  ldr x0, = .tex_52
  bl _escrever_tex
  ldr x1, [x29, -64]
  ldr w0, [x1]
  str w0, [sp, -16]!
  mov w0, 5
  ldr w1, [sp], 16
  add w0, w1, w0
  ldr x1, [x29, -64]
  str w0, [x1]
  ldr x0, = .tex_53
  bl _escrever_tex
  ldr w0, [x29, -48]
  bl _escrever_int
  ldr x0, = .tex_1
  bl _escrever_tex
  ldr x0, = .tex_54
  str x0, [x29, -80]
  ldr x0, = .tex_55
  bl _escrever_tex
  ldr x0, [x29, -80]
  bl _escrever_tex
  ldr x0, = .tex_56
  bl _escrever_tex
  ldr x0, [x29, -80]
  str x0, [sp, -16]!
  ldr x0, [sp, 0]
  add sp, sp, 16
  bl textam
  bl _escrever_int
  ldr x0, [x29, -80]
  str x0, [sp, -16]!
  mov w0, 116
  str w0, [sp, -16]!
  ldr x0, [sp, 16]
  ldr x1, [sp, 0]
  add sp, sp, 32
  bl texcar
  str w0, [x29, -96]
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, ge
  cmp w0, 0
  beq .B26
  ldr x0, = .tex_57
  bl _escrever_tex
  ldr w0, [x29, -96]
  bl _escrever_int
  b .B27
.B26:
  ldr x0, = .tex_58
  bl _escrever_tex
.B27:
  ldr x0, = .tex_59
  bl _escrever_tex
  ldr x0, = .tex_60
  str x0, [x29, -112]
  mov w1, 101
  strb w1, [x29, -128]
  mov w1, 120
  strb w1, [x29, -127]
  mov w1, 101
  strb w1, [x29, -126]
  mov w1, 109
  strb w1, [x29, -125]
  mov w1, 112
  strb w1, [x29, -124]
  mov w1, 108
  strb w1, [x29, -123]
  mov w1, 111
  strb w1, [x29, -122]
  mov w1, 0
  strb w1, [x29, -121]
  ldr x0, = .tex_61
  bl _escrever_tex
  add x0, x29, -128
  bl _escrever_tex
  add x0, x29, -128
  str x0, [sp, -16]!
  ldr x0, [x29, -112]
  str x0, [sp, -16]!
  ldr x0, [x29, -112]
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
  ldr x0, = .tex_62
  bl _escrever_tex
  add x0, x29, -128
  bl _escrever_tex
  add x0, x29, -128
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
  ldr x0, = .tex_63
  bl _escrever_tex
  add x0, x29, -128
  bl _escrever_tex
  ldr x0, = .tex_64
  bl _escrever_tex
  mov w0, 0
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  add x2, x29, -128
  add x2, x2, x0
  ldrb w0, [x2]
  strb w0, [x29, -144]
  ldr x0, = .tex_65
  bl _escrever_tex
  ldrb w0, [x29, -144]
  bl _escrever_car
  ldr x0, = .tex_1
  bl _escrever_tex
  mov w0, 0
  str w0, [x29, -160]
  mov w0, 1
  str w0, [x29, -156]
  mov w0, 2
  str w0, [x29, -152]
  mov w0, 5
  str w0, [x29, -148]
  ldr x0, = .tex_66
  bl _escrever_tex
  mov w0, 0
  str w0, [x29, 304]
.B29:
  ldr w0, [x29, 304]
  str w0, [sp, -16]!
  mov w0, 4
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B30
  ldr x0, = .tex_67
  bl _escrever_tex
  ldr w0, [x29, 304]
  bl _escrever_int
  ldr x0, = .tex_68
  bl _escrever_tex
  ldr w0, [x29, 304]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 4
  mul w1, w1, w2
  add w0, w0, w1
  add x2, x29, -160
  add x2, x2, x0
  ldr w0, [x2]
  bl _escrever_int
  ldr x0, = .tex_1
  bl _escrever_tex
  // incremento
  ldr w0, [x29, 304]
  add w0, w0, 1
  str w0, [x29, 304]
  b .B29
.B30:
  ldr x0, = const_5
  ldr s0, [x0]
  str s0, [x29, -176]
  ldr x0, = const_6
  ldr s0, [x0]
  str s0, [x29, -172]
  ldr x0, = const_7
  ldr s0, [x0]
  str s0, [x29, -168]
  ldr x0, = const_7
  ldr s0, [x0]
  str s0, [x29, -164]
  ldr x0, = .tex_69
  bl _escrever_tex
  mov w0, 0
  str w0, [x29, 288]
.B32:
  ldr w0, [x29, 288]
  str w0, [sp, -16]!
  mov w0, 4
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B33
  ldr x0, = .tex_67
  bl _escrever_tex
  ldr w0, [x29, 288]
  bl _escrever_int
  ldr x0, = .tex_68
  bl _escrever_tex
  ldr w0, [x29, 288]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 4
  mul w1, w1, w2
  add w0, w0, w1
  add x2, x29, -176
  add x2, x2, x0
  ldr s0, [x2]
  bl _escrever_flu
  ldr x0, = .tex_1
  bl _escrever_tex
  // incremento
  ldr w0, [x29, 288]
  add w0, w0, 1
  str w0, [x29, 288]
  b .B32
.B33:
  b .epilogo_17
.epilogo_17:
  ldp x29, x30, [sp, 304]
  add sp, sp, 320
  ret
// fim: [testeMemoria]
// fn: [testeLoops] (vars: 32, total: 160)
.align 2
testeLoops:
  sub sp, sp, 160
  stp x29, x30, [sp, 144]
  add x29, sp, 144
  ldr x0, = .tex_70
  bl _escrever_tex
  ldr x0, = .tex_71
  bl _escrever_tex
  mov w0, 0
  str w0, [x29, -32]
.B34:
  ldr w0, [x29, -32]
  str w0, [sp, -16]!
  mov w0, 10
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B35
  ldr x0, = .tex_72
  bl _escrever_tex
  ldr w0, [x29, -32]
  bl _escrever_int
  ldr w0, [x29, -32]
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  add w0, w1, w0
  str w0, [x29, -32]
  b .B34
.B35:
  ldr x0, = .tex_73
  bl _escrever_tex
  mov w0, 0
  str w0, [x29, 144]
.B37:
  ldr w0, [x29, 144]
  str w0, [sp, -16]!
  mov w0, 10
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B38
  ldr x0, = .tex_74
  bl _escrever_tex
  ldr w0, [x29, 144]
  bl _escrever_int
  ldr x0, = .tex_1
  bl _escrever_tex
  // incremento
  ldr w0, [x29, 144]
  add w0, w0, 1
  str w0, [x29, 144]
  b .B37
.B38:
  b .epilogo_18
.epilogo_18:
  ldp x29, x30, [sp, 144]
  add sp, sp, 160
  ret
// fim: [testeLoops]
// fn: [testeMatrizes] (vars: 32, total: 160)
.align 2
testeMatrizes:
  sub sp, sp, 160
  stp x29, x30, [sp, 144]
  add x29, sp, 144
  ldr x0, = .tex_75
  bl _escrever_tex
  mov w0, 0
  str w0, [x29, -32]
  mov w0, 1
  str w0, [x29, -28]
  mov w0, 4
  str w0, [x29, -24]
  mov w0, 4
  str w0, [x29, -28]
  mov w0, 1
  str w0, [x29, -24]
  mov w0, 0
  str w0, [x29, -20]
  ldr x0, = .tex_76
  bl _escrever_tex
  mov w0, 0
  str w0, [sp, -16]!
  mov w0, 1
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 4
  mul w1, w1, w2
  add w0, w0, w1
  ldr w1, [sp], 16
  mov w2, 4
  mul w1, w1, w2
  add w0, w0, w1
  add x2, x29, -32
  add x2, x2, x0
  ldr w0, [x2]
  bl _escrever_int
  ldr x0, = .tex_1
  bl _escrever_tex
  ldr x0, = const_8
  ldr s0, [x0]
  str s0, [x29, -48]
  ldr x0, = const_9
  ldr s0, [x0]
  str s0, [x29, -44]
  ldr x0, = const_10
  ldr s0, [x0]
  str s0, [x29, -40]
  ldr x0, = const_11
  ldr s0, [x0]
  str s0, [x29, -44]
  ldr x0, = const_12
  ldr s0, [x0]
  str s0, [x29, -40]
  ldr x0, = const_13
  ldr s0, [x0]
  str s0, [x29, -36]
  ldr x0, = .tex_77
  bl _escrever_tex
  mov w0, 0
  str w0, [sp, -16]!
  mov w0, 1
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 4
  mul w1, w1, w2
  add w0, w0, w1
  ldr w1, [sp], 16
  mov w2, 4
  mul w1, w1, w2
  add w0, w0, w1
  add x2, x29, -48
  add x2, x2, x0
  ldr s0, [x2]
  bl _escrever_flu
  ldr x0, = .tex_1
  bl _escrever_tex
  b .epilogo_19
.epilogo_19:
  ldp x29, x30, [sp, 144]
  add sp, sp, 160
  ret
// fim: [testeMatrizes]
  .section .rodata
  .align 8
const_0:
  .float 3.140000
const_1:
  .quad 1763400788119
const_2:
  .float 1.100000
const_3:
  .float -1.100000
const_4:
  .float 0.500000
const_5:
  .float 0.200000
const_6:
  .float 1.500000
const_7:
  .float 5.100000
const_8:
  .float 3.000000
const_9:
  .float 5.000000
const_10:
  .float 7.000000
const_11:
  .float 2.000000
const_12:
  .float 9.000000
const_13:
  .float 10.000000
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
.tex_8: .asciz "\nlongo: "
.tex_9: .asciz "\n\nTestando função soma:\n"
.tex_10: .asciz "\nsoma com retorno 5 + 7 = esperando 12, veio: "
.tex_11: .asciz "\n\nVariavel final inteira:\n"
.tex_12: .asciz "\nsoma comum 5 + 7 = esperando 12, veio: "
.tex_13: .asciz "\n\nTestando atribuições:\n"
.tex_14: .asciz "\nnovo inteiro: "
.tex_15: .asciz "\nnovo caractere: "
.tex_16: .asciz "\nnovo booleano: "
.tex_17: .asciz "\nnovo longo: "
.tex_18: .asciz "\nTeste de positivo e negativo:\n\n"
.tex_19: .asciz "inteiro positivo: "
.tex_20: .asciz "inteiro negativo: "
.tex_21: .asciz "flutuante positivo: "
.tex_22: .asciz "flutuante negativo: "
.tex_23: .asciz "\n\nTeste de operações matematicas:\n\n"
.tex_24: .asciz "operação 5 + 5 * 5, esperado: 30, veio: "
.tex_25: .asciz "operação (5 + 5) * 5, esperado: 50, veio: "
.tex_26: .asciz "10 % 3 = ?, esperado: 1, recebido: "
.tex_27: .asciz "\n\nTeste de operações entre tipos:\n\n"
.tex_28: .asciz "x: "
.tex_29: .asciz " * y: "
.tex_30: .asciz ", resultado: "
.tex_31: .asciz "10 << 2, resultado: "
.tex_32: .asciz "\nTeste comparações:\n\n"
.tex_33: .asciz "x é maior que 5\n"
.tex_34: .asciz "x é maior ou igual a 5\n"
.tex_35: .asciz "x não é maior nem igual a 5\n"
.tex_36: .asciz "y >= 4 && x > 4 é verdadeiro\n"
.tex_37: .asciz "y >= 4 && x > 4 é falso\n"
.tex_38: .asciz "y == x || x > 3 é verdadeiro\n"
.tex_39: .asciz "y == x || x > 3 é falso\n"
.tex_40: .asciz "\nComparação com textos:\n\n"
.tex_41: .asciz "texto 1"
.tex_42: .asciz "texto 2"
.tex_43: .asciz "texto 1 é igual a texto 2\n"
.tex_44: .asciz "texto 1 não é igual a texto 2\n"
.tex_45: .asciz "o primeiro texto é texto 1"
.tex_46: .asciz "o primeiro texto não é texto 1"
.tex_47: .asciz "\nTeste de array:\n"
.tex_48: .asciz "\nvalor do array: "
.tex_49: .asciz "\narray mudado no indice 0: "
.tex_50: .asciz "\n\nTeste de ponteiro:\n\n"
.tex_51: .asciz "int x1 = 10; int* p1 = @x1;\n"
.tex_52: .asciz "p1 = p1 + 5; x1 = ?\n"
.tex_53: .asciz "x1 = "
.tex_54: .asciz "exemplo de ponteiro"
.tex_55: .asciz "\nponteiro texto, valor: "
.tex_56: .asciz "\ntamamho em bytes: "
.tex_57: .asciz "\no ponteiro tem t no indice: "
.tex_58: .asciz "\no ponteiro não tem t\n"
.tex_59: .asciz "\nTeste de manipulação da memoria:\n"
.tex_60: .asciz "XxXmplo maior"
.tex_61: .asciz "\nArray padrão: "
.tex_62: .asciz "\nArray copiado da memoria: "
.tex_63: .asciz "\nArray usando subscar(array2, 'X', 'e'): "
.tex_64: .asciz "\nTeste de acesso a itens array:\n"
.tex_65: .asciz "item do indice 0 do array: "
.tex_66: .asciz "\nArray de inteiros: \n\n"
.tex_67: .asciz "no indice: "
.tex_68: .asciz " valor: "
.tex_69: .asciz "\nArray de flutuantes: \n\n"
.tex_70: .asciz "\n\nTeste de loops"
.tex_71: .asciz "\nEnquanto:"
.tex_72: .asciz "\nvalor de i: "
.tex_73: .asciz "\n\nPor:\n"
.tex_74: .asciz "indice: "
.tex_75: .asciz "\n\nTeste de matrizes:\n\n"
.tex_76: .asciz "matriz 2D int m2[0][1]: "
.tex_77: .asciz "matriz 2D flu m2f[0][1]: "
.section .text

