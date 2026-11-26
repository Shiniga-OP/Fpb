// util/arquiteturas/arm64/arm64_fluxo.h
#pragma once
#include "arm64.h"

// devolve o mnemonico de salto CONDICIONAL INVERTIDO: "se a condicao OP
// for FALSA, salta". Usado tanto por emitirSe(pula pro "senao"/fim)
// quanto por emitirEnquanto(pula pro fim do laço). Usa as condicoes
// SIGNED do ARM64(lt/gt/le/ge), coerente com o resto do compilador
// tratar todo inteiro como signed(ex: "sdiv" em vez de "udiv" na
// divisao em emitirOperacaoAritmetica).
inline const char* ARM64::mnemonicoSaltoSeFalso(OperadorComparacaoIR operador, bool flutuante) {
    // fcmp com NaN(nao ordenado) tem que cair em "falso" pra qualquer comparacao exceto "!=": por isso < e <= usam pl e hi em vez de ge e gt
    if(flutuante && operador == OperadorComparacaoIR::MENOR) return "b.pl";
    if(flutuante && operador == OperadorComparacaoIR::MENOR_IGUAL) return "b.hi";
    switch(operador) {
        case OperadorComparacaoIR::IGUAL: return "b.ne";
        case OperadorComparacaoIR::DIFERENTE: return "b.eq";
        case OperadorComparacaoIR::MAIOR: return "b.le";
        case OperadorComparacaoIR::MENOR: return "b.ge";
        case OperadorComparacaoIR::MAIOR_IGUAL: return "b.lt";
        default: return "b.gt"; // MENOR_IGUAL
    }
}

// emite "se(esquerda OP direita) { então } senao { senaoCorpo }".
// usa x9/x10 como registradores de rascunho(fora da faixa x0-x5, mesma
// reserva documentada em arm64.h) e rotulos unicos por chamada.
inline void ARM64::emitirSe(std::ostringstream& saida, const InstrucaoSe& instrucao, const ProgramaIR& programa) {
    int idRotulo = contadorRotulos++;
    std::string rotuloSenao = "_se_senao_" + std::to_string(idRotulo);
    std::string rotuloFim = "_se_fim_" + std::to_string(idRotulo);

    emitirComparacao(saida, instrucao.esquerda, instrucao.direita);

    bool temSenao = instrucao.temSenao;
    const char* saltoSeFalso = mnemonicoSaltoSeFalso(instrucao.operador, instrucao.esquerda.ehFlutuante);
    saida << saltoSeFalso << " " << (temSenao ? rotuloSenao : rotuloFim) << "\n\n";

    emitirBlocoComandos(saida, *instrucao.entao, programa);

    if(temSenao) {
        saida << "b " << rotuloFim << "\n\n";
        saida << rotuloSenao << ":\n";
        emitirBlocoComandos(saida, *instrucao.senaoCorpo, programa);
    }
    saida << rotuloFim << ":\n\n";
}

// emite "enquanto(esquerda OP direita) { corpo }": testa a condição no
// topo do laço(rotuloTopo), salta pro fim(rotuloFim) se falsa, senão
// executa o corpo e volta a testar de novo. Mesma reserva de x9/x10
// documentada em emitirSe. IMPORTANTE: comandosCondicao(instrucoes
// auxiliares que materializam esquerda/direita, ex: LEITURA_INDEXADA de
// "txt[i]") sao reemitidas a CADA volta do rotuloTopo, nao uma unica vez
// antes do laço: a condicao pode depender de estado alterado pelo corpo
// (ex: "i" mudando a cada iteracao em "enq(txt[i] != 0)"), entao emitir
// so uma vez antes congelaria o resultado da primeira avaliação e o
// laço nunca releria a condição de verdade.
inline void ARM64::emitirEnquanto(std::ostringstream& saida, const InstrucaoEnquanto& instrucao, const ProgramaIR& programa) {
    int idRotulo = contadorRotulos++;
    std::string rotuloTopo = "_enquanto_topo_" + std::to_string(idRotulo);
    std::string rotuloFim = "_enquanto_fim_" + std::to_string(idRotulo);

    saida << rotuloTopo << ":\n";
    emitirBlocoComandos(saida, *instrucao.comandosCondicao, programa);
    emitirComparacao(saida, instrucao.esquerda, instrucao.direita);

    const char* saltoSeFalso = mnemonicoSaltoSeFalso(instrucao.operador, instrucao.esquerda.ehFlutuante);
    saida << saltoSeFalso << " " << rotuloFim << "\n\n";

    emitirBlocoComandos(saida, *instrucao.corpo, programa);

    saida << "b " << rotuloTopo << "\n";
    saida << rotuloFim << ":\n\n";
}

// emite "retorne;" ou "retorne valor;": materializa 'valor'(se houver)
// diretamente em x0(registrador de retorno da convencao AAPCS64) e
// desvia pro rotulo de fim da funcao atual(_fim_<nome>), que desfaz o
// frame e emite "ret". Funciona de qualquer ponto do corpo, inclusive
// dentro de blocos aninhados de "se"/"enquanto", ja que "b" e um salto
// incondicional direto ao rotulo.
inline void ARM64::emitirRetorne(std::ostringstream& saida, const InstrucaoRetorne& instrucao) {
    if(instrucao.temValor) {
        const ArgumentoIR& valor = instrucao.valor;
        if(funcaoAtual->retornoFlutuante) {
            emitirCarregarValorEm(saida, "x9", valor);
            saida << (funcaoAtual->retornoLargura == 4 ? "fmov s0, w9\n" : "fmov d0, x9\n");
        } else if(valor.tipo == ArgumentoIR::Tipo::SIMBOLO_TEXTO) {
            emitirEnderecoSimbolo(saida, "x0", valor.simbolo, valor.ehFuncao);
        } else {
            emitirCarregarValorEm(saida, "x0", valor);
        }
    }
    saida << "b _fim_" << funcaoAtual->nome << "\n\n";
}
// compara esquerda com direita: "cmp" de 64 bits pra inteiros, "fcmp" pra flutuantes(os dois lados ja foram promovidos pro mesmo tipo no gerador).
inline void ARM64::emitirComparacao(std::ostringstream& saida, const ArgumentoIR& esq, const ArgumentoIR& dir) {
    if(esq.ehFlutuante) {
        bool dobro = esq.larguraBytes == 8;
        const char* prefixo = dobro ? "d" : "s";
        emitirCarregarValorEm(saida, "x9", esq);
        emitirCarregarValorEm(saida, "x10", dir);
        saida << "fmov " << prefixo << "16, " << (dobro ? "x9" : "w9") << "\n";
        saida << "fmov " << prefixo << "17, " << (dobro ? "x10" : "w10") << "\n";
        saida << "fcmp " << prefixo << "16, " << prefixo << "17\n";
        return;
    }
    emitirCarregarValorEmComLargura(saida, "x9", esq, 8);
    emitirCarregarValorEmComLargura(saida, "x10", dir, 8);
    saida << "cmp x9, x10\n";
}