.global _start
.section .data
texto: .asciz "texto de exemplo\n"
tam = . - texto
msgS: .asciz "o texto tem X\n"
tamMsgS = . - msgS
msgN: .asciz "o texto não tem X\n"
tamMsgN = . - msgN

.section .text
_start:
	ldr x1, = texto
	mov x2, tam
	bl log
	
	sub sp, sp, tam // reserva os bytes do tamanho da string
	mov x0, sp
	// copia a string
	bl copiar_string
	
	mov x0, sp  // cópia da string
	
	mov w1, 'o' // char alvo
	mov w2, 'X' // char novo
	bl substituir_chars
	
	mov x1, sp // string modificada
	mov x2, tam
	bl log
	
	mov x0, sp
	// testando a string modificada
	mov w1, 'X'
	bl achar_char
	bl comparar
	bl log
	// testando texto original
	mov x0, sp
	mov x2, tam
	bl limp_pilha
	mov x0, sp
	ldr x1, = texto
	bl copiar_string
	mov x0, sp
	bl str_tam // pegando o tamamho
	// mostrando o original
	mov x1, sp
	mov x2, x0
	mov x0, sp
	bl log
	
	mov x0, sp
	mov w1, 'X'
	bl achar_char
	bl comparar
	bl log
	
	add sp, sp, tam // libera memória
	bl teste
	bl fim;
	
comparar:
	cmp x0, -1 // compara x0 com -1 (que significa falso)
	b.ne tem_t // senão for
	ldr x1, = msgN
	mov x2, tamMsgN
	ret
tem_t:
	ldr x1, = msgS
	mov x2, tamMsgS
	ret
	
log:
	mov x0, 1
	mov x8, 64
	svc 0
	ret
	
fim:
    mov x0, 0
    mov x8, 93
    svc 0
// x0 = string, retorna w0 = tamanho
str_tam:
    mov x1, x0
tam_loop:
    ldrb w2, [x1], 1
    cbnz x2, tam_loop
    sub x0, x1, x0
    sub x0, x0, 1
    ret
copiar_string:
    ldrb w3, [x1], 1   // carrega byte e incrementar ponteiro
    strb w3, [x0], 1   // armazena byte e incrementar ponteiro
    subs x2, x2, 1  // decrementa contador
    b.gt copiar_string  // continua se não terminou
    ret
// x0: string, w1: char a substituir, w2: novo char
substituir_chars:
    ldrb w3, [x0] // carrega caractere atual
    cmp w3, 0 // verifica fim da string
    b.eq retorne_subs_char // se for igual executa "retorne_subs_char"
    cmp w3, w1 // verifica se é o caractere alvo
    b.ne proximo // senão, executa "próximo"
    strb w2, [x0] // substituí pelo novo caractere
proximo:
    add x0, x0, 1 // avança para próximo caractere
    b substituir_chars
retorne_subs_char:
    ret
// x0 = string, w1 = char, x0 = retorna posição ou -1 se não encontrar
achar_char:
    mov x2, x0
loop_achar:
    ldrb w3, [x2]
    cbz w3, nao_achado
    cmp w3, w1
    b.eq achado
    add x2, x2, 1
    b loop_achar
achado:
    mov x0, x2
    ret
nao_achado:
    mov x0, -1
    ret
// x0 = endereço da pilha, x2 = tamanho
limp_pilha:
    mov x3, 0
    mov x4, x2
loop_limp:
    cbz x4, retorne_limp
    strb w3, [x0], 1
    subs x4, x4, 1
    b.gt loop_limp
    mov x2, x2
retorne_limp:
	ret

teste:
	mov x29, sp // define x29 como novo frame pointer
	mov w0, 5 // argumento pra imprimir
	bl log_int // chamada da função
	bl fim
    .section .text
    .align 2

// w0: número int a imprimir, x29: novo frame pointer
log_int:
    stp     x29, x30, [sp, -32]!
    mov     x29, sp
    adr     x1, .Lint_buffer
    mov     x2, 0
    cmp     w0, #0
    b.ge    .Lpos
    neg     w0, w0
    mov     w3, '-'
    strb    w3, [x1], 1
    mov     x2, 1

.Lpos:
    .align 2
    mov     x3, x1

.Lloop:
    .align 2
    mov     w4, 10
    udiv    w5, w0, w4
    msub    w6, w5, w4, w0
    add     w6, w6, '0'
    strb    w6, [x3], 1
    add     x2, x2, 1
    mov     w0, w5
    cbnz    w0, .Lloop

    sub     x3, x3, #1

.Lrev:
    .align 2
    cmp     x1, x3
    b.ge    .Lfim
    ldrb    w5, [x1]
    ldrb    w6, [x3]
    strb    w5, [x3], -1
    strb    w6, [x1], 1
    b       .Lrev

.Lfim:
    .align 2
    adr     x1, .Lint_buffer
    mov     x0, 1
    mov     x8, 64
    svc     0
    ldp     x29, x30, [sp], 32
    ret

    .section .data
    .align 2
.Lint_buffer:
    .fill   32, 1, 0

    .section .text
    .align 2
log_float:
    stp     x29, x30, [sp, -32]!
    mov     x29, sp
    adr     x1, .Lfloat_buffer
    mov     x0, 1
    mov     x2, 12
    mov     x8, 64
    svc     0
    ldp     x29, x30, [sp], 32
    ret

    .section .data
    .align 2
.Lfloat_buffer:
    .asciz  "[float]"

    .section .text
    .align 2
log_double:
    stp     x29, x30, [sp, -32]!
    mov     x29, sp
    adr     x1, .Ldouble_buffer
    mov     x0, 1
    mov     x2, 12
    mov     x8, 64
    svc     0
    ldp     x29, x30, [sp], 32
    ret

    .section .data
    .align 2
.Ldouble_buffer:
    .asciz  "[double]"

    .section .text
    .align 2
log_char:
    stp     x29, x30, [sp, -16]!
    mov     x29, sp
    strb    w0, [sp, #-1]!
    mov     x0, 1
    mov     x1, sp
    mov     x2, 1
    mov     x8, 64
    svc     0
    add     sp, sp, #1
    ldp     x29, x30, [sp], 16
    ret

    .section .text
    .align 2
log_bool:
    stp     x29, x30, [sp, -16]!
    mov     x29, sp
    cmp     w0, #0
    adr     x1, .Lfalso
    adr     x2, .Lverdade
    csel    x1, x1, x2, eq
    mov     x0, 1
    mov     x2, 7
    cbnz    w0, .Limpr
    mov     x2, 5

.Limpr:
    .align 2
    mov     x8, 64
    svc     0
    ldp     x29, x30, [sp], 16
    ret

    .section .data
    .align 2
.Lverdade:
    .asciz  "verdade"
    .align 2
.Lfalso:
    .asciz  "falso"
