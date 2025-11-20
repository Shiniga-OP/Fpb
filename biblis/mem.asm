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
// x0: tamanho em bytes
// retorna: ponteiro para memoria alocada ou 0 se erro
.align 2
memalocar:
    stp x29, x30, [sp, -16]!
    mov x29, sp
    
    // salva o tamanho
    mov x19, x0
    
    // chama brk(0) pra obter o brk atual
    mov x0, 0
    mov x8, 214 // chamada de sistema pra brk

    svc 0
    
    cmp x0, -1 // verifica erro
    b.eq 2f
    
    mov x1, x0
    add x1, x1, x19 // + tamanho requisitado
    
    mov x0, x1
    mov x8, 214 // chamada de sistema brk
    svc 0
    
    cmp x0, -1 // verifica erro
    b.eq 2f
    
    // retorna ponteiro pro inicio da memoria alocada
    sub x0, x0, x19
1:
    ldp x29, x30, [sp], 16 // restaura registradores
    ret
2:
    mov x0, 0 // retorna 0 em caso de erro
    b 1b
// fim: [memalocar]