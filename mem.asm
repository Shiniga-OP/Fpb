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
// fim de biblis/impressao.asm


// inicio de biblis/texs.asm
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

.global inicio
// fn: [inicio] (vars: 0, total: 128)
.align 2
inicio:
  sub sp, sp, 128
  stp x29, x30, [sp, 112]
  add x29, sp, 112
  ldr x0, = global_teste
  str x0, [sp, -16]!
  ldr x0, = global_teste2
  ldr x0, [x0]
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
  beq .B1
  ldr x0, = global_teste
  bl _escrever_tex
  b .B2
.B1:
.B2:
  b 1f
// epilogo
1:
  ldp x29, x30, [sp, 112]
  add sp, sp, 128
  mov x0, 0
  mov x8, 93
  svc 0
  ret
// fim: [inicio]
.section .rodata
.align 2
.tex_0: .asciz "teste"
.section .text


.section .data
.align 3
global_teste:
  .asciz "teste"
global_teste2:
  .quad .tex_0

.section .text

.section .rodata
.align 2
