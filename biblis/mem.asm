.section .text
.align 2
// w0: array, w1: endereço da memoria, w2: tamanho
memcp:
    str x30, [sp, -16]! // salva o registrador de retorno
    ldr x0, [sp, 48] // carrega o ponteiro do texto
    ldr w1, [sp, 32] // carrega o caractere a buscar
    ldr w2, [sp, 16] // carrega o caractere substituto
1:
    ldrb w3, [x1], 1 // carrega byte e incrementa ponteiro
    strb w3, [x0], 1 // armazena byte e incrementa ponteiro
    subs x2, x2, 1 // decrementa contador
    b.gt 1b // continua se não terminou
    ldr x30, [sp], 16 // restaura o registrador de retorno
    ret
memlimp:
    str x30, [sp, -16]! // salva o registrador de retorno
    ldr w0, [sp, 32] // carrega o ponteiro do texto
    ldr w2, [sp, 16] // carrega o caractere a buscar
    mov w3, 0
1:
    cbz x2, 2f
    strb w3, [x0], 1
    subs x2, x2, 1
    b.gt 1b
2:
    ldr x30, [sp], 16 // restaura o registrador de retorno
    ret