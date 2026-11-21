// util/arquiteturas/arm64/arm64_memoria.h
#pragma once
#include "arm64.h"

// gera assembly que calcula, em tempo de execução, o tamanho de um texto
// terminada em '\0' apontada pelo ponteiro guardado no slot de
// "argumentoPonteiro"(pode ser parametro OU variavel/temporario, ex: o
// resultado de uma função que retorna car*), e deixa
// x0=1(fd stdout), x1=ponteiro, x2=tamanho, prontos pra chamada de sistema
// de escrever. Usa um rotulo unico por chamada pra evitar colisão
// quando escrever(parâmetro) aparece mais de uma vez no programa.
// NOTA: x9/w9 e reservado como registrador de rascunho desta arquitetura(fora da faixa x0-x5 usada por parâmetros/args, então seguro).
// nenhuma outra função de emissao deste arquivo deve usar x9.
inline void ARM64::emitirTexTam(std::ostringstream& saida, const ArgumentoIR& argumentoPonteiro) {
    int idRotulo = contadorRotulos++;
    std::string rotuloLoop = "_textam_loop_" + std::to_string(idRotulo);
    std::string rotuloFim = "_textam_fim_" + std::to_string(idRotulo);

    // x1 = ponteiro. emitirCarregarSlot(não emitirCarregarParametro) pois
    // "registrador" aqui e' um indice de slot genérico(parâmetro OU
    // variavel/temporário), não necessariamente 0..5.
    emitirCarregarSlot(saida, "x1", argumentoPonteiro.registrador, 8);
    saida << "mov x2, 0\n"; // x2 = contador de bytes
    saida << rotuloLoop << ":\n";
    saida << "ldrb w9, [x1, x2]\n"; // w9 = byte atual (x1 + x2)
    saida << "cbz w9, " << rotuloFim << "\n"; // achou '\0': para
    saida << "add x2, x2, 1\n";
    saida << "b " << rotuloLoop << "\n";
    saida << rotuloFim << ":\n";
    saida << "mov x0, 1\n"; // fd stdout
}

// emite "slot = ponteiro[indice]": carrega o ponteiro base em x9 e o
// índice em x10(mesmos registradores de rascunho de emitirOperacaoAritmetica). Multiplica o indice pela largura de cada elemento
// apontado(ponteiro.larguraElemento: 1 pra car*, 4 pra int*, 8 pra
// longo*, ou qualquer largura futura) via "mul"(generico, funciona pra
// qualquer largura, não só potência de 2), soma ao ponteiro pra achar o
// endereco final, e le essa mesma largura de bytes desse endereco
// (ldrb se largura==1, ldr w se largura==4, ldr x se largura==8),
// gravando o resultado no slot destino.
inline void ARM64::emitirLeituraIndexada(std::ostringstream& saida, const InstrucaoLeituraIndexada& instrucao) {
    emitirCarregarValorEmComLargura(saida, "x9", instrucao.ponteiro, 8);
    emitirCarregarValorEmComLargura(saida, "x10", instrucao.indice, 8);
    int largura = instrucao.ponteiro.larguraElemento;
    saida << "mov x11, " << largura << "\n";
    saida << "mul x10, x10, x11\n"; // x10 = indice * largura do elemento
    saida << "add x9, x9, x10\n"; // x9 = endereco final(ponteiro + indice*largura)
    if(largura == 1) {
        saida << "ldrb w9, [x9]\n";
        emitirGravarSlot(saida, "x9", instrucao.slotDestino, 1);
    } else if(largura == 2) {
        saida << "ldrh w9, [x9]\n";
        emitirGravarSlot(saida, "x9", instrucao.slotDestino, 2);
    } else if(largura == 4) {
        saida << "ldr w9, [x9]\n";
        emitirGravarSlot(saida, "x9", instrucao.slotDestino, 4);
    } else {
        saida << "ldr x9, [x9]\n";
        emitirGravarSlot(saida, "x9", instrucao.slotDestino, 8);
    }
}

// emite "ponteiro[indice] = valor": usa x9/x10/x11(mesma reserva de
// rascunho documentada em emitirTexTam; x19..x27 sao fisicos de slots e
// NÃO podem ser tocados aqui). Multiplica o indice pela largura de cada
// elemento apontado(ponteiro.larguraElemento) via "mul"(generico, funciona
// pra qualquer largura), soma ao ponteiro pra achar o endereco final, move
// pra x10(libera x9), materializa "valor" em x9 e grava essa mesma
// largura de bytes no endereço: largura==1(car*) usa strb, largura==4
// (int*) usa str num registrador w(32 bits), largura==8(longo*) usa str
// num registrador x(64 bits).
inline void ARM64::emitirEscritaIndexada(std::ostringstream& saida, const InstrucaoEscritaIndexada& instrucao) {
    emitirCarregarValorEmComLargura(saida, "x9", instrucao.ponteiro, 8);
    emitirCarregarValorEmComLargura(saida, "x10", instrucao.indice, 8);
    int largura = instrucao.ponteiro.larguraElemento;
    saida << "mov x11, " << largura << "\n";
    saida << "mul x10, x10, x11\n"; // x10 = indice * largura do elemento
    saida << "add x9, x9, x10\n"; // x9 = endereco final(ponteiro + indice*largura)
    saida << "mov x10, x9\n"; // move o endereco pra x10, libera x9 pro valor
    if(instrucao.valor.tipo == ArgumentoIR::Tipo::SIMBOLO_TEXTO) {
        emitirEnderecoSimbolo(saida, "x9", instrucao.valor.simbolo, instrucao.valor.ehFuncao);
    } else {
        emitirCarregarValorEmComLargura(saida, "x9", instrucao.valor, largura == 8 ? 8 : 4);
    }
    if(largura == 1) {
        saida << "strb w9, [x10]\n";
    } else if(largura == 2) {
        saida << "strh w9, [x10]\n";
    } else if(largura == 4) {
        saida << "str w9, [x10]\n";
    } else {
        saida << "str x9, [x10]\n";
    }
}

// emite "slot = endereco de um array local": os arrays ficam no frame,
// depois da area de x29/x30(baseArrays), e o slot guarda o ponteiro pra eles.
inline void ARM64::emitirEnderecoFrame(std::ostringstream& saida, const InstrucaoEnderecoFrame& instrucao) {
    saida << "add x9, sp, #" << (baseArrays + instrucao.posArray) << "\n";
    emitirGravarSlot(saida, "x9", instrucao.slotDestino, 8);
}
// emite "slot destino = endereco do slot de origem": o slot de origem mora na pilha(o alocador de registradores nunca dá registrador
// físico a um slot com ENDERECO_SLOT), em [sp, baseSlots + slot*16].
inline void ARM64::emitirEnderecoSlot(std::ostringstream& saida, const InstrucaoEnderecoSlot& instrucao) {
    saida << "add x9, sp, #" << (baseSlots + instrucao.slotOrigem * 16) << "\n";
    emitirGravarSlot(saida, "x9", instrucao.slotDestino, 8);
}