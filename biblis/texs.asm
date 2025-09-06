.section .data  
  .align 2  
.section .text  
// w0/x0 = ponteiro, w0/x0 = tamanho do texto  
.align 2
textam:  
    stp x29, x30, [sp, -16]!  
    mov x29, sp  
  
    mov x1, x0 // ponteiro de iteração  
    mov x2, x0 // ponteiro inicial  
1:  
    ldrb w3, [x1], 1 // carrega byte e incrementa ponteiro  
    cbz  w3, 2f // se for 0 = fim  
    b    1b  
2:  
    sub  x0, x1, x2 // usar 64-bit para ponteiros  
    sub  x0, x0, #1 // não contar o terminador nulo  
    // x0 contém o tamanho; w0 já tem o valor baixo pra retorno  
    mov  sp, x29  
    ldp  x29, x30, [sp], 16  
    ret  
.align 2
texcp:
    str x30, [sp, -16]! // salva o registrador de retorno
    ldrb w3, [x0], 1 // carrega byte e incrementar ponteiro  
    strb w3, [x1], 1 // armazena byte e incrementar ponteiro  
    subs x2, x2, 1  // decrementa contador  
    b.gt texcp  // continua se não terminou  
    mov x1, x0
    ldr x30, [sp], 16 // restaura o registrador de retorno
    ret  
// x0: string, w1: car a substituir, w2: novo car
.align 2
subscar:
    str x30, [sp, -16]! // salva o registrador de retorno
    ldr x0, [sp, 48] // carrega o ponteiro do texto
    ldr w1, [sp, 32] // carrega o caractere a buscar
    ldr w2, [sp, 16] // carrega o caractere substituto
    
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
    ldr x30, [sp], 16 // restaura o registrador de retorno
    ret
// verifica se tem um car no texto e em qual indice:
.align 2
texcar:
    str x30, [sp, -16]! // salva o registrador de retorno
    ldr x0, [sp, 32] // carrega o primeiro argumento(ponteiro)
    ldr w1, [sp, 16] // carrega o segundo argumento(caractere)

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
    ldr x30, [sp], 16 // restaura o registrador de retorno
    ret