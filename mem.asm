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
// fim de biblis/impressao.asm


// inicio de biblis/mem.fpb

// inicio de biblis/mem.asm
// fn: [memalocar]
// x0 = tamanho
// retorna ponteiro ou 0
.align 2
memalocar:
    stp x29, x30, [sp, -16]!
    mov x29, sp

    mov x19, x0 // guarda tamanho

    // prepara parametros do mmap
    mov x0, 0 // endereço = 0
    mov x1, x19 // tamanho
    mov x2, 3 // PROT_READ | PROT_WRITE
    mov x3, 34 // MAP_PRIVATE | MAP_ANONYMOUS
    mov x4, -1 // fd = -1
    mov x5, 0 // pos = 0
    mov x8, 222 // chamada mmap
    svc 0
    // verifica erro: mmap retorna valores negativos entre -1 e -4095
    // testa se x0 é negativo
    // se x0 < 0 = erro
    tbz x0, 63, 1f // bit 63 = sinal, se zero, é sucesso
    // erro
    mov x0, 0
    b 2f
1:  // sucesso
    // x0 ja contem o ponteiro
2:
    ldp x29, x30, [sp], 16
    ret
// fim: [memalocar]
// fn: [memliberar]
// x0 = ponteiro
// x1 = tamanho
// retorna 0 ou -1
.align 2
memliberar:
    stp x29, x30, [sp, -16]!
    mov x29, sp

    mov x8, 215 // munmap
    svc 0

    // se retorno != 0 = erro
    cmp x0, 0
    b.eq 1f
    
    mov x0, -1
    b 2f
1:
    mov x0, 0
2:
    ldp x29, x30, [sp], 16
    ret
// fim: [memliberar]
// fim de biblis/mem.asm


// fim de biblis/mem.fpb

.global inicio
// fn: [inicio] (vars: 32, total: 160)
.align 2
inicio:
  sub sp, sp, 160
  stp x29, x30, [sp, 144]
  add x29, sp, 144
  ldr x0, = .tex_0
  bl _escrever_tex
  mov w0, 4
  str w0, [sp, -16]!  // salva param 0 (int/bool/char/byte)
  ldr w0, [sp, 0]  // carrega param 0 (int/bool) em w0
  mov x0, x0  // estende pra 64 bits
  add sp, sp, 16  // limpa temporarios
  bl memalocar
  mov x19, x0
  str x0, [x29, -32]
  ldr x0, = .tex_1
  bl _escrever_tex
  ldr x1, [x29, -32]
  ldr w0, [x1]
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  mov w0, 2
  mov w19, w0
  ldr x1, [x29, -32]
  str w0, [x1]
  ldr x0, = .tex_2
  bl _escrever_tex
  ldr x1, [x29, -32]
  ldr w0, [x1]
  bl _escrever_int
  mov w0, 10
  bl _escrever_car
  ldr x0, [x29, -32]
  str x0, [sp, -16]!  // salva param 0 (ponteiro/longo)
  mov w0, 4
  str w0, [sp, -16]!  // salva param 1 (int/bool/char/byte)
  ldr w1, [sp, 0]  // carrega param 1 (int/bool) em w1
  mov x1, x1  // estende pra 64 bits
  ldr x0, [sp, 16]  // carrega param 0 (ptr/longo) em x0
  add sp, sp, 32  // limpa temporarios
  bl memliberar
  mov w19, w0
  str w0, [x29, -48]
  ldr x0, = .tex_3
  bl _escrever_tex
  ldr w0, [x29, -48]
  mov w19, w0
  mov w0, 0
  mov w1, w19
  cmp w1, w0
  cset w0, eq
  cmp w0, 0
  beq .ternario_falso_0
  ldr x0, = .tex_4
  b .ternario_fim_0
.ternario_falso_0:
  ldr x0, = .tex_5
.ternario_fim_0:
  bl _escrever_tex
  mov w0, 10
  bl _escrever_car
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
.align 2
.tex_0: .asciz "\n=== Teste de aloca\303\247\303\243o ===\n"
.tex_1: .asciz "4 bytes alocados: "
.tex_2: .asciz "n\303\272mero modificado: "
.tex_3: .asciz "4 bytes liberados liberado: "
.tex_4: .asciz "liberado!"
.tex_5: .asciz "erro!"
.section .text


.section .rodata
.align 2
