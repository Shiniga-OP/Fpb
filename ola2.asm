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


// inicio de biblis/texs.fpb

// inicio de biblis/texs.asm
// fn: [textam]
// x0: texto, w0: retorno
.align 2
textam:
    mov x1, x0
1:
    ldrb w2, [x1], 1
    cbnz w2, 1b
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

// fn: [texcp] (vars: 16, total: 160)
.align 2
texcp:
  sub sp, sp, 160
  stp x29, x30, [sp, 144]
  add x29, sp, 144
  str x0, [x29, -16]  // param arr
  str x1, [x29, -24]  // param p
  mov w0, 0
  mov w19, w0
  str w0, [x29, -48]
.B1:
  mov w0, 1
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B2
  ldr w0, [x29, -48]
  mov w1, w0
  str w1, [sp, -16]!
  ldr w0, [x29, -48]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -24]
  add x2, x2, x0
  ldrb w0, [x2]
  ldr w1, [sp], 16
  ldr x2, [x29, -16]
  add x2, x2, x1
  strb w0, [x2]
  ldr w0, [x29, -48]
  str w0, [sp, -16]!
  mov w0, 0
  ldr w1, [sp], 16
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -24]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  mov w0, 0
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B3
  b .B2
  b .B4
.B3:
.B4:
  ldr w0, [x29, -48]
  add w0, w0, 1
  str w0, [x29, -48]
  b .B1
.B2:
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 144]
  add sp, sp, 160
  ret
// fim: [texcp]

// fim de biblis/texs.fpb


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
// fn: [cvtint] (vars: 128, total: 304)
.align 2
cvtint:
  sub sp, sp, 304
  stp x29, x30, [sp, 288]
  add x29, sp, 288
  stp x19, x20, [x29, -16]
  stp x21, x22, [x29, -32]
  str x0, [x29, -48]  // param v
  str x1, [x29, -56]  // param tam
  ldr w0, [x29, -56]
  mov w19, w0
  str w0, [x29, -80]
  ldr w0, [x29, -80]
  mov w19, w0
  mov w0, 0
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B5
  mov w0, 0
  b 1f
  b .B6
.B5:
.B6:
  mov w0, 0
  mov w19, w0
  str w0, [x29, -96]
  mov w0, 0
  mov w19, w0
  strb w0, [x29, -112]
  mov w0, 0
  mov w1, w0 // otimizado(literal)
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  strb w0, [x29, -128]
  ldrb w0, [x29, -128]
  mov w19, w0
  mov w0, 45
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B7
  mov w0, 1
  mov w19, w0
  mov w0, w19
  strb w0, [x29, -112]
  mov w0, 1
  mov w19, w0
  mov w0, w19
  str w0, [x29, -96]
  b .B8
.B7:
  ldrb w0, [x29, -128]
  mov w19, w0
  mov w0, 43
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B9
  mov w0, 1
  mov w19, w0
  mov w0, w19
  str w0, [x29, -96]
  b .B10
.B9:
.B10:
.B8:
  ldr w0, [x29, -80]
  str w0, [sp, -16]!
  ldr w0, [x29, -96]
  ldr w1, [sp], 16
  sub w0, w1, w0
  mov w19, w0
  str w0, [x29, -144]
  ldr w0, [x29, -144]
  mov w19, w0
  mov w0, 0
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B11
  mov w0, 0
  b 1f
  b .B12
.B11:
.B12:
  ldr w0, [x29, -144]
  mov w19, w0
  mov w0, 4
  mov w1, w19
  cmp w1, w0
  cset w0, le
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B13
  ldr w0, [x29, -144]
  mov w19, w0
  mov w0, 1
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B14
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
  mov w19, w0
  mov w0, w19
  str w0, [x29, 288]
  b .B15
.B14:
  ldr w0, [x29, -144]
  mov w19, w0
  mov w0, 2
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B16
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
  mov w19, w0  // salva em reg
  mov w0, 10
  mov w1, w19  // restaura do reg
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
  mov w19, w0
  mov w0, w19
  str w0, [x29, 288]
  b .B17
.B16:
  ldr w0, [x29, -144]
  mov w19, w0
  mov w0, 3
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
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
  mov w19, w0  // salva em reg
  mov w0, 100
  mov w1, w19  // restaura do reg
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
  mov w19, w0  // salva em reg
  mov w0, 10
  mov w1, w19  // restaura do reg
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
  mov w19, w0
  mov w0, w19
  str w0, [x29, 288]
  b .B19
.B18:
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
  mov w19, w0  // salva em reg
  mov w0, 1000
  mov w1, w19  // restaura do reg
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
  mov w19, w0  // salva em reg
  mov w0, 100
  mov w1, w19  // restaura do reg
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
  mov w19, w0  // salva em reg
  mov w0, 10
  mov w1, w19  // restaura do reg
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
  mov w19, w0
  mov w0, w19
  str w0, [x29, 288]
.B19:
.B17:
.B15:
  mov w0, 0
  mov w19, w0
  str w0, [x29, 272]
  ldrb w0, [x29, -112]
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B20
  ldr w0, [x29, 288]
  neg w0, w0
  mov w19, w0
  mov w0, w19
  str w0, [x29, 272]
  b .B21
.B20:
  ldr w0, [x29, 288]
  mov w19, w0
  mov w0, w19
  str w0, [x29, 272]
.B21:
  ldr w0, [x29, 272]
  b 1f
  b .B22
.B13:
.B22:
  mov w0, 0
  mov w19, w0
  str w0, [x29, -160]
.B24:
  ldr w0, [x29, -96]
  mov w19, w0
  ldr w0, [x29, -80]
  mov w1, w19
  cmp w1, w0
  cset w0, lt
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B25
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
  mov w19, w0
  mov w0, 48
  mov w1, w19
  cmp w1, w0
  cset w0, ge
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
  mov w19, w0
  mov w0, 57
  mov w1, w19
  cmp w1, w0
  cset w0, le
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B26
  ldr w0, [x29, -160]
  mov w19, w0  // salva em reg
  mov w0, 10
  mov w1, w19  // restaura do reg
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
  mov w19, w0
  mov w0, w19
  str w0, [x29, -160]
  b .B27
.B26:
.B27:
  ldr w0, [x29, -96]
  add w0, w0, 1
  str w0, [x29, -96]
  b .B24
.B25:
  ldrb w0, [x29, -112]
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B28
  ldr w0, [x29, -160]
  neg w0, w0
  mov w19, w0
  mov w0, w19
  str w0, [x29, 272]
  b .B29
.B28:
  ldr w0, [x29, -160]
  mov w19, w0
  mov w0, w19
  str w0, [x29, 272]
.B29:
  ldr w0, [x29, 272]
  b 1f
  b 1f
// epilogo
1:
  ldp x19, x20, [x29, -16]
  ldp x21, x22, [x29, -32]
  ldp x29, x30, [sp, 288]
  add sp, sp, 304
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
  mov w19, w0
  mov w0, 0
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B30
  ldr x0, = const_0
  ldr s0, [x0]
  b 1f
  b .B31
.B30:
.B31:
  mov w0, 0
  mov w19, w0
  str w0, [x29, -80]
  mov w0, 0
  mov w19, w0
  strb w0, [x29, -96]
  mov w0, 0
  mov w1, w0 // otimizado(literal)
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  mov w0, 45
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B32
  mov w0, 1
  mov w19, w0
  mov w0, w19
  strb w0, [x29, -96]
  mov w0, 1
  mov w19, w0
  mov w0, w19
  str w0, [x29, -80]
  b .B33
.B32:
  mov w0, 0
  mov w1, w0 // otimizado(literal)
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  ldr x2, [x29, -48]
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  mov w0, 43
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B34
  mov w0, 1
  mov w19, w0
  mov w0, w19
  str w0, [x29, -80]
  b .B35
.B34:
.B35:
.B33:
  ldr x0, = const_0
  ldr s0, [x0]
  fmov s19, s0
  str s0, [x29, -112]
.B37:
  ldr w0, [x29, -80]
  mov w19, w0
  ldr w0, [x29, -56]
  mov w1, w19
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
  mov w19, w0
  mov w0, 46
  mov w1, w19
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
  mov w19, w0
  mov w0, 101
  mov w1, w19
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
  mov w19, w0
  mov w0, 69
  mov w1, w19
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
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B38
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
  mov w19, w0
  mov w0, 48
  mov w1, w19
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
  mov w19, w0
  mov w0, 57
  mov w1, w19
  cmp w1, w0
  cset w0, le
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B39
  ldr s0, [x29, -112]
  fmov s19, s0  // salva em reg
  ldr x0, = const_1
  ldr s0, [x0]
  fmov s1, s19  // restaura do reg
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
  fmov s19, s0
  fmov s0, s19
  str s0, [x29, -112]
  b .B40
.B39:
.B40:
  ldr w0, [x29, -80]
  add w0, w0, 1
  str w0, [x29, -80]
  b .B37
.B38:
  ldr w0, [x29, -80]
  mov w19, w0
  ldr w0, [x29, -56]
  mov w1, w19
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
  mov w19, w0
  mov w0, 46
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B41
  ldr w0, [x29, -80]
  add w0, w0, 1
  str w0, [x29, -80]
  ldr x0, = const_2
  ldr s0, [x0]
  fmov s19, s0
  str s0, [x29, 224]
.B43:
  ldr w0, [x29, -80]
  mov w19, w0
  ldr w0, [x29, -56]
  mov w1, w19
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
  mov w19, w0
  mov w0, 48
  mov w1, w19
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
  mov w19, w0
  mov w0, 57
  mov w1, w19
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
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B44
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
  fmov s19, s0  // salva em reg
  ldr s0, [x29, 224]
  fmov s1, s19  // restaura do reg
  fmul s0, s1, s0
  ldr s1, [sp], 16
  fadd s0, s1, s0
  fmov s19, s0
  fmov s0, s19
  str s0, [x29, -112]
  ldr s0, [x29, 224]
  fmov s19, s0  // salva em reg
  ldr x0, = const_2
  ldr s0, [x0]
  fmov s1, s19  // restaura do reg
  fmul s0, s1, s0
  fmov s19, s0
  fmov s0, s19
  str s0, [x29, 224]
  ldr w0, [x29, -80]
  add w0, w0, 1
  str w0, [x29, -80]
  b .B43
.B44:
  b .B45
.B41:
.B45:
  ldrb w0, [x29, -96]
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B46
  ldr s0, [x29, -112]
  fneg s0, s0
  fmov s19, s0
  fmov s0, s19
  str s0, [x29, -112]
  b .B47
.B46:
.B47:
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


// inicio de biblis/mat.fpb
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
  beq .B48
  ldr s0, [x29, -48]
  fneg s0, s0
  str s0, [sp, -16]!  // salva param 0 (float)
  ldr s0, [sp, 0]  // carrega param 0 (flu) em s0
  add sp, sp, 16  // limpa temporarios
  bl ftanh
  fmov s0, s0
  fneg s0, s0
  b 1f
  b .B49
.B48:
.B49:
  ldr s0, [x29, -48]
  fmov s19, s0
  ldr x0, = const_3
  ldr s0, [x0]
  fmov s1, s19
  fcmp s1, s0
  cset w0, gt
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B50
  ldr x0, = const_4
  ldr s0, [x0]
  b 1f
  b .B51
.B50:
.B51:
  ldr x0, = const_4
  ldr s0, [x0]
  fmov s19, s0
  str s0, [x29, -80]
  ldr x0, = const_4
  ldr s0, [x0]
  fmov s19, s0
  str s0, [x29, -96]
  ldr x0, = const_5
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
.B53:
  ldr w0, [x29, 224]
  mov w19, w0
  mov w0, 16
  mov w1, w19
  cmp w1, w0
  cset w0, lt
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B54
  ldr s0, [x29, -96]
  str s0, [sp, -16]!
  ldr s0, [x29, -112]
  fmov s19, s0  // salva em reg
  ldr w0, [x29, 224]
  scvtf s0, w0
  fmov s1, s19  // restaura do reg
  fdiv s0, s1, s0
  ldr s1, [sp], 16
  fmul s0, s1, s0
  str s0, [x29, -96]
  ldr s0, [x29, -80]
  str s0, [sp, -16]!
  ldr s0, [x29, -96]
  ldr s1, [sp], 16
  fadd s0, s1, s0
  str s0, [x29, -80]
  ldr w0, [x29, 224]
  add w0, w0, 1
  str w0, [x29, 224]
  b .B53
.B54:
  ldr s0, [x29, -80]
  str s0, [sp, -16]!
  ldr x0, = const_4
  ldr s0, [x0]
  ldr s1, [sp], 16
  fsub s0, s1, s0
  fmov s19, s0  // salva em reg
  ldr s0, [x29, -80]
  str s0, [sp, -16]!
  ldr x0, = const_4
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
// fn: [fexp] (vars: 48, total: 224)
.align 2
fexp:
  sub sp, sp, 224
  stp x29, x30, [sp, 208]
  add x29, sp, 208
  stp x19, x20, [x29, -16]
  stp x21, x22, [x29, -32]
  str d0, [x29, -48]  // param x
  ldr s0, [x29, -48]
  fmov s19, s0
  ldr x0, = const_0
  ldr s0, [x0]
  fmov s1, s19
  fcmp s1, s0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B55
  ldr x0, = const_4
  ldr s0, [x0]
  b 1f
  b .B56
.B55:
.B56:
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
  beq .B57
  ldr x0, = const_4
  ldr s0, [x0]
  fmov s19, s0  // salva em reg
  ldr s0, [x29, -48]
  fneg s0, s0
  str s0, [sp, -16]!  // salva param 0 (float)
  ldr s0, [sp, 0]  // carrega param 0 (flu) em s0
  add sp, sp, 16  // limpa temporarios
  bl fexp
  fmov s0, s0
  fmov s1, s19  // restaura do reg
  fdiv s0, s1, s0
  b 1f
  b .B58
.B57:
.B58:
  ldr x0, = const_4
  ldr s0, [x0]
  fmov s19, s0
  str s0, [x29, -80]
  ldr x0, = const_4
  ldr s0, [x0]
  fmov s19, s0
  str s0, [x29, -96]
  mov w0, 1
  mov w19, w0
  str w0, [x29, 208]
.B60:
  ldr w0, [x29, 208]
  mov w19, w0
  mov w0, 16
  mov w1, w19
  cmp w1, w0
  cset w0, lt
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B61
  ldr s0, [x29, -96]
  str s0, [sp, -16]!
  ldr s0, [x29, -48]
  fmov s19, s0  // salva em reg
  ldr w0, [x29, 208]
  scvtf s0, w0
  fmov s1, s19  // restaura do reg
  fdiv s0, s1, s0
  ldr s1, [sp], 16
  fmul s0, s1, s0
  str s0, [x29, -96]
  ldr s0, [x29, -80]
  str s0, [sp, -16]!
  ldr s0, [x29, -96]
  ldr s1, [sp], 16
  fadd s0, s1, s0
  fmov s19, s0
  fmov s0, s19
  str s0, [x29, -80]
  ldr w0, [x29, 208]
  add w0, w0, 1
  str w0, [x29, 208]
  b .B60
.B61:
  ldr s0, [x29, -80]
  b 1f
  b 1f
// epilogo
1:
  ldp x19, x20, [x29, -16]
  ldp x21, x22, [x29, -32]
  ldp x29, x30, [sp, 208]
  add sp, sp, 224
  ret
// fim: [fexp]
// fn: [fraiz] (vars: 32, total: 208)
.align 2
fraiz:
  sub sp, sp, 208
  stp x29, x30, [sp, 192]
  add x29, sp, 192
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
  beq .B62
  ldr x0, = const_0
  ldr s0, [x0]
  b 1f
  b .B63
.B62:
.B63:
  ldr s0, [x29, -48]
  fmov s19, s0
  ldr x0, = const_0
  ldr s0, [x0]
  fmov s1, s19
  fcmp s1, s0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B64
  ldr x0, = const_0
  ldr s0, [x0]
  b 1f
  b .B65
.B64:
.B65:
  ldr s0, [x29, -48]
  fmov s19, s0
  str s0, [x29, -80]
  mov w0, 0
  mov w19, w0
  str w0, [x29, 192]
.B67:
  ldr w0, [x29, 192]
  mov w19, w0
  mov w0, 10
  mov w1, w19
  cmp w1, w0
  cset w0, lt
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B68
  ldr x0, = const_6
  ldr s0, [x0]
  fmov s19, s0  // salva em reg
  ldr s0, [x29, -80]
  str s0, [sp, -16]!
  ldr s0, [x29, -48]
  str s0, [sp, -16]!
  ldr s0, [x29, -80]
  ldr s1, [sp], 16
  fdiv s0, s1, s0
  ldr s1, [sp], 16
  fadd s0, s1, s0
  fmov s1, s19  // restaura do reg
  fmul s0, s1, s0
  fmov s19, s0
  fmov s0, s19
  str s0, [x29, -80]
  ldr w0, [x29, 192]
  add w0, w0, 1
  str w0, [x29, 192]
  b .B67
.B68:
  ldr s0, [x29, -80]
  b 1f
  b 1f
// epilogo
1:
  ldp x19, x20, [x29, -16]
  ldp x21, x22, [x29, -32]
  ldp x29, x30, [sp, 192]
  add sp, sp, 208
  ret
// fim: [fraiz]

// fim de biblis/mat.fpb

.global inicio
// Espaço "Pessoa" definido: 48 bytes
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
  ldr x0, = .tex_comb_0
  bl _escrever_tex
  ldr x0, = .tex_4
  bl _escrever_tex
  mov w0, 65
  mov w19, w0
  strb w0, [x29, -32]
  ldr x0, = .tex_5
  bl _escrever_tex
  ldrb w0, [x29, -32]
  bl _escrever_car
  mov w0, 42
  mov w19, w0
  str w0, [x29, -48]
  ldr x0, = .tex_6
  bl _escrever_tex
  ldr w0, [x29, -48]
  bl _escrever_int
  ldr x0, = const_7
  ldr s0, [x0]
  fmov s19, s0
  str s0, [x29, -64]
  ldr x0, = .tex_7
  bl _escrever_tex
  ldr s0, [x29, -64]
  bl _escrever_flu
  mov w0, 1
  mov w19, w0
  strb w0, [x29, -80]
  ldr x0, = .tex_8
  bl _escrever_tex
  ldrb w0, [x29, -80]
  bl _escrever_bool
  add sp, sp, 0  // limpa temporarios
  bl obter_tempo_milis
  mov x19, x0
  str x0, [x29, -96]
  ldr x0, = .tex_9
  bl _escrever_tex
  ldr x0, [x29, -96]
  bl _escrever_longo
  mov w0, 255 // byte: 0xFF
  mov w19, w0
  strb w0, [x29, -112]
  ldr x0, = .tex_10
  bl _escrever_tex
  ldrb w0, [x29, -112]
  bl _escrever_int
  ldr x0, = .tex_comb_1
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
  mov w19, w0
  str w0, [x29, -128]
  ldr x0, = .tex_13
  bl _escrever_tex
  ldr w0, [x29, -128]
  bl _escrever_int
  mov w0, 1
  mov w19, w0
  str w0, [x29, -144]
  ldr x0, = .tex_14
  bl _escrever_tex
  ldr w0, [x29, -144]
  bl _escrever_int
  ldr x0, = .tex_15
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
  mov w19, w0
  mov w0, w19
  str w0, [x29, -16]
  ldr x0, = .tex_16
  bl _escrever_tex
  ldr w0, [x29, -16]
  bl _escrever_int
  ldr x0, = .tex_17
  bl _escrever_tex
  mov w0, 100
  mov w19, w0
  mov w0, w19
  str w0, [x29, -24]
  mov w0, 90
  mov w19, w0
  mov w0, w19
  strb w0, [x29, -32]
  mov w0, 0
  mov w19, w0
  mov w0, w19
  strb w0, [x29, -40]
  movz x0, 43159, lsl 0
  movk x0, 37600, lsl 16
  movk x0, 410, lsl 32
  mov x19, x0
  mov x0, x19
  str x0, [x29, -48]
  ldr x0, = .tex_18
  bl _escrever_tex
  ldr w0, [x29, -24]
  bl _escrever_int
  ldr x0, = .tex_19
  bl _escrever_tex
  ldrb w0, [x29, -32]
  bl _escrever_car
  ldr x0, = .tex_20
  bl _escrever_tex
  ldrb w0, [x29, -40]
  bl _escrever_bool
  ldr x0, = .tex_21
  bl _escrever_tex
  ldr x0, [x29, -48]
  bl _escrever_longo
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_22
  bl _escrever_tex
  mov w0, 1
  mov w19, w0
  str w0, [x29, -80]
  ldr x0, = .tex_23
  bl _escrever_tex
  ldr w0, [x29, -80]
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  movz w0, 65535
  movk w0, 65535, lsl 16
  mov w19, w0
  mov w0, w19
  str w0, [x29, -80]
  ldr x0, = .tex_24
  bl _escrever_tex
  ldr w0, [x29, -80]
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr x0, = const_8
  ldr s0, [x0]
  fmov s19, s0
  str s0, [x29, -96]
  ldr x0, = .tex_25
  bl _escrever_tex
  ldr s0, [x29, -96]
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  ldr x0, = const_9
  ldr s0, [x0]
  fmov s19, s0
  fmov s0, s19
  str s0, [x29, -96]
  ldr x0, = .tex_26
  bl _escrever_tex
  ldr s0, [x29, -96]
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_comb_2
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
  add sp, sp, 0  // limpa temporarios
  bl testeAsm
  add sp, sp, 0  // limpa temporarios
  bl testeMat
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 192]
  add sp, sp, 208
  ret
// fim: [testeAlteracoes]
// fn: [testeOperacoes] (vars: 48, total: 176)
.align 2
testeOperacoes:
  sub sp, sp, 176
  stp x29, x30, [sp, 160]
  add x29, sp, 160
  ldr x0, = .tex_comb_3
  bl _escrever_tex
  mov w0, 5
  str w0, [sp, -16]!
  mov w0, 5
  mov w19, w0  // salva em reg
  mov w0, 5
  mov w1, w19  // restaura do reg
  mul w0, w1, w0
  ldr w1, [sp], 16
  add w0, w1, w0
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_31
  bl _escrever_tex
  mov w0, 5
  mov w1, w0 // otimizado(literal)
  add w0, w1, w0
  mov w19, w0  // salva em reg
  mov w0, 5
  mov w1, w19  // restaura do reg
  mul w0, w1, w0
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  mov w0, 5
  mov w1, w0 // otimizado(literal)
  add w0, w1, w0
  bl _escrever_int
  ldr x0, = .tex_15
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
  ldr x0, = .tex_32
  bl _escrever_tex
  mov w0, 10
  mov w19, w0  // salva em reg
  mov w0, 3
  mov w1, w19  // restaura do reg
  sdiv w2, w1, w0
  msub w0, w2, w0, w1
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_33
  bl _escrever_tex
  mov w0, 3
  mov w19, w0
  str w0, [x29, -32]
  ldr x0, = const_6
  ldr s0, [x0]
  fmov s19, s0
  str s0, [x29, -48]
  ldr x0, = .tex_34
  bl _escrever_tex
  ldr s0, [x29, -48]
  bl _escrever_flu
  ldr x0, = .tex_35
  bl _escrever_tex
  ldr w0, [x29, -32]
  bl _escrever_int
  ldr x0, = .tex_36
  bl _escrever_tex
  ldr s0, [x29, -48]
  fmov s19, s0  // salva em reg
  ldr w0, [x29, -32]
  scvtf s0, w0
  fmov s1, s19  // restaura do reg
  fmul s0, s1, s0
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_37
  bl _escrever_tex
  mov w0, 10
  mov w19, w0  // salva em reg
  mov w0, 2
  mov w1, w19  // restaura do reg
  lsl w0, w1, w0
  bl _escrever_int
  ldr x0, = .tex_comb_4
  bl _escrever_tex
  mov w0, 10
  mov w19, w0  // salva em reg
  mov w0, 2
  mov w1, w19  // restaura do reg
  lsr w0, w1, w0
  bl _escrever_int
  ldr x0, = .tex_comb_5
  bl _escrever_tex
  mov w0, 124
  mov w19, w0  // salva em reg
  mov w0, 15
  mov w1, w19  // restaura do reg
  and w0, w1, w0
  bl _escrever_int
  ldr x0, = .tex_comb_6
  bl _escrever_tex
  mov w0, 124
  mov w19, w0  // salva em reg
  mov w0, 15 // byte: 0xF
  mov w1, w19  // restaura do reg
  and w0, w1, w0
  bl _escrever_int
  ldr x0, = .tex_comb_7
  bl _escrever_tex
  mov w0, 1
  str w0, [sp, -16]!
  mov w0, 2
  ldr w1, [sp], 16
  orr w0, w1, w0
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_42
  bl _escrever_tex
  ldr s0, [x29, -48]
  str s0, [sp, -16]!
  ldr x0, = const_10
  ldr s0, [x0]
  ldr s1, [sp], 16
  fadd s0, s1, s0
  str s0, [x29, -48]
  ldr x0, = .tex_43
  bl _escrever_tex
  ldr s0, [x29, -48]
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  ldr s0, [x29, -48]
  str s0, [sp, -16]!
  mov w0, 2
  ldr s1, [sp], 16
  scvtf s0, w0
  fmul s0, s1, s0
  str s0, [x29, -48]
  ldr x0, = .tex_44
  bl _escrever_tex
  ldr s0, [x29, -48]
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  ldr s0, [x29, -48]
  str s0, [sp, -16]!
  mov w0, 2
  ldr s1, [sp], 16
  scvtf s0, w0
  fsub s0, s1, s0
  str s0, [x29, -48]
  ldr x0, = .tex_45
  bl _escrever_tex
  ldr s0, [x29, -48]
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  ldr s0, [x29, -48]
  str s0, [sp, -16]!
  mov w0, 2
  ldr s1, [sp], 16
  scvtf s0, w0
  fdiv s0, s1, s0
  str s0, [x29, -48]
  ldr x0, = .tex_46
  bl _escrever_tex
  ldr s0, [x29, -48]
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  mov w0, 10
  mov w19, w0
  str w0, [x29, -64]
  ldr w0, [x29, -64]
  str w0, [sp, -16]!
  mov w0, 5
  ldr w1, [sp], 16
  sdiv w2, w1, w0
  msub w0, w2, w0, w1
  str w0, [x29, -64]
  ldr x0, = .tex_47
  bl _escrever_tex
  ldr w0, [x29, -64]
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 160]
  add sp, sp, 176
  ret
// fim: [testeOperacoes]
// fn: [testeComparacoes] (vars: 80, total: 208)
.align 2
testeComparacoes:
  sub sp, sp, 208
  stp x29, x30, [sp, 192]
  add x29, sp, 192
  ldr x0, = .tex_48
  bl _escrever_tex
  mov w0, 4
  mov w19, w0
  str w0, [x29, -32]
  ldr w0, [x29, -32]
  mov w19, w0
  mov w0, 5
  mov w1, w19
  cmp w1, w0
  cset w0, gt
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B69
  ldr x0, = .tex_49
  bl _escrever_tex
  b .B70
.B69:
  ldr w0, [x29, -32]
  mov w19, w0
  mov w0, 5
  mov w1, w19
  cmp w1, w0
  cset w0, ge
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B71
  ldr x0, = .tex_50
  bl _escrever_tex
  b .B72
.B71:
  ldr x0, = .tex_51
  bl _escrever_tex
.B72:
.B70:
  mov w0, 5
  mov w19, w0
  str w0, [x29, -48]
  ldr w0, [x29, -48]
  mov w19, w0
  mov w0, 4
  mov w1, w19
  cmp w1, w0
  cset w0, ge
  str w0, [sp, -16]!
  ldr w0, [x29, -32]
  mov w19, w0
  mov w0, 4
  mov w1, w19
  cmp w1, w0
  cset w0, gt
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  and w0, w1, w0
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B73
  ldr x0, = .tex_52
  bl _escrever_tex
  b .B74
.B73:
  ldr x0, = .tex_53
  bl _escrever_tex
.B74:
  ldr w0, [x29, -48]
  mov w19, w0
  ldr w0, [x29, -32]
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  str w0, [sp, -16]!
  ldr w0, [x29, -32]
  mov w19, w0
  mov w0, 3
  mov w1, w19
  cmp w1, w0
  cset w0, gt
  ldr w1, [sp], 16
  cmp w1, 0
  cset w1, ne
  cmp w0, 0
  cset w0, ne
  orr w0, w1, w0
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B75
  ldr x0, = .tex_54
  bl _escrever_tex
  b .B76
.B75:
  ldr x0, = .tex_55
  bl _escrever_tex
.B76:
  ldr x0, = .tex_56
  bl _escrever_tex
  ldr x0, = .tex_57
  str x0, [x29, -64]
  ldr x0, = .tex_58
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
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B77
  ldr x0, = .tex_59
  bl _escrever_tex
  b .B78
.B77:
.B78:
  ldr x0, [x29, -64]
  str x0, [sp, -16]!  // salva param 0 (ponteiro/longo)
  ldr x0, [x29, -80]
  str x0, [sp, -16]!  // salva param 1 (ponteiro/longo)
  ldr x1, [sp, 0]  // carrega param 1 (ptr/longo) em x1
  ldr x0, [sp, 16]  // carrega param 0 (ptr/longo) em x0
  add sp, sp, 32  // limpa temporarios
  bl texcmp
  cmp w0, 0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B79
  ldr x0, = .tex_60
  bl _escrever_tex
  b .B80
.B79:
.B80:
  ldr x0, [x29, -64]
  str x0, [sp, -16]!  // salva param 0 (ponteiro/longo)
  ldr x0, [x29, -64]
  str x0, [sp, -16]!  // salva param 1 (ponteiro/longo)
  ldr x1, [sp, 0]  // carrega param 1 (ptr/longo) em x1
  ldr x0, [sp, 16]  // carrega param 0 (ptr/longo) em x0
  add sp, sp, 32  // limpa temporarios
  bl texcmp
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B81
  ldr x0, = .tex_61
  bl _escrever_tex
  b .B82
.B81:
  ldr x0, = .tex_62
  bl _escrever_tex
.B82:
  ldr x0, = .tex_63
  bl _escrever_tex
  mov w0, 5
  mov w19, w0
  mov w0, 6
  mov w1, w19
  cmp w1, w0
  cset w0, gt
  cmp w0, 0
  beq .ternario_falso_0
  mov w0, 10
  b .ternario_fim_0
.ternario_falso_0:
  mov w0, 20
.ternario_fim_0:
  mov w19, w0
  str w0, [x29, -96]
  ldr x0, = .tex_64
  bl _escrever_tex
  ldr w0, [x29, -96]
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 192]
  add sp, sp, 208
  ret
// fim: [testeComparacoes]
// fn: [testeMemoria] (vars: 192, total: 320)
.align 2
testeMemoria:
  sub sp, sp, 320
  stp x29, x30, [sp, 304]
  add x29, sp, 304
  ldr x0, = .tex_65
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
  ldr x0, = .tex_66
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
  ldr x0, = .tex_67
  bl _escrever_tex
  add x0, x29, -32
  bl _escrever_tex
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_68
  bl _escrever_tex
  mov w0, 10
  mov w19, w0
  str w0, [x29, -48]
  ldr x0, = .tex_69
  bl _escrever_tex
  add x0, x29, -48
  mov x19, x0
  str x0, [x29, -64]
  ldr x0, = .tex_70
  bl _escrever_tex
  ldr x1, [x29, -64]
  ldr w0, [x1]
  str w0, [sp, -16]!
  mov w0, 5
  ldr w1, [sp], 16
  add w0, w1, w0
  mov w19, w0
  str x0, [x29, -64]
  ldr x0, = .tex_71
  bl _escrever_tex
  ldr w0, [x29, -48]
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_72
  str x0, [x29, -80]
  ldr x0, = .tex_73
  bl _escrever_tex
  ldr x0, [x29, -80]
  bl _escrever_tex
  ldr x0, = .tex_74
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
  mov w19, w0
  str w0, [x29, -96]
  ldr w0, [x29, -96]
  mov w19, w0
  mov w0, 0
  mov w1, w19
  cmp w1, w0
  cset w0, ge
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B83
  ldr x0, = .tex_75
  bl _escrever_tex
  ldr w0, [x29, -96]
  bl _escrever_int
  b .B84
.B83:
  ldr x0, = .tex_76
  bl _escrever_tex
.B84:
  ldr x0, = .tex_77
  bl _escrever_tex
  ldr x0, = .tex_78
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
  ldr x0, = .tex_79
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
  ldr x0, = .tex_80
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
  ldr x0, = .tex_81
  bl _escrever_tex
  add x0, x29, -128
  bl _escrever_tex
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_82
  bl _escrever_tex
  mov w0, 0
  mov w1, w0 // otimizado(literal)
  mov w2, 1
  mul w1, w1, w2
  add w0, w0, w1
  add x2, x29, -128
  add x2, x2, x0
  ldrb w0, [x2]
  mov w19, w0
  strb w0, [x29, -144]
  ldr x0, = .tex_83
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
  ldr x0, = .tex_84
  bl _escrever_tex
  mov w0, 0
  mov w19, w0
  str w0, [x29, 304]
.B86:
  ldr w0, [x29, 304]
  mov w19, w0
  mov w0, 4
  mov w1, w19
  cmp w1, w0
  cset w0, lt
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B87
  ldr x0, = .tex_85
  bl _escrever_tex
  ldr w0, [x29, 304]
  bl _escrever_int
  ldr x0, = .tex_86
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
  b .B86
.B87:
  ldr x0, = const_11
  ldr s0, [x0]
  str s0, [x29, -176]
  ldr x0, = const_10
  ldr s0, [x0]
  str s0, [x29, -172]
  ldr x0, = const_12
  ldr s0, [x0]
  str s0, [x29, -168]
  ldr x0, = const_12
  ldr s0, [x0]
  str s0, [x29, -164]
  ldr x0, = .tex_87
  bl _escrever_tex
  mov w0, 0
  mov w19, w0
  str w0, [x29, 288]
.B89:
  ldr w0, [x29, 288]
  mov w19, w0
  mov w0, 4
  mov w1, w19
  cmp w1, w0
  cset w0, lt
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B90
  ldr x0, = .tex_85
  bl _escrever_tex
  ldr w0, [x29, 288]
  bl _escrever_int
  ldr x0, = .tex_86
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
  b .B89
.B90:
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
  ldr x0, = .tex_comb_8
  bl _escrever_tex
  mov w0, 0
  mov w19, w0
  str w0, [x29, -32]
.B92:
  ldr w0, [x29, -32]
  mov w19, w0
  mov w0, 10
  mov w1, w19
  cmp w1, w0
  cset w0, lt
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B93
  ldr x0, = .tex_90
  bl _escrever_tex
  ldr w0, [x29, -32]
  bl _escrever_int
  ldr w0, [x29, -32]
  add w0, w0, 1
  str w0, [x29, -32]
  b .B92
.B93:
  mov w0, 0
  mov w19, w0
  str w0, [x29, -48]
  ldr x0, = .tex_91
  bl _escrever_tex
.B95:
  ldr w0, [x29, -48]
  mov w19, w0
  mov w0, 10
  mov w1, w19
  cmp w1, w0
  cset w0, lt
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B96
  ldr w0, [x29, -48]
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr w0, [x29, -48]
  mov w19, w0
  mov w0, 5
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B97
  ldr x0, = .tex_92
  bl _escrever_tex
  b .B96
  b .B98
.B97:
.B98:
  ldr w0, [x29, -48]
  add w0, w0, 1
  str w0, [x29, -48]
  b .B95
.B96:
  ldr x0, = .tex_93
  bl _escrever_tex
  mov w0, 0
  mov w19, w0
  str w0, [x29, 160]
.B100:
  ldr w0, [x29, 160]
  mov w19, w0
  mov w0, 10
  mov w1, w19
  cmp w1, w0
  cset w0, lt
  mov w19, w0
  mov w0, w19
  cmp w0, 0
  beq .B101
  ldr x0, = .tex_94
  bl _escrever_tex
  ldr w0, [x29, 160]
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr w0, [x29, 160]
  add w0, w0, 1
  str w0, [x29, 160]
  b .B100
.B101:
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
  ldr x0, = .tex_95
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
  ldr x0, = .tex_96
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
  ldr x0, = const_13
  ldr s0, [x0]
  str s0, [x29, -48]
  ldr x0, = const_14
  ldr s0, [x0]
  str s0, [x29, -44]
  ldr x0, = const_15
  ldr s0, [x0]
  str s0, [x29, -40]
  ldr x0, = const_5
  ldr s0, [x0]
  str s0, [x29, -44]
  ldr x0, = const_16
  ldr s0, [x0]
  str s0, [x29, -40]
  ldr x0, = const_1
  ldr s0, [x0]
  str s0, [x29, -36]
  ldr x0, = .tex_97
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
  ldr x0, = .tex_98
  bl _escrever_tex
  add x0, x29, -64
  str x0, [sp, -16]!  // salva param 0 (ponteiro/longo)
  ldr x0, = .tex_99
  str w0, [sp, -16]!  // salva param 1 (int/bool/char/byte)
  ldr w1, [sp, 0]  // carrega param 1 (int/bool) em w1
  mov x1, x1  // estende pra 64 bits
  ldr x0, [sp, 16]  // carrega param 0 (ptr/longo) em x0
  add sp, sp, 32  // limpa temporarios
  bl texcp
  mov w0, 1991
  add x1, x29, -32
  str w0, [x1]
  add x0, x29, -112
  str x0, [sp, -16]!  // salva param 0 (ponteiro/longo)
  ldr x0, = .tex_100
  str w0, [sp, -16]!  // salva param 1 (int/bool/char/byte)
  ldr w1, [sp, 0]  // carrega param 1 (int/bool) em w1
  mov x1, x1  // estende pra 64 bits
  ldr x0, [sp, 16]  // carrega param 0 (ptr/longo) em x0
  add sp, sp, 32  // limpa temporarios
  bl texcp
  mov w0, 1992
  add x1, x29, -80
  str w0, [x1]
  ldr x0, = .tex_101
  bl _escrever_tex
  add x0, x29, -64
  bl _escrever_tex
  ldr x0, = .tex_102
  bl _escrever_tex
  add x0, x29, -32
  ldr w0, [x0]
  bl _escrever_int
  ldr x0, = .tex_103
  bl _escrever_tex
  add x0, x29, -112
  bl _escrever_tex
  ldr x0, = .tex_102
  bl _escrever_tex
  add x0, x29, -80
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
  ldr x0, = .tex_comb_9
  bl _escrever_tex
  ldr x0, = .tex_106
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
  ldr x0, = .tex_107
  bl _escrever_tex
  ldr x0, = .tex_108
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
  ldr x0, = const_6
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
// fn: [testeAsm] (vars: 32, total: 160)
.align 2
testeAsm:
  sub sp, sp, 160
  stp x29, x30, [sp, 144]
  add x29, sp, 144
  ldr x0, = .tex_109
  bl _escrever_tex
  ldr x0, = .tex_110
  str x0, [x29, -32]
  ldr x0, [x29, -32]
  str x0, [sp, -16]!  // salva param 0 (ponteiro/longo)
  ldr x0, [sp, 0]  // carrega param 0 (ptr/longo) em x0
  add sp, sp, 16  // limpa temporarios
  bl textam
  mov w19, w0
  sxtw x0, w0
  str x0, [x29, -48]
// inicio assembly manual
   mov x0, 1
   ldr x1, [x29, -32]
   ldr x2, [x29, -48]
   mov x8, 64
   svc 0

// fim assembly manual
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 144]
  add sp, sp, 160
  ret
// fim: [testeAsm]
// fn: [testeMat] (vars: 0, total: 128)
.align 2
testeMat:
  sub sp, sp, 128
  stp x29, x30, [sp, 112]
  add x29, sp, 112
  ldr x0, = .tex_comb_10
  bl _escrever_tex
  ldr x0, = const_6
  ldr s0, [x0]
  str s0, [sp, -16]!  // salva param 0 (float)
  ldr s0, [sp, 0]  // carrega param 0 (flu) em s0
  add sp, sp, 16  // limpa temporarios
  bl ftanh
  fmov s0, s0
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_113
  bl _escrever_tex
  ldr x0, = const_17
  ldr s0, [x0]
  str s0, [sp, -16]!  // salva param 0 (float)
  ldr s0, [sp, 0]  // carrega param 0 (flu) em s0
  add sp, sp, 16  // limpa temporarios
  bl fexp
  fmov s0, s0
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_114
  bl _escrever_tex
  ldr x0, = const_18
  ldr s0, [x0]
  str s0, [sp, -16]!  // salva param 0 (float)
  ldr s0, [sp, 0]  // carrega param 0 (flu) em s0
  add sp, sp, 16  // limpa temporarios
  bl fraiz
  fmov s0, s0
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_115
  bl _escrever_tex
  ldr x0, = global_PI
  ldr s0, [x0]
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_116
  bl _escrever_tex
  ldr x0, = global_E
  ldr s0, [x0]
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_117
  bl _escrever_tex
  ldr x0, = global_RAIZ2
  ldr s0, [x0]
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_118
  bl _escrever_tex
  ldr x0, = global_LOG2E
  ldr s0, [x0]
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  ldr x0, = .tex_119
  bl _escrever_tex
  ldr x0, = global_LOGE10
  ldr s0, [x0]
  bl _escrever_flu
  mov w0, 10
  bl _escrever_car
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 112]
  add sp, sp, 128
  ret
// fim: [testeMat]
  .section .rodata
  .align 8
const_0:
  .float 0.000000
const_1:
  .float 10.000000
const_2:
  .float 0.100000
const_3:
  .float 4.000000
const_4:
  .float 1.000000
const_5:
  .float 2.000000
const_6:
  .float 0.500000
const_7:
  .float 3.140000
const_8:
  .float 1.100000
const_9:
  .float -1.100000
const_10:
  .float 1.500000
const_11:
  .float 0.200000
const_12:
  .float 5.100000
const_13:
  .float 3.000000
const_14:
  .float 5.000000
const_15:
  .float 7.000000
const_16:
  .float 9.000000
const_17:
  .float 0.800000
const_18:
  .float 25.000000
.section .rodata
.align 2
.tex_0: .asciz "valor a: "
.tex_1: .asciz "valor b: "
.tex_2: .asciz "Testando escape:\n"
.tex_3: .asciz "\"teste\"\n\n"
.tex_4: .asciz "Testando tipos b\303\241sicos:\n"
.tex_5: .asciz "\ncaractere: "
.tex_6: .asciz "\ninteiro: "
.tex_7: .asciz "\nflutuante: "
.tex_8: .asciz "\nbooleano: "
.tex_9: .asciz "\nlongo: "
.tex_10: .asciz "\nbyte: "
.tex_11: .asciz " (0xFF)"
.tex_13: .asciz "\nsoma com retorno 5 + 7 = esperando 12, veio: "
.tex_14: .asciz "\n\nVariavel final inteira:\n"
.tex_15: .asciz "\n"
.tex_16: .asciz "\nsoma comum 5 + 7 = esperando 12, veio: "
.tex_17: .asciz "\n\nTestando atribui\303\247\303\265es:\n"
.tex_18: .asciz "\nnovo inteiro: "
.tex_19: .asciz "\nnovo caractere: "
.tex_20: .asciz "\nnovo booleano: "
.tex_21: .asciz "\nnovo longo: "
.tex_22: .asciz "\nTeste de positivo e negativo:\n\n"
.tex_23: .asciz "inteiro positivo: "
.tex_24: .asciz "inteiro negativo: "
.tex_25: .asciz "flutuante positivo: "
.tex_26: .asciz "flutuante negativo: "
.tex_31: .asciz "opera\303\247\303\243o (5 + 5) * 5, esperado: 50, veio: "
.tex_32: .asciz "10 % 3 = ?, esperado: 1, recebido: "
.tex_33: .asciz "\n\nTeste de opera\303\247\303\265es entre tipos:\n\n"
.tex_34: .asciz "x: "
.tex_35: .asciz " * y: "
.tex_36: .asciz ", resultado: "
.tex_37: .asciz "10 << 2, resultado: "
.tex_42: .asciz "\nTeste de atribui\303\247\303\265es compostas:\n\n"
.tex_43: .asciz "x = 0.5f, x += 1.5f, x agora \303\251 "
.tex_44: .asciz "x = 2.0f, x *= 2, x agora \303\251 "
.tex_45: .asciz "x = 4.0f, x -= 2, x agora \303\251 "
.tex_46: .asciz "x = 2.0f, x /= 2, x agora \303\251 "
.tex_47: .asciz "x2 = 10, x2 %= 5, x2 agora \303\251 "
.tex_48: .asciz "\nTeste compara\303\247\303\265es:\n\n"
.tex_49: .asciz "x \303\251 maior que 5\n"
.tex_50: .asciz "x \303\251 maior ou igual a 5\n"
.tex_51: .asciz "x n\303\243o \303\251 maior nem igual a 5\n"
.tex_52: .asciz "y >= 4 && x > 4 \303\251 verdadeiro\n"
.tex_53: .asciz "y >= 4 && x > 4 \303\251 falso\n"
.tex_54: .asciz "y == x || x > 3 \303\251 verdadeiro\n"
.tex_55: .asciz "y == x || x > 3 \303\251 falso\n"
.tex_56: .asciz "\nCompara\303\247\303\243o com textos:\n\n"
.tex_57: .asciz "texto 1"
.tex_58: .asciz "texto 2"
.tex_59: .asciz "texto 1 \303\251 igual a texto 2\n"
.tex_60: .asciz "texto 1 n\303\243o \303\251 igual a texto 2\n"
.tex_61: .asciz "o primeiro texto \303\251 texto 1\n"
.tex_62: .asciz "o primeiro texto n\303\243o \303\251 texto 1\n"
.tex_63: .asciz "\nTeste de operador Tern\303\241rio:\n\n"
.tex_64: .asciz "int x = 5 > 6 ? 10 : 20; x \303\251 igual a = "
.tex_65: .asciz "\nTeste de array:\n"
.tex_66: .asciz "\nvalor do array: "
.tex_67: .asciz "\narray mudado no indice 0: "
.tex_68: .asciz "\nTeste de ponteiro:\n\n"
.tex_69: .asciz "int x1 = 10; int* p1 = @x1;\n"
.tex_70: .asciz "p1 = p1 + 5; x1 = ?\n"
.tex_71: .asciz "x1 = "
.tex_72: .asciz "exemplo de ponteiro"
.tex_73: .asciz "\nponteiro texto, valor: "
.tex_74: .asciz "\ntamamho em bytes: "
.tex_75: .asciz "\no ponteiro tem t no indice: "
.tex_76: .asciz "\no ponteiro n\303\243o tem t\n"
.tex_77: .asciz "\nTeste de manipula\303\247\303\243o da memoria:\n\n"
.tex_78: .asciz "XxXmplo maior"
.tex_79: .asciz "\nArray padr\303\243o: "
.tex_80: .asciz "\nArray copiado da memoria: "
.tex_81: .asciz "\nArray usando subscar(array2, 'X', 'e'): "
.tex_82: .asciz "\nTeste de acesso a itens array:\n\n"
.tex_83: .asciz "item do indice 0 do array: "
.tex_84: .asciz "\nArray de inteiros: \n\n"
.tex_85: .asciz "no indice: "
.tex_86: .asciz " valor: "
.tex_87: .asciz "\nArray de flutuantes: \n\n"
.tex_90: .asciz "\nvalor de i: "
.tex_91: .asciz "\n\nTeste de parada do loop (deve parar em 5)\n"
.tex_92: .asciz "parando\n"
.tex_93: .asciz "\nLoop Por:\n"
.tex_94: .asciz "indice: "
.tex_95: .asciz "\nTeste de matrizes:\n\n"
.tex_96: .asciz "matriz 2D int m2[0][1]: "
.tex_97: .asciz "matriz 2D flu m2f[0][1]: "
.tex_98: .asciz "\nTeste de estrutura de dados (#espaco):\n\n"
.tex_99: .asciz "ronaldo"
.tex_100: .asciz "el pepe"
.tex_101: .asciz "Pessoa 1:\nNome: "
.tex_102: .asciz "\nIdade: "
.tex_103: .asciz "\nPessoa 2:\nNome: "
.tex_106: .asciz "123"
.tex_107: .asciz "1.50 + 0.50 = "
.tex_108: .asciz "1.50"
.tex_109: .asciz "\nTestando assembly manual:\n"
.tex_110: .asciz "assembly testado com sucesso\n"
.tex_113: .asciz "fexp(0.8f) = "
.tex_114: .asciz "fraiz(25f) = "
.tex_115: .asciz "PI = "
.tex_116: .asciz "E = "
.tex_117: .asciz "RAIZ2 = "
.tex_118: .asciz "LOG2E = "
.tex_119: .asciz "LOGE10 = "
.section .text


.section .rodata
.align 3
global_PI:
  .float 3.141593
global_E:
  .float 2.718282
global_RAIZ2:
  .float 1.414214
global_LOG2E:
  .float 1.442695
global_LOGE10:
  .float 2.302585

.section .data
.align 3
global_varGlobal:
  .word 10

.section .rodata
.align 2
.tex_comb_0: .asciz "Testando escape:\n\"teste\"\n\n"
.tex_comb_1: .asciz " (0xFF)\n\nTestando fun\303\247\303\243o soma:\n"
.tex_comb_2: .asciz "\nTeste de convers\303\243o:\n\n(car)65 = "
.tex_comb_3: .asciz "\nTeste de opera\303\247\303\265es matematicas:\n\nopera\303\247\303\243o 5 + 5 * 5, esperado: 30, veio: "
.tex_comb_4: .asciz "\n10 >> 2, resultado: "
.tex_comb_5: .asciz "\n124 & 15, resultado: "
.tex_comb_6: .asciz "\n124 & 0xF, resultado: "
.tex_comb_7: .asciz "\n1 | 2, resultado: "
.tex_comb_8: .asciz "\nTeste de loops:\nLoop Enquanto:"
.tex_comb_9: .asciz "\nTeste de convers\303\243o de texto:\n123 + 1 = "
.tex_comb_10: .asciz "\nTeste da biblioteca de matematica:\n\nftanh(0.5f) = "
