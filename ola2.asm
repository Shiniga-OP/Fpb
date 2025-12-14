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
  mov w1, w0 // otimizado(literal)
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
  mov w8, w0
  mov w0, 10
  mov w1, w8
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
  mov w8, w0
  mov w0, 100
  mov w1, w8
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
  mov w8, w0
  mov w0, 10
  mov w1, w8
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
  mov w8, w0
  mov w0, 1000
  mov w1, w8
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
  mov w8, w0
  mov w0, 100
  mov w1, w8
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
  mov w8, w0
  mov w0, 10
  mov w1, w8
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
  mov w8, w0
  mov w0, 10
  mov w1, w8
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
  mov w1, w0 // otimizado(literal)
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
  mov w1, w0 // otimizado(literal)
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
  fmov s8, s0
  ldr x0, = const_1
  ldr s0, [x0]
  fmov s1, s8
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
  fmov s8, s0
  ldr s0, [x29, 224]
  fmov s1, s8
  fmul s0, s1, s0
  ldr s1, [sp], 16
  fadd s0, s1, s0
  str s0, [x29, -112]
  ldr s0, [x29, 224]
  fmov s8, s0
  ldr x0, = const_2
  ldr s0, [x0]
  fmov s1, s8
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
// Espaço "Pessoa" definido: 16 bytes
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
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_1
  bl _escrever_tex
  ldr w0, [x29, -56]
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr w0, [x29, -48]
  str w0, [sp, -16]!
  ldr w0, [x29, -56]
  ldr w1, [sp], 16
  add w0, w1, w0
  b 1f
  b 1f
// epilogo
1:
  ldp x19, x20, [x29, -16]
  ldp x21, x22, [x29, -32]
  ldp x29, x30, [sp, 160]
  add sp, sp, 176
  ret
// fim: [somar]
// fn: [inicio] (vars: 128, total: 256)
.align 2
inicio:
  sub sp, sp, 256
  stp x29, x30, [sp, 240]
  add x29, sp, 240
  ldr x0, = .tex_2
  bl _escrever_tex
  mov w0, 65
  strb w0, [x29, -32]
  ldr x0, = .tex_3
  bl _escrever_tex
  ldrb w0, [x29, -32]
  bl _escrever_car
  mov w0, 42
  str w0, [x29, -48]
  ldr x0, = .tex_4
  bl _escrever_tex
  ldr w0, [x29, -48]
  bl _escrever_int
  ldr x0, = const_3
  ldr s0, [x0]
  str s0, [x29, -64]
  ldr x0, = .tex_5
  bl _escrever_tex
  ldr s0, [x29, -64]
  bl _escrever_flu
  mov w0, 1
  strb w0, [x29, -80]
  ldr x0, = .tex_6
  bl _escrever_tex
  ldrb w0, [x29, -80]
  bl _escrever_bool
  add sp, sp, 0  // limpa temporarios
  bl obter_tempo_milis
  str x0, [x29, -96]
  ldr x0, = .tex_7
  bl _escrever_tex
  ldr x0, [x29, -96]
  bl _escrever_longo
  mov w0, 255 // byte: 0xFF
  strb w0, [x29, -112]
  ldr x0, = .tex_8
  bl _escrever_tex
  ldrb w0, [x29, -112]
  bl _escrever_int
  ldr x0, = .tex_comb_0
  bl _escrever_tex
  mov w0, 5
  str w0, [sp, -16]!  // salva param 0 (int/bool/char/byte)
  mov w0, 7
  str w0, [sp, -16]!  // salva param 1 (int/bool/char/byte)
  ldr w1, [sp, 0]  // carrega param 1 (int/bool) em w1
  mov x1, x1  // estende pra 64 bits
  ldr w0, [sp, 16]  // carrega param 0 (int/bool) em w0
  mov x0, x0  // estende pra 64 bits
  add sp, sp, 32  // limpa temporarios
  bl somar
  str w0, [x29, -128]
  ldr x0, = .tex_11
  bl _escrever_tex
  ldr w0, [x29, -128]
  bl _escrever_int
  mov w0, 1
  str w0, [x29, -144]
  ldr x0, = .tex_12
  bl _escrever_tex
  ldr w0, [x29, -144]
  bl _escrever_int
  ldr x0, = .tex_13
  bl _escrever_tex
  ldr w0, [x29, -128]
  str w0, [sp, -16]!  // salva param 0 (int/bool/char/byte)
  ldr w0, [x29, -48]
  str w0, [sp, -16]!  // salva param 1 (int/bool/char/byte)
  ldrb w0, [x29, -32]
  str w0, [sp, -16]!  // salva param 2 (int/bool/char/byte)
  ldrb w0, [x29, -80]
  str w0, [sp, -16]!  // salva param 3 (int/bool/char/byte)
  ldr x0, [x29, -96]
  str x0, [sp, -16]!  // salva param 4 (ponteiro/longo)
  ldr x4, [sp, 0]  // carrega param 4 (ptr/longo) em x4
  ldr w3, [sp, 16]  // carrega param 3 (int/bool) em w3
  mov x3, x3  // estende pra 64 bits
  ldr w2, [sp, 32]  // carrega param 2 (int/bool) em w2
  mov x2, x2  // estende pra 64 bits
  ldr w1, [sp, 48]  // carrega param 1 (int/bool) em w1
  mov x1, x1  // estende pra 64 bits
  ldr w0, [sp, 64]  // carrega param 0 (int/bool) em w0
  mov x0, x0  // estende pra 64 bits
  add sp, sp, 80  // limpa temporarios
  bl testeAlteracoes
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 240]
  add sp, sp, 256
  mov x0, 0
  mov x8, 93
  svc 0
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
  ldr x0, = .tex_14
  bl _escrever_tex
  ldr w0, [x29, -16]
  bl _escrever_int
  ldr x0, = .tex_15
  bl _escrever_tex
  mov w0, 100
  str w0, [x29, -24]
  mov w0, 90
  strb w0, [x29, -32]
  mov w0, 0
  strb w0, [x29, -40]
  ldr x0, = const_4
  ldr x0, [x0]
  str x0, [x29, -48]
  ldr x0, = .tex_16
  bl _escrever_tex
  ldr w0, [x29, -24]
  bl _escrever_int
  ldr x0, = .tex_17
  bl _escrever_tex
  ldrb w0, [x29, -32]
  bl _escrever_car
  ldr x0, = .tex_18
  bl _escrever_tex
  ldrb w0, [x29, -40]
  bl _escrever_bool
  ldr x0, = .tex_19
  bl _escrever_tex
  ldr x0, [x29, -48]
  bl _escrever_longo
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_20
  bl _escrever_tex
  mov w0, 1
  str w0, [x29, -80]
  ldr x0, = .tex_21
  bl _escrever_tex
  ldr w0, [x29, -80]
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  mov w0, -1
  str w0, [x29, -80]
  ldr x0, = .tex_22
  bl _escrever_tex
  ldr w0, [x29, -80]
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr x0, = const_5
  ldr s0, [x0]
  str s0, [x29, -96]
  ldr x0, = .tex_23
  bl _escrever_tex
  ldr s0, [x29, -96]
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  ldr x0, = const_6
  ldr s0, [x0]
  str s0, [x29, -96]
  ldr x0, = .tex_24
  bl _escrever_tex
  ldr s0, [x29, -96]
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_comb_1
  bl _escrever_tex
  mov w0, 65
  and w0, w0, 0xFF
  bl _escrever_car
  add sp, sp, 0  // limpa temporarios
  bl testeOperacoes
  add sp, sp, 0  // limpa temporarios
  bl testeComparacoes
  add sp, sp, 0  // limpa temporarios
  bl testeLoops
  add sp, sp, 0  // limpa temporarios
  bl testeMemoria
  add sp, sp, 0  // limpa temporarios
  bl testeMatrizes
  add sp, sp, 0  // limpa temporarios
  bl testeEspaco
  add sp, sp, 0  // limpa temporarios
  bl testeConversao
  b 1f
// epilogo
1:
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
  ldr x0, = .tex_comb_2
  bl _escrever_tex
  mov w0, 5
  str w0, [sp, -16]!
  mov w0, 5
  mov w8, w0
  mov w0, 5
  mov w1, w8
  mul w0, w1, w0
  ldr w1, [sp], 16
  add w0, w1, w0
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_29
  bl _escrever_tex
  mov w0, 5
  mov w1, w0 // otimizado(literal)
  add w0, w1, w0
  mov w8, w0
  mov w0, 5
  mov w1, w8
  mul w0, w1, w0
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  mov w0, 5
  mov w1, w0 // otimizado(literal)
  add w0, w1, w0
  bl _escrever_int
  ldr x0, = .tex_13
  bl _escrever_tex
  mov w0, 5
  str w0, [sp, -16]!  // salva param 0 (int/bool/char/byte)
  mov w0, 5
  str w0, [sp, -16]!  // salva param 1 (int/bool/char/byte)
  ldr w1, [sp, 0]  // carrega param 1 (int/bool) em w1
  mov x1, x1  // estende pra 64 bits
  ldr w0, [sp, 16]  // carrega param 0 (int/bool) em w0
  mov x0, x0  // estende pra 64 bits
  add sp, sp, 32  // limpa temporarios
  bl somar
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_30
  bl _escrever_tex
  mov w0, 10
  mov w8, w0
  mov w0, 3
  mov w1, w8
  sdiv w2, w1, w0
  msub w0, w2, w0, w1
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_31
  bl _escrever_tex
  mov w0, 3
  str w0, [x29, -32]
  ldr x0, = const_7
  ldr s0, [x0]
  str s0, [x29, -48]
  ldr x0, = .tex_32
  bl _escrever_tex
  ldr s0, [x29, -48]
  bl _escrever_flu
  ldr x0, = .tex_33
  bl _escrever_tex
  ldr w0, [x29, -32]
  bl _escrever_int
  ldr x0, = .tex_34
  bl _escrever_tex
  ldr s0, [x29, -48]
  fmov s8, s0
  ldr w0, [x29, -32]
  fmov s1, s8
  mul w0, w1, w0
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_35
  bl _escrever_tex
  mov w0, 10
  mov w8, w0
  mov w0, 2
  mov w1, w8
  lsl w0, w1, w0
  bl _escrever_int
  ldr x0, = .tex_comb_3
  bl _escrever_tex
  mov w0, 10
  mov w8, w0
  mov w0, 2
  mov w1, w8
  lsr w0, w1, w0
  bl _escrever_int
  ldr x0, = .tex_comb_4
  bl _escrever_tex
  mov w0, 124
  mov w8, w0
  mov w0, 15
  mov w1, w8
  and w0, w1, w0
  bl _escrever_int
  ldr x0, = .tex_comb_5
  bl _escrever_tex
  mov w0, 124
  mov w8, w0
  mov w0, 15 // byte: 0xF
  mov w1, w8
  and w0, w1, w0
  bl _escrever_int
  ldr x0, = .tex_comb_6
  bl _escrever_tex
  mov w0, 1
  str w0, [sp, -16]!
  mov w0, 2
  ldr w1, [sp], 16
  orr w0, w1, w0
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  b 1f
// epilogo
1:
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
  ldr x0, = .tex_40
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
  beq .B41
  ldr x0, = .tex_41
  bl _escrever_tex
  b .B42
.B41:
  ldr w0, [x29, -32]
  str w0, [sp, -16]!
  mov w0, 5
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, ge
  cmp w0, 0
  beq .B43
  ldr x0, = .tex_42
  bl _escrever_tex
  b .B44
.B43:
  ldr x0, = .tex_43
  bl _escrever_tex
.B44:
.B42:
  mov w0, 5
  str w0, [x29, -48]
  ldr w0, [x29, -48]
  str w0, [sp, -16]!
  mov w0, 4
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, ge
  str w0, [sp, -16]!
  ldr w0, [x29, -32]
  str w0, [sp, -16]!
  mov w0, 4
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, gt
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  cmp w0, 0
  beq .B45
  ldr x0, = .tex_44
  bl _escrever_tex
  b .B46
.B45:
  ldr x0, = .tex_45
  bl _escrever_tex
.B46:
  ldr w0, [x29, -48]
  str w0, [sp, -16]!
  ldr w0, [x29, -32]
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  str w0, [sp, -16]!
  ldr w0, [x29, -32]
  str w0, [sp, -16]!
  mov w0, 3
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, gt
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  orr w0, w1, w0
  cmp w0, 0
  beq .B47
  ldr x0, = .tex_46
  bl _escrever_tex
  b .B48
.B47:
  ldr x0, = .tex_47
  bl _escrever_tex
.B48:
  ldr x0, = .tex_48
  bl _escrever_tex
  ldr x0, = .tex_49
  str x0, [x29, -64]
  ldr x0, = .tex_50
  str x0, [x29, -80]
  ldr x0, [x29, -64]
  bl _escrever_tex
  mov w0, 10
  bl _escrever_car
  ldr x0, [x29, -80]
  bl _escrever_tex
  mov w0, 10
  bl _escrever_car
  ldr x0, [x29, -64]
  str x0, [sp, -16]!  // salva param 0 (ponteiro/longo)
  ldr x0, [x29, -80]
  str x0, [sp, -16]!  // salva param 1 (ponteiro/longo)
  ldr x1, [sp, 0]  // carrega param 1 (ptr/longo) em x1
  ldr x0, [sp, 16]  // carrega param 0 (ptr/longo) em x0
  add sp, sp, 32  // limpa temporarios
  bl texcmp
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B49
  ldr x0, = .tex_51
  bl _escrever_tex
  b .B50
.B49:
.B50:
  ldr x0, [x29, -64]
  str x0, [sp, -16]!  // salva param 0 (ponteiro/longo)
  ldr x0, [x29, -80]
  str x0, [sp, -16]!  // salva param 1 (ponteiro/longo)
  ldr x1, [sp, 0]  // carrega param 1 (ptr/longo) em x1
  ldr x0, [sp, 16]  // carrega param 0 (ptr/longo) em x0
  add sp, sp, 32  // limpa temporarios
  bl texcmp
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B51
  ldr x0, = .tex_52
  bl _escrever_tex
  b .B52
.B51:
.B52:
  ldr x0, [x29, -64]
  str x0, [sp, -16]!  // salva param 0 (ponteiro/longo)
  ldr x0, [x29, -64]
  str x0, [sp, -16]!  // salva param 1 (ponteiro/longo)
  ldr x1, [sp, 0]  // carrega param 1 (ptr/longo) em x1
  ldr x0, [sp, 16]  // carrega param 0 (ptr/longo) em x0
  add sp, sp, 32  // limpa temporarios
  bl texcmp
  str w0, [sp, -16]!
  mov w0, 1
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B53
  ldr x0, = .tex_53
  bl _escrever_tex
  b .B54
.B53:
  ldr x0, = .tex_54
  bl _escrever_tex
.B54:
  b 1f
// epilogo
1:
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
  ldr x0, = .tex_55
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
  ldr x0, = .tex_56
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
  ldr x0, = .tex_57
  bl _escrever_tex
  add x0, x29, -32
  bl _escrever_tex
  ldr x0, = .tex_58
  bl _escrever_tex
  mov w0, 10
  str w0, [x29, -48]
  ldr x0, = .tex_59
  bl _escrever_tex
  add x0, x29, -48
  str x0, [x29, -64]
  ldr x0, = .tex_60
  bl _escrever_tex
  ldr x1, [x29, -64]
  ldr w0, [x1]
  str w0, [sp, -16]!
  mov w0, 5
  ldr w1, [sp], 16
  add w0, w1, w0
  ldr x1, [x29, -64]
  str w0, [x1]
  ldr x0, = .tex_61
  bl _escrever_tex
  ldr w0, [x29, -48]
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_62
  str x0, [x29, -80]
  ldr x0, = .tex_63
  bl _escrever_tex
  ldr x0, [x29, -80]
  bl _escrever_tex
  ldr x0, = .tex_64
  bl _escrever_tex
  ldr x0, [x29, -80]
  str x0, [sp, -16]!  // salva param 0 (ponteiro/longo)
  ldr x0, [sp, 0]  // carrega param 0 (ptr/longo) em x0
  add sp, sp, 16  // limpa temporarios
  bl textam
  bl _escrever_int
  ldr x0, [x29, -80]
  str x0, [sp, -16]!  // salva param 0 (ponteiro/longo)
  mov w0, 116
  str w0, [sp, -16]!  // salva param 1 (int/bool/char/byte)
  ldr w1, [sp, 0]  // carrega param 1 (int/bool) em w1
  mov x1, x1  // estende pra 64 bits
  ldr x0, [sp, 16]  // carrega param 0 (ptr/longo) em x0
  add sp, sp, 32  // limpa temporarios
  bl texcar
  str w0, [x29, -96]
  ldr w0, [x29, -96]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, ge
  cmp w0, 0
  beq .B55
  ldr x0, = .tex_65
  bl _escrever_tex
  ldr w0, [x29, -96]
  bl _escrever_int
  b .B56
.B55:
  ldr x0, = .tex_66
  bl _escrever_tex
.B56:
  ldr x0, = .tex_67
  bl _escrever_tex
  ldr x0, = .tex_68
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
  ldr x0, = .tex_69
  bl _escrever_tex
  add x0, x29, -128
  bl _escrever_tex
  add x0, x29, -128
  str x0, [sp, -16]!  // salva param 0 (ponteiro/longo)
  ldr x0, [x29, -112]
  str x0, [sp, -16]!  // salva param 1 (ponteiro/longo)
  ldr x0, [x29, -112]
  str x0, [sp, -16]!  // salva param 0 (ponteiro/longo)
  ldr x0, [sp, 0]  // carrega param 0 (ptr/longo) em x0
  add sp, sp, 16  // limpa temporarios
  bl textam
  str w0, [sp, -16]!  // salva param 2 (int/bool/char/byte)
  ldr w2, [sp, 0]  // carrega param 2 (int/bool) em w2
  mov x2, x2  // estende pra 64 bits
  ldr x1, [sp, 16]  // carrega param 1 (ptr/longo) em x1
  ldr x0, [sp, 32]  // carrega param 0 (ptr/longo) em x0
  add sp, sp, 48  // limpa temporarios
  bl memcp
  ldr x0, = .tex_70
  bl _escrever_tex
  add x0, x29, -128
  bl _escrever_tex
  add x0, x29, -128
  str x0, [sp, -16]!  // salva param 0 (ponteiro/longo)
  mov w0, 88
  str w0, [sp, -16]!  // salva param 1 (int/bool/char/byte)
  mov w0, 101
  str w0, [sp, -16]!  // salva param 2 (int/bool/char/byte)
  ldr w2, [sp, 0]  // carrega param 2 (int/bool) em w2
  mov x2, x2  // estende pra 64 bits
  ldr w1, [sp, 16]  // carrega param 1 (int/bool) em w1
  mov x1, x1  // estende pra 64 bits
  ldr x0, [sp, 32]  // carrega param 0 (ptr/longo) em x0
  add sp, sp, 48  // limpa temporarios
  bl subscar
  ldr x0, = .tex_71
  bl _escrever_tex
  add x0, x29, -128
  bl _escrever_tex
  ldr x0, = .tex_72
  bl _escrever_tex
  mov w0, 0
  mov w1, w0 // otimizado(literal)
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  add x2, x29, -128
  add x2, x2, x0
  ldrb w0, [x2]
  strb w0, [x29, -144]
  ldr x0, = .tex_73
  bl _escrever_tex
  ldrb w0, [x29, -144]
  bl _escrever_car
  mov w0, 10
  bl _escrever_car
  mov w0, 0
  str w0, [x29, -160]
  mov w0, 1
  str w0, [x29, -156]
  mov w0, 2
  str w0, [x29, -152]
  mov w0, 5
  str w0, [x29, -148]
  ldr x0, = .tex_74
  bl _escrever_tex
  mov w0, 0
  str w0, [x29, 304]
.B58:
  ldr w0, [x29, 304]
  str w0, [sp, -16]!
  mov w0, 4
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B59
  ldr x0, = .tex_75
  bl _escrever_tex
  ldr w0, [x29, 304]
  bl _escrever_int
  ldr x0, = .tex_76
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
  mov w0, 10
  bl _escrever_car
  ldr w0, [x29, 304]
  add w0, w0, 1
  str w0, [x29, 304]
  b .B58
.B59:
  ldr x0, = const_8
  ldr s0, [x0]
  str s0, [x29, -176]
  ldr x0, = const_9
  ldr s0, [x0]
  str s0, [x29, -172]
  ldr x0, = const_10
  ldr s0, [x0]
  str s0, [x29, -168]
  ldr x0, = const_10
  ldr s0, [x0]
  str s0, [x29, -164]
  ldr x0, = .tex_77
  bl _escrever_tex
  mov w0, 0
  str w0, [x29, 288]
.B61:
  ldr w0, [x29, 288]
  str w0, [sp, -16]!
  mov w0, 4
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B62
  ldr x0, = .tex_75
  bl _escrever_tex
  ldr w0, [x29, 288]
  bl _escrever_int
  ldr x0, = .tex_76
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
  mov w0, 10
  bl _escrever_car
  ldr w0, [x29, 288]
  add w0, w0, 1
  str w0, [x29, 288]
  b .B61
.B62:
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 304]
  add sp, sp, 320
  ret
// fim: [testeMemoria]
// fn: [testeLoops] (vars: 48, total: 176)
.align 2
testeLoops:
  sub sp, sp, 176
  stp x29, x30, [sp, 160]
  add x29, sp, 160
  ldr x0, = .tex_comb_7
  bl _escrever_tex
  mov w0, 0
  str w0, [x29, -32]
.B64:
  ldr w0, [x29, -32]
  str w0, [sp, -16]!
  mov w0, 10
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B65
  ldr x0, = .tex_80
  bl _escrever_tex
  ldr w0, [x29, -32]
  bl _escrever_int
  ldr w0, [x29, -32]
  add w0, w0, 1
  str w0, [x29, -32]
  b .B64
.B65:
  mov w0, 0
  str w0, [x29, -48]
  ldr x0, = .tex_81
  bl _escrever_tex
.B67:
  ldr w0, [x29, -48]
  str w0, [sp, -16]!
  mov w0, 10
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B68
  ldr w0, [x29, -48]
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr w0, [x29, -48]
  str w0, [sp, -16]!
  mov w0, 5
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .B69
  ldr x0, = .tex_82
  bl _escrever_tex
  b .B68
  b .B70
.B69:
.B70:
  ldr w0, [x29, -48]
  add w0, w0, 1
  str w0, [x29, -48]
  b .B67
.B68:
  ldr x0, = .tex_83
  bl _escrever_tex
  mov w0, 0
  str w0, [x29, 160]
.B72:
  ldr w0, [x29, 160]
  str w0, [sp, -16]!
  mov w0, 10
  ldr w1, [sp], 16
  cmp w1, w0
  cset w0, lt
  cmp w0, 0
  beq .B73
  ldr x0, = .tex_84
  bl _escrever_tex
  ldr w0, [x29, 160]
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr w0, [x29, 160]
  add w0, w0, 1
  str w0, [x29, 160]
  b .B72
.B73:
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 160]
  add sp, sp, 176
  ret
// fim: [testeLoops]
// fn: [testeMatrizes] (vars: 32, total: 160)
.align 2
testeMatrizes:
  sub sp, sp, 160
  stp x29, x30, [sp, 144]
  add x29, sp, 144
  ldr x0, = .tex_85
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
  ldr x0, = .tex_86
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
  mov w0, 10
  bl _escrever_car
  ldr x0, = const_11
  ldr s0, [x0]
  str s0, [x29, -48]
  ldr x0, = const_12
  ldr s0, [x0]
  str s0, [x29, -44]
  ldr x0, = const_13
  ldr s0, [x0]
  str s0, [x29, -40]
  ldr x0, = const_14
  ldr s0, [x0]
  str s0, [x29, -44]
  ldr x0, = const_15
  ldr s0, [x0]
  str s0, [x29, -40]
  ldr x0, = const_1
  ldr s0, [x0]
  str s0, [x29, -36]
  ldr x0, = .tex_87
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
  mov w0, 10
  bl _escrever_car
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 144]
  add sp, sp, 160
  ret
// fim: [testeMatrizes]
// fn: [testeEspaco] (vars: 0, total: 128)
.align 2
testeEspaco:
  sub sp, sp, 128
  stp x29, x30, [sp, 112]
  add x29, sp, 112
  ldr x0, = .tex_88
  bl _escrever_tex
  mov w0, 65
  add x1, x29, -32
  strb w0, [x1]
  mov w0, 1991
  add x1, x29, -24
  str w0, [x1]
  mov w0, 66
  add x1, x29, -48
  strb w0, [x1]
  mov w0, 1992
  add x1, x29, -40
  str w0, [x1]
  ldr x0, = .tex_89
  bl _escrever_tex
  add x0, x29, -32
  ldrb w0, [x0]
  bl _escrever_car
  ldr x0, = .tex_90
  bl _escrever_tex
  add x0, x29, -24
  ldr w0, [x0]
  bl _escrever_int
  ldr x0, = .tex_91
  bl _escrever_tex
  add x0, x29, -48
  ldrb w0, [x0]
  bl _escrever_car
  ldr x0, = .tex_90
  bl _escrever_tex
  add x0, x29, -40
  ldr w0, [x0]
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 112]
  add sp, sp, 128
  ret
// fim: [testeEspaco]
// fn: [testeConversao] (vars: 0, total: 128)
.align 2
testeConversao:
  sub sp, sp, 128
  stp x29, x30, [sp, 112]
  add x29, sp, 112
  ldr x0, = .tex_comb_8
  bl _escrever_tex
  ldr x0, = .tex_94
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
  ldr x0, = .tex_95
  bl _escrever_tex
  ldr x0, = .tex_96
  str w0, [sp, -16]!  // salva param 0 (int/bool/char/byte)
  mov w0, 4
  str w0, [sp, -16]!  // salva param 1 (int/bool/char/byte)
  ldr w1, [sp, 0]  // carrega param 1 (int/bool) em w1
  mov x1, x1  // estende pra 64 bits
  ldr w0, [sp, 16]  // carrega param 0 (int/bool) em w0
  mov x0, x0  // estende pra 64 bits
  add sp, sp, 32  // limpa temporarios
  bl cvtflu
  fmov s0, s0
  str s0, [sp, -16]!
  ldr x0, = const_7
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
  ret
// fim: [testeConversao]
  .section .rodata
  .align 8
const_0:
  .float 0.000000
const_1:
  .float 10.000000
const_2:
  .float 0.100000
const_3:
  .float 3.140000
const_4:
  .quad 1763400788119
const_5:
  .float 1.100000
const_6:
  .float -1.100000
const_7:
  .float 0.500000
const_8:
  .float 0.200000
const_9:
  .float 1.500000
const_10:
  .float 5.100000
const_11:
  .float 3.000000
const_12:
  .float 5.000000
const_13:
  .float 7.000000
const_14:
  .float 2.000000
const_15:
  .float 9.000000
.section .rodata
.align 2
.tex_0: .asciz "valor a: "
.tex_1: .asciz "valor b: "
.tex_2: .asciz "Testando tipos b\303\241sicos:\n"
.tex_3: .asciz "\ncaractere: "
.tex_4: .asciz "\ninteiro: "
.tex_5: .asciz "\nflutuante: "
.tex_6: .asciz "\nbooleano: "
.tex_7: .asciz "\nlongo: "
.tex_8: .asciz "\nbyte: "
.tex_9: .asciz " (0xFF)"
.tex_11: .asciz "\nsoma com retorno 5 + 7 = esperando 12, veio: "
.tex_12: .asciz "\n\nVariavel final inteira:\n"
.tex_13: .asciz "\n"
.tex_14: .asciz "\nsoma comum 5 + 7 = esperando 12, veio: "
.tex_15: .asciz "\n\nTestando atribui\303\247\303\265es:\n"
.tex_16: .asciz "\nnovo inteiro: "
.tex_17: .asciz "\nnovo caractere: "
.tex_18: .asciz "\nnovo booleano: "
.tex_19: .asciz "\nnovo longo: "
.tex_20: .asciz "\nTeste de positivo e negativo:\n\n"
.tex_21: .asciz "inteiro positivo: "
.tex_22: .asciz "inteiro negativo: "
.tex_23: .asciz "flutuante positivo: "
.tex_24: .asciz "flutuante negativo: "
.tex_29: .asciz "opera\303\247\303\243o (5 + 5) * 5, esperado: 50, veio: "
.tex_30: .asciz "10 % 3 = ?, esperado: 1, recebido: "
.tex_31: .asciz "\n\nTeste de opera\303\247\303\265es entre tipos:\n\n"
.tex_32: .asciz "x: "
.tex_33: .asciz " * y: "
.tex_34: .asciz ", resultado: "
.tex_35: .asciz "10 << 2, resultado: "
.tex_40: .asciz "\nTeste compara\303\247\303\265es:\n\n"
.tex_41: .asciz "x \303\251 maior que 5\n"
.tex_42: .asciz "x \303\251 maior ou igual a 5\n"
.tex_43: .asciz "x n\303\243o \303\251 maior nem igual a 5\n"
.tex_44: .asciz "y >= 4 && x > 4 \303\251 verdadeiro\n"
.tex_45: .asciz "y >= 4 && x > 4 \303\251 falso\n"
.tex_46: .asciz "y == x || x > 3 \303\251 verdadeiro\n"
.tex_47: .asciz "y == x || x > 3 \303\251 falso\n"
.tex_48: .asciz "\nCompara\303\247\303\243o com textos:\n\n"
.tex_49: .asciz "texto 1"
.tex_50: .asciz "texto 2"
.tex_51: .asciz "texto 1 \303\251 igual a texto 2\n"
.tex_52: .asciz "texto 1 n\303\243o \303\251 igual a texto 2\n"
.tex_53: .asciz "o primeiro texto \303\251 texto 1"
.tex_54: .asciz "o primeiro texto n\303\243o \303\251 texto 1"
.tex_55: .asciz "\nTeste de array:\n"
.tex_56: .asciz "\nvalor do array: "
.tex_57: .asciz "\narray mudado no indice 0: "
.tex_58: .asciz "\n\nTeste de ponteiro:\n\n"
.tex_59: .asciz "int x1 = 10; int* p1 = @x1;\n"
.tex_60: .asciz "p1 = p1 + 5; x1 = ?\n"
.tex_61: .asciz "x1 = "
.tex_62: .asciz "exemplo de ponteiro"
.tex_63: .asciz "\nponteiro texto, valor: "
.tex_64: .asciz "\ntamamho em bytes: "
.tex_65: .asciz "\no ponteiro tem t no indice: "
.tex_66: .asciz "\no ponteiro n\303\243o tem t\n"
.tex_67: .asciz "\nTeste de manipula\303\247\303\243o da memoria:\n"
.tex_68: .asciz "XxXmplo maior"
.tex_69: .asciz "\nArray padr\303\243o: "
.tex_70: .asciz "\nArray copiado da memoria: "
.tex_71: .asciz "\nArray usando subscar(array2, 'X', 'e'): "
.tex_72: .asciz "\nTeste de acesso a itens array:\n"
.tex_73: .asciz "item do indice 0 do array: "
.tex_74: .asciz "\nArray de inteiros: \n\n"
.tex_75: .asciz "no indice: "
.tex_76: .asciz " valor: "
.tex_77: .asciz "\nArray de flutuantes: \n\n"
.tex_80: .asciz "\nvalor de i: "
.tex_81: .asciz "\n\nTeste de parada do loop (deve parar em 5)\n"
.tex_82: .asciz "parando\n"
.tex_83: .asciz "\nLoop Por:\n"
.tex_84: .asciz "indice: "
.tex_85: .asciz "\n\nTeste de matrizes:\n\n"
.tex_86: .asciz "matriz 2D int m2[0][1]: "
.tex_87: .asciz "matriz 2D flu m2f[0][1]: "
.tex_88: .asciz "\nTeste de estrutura de dados (#espaco):\n\n"
.tex_89: .asciz "Pessoa 1:\nNome: "
.tex_90: .asciz "\nIdade: "
.tex_91: .asciz "\nPessoa 2:\nNome: "
.tex_94: .asciz "123"
.tex_95: .asciz "1.50 + 0.50 = "
.tex_96: .asciz "1.50"
.section .text


.section .data
.align 3
global_varGlobal:
  .word 10

.section .rodata
.align 2
.tex_comb_0: .asciz " (0xFF)\n\nTestando fun\303\247\303\243o soma:\n"
.tex_comb_1: .asciz "\nTeste de convers\303\243o:\n\n(car)65 = "
.tex_comb_2: .asciz "\n\nTeste de opera\303\247\303\265es matematicas:\n\nopera\303\247\303\243o 5 + 5 * 5, esperado: 30, veio: "
.tex_comb_3: .asciz "\n10 >> 2, resultado: "
.tex_comb_4: .asciz "\n124 & 15, resultado: "
.tex_comb_5: .asciz "\n124 & 0xF, resultado: "
.tex_comb_6: .asciz "\n1 | 2, resultado: "
.tex_comb_7: .asciz "\n\nTeste de loops:\nEnquanto:"
.tex_comb_8: .asciz "\nTeste de convers\303\243o de texto:\n123 + 1 = "
