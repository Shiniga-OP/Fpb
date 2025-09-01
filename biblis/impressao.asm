.section .text
    .align 2

_escrever_int:
    stp     x29, x30, [sp, -32]!
    mov     x29, sp
    adr     x1, .Lint_buffer
    mov     x2, 0
    cmp     w0, 0
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

.Lint_buffer:
    .fill   32, 1, 0

_escrever_flu:
    stp x29, x30, [sp, -64]!
    mov x29, sp
    // s0 contém o valor flutuante
    adr x1, .Lfloat_buffer // buffer de saída
    mov x6, x1 // salva início do buffer
    // verifica se é negativo
    fcmp s0, 0.0
    b.ge .Lpositivo
    mov w2, '-' // sinal negativo
    strb w2, [x1], 1
    fneg s0, s0 // torna positivo para conversão
    b .Lconverter
    
.Lpositivo:
    mov     w2, ' ' // espaço para positivo
    strb    w2, [x1], 1
    
.Lconverter:
    // parte inteira
    fcvtzs  w2, s0 // converte flutuante para inteiro(trunca)
    mov w3, 10
    mov w4, 0 // contador de dígitos
    mov x5, sp // usa stack para empilhar dígitos
    // se parte inteira for zero
    cbz w2, .Lzero_int
    // converte parte inteira
.Lloop_int:
    udiv w7, w2, w3
    msub w8, w7, w3, w2
    add w8, w8, '0'
    strb w8, [x5, -1]! // empilha dígitos
    add w4, w4, 1
    mov w2, w7
    cbnz w2, .Lloop_int
    b .Ldesempilha
    
.Lzero_int:
    mov w7, '0'
    strb w7, [x1], 1
    // desempilha dígitos inteiros
.Ldesempilha:
    ldrb w7, [x5], 1
    strb w7, [x1], 1
    subs w4, w4, 1
    b.ne .Ldesempilha
    // ponto decimal
    mov w2, '.'
    strb w2, [x1], 1
    // parte fracionária(2 casas)
    fcvtzs w2, s0 // parte inteira
    scvtf s1, w2 // converte inteiro de volta para flutuante
    fsub s0, s0, s1 // s0 = parte fracionária
    // primeiro dígito decimal
    fmov s2, 10.0
    fmul s0, s0, s2 // *10
    fcvtzs w2, s0 // primeiro dígito
    add w2, w2, '0'
    strb w2, [x1], 1
    // segundo dígito decimal
    fcvtzs w2, s0 // parte inteira
    scvtf s1, w2 // converte para flutuante
    fsub s0, s0, s1 // remove parte inteira
    fmul s0, s0, s2 // *10
    fcvtzs w2, s0 // segundo dígito
    add w2, w2, '0'
    strb w2, [x1], 1
    // finaliza string
    mov w2, 0
    strb w2, [x1]
    // imprimir - CALCULAR TAMANHO CORRETAMENTE
    mov x0, 1 // saida de impressão
    mov x1, x6 // início do buffer
    // calcula tamanho: x1 aponta para início, x1+... para final
    adr x2, .Lfloat_buffer
    sub x3, x1, x2 // deslocamento atual
    add x2, x2, x3 // x2 = posição atual
    sub x2, x1, x6 // ERRADO - vamos fazer diferente
    // estratégia simples: vamos contar até encontrar o null
    mov x2, 0 // contador
    mov x3, x6 // ponteiro
.Lcontar:
    ldrb w4, [x3], 1
    cbz w4, .Lfim_contagem
    add x2, x2, 1
    b .Lcontar
    
.Lfim_contagem:
    mov x1, x6 // string do flutuante
    mov x8, 64 // syscall write
    svc 0
    
    ldp x29, x30, [sp], 64
    ret
    .section .data
    .align 2
.Lfloat_buffer:
    .fill   32, 1, 0
    
_escrever_double:
    stp     x29, x30, [sp, #-32]!
    mov     x29, sp
    adr     x1, .Ldouble_buffer
    mov     x0, 1
    mov     x2, 12
    mov     x8, 64
    svc     0
    ldp     x29, x30, [sp], #32
    ret

.Ldouble_buffer:
    .asciz  "[double]"

_escrever_char:
    stp x29, x30, [sp, -16]!
    mov x29, sp
    strb w0, [sp, -1]!
    mov x0, 1
    mov x1, sp
    mov x2, 1
    mov x8, 64
    svc 0
    add sp, sp, 1
    ldp x29, x30, [sp], 16
    ret

_escrever_bool:
    stp x29, x30, [sp, -16]!
    mov x29, sp
    cmp w0, 0
    adr x1, .Lfalso
    adr x2, .Lverdade
    csel x1, x1, x2, eq
    mov x0, 1
    mov x2, 7
    cbnz w0, .Limpr
    mov x2, 5

.Limpr:
    .align 2
    mov x8, 64
    svc 0
    ldp x29, x30, [sp], 16
    ret

.Lverdade:
    .asciz  "verdade"
    .align 2
.Lfalso:
    .asciz  "falso"