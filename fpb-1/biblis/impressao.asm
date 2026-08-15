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
