.global _start
.section .rodata
.align 2
.tex_0: .asciz "valor do array normal: "
.align 2
.tex_1: .asciz " maior\n"
.align 2
.tex_2: .asciz "\nvalor do array + array2: "
.section .text
.align 2
_start:
  mov x0, 0
  mov x1, 0
  mov x2, 0
  stp x29, x30, [sp, -64]!
  mov x29, sp
  stp x19, x20, [x29, 16]
  stp x21, x22, [x29, 32]
  add x0, x29, -32
  mov w1, 101
  strb w1, [x0, 0]
  mov w1, 120
  strb w1, [x0, 1]
  mov w1, 101
  strb w1, [x0, 2]
  mov w1, 109
  strb w1, [x0, 3]
  mov w1, 112
  strb w1, [x0, 4]
  mov w1, 108
  strb w1, [x0, 5]
  mov w1, 111
  strb w1, [x0, 6]
  mov w1, 0
  strb w1, [x0, 7]

  ldr x0, =.tex_0
  bl _escrever_tex

  add x0, x29, -32
  bl _escrever_tex

  add x0, x29, -32
  ldr x1, =.tex_1
  bl memadd

  ldr x0, =.tex_2
  bl _escrever_tex

  add x0, x29, -32
  bl _escrever_tex
  mov x0, 0
  b .epilogo_2
.epilogo_2:
  ldp x19, x20, [x29, 16]
  ldp x21, x22, [x29, 32]
  mov sp, x29
  ldp x29, x30, [sp], 64
  mov x0, 0
  mov x8, 93
  svc 0
  ret
.align 2
// [TEXTO]
_escrever_tex:
    mov x1, x0 // x1 = texto
    mov x2, 0 // x2 = contador
    // conta caracteres até encontrar null
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
.align 2
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

.align 2
// x0: destino, x1: fonte - Concatena fonte no final do destino
memadd:
    // Encontra o final da string destino
    mov x2, x0                   // x2 = ponteiro para percorrer destino
procurar_fim:
    ldrb w3, [x2]                // Carrega byte
    cbz w3, copiar               // Se zero, encontrou o final
    add x2, x2, 1                // Próximo caractere
    b procurar_fim

copiar:
    // Copia a string fonte para o final do destino
    ldrb w3, [x1], 1             // Carrega byte da fonte e incrementa
    strb w3, [x2], 1             // Armazena no destino e incrementa
    cbnz w3, copiar              // Continua até encontrar zero

fim_memadd:
    ret
