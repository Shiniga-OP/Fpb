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
    if(instrucao.esquerda.ehFlutuante) {
        // os dois lados ja foram promovidos pro mesmo tipo no gerador. v16/v17 sao caller-saved e fora do que esta arquitetura usa.
        bool dobro = instrucao.esquerda.larguraBytes == 8;
        const char* prefixo = dobro ? "d" : "s";
        emitirCarregarValorEm(saida, "x9", instrucao.esquerda);
        emitirCarregarValorEm(saida, "x10", instrucao.direita);
        saida << "fmov " << prefixo << "16, " << (dobro ? "x9" : "w9") << "\n";
        saida << "fmov " << prefixo << "17, " << (dobro ? "x10" : "w10") << "\n";
        const char* mnemonico = "fadd";
        if(instrucao.operador == OperadorAritmeticoIR::SUBTRACAO) mnemonico = "fsub";
        else if(instrucao.operador == OperadorAritmeticoIR::MULTIPLICACAO) mnemonico = "fmul";
        else if(instrucao.operador == OperadorAritmeticoIR::DIVISAO) mnemonico = "fdiv";
        saida << mnemonico << " " << prefixo << "16, " << prefixo << "16, " << prefixo << "17\n";
        saida << "fmov " << (dobro ? "x9" : "w9") << ", " << prefixo << "16\n";
        emitirGravarSlot(saida, "x9", instrucao.slotDestino, dobro ? 8 : 4);
        return;
    }
    // regra de promocao: usa a MAIOR largura entre os dois operandos, decide se opera em w(32 bits, largura<=4) ou x(64 bits, largura==8)
    // deslocamento usa so a largura do operando da esquerda(mesma regra de resolverExpressao no gerador)
    bool ehDeslocamento = instrucao.operador == OperadorAritmeticoIR::DESL_ESQ || instrucao.operador == OperadorAritmeticoIR::DESL_DIR;
    int largura = ehDeslocamento ? instrucao.esquerda.larguraBytes : ((instrucao.esquerda.larguraBytes > instrucao.direita.larguraBytes) ? instrucao.esquerda.larguraBytes : instrucao.direita.larguraBytes);
    if(largura < 4) largura = 4; // promoção pra int: operandos de 1 e 2 bytes são estendidos e operados em 32 bits
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
    } else if(instrucao.operador == OperadorAritmeticoIR::E_BIT) {
        saida << (usaW ? "and w9, w9, w10\n" : "and x9, x9, x10\n");
    } else if(instrucao.operador == OperadorAritmeticoIR::OU_BIT) {
        saida << (usaW ? "orr w9, w9, w10\n" : "orr x9, x9, x10\n");
    } else if(instrucao.operador == OperadorAritmeticoIR::XOU) {
        saida << (usaW ? "eor w9, w9, w10\n" : "eor x9, x9, x10\n");
    } else if(instrucao.operador == OperadorAritmeticoIR::DESL_ESQ) {
        saida << (usaW ? "lsl w9, w9, w10\n" : "lsl x9, x9, x10\n");
    } else if(instrucao.operador == OperadorAritmeticoIR::DESL_DIR) {
        // esquerda com sinal(int, byte, curto) faz deslocamento aritmetico(asr); sem sinal(car, bool) faz logico(lsr)
        const char* mnemonico = instrucao.esquerda.sinalizado ? "asr" : "lsr";
        saida << mnemonico << (usaW ? " w9, w9, w10\n" : " x9, x9, x10\n");
    } else { // MODULO: resto = dividendo - (dividendo/divisor)*divisor.
        // x9=dividendo, x10=divisor. x11 e rascunho(caller-saved, nao entra no pool de TOTAL_REGISTRADORES_FISICOS):
        // guarda o dividendo enquanto x9 vira o quociente(sdiv), pro msub calcular "dividendo - quociente*divisor".
        // MSUB Xd,Xn,Xm,Xa calcula Xd = Xa - (Xn*Xm): Xa(minuendo) = x11(dividendo original), Xn*Xm = quociente*divisor.
        emitirChecagemDivisorZero(saida);
        saida << "mov x11, x9\n";
        saida << (usaW ? "sdiv w9, w9, w10\n" : "sdiv x9, x9, x10\n");
        saida << (usaW ? "msub w9, w9, w10, w11\n" : "msub x9, x9, x10, x11\n");
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
// emite "slot = (tipo destino)origem" entre inteiro e flutuante: scvtf(int -> flu/dobro), fcvtzs(flu/dobro -> int, trunca pra zero) e fcvt(flu <-> dobro).
inline void ARM64::emitirConversaoNumerica(std::ostringstream& saida, const InstrucaoConversaoNumerica& instrucao) {
    const ArgumentoIR& origem = instrucao.origem;
    int largura = instrucao.larguraDestino;
    if(instrucao.destinoFlutuante) {
        const char* destino = largura == 4 ? "s16" : "d16";
        if(origem.ehFlutuante) {
            bool origemFlu = origem.larguraBytes == 4;
            emitirCarregarValorEm(saida, "x9", origem);
            saida << (origemFlu ? "fmov s17, w9\n" : "fmov d17, x9\n");
            saida << "fcvt " << destino << ", " << (origemFlu ? "s17" : "d17") << "\n";
        } else {
            emitirCarregarValorEmComLargura(saida, "x9", origem, 8);
            saida << "scvtf " << destino << ", x9\n";
        }
        saida << (largura == 4 ? "fmov w9, s16\n" : "fmov x9, d16\n");
    } else {
        bool origemFlu = origem.larguraBytes == 4;
        emitirCarregarValorEm(saida, "x9", origem);
        saida << (origemFlu ? "fmov s16, w9\n" : "fmov d16, x9\n");
        saida << "fcvtzs " << (largura == 8 ? "x9" : "w9") << ", " << (origemFlu ? "s16" : "d16") << "\n";
    }
    emitirGravarSlot(saida, "x9", instrucao.slotDestino, largura);
}