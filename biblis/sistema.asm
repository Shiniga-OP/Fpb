.section .text
// retorna o total de milissegundos desde a epoca(1970)
// O resultado(64 bits) em x0
.align 2
obter_tempo_milis:
    // prepara a Pilha
    // precisa de 16 bytes pra a struct timespec(2x 64-bit: tv_sec, tv_nsec)
    // alocamo 32 bytes(pra manter alinhamento de 16) e salvar registradores
    stp x29, x30, [sp, -32]!  // salva frame pointer(fp) e link register(lr)
    mov x29, sp // define novo frame pointer
    // x0 = clock_id(o tempo "real")
    // x1 = ponteiro pra a struct timespec(usaremos o espaço na pilha)
    // x8 = numero da chamada de sistema(113 = obter o tempo)
    
    mov x0, 0 // x0 = tempo
    add x1, sp, 16 // x1 = aponta pra o buffer de 16 bytes na pilha
    mov x8, 113 // função
    svc 0
    
    // converte o resultado(struct) pra milissegundos
    // a chamada de sistema preencheu o buffer na pilha:
    // [sp+16] = tv_sec(segundos, 64 bits)
    // [sp+24] = tv_nsec(nanossegundos, 64 bits)
    
    // formula: total_ms = (segundos*1000)+(nanossegundos/1_000_000)
    ldr x2, [sp, 16] // x2 = segundos(tv_sec)
    ldr x3, [sp, 24] // x3 = nanossegundos(tv_nsec)
    
    // calcula(segundos * 1000)
    ldr x4, = 1000
    mul x2, x2, x4 // x2 = total de milissegundos dos segundos
    
    // calcula (nanossegundos / 1_000_000)
    ldr x4, = 1000000
    udiv x3, x3, x4 // x3 = total de milissegundos dos nanossegundos
    
    // soma os dois e coloca em x0(retorno)
    add x0, x2, x3 // x0 = (segundos*1000)+(nanos/1M)
    
    // restaura a pilha e retorna
    ldp x29, x30, [sp], 32  // restaura fp, lr e libera a pilha
    ret