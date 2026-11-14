// util/arquiteturas/arm64/arm64_aritmetica.h
#pragma once
#include "arm64.h"

// emite "slot = esquerda OP direita": carrega os dois operandos em
// x9/x10(mesmos registradores de rascunho documentados em emitirTexTam),
// calcula em x9 e grava no slot de 16 bytes do destino(temporário ou
// variável, tanto faz: mesmo espaço de slots de emitirReatribuicao).
// DIVISAO e MODULO checam o divisor antes de usar "sdiv": divisao por
// zero e matematicamente indefinida, e o ARM64 não gera nenhuma
// excecao de hardware pra isso(sdiv por zero silenciosamente devolve 0),
// entao a checagem e emitida aqui em software, abortando o processo
// com "sair(1)" se o divisor for zero, em vez de deixar passar um
// resultado incorreto sem aviso nenhum.
inline void ARM64::emitirOperacaoAritmetica(std::ostringstream& saida, const InstrucaoOperacaoAritmetica& instrucao) {
    // regra de promocao: usa a MAIOR largura entre os dois operandos, decide se opera em w(32 bits, largura<=4) ou x(64 bits, largura==8)
    int largura = (instrucao.esquerda.larguraBytes > instrucao.direita.larguraBytes) ? instrucao.esquerda.larguraBytes : instrucao.direita.larguraBytes;
    bool usaW = (largura <= 4);
    emitirCarregarValorEmComLargura(saida, "x9", instrucao.esquerda, largura);
    emitirCarregarValorEmComLargura(saida, "x10", instrucao.direita, largura);
    if(instrucao.operador == OperadorAritmeticoIR::SOMA) {
        saida << (usaW ? "add w9, w9, w10\n" : "add x9, x9, x10\n");
    } else if(instrucao.operador == OperadorAritmeticoIR::SUBTRACAO) {
        saida << (usaW ? "sub w9, w9, w10\n" : "sub x9, x9, x10\n");
    } else if(instrucao.operador == OperadorAritmeticoIR::MULTIPLICACAO) {
        saida << (usaW ? "mul w9, w9, w10\n" : "mul x9, x9, x10\n");
    } else if(instrucao.operador == OperadorAritmeticoIR::DIVISAO) {
        emitirChecagemDivisorZero(saida);
        saida << (usaW ? "sdiv w9, w9, w10\n" : "sdiv x9, x9, x10\n");
    } else { // MODULO: resto = dividendo - (dividendo/divisor)*divisor.
        // x9=dividendo, x10=divisor. x28 e reservado(fora do pool de
        // TOTAL_REGISTRADORES_FISICOS, ver ir.h) especificamente pra este
        // caso: guarda o dividendo enquanto x9 vira o quociente(sdiv),
        // pro msub calcular "dividendo - quociente*divisor" em seguida.
        // MSUB Xd,Xn,Xm,Xa calcula Xd = Xa - (Xn*Xm): Xa(minuendo) =
        // x28(dividendo original), Xn*Xm = quociente*divisor.
        emitirChecagemDivisorZero(saida);
        saida << "mov x28, x9\n";
        saida << (usaW ? "sdiv w9, w9, w10\n" : "sdiv x9, x9, x10\n");
        saida << (usaW ? "msub w9, w9, w10, w28\n" : "msub x9, x9, x10, x28\n");
    }
    emitirGravarSlot(saida, "x9", instrucao.slotDestino, largura);
}

// emite a checagem "se x10(divisor) == 0, aborta o processo": usada por
// DIVISAO e MODULO antes de qualquer "sdiv". Usa um rotulo unico por
// chamada(mesmo padrao de emitirSe/emitirEnquanto/emitirTexTam) pra nao
// colidir quando o programa tem mais de uma divisao/modulo. O código de
// saída 1 segue a convenção Unix de "erro genérico"(0 == sucesso).
inline void ARM64::emitirChecagemDivisorZero(std::ostringstream& saida) {
    int idRotulo = contadorRotulos++;
    std::string rotuloContinua = "_divisor_ok_" + std::to_string(idRotulo);
    saida << "cbnz x10, " << rotuloContinua << "\n";
    saida << "mov x0, 1\n"; // codigo de saida: erro(divisao por zero)
    saida << "mov x8, " << numSistema(ChamadaSistemaId::SAIR) << "\n";
    saida << "svc 0\n";
    saida << rotuloContinua << ":\n";
}