// util/ir/gerador_condicoes.h
#pragma once
#include "geradorir.h"

inline void GeradorIR::atribuirMarcacao(int slot, long valor, Vetor<ComandoIR>& saida) {
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::REATRIBUICAO;
    comandoIR.instrucaoReatribuicao.slotDestino = slot;
    comandoIR.instrucaoReatribuicao.valor = imediatoInt(valor);
    comandoIR.instrucaoReatribuicao.larguraDestino = 4;
    saida.empurrar(comandoIR);
}

// invariante: a marcação "slot" vale 0 na entrada. Na saída vale 1 se "cond" é verdadeira, 0 senão.
// operandos com efeito colateral(chamadas) vão pro bloco onde só executam quando necessário(curto-circuito).
inline void GeradorIR::marcarSeVerdadeira(const Condicao& cond, int slot, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::SE;
    comandoIR.instrucaoSe.iniciar();
    
    if(cond.tipo == Condicao::Tipo::COMPARACAO) {
        comandoIR.instrucaoSe.esquerda = resolverExpressao(cond.esquerda, saida, ctx);
        comandoIR.instrucaoSe.direita = resolverExpressao(cond.direita, saida, ctx);
        promoverFlutuantes(comandoIR.instrucaoSe.esquerda, comandoIR.instrucaoSe.direita, saida, ctx);
        comandoIR.instrucaoSe.operador = traduzirOperadorComparacao(cond.operador);
        atribuirMarcacao(slot, 1, *comandoIR.instrucaoSe.entao);
        saida.empurrar(comandoIR);
        return;
    }
    marcarSeVerdadeira(*cond.condEsquerda, slot, saida, ctx);
    comandoIR.instrucaoSe.esquerda = referenciaSlotInt(slot);
    comandoIR.instrucaoSe.direita = imediatoInt(0);
    if(cond.tipo == Condicao::Tipo::E) {
        // esquerda foi verdadeira: zera e deixa só a direita decidir
        comandoIR.instrucaoSe.operador = OperadorComparacaoIR::DIFERENTE;
        atribuirMarcacao(slot, 0, *comandoIR.instrucaoSe.entao);
    } else {
        // esquerda foi falsa: a direita decide
        comandoIR.instrucaoSe.operador = OperadorComparacaoIR::IGUAL;
    }
    marcarSeVerdadeira(*cond.condDireita, slot, *comandoIR.instrucaoSe.entao, ctx);
    saida.empurrar(comandoIR);
}

// comparação simples continua igual a antes(sem custo extra). && / || viram uma marcação temporária comparada com 0.
inline void GeradorIR::resolverCondicao(const Condicao& cond, Vetor<ComandoIR>& saida, ContextoFuncao& ctx, ArgumentoIR& esquerda, OperadorComparacaoIR& operador, ArgumentoIR& direita) {
    if(cond.tipo == Condicao::Tipo::COMPARACAO) {
        esquerda = resolverExpressao(cond.esquerda, saida, ctx);
        direita = resolverExpressao(cond.direita, saida, ctx);
        promoverFlutuantes(esquerda, direita, saida, ctx);
        operador = traduzirOperadorComparacao(cond.operador);
        return;
    }
    int slot = ctx.variaveisLocais.proximoSlot++;
    atribuirMarcacao(slot, 0, saida);
    marcarSeVerdadeira(cond, slot, saida, ctx);
    esquerda = referenciaSlotInt(slot);
    operador = OperadorComparacaoIR::DIFERENTE;
    direita = imediatoInt(0);
}