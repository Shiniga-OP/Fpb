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