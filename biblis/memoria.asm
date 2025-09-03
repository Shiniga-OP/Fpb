.global _start
.section .rodata
msg: .asciz "texto pra ser copiado na memória\n"
tam = . - msg
.section .text
_start:
	// string temporária:
	sub sp, sp, 4 // reservando 16 bytes na pilha
	mov x2, sp // guardando o início
	
	// escrevendo no array
	mov w1, 'o'
	strb w1, [x2], 1 // escrevendo na pilha
	
	mov w1, 'l'
	strb w1, [x2], 1
	
	mov w1, 'a'
	strb w1, [x2], 1
	mov w1, '\n'
	strb w1, [x2], 1
	mov x1, sp // passando o ponteiro pra escrever
	sub x2, x2, x1 // calculando o tamanho (início - fim)
	bl log
	add sp, sp, 4
	
	// teste de cópia de string na memória
	sub sp, sp, tam
	
	mov x2, tam
	ldr x1, = msg
	mov x0, sp
	bl copiar_string
	mov x0, sp
	mov x1, x0
	mov x2, tam // restaura o tamanho
	bl log
	ldr x1, = msg
	mov x2, tam
	mov x0, sp
	bl limp_pilha
	mov x0, sp
	ldr x1, = msg
	bl copiar_string
	mov x0, sp
	mov x1, x0
	mov x2, tam
	
	bl log
	ldr x1, = msg
	mov x2, tam
	bl log
	bl fim
log:
	mov x0, 1
	mov x8, 64
	svc 0
	ret
fim:
    mov x0, 0
    mov x8, 93
    svc 0

// x0: pilha pra cópia, x1: string a ser copiada, x2: o tamanho da string
copiar_string:
    ldrb w3, [x1], 1   // carrega byte e incrementar ponteiro
    strb w3, [x0], 1   // armazena byte e incrementar ponteiro
    subs x2, x2, 1  // decrementa contador
    b.gt copiar_string  // continua se não terminou
    ret
limp_pilha:
    mov x3, 0
    mov x4, x2
loop_limp:
    cbz x4, retorne
    strb w3, [x0], 1
    subs x4, x4, 1
    b.gt loop_limp
    mov x2, x2
retorne:
	ret
	
