.global _start

.section .data
nome:   .asciz "saida.txt"
texto:  .asciz "exemplo de arquivo feito"
tam = . - texto

.section .text
_start:
    mov x8, 56 // abrir
    mov x0, -100 // diretório atual
    ldr x1, =nome
    mov x2, 65 // escrever + criar(1 | 64)
    mov x3, 0644 // permissão rw-r--r--
    svc 0
    mov x19, x0 // resultado

    mov x8, 64 // escrever
    mov x0, x19
    ldr x1, =texto // buffer com dados
    mov x2, tam // tamanho em bytes
    svc 0

    mov x8, 57 // fechar
    mov x0, x19
    svc 0
    // finalizar o programa
    mov x8, 93
    mov x0, 0
    svc 0
