// util/arquiteturas/arm64/arm64_chamadas.h
#pragma once
#include "arm64.h"

// emite #chamada_sistema id, arg1, arg2, ...; cru: o primeiro identificador
// e o ID ABSTRATO da chamada de sistema (mesmo espaco de ChamadaSistemaId,
// ex: ESCREVER=1, SAIR=2), traduzido aqui pro numero real da chamada de
// sistema desta arquitetura via numSistema(). Os demais argumentos vao
// posicionalmente pra x0, x1, x2... na ordem em que foram escritos.
// suporta ate 6 argumentos posicionais(x0-x5), limite da convenção ARM64.
inline void ARM64::emitirChamadaSistemaCrua(std::ostringstream& saida, const InstrucaoChamadaSistemaCrua& instrucao) {
    static const char* registradores[] = { "x0", "x1", "x2", "x3", "x4", "x5" };
    int totalArgumentosPosicionais = instrucao.argumentos.tam - 1;
    if(totalArgumentosPosicionais > 6) {
        throw std::runtime_error("Arquitetura ARM64: #chamada_sistema aceita no máximo 6 argumentos posicionais");
    }
    for(int i = 1; i < instrucao.argumentos.tam; i++) {
        const ArgumentoIR& arg = instrucao.argumentos[i];
        const char* registrador = registradores[i - 1];
        if(arg.tipo == ArgumentoIR::Tipo::SIMBOLO_TEXTO) {
            emitirEnderecoSimbolo(saida, registrador, arg.simbolo, arg.ehFuncao);
        } else if(arg.tipo == ArgumentoIR::Tipo::REGISTRADOR) {
            // ldrb/ldr(w) num registrador w zera os bits altos do x correspondente, entao o x0-x5 completo chega correto pra chamada de sistema mesmo pra um valor "car"(1 byte) ou "int"(4 bytes).
            emitirCarregarSlot(saida, registrador, arg.registrador, arg.larguraBytes, arg.sinalizado);
        } else {
            bool usaW = (arg.larguraBytes <= 4);
            emitirMovImediato(saida, registrador, arg.imediato, usaW);
        }
    }
    const ArgumentoIR& idAbstrato = instrucao.argumentos[0];
    if(idAbstrato.tipo != ArgumentoIR::Tipo::IMEDIATO) {
        throw std::runtime_error("Arquitetura ARM64: o ID da #chamada_sistema precisa ser um valor imediato (int)");
    }
    ChamadaSistemaId id = static_cast<ChamadaSistemaId>(idAbstrato.imediato);
    saida << "mov x8, " << numSistema(id) << "\n";
    saida << "svc 0\n";
    if(instrucao.capturaResultado) {
        emitirGravarSlot(saida, "x0", instrucao.slotDestino, 8); // resultado de syscall(ex: ponteiro do mmap) e sempre 8 bytes
    }
    saida << "\n";
}

// emite uma chamada real a uma função de usuário: nome(arg1, arg2, ...);
// argumentos vao posicionalmente pra x0, x1, x2...(convencao AAPCS64),
// seguido de "bl nome". Suporta até 6 argumentos(x0-x5). Se a chamada
// for usada como valor de expressão(capturaResultado == true, ex:
// "int x = f();"), grava x0(valor de retorno, convenção AAPCS64) no
// slot destino logo apos o bl, antes que qualquer outra chamada possa sujar x0.
inline void ARM64::emitirChamadaFuncaoUsuario(std::ostringstream& saida, const InstrucaoChamadaFuncaoUsuario& instrucao) {
    int totalInteiros = 0;
    int totalFlutuantes = 0;
    int totalPilha = 0;
    for(int i = 0; i < instrucao.argumentos.tam; i++) {
        emitirArgumentoChamada(saida, instrucao.argumentos[i], totalInteiros, totalFlutuantes, totalPilha);
    }
    if(instrucao.indireta) {
        // chamada por endereço: x16(IP0, rascunho de chamada da AAPCS64, fora dos que esta arquitetura usa) recebe o endereço DEPOIS dos
        // argumentos, pois carregar os argumentos usa só x0-x5 e a pilha.
        emitirCarregarSlot(saida, "x16", instrucao.enderecoAlvo.registrador, 8);
        saida << "blr x16\n";
    } else {
        saida << "bl " << instrucao.nome << "\n";
    }
    if(instrucao.capturaResultado) {
        if(instrucao.resultadoFlutuante) {
            saida << (instrucao.resultadoLargura == 4 ? "fmov w9, s0\n" : "fmov x9, d0\n");
            emitirGravarSlot(saida, "x9", instrucao.slotDestino, instrucao.resultadoLargura);
        } else {
            emitirGravarSlot(saida, "x0", instrucao.slotDestino, instrucao.resultadoLargura);
        }
    }
    saida << "\n";
}
// emite "#ler_reg<TLS>"(mrs) ou "#escrever_reg<TLS, valor>"(msr). TLS = tpidr_el0. x9 e o rascunho.
inline void ARM64::emitirRegistradorSistema(std::ostringstream& saida, const InstrucaoRegistradorSistema& instrucao) {
    if(instrucao.escrever) {
        emitirCarregarValorEmComLargura(saida, "x9", instrucao.valor, 8);
        saida << "msr tpidr_el0, x9\n\n";
        return;
    }
    saida << "mrs x9, tpidr_el0\n";
    emitirGravarSlot(saida, "x9", instrucao.slotDestino, 8);
    saida << "\n";
}