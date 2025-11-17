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
    cbz w4, 3f // se zero, fim do texto
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