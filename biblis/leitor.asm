/*
esse leitor de texto consegue ler em um arquivo de até 128 bytes
*/
.global _start

.section .data
nome:    .asciz "ola.txt" // caminho
buffer:  .space 128 // 128 bytes pra leitura

.section .text
_start:
    mov x8, 56 // abrir
    mov x0, -100 // diretório atual
    ldr x1, = nome  // caminho
    mov x2, 0 // só leitura
    mov x3, 0
    svc 0
    mov x19, x0  // resultado do processo

    mov x8, 63 // leitura
    mov x0, x19
    ldr x1, = buffer // destino
    mov x2, 128  // tamanho
    svc 0
    mov x20, x0  // quantidade de bytes lidos

    mov x0, 1 // saída 
    ldr x1, = buffer
    mov x2, x20
    mov x8, 64 // escrita
    svc 0

    mov x8, 57 // fechar
    mov x0, x19
    svc 0

    // finalizar
    mov x8, 93
    mov x0, 0
    svc 0
