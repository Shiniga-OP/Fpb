// util/ir/gerador_espacos.h
#pragma once
#include "geradorir.h"

// registra cada #espaco de "programa" na tabela global, checando nome de espaço repetido(programa principal ou biblioteca
// anterior) e nome de campo repetido dentro do mesmo espaço. "origem" é usada só na mensagem de erro.
inline void GeradorIR::registrarEspacos(const Programa& programa, const std::string& origem) {
    for(int i = 0; i < programa.espacos.tam; i++) {
        const DeclaracaoEspaco& espaco = programa.espacos[i];
        std::string nomeStr = espaco.nome;
        if(espacos.buscar(espaco.nome) >= 0) {
            throw std::runtime_error("Espaço '" + nomeStr + "' definido mais de uma vez (conflito ao incluir '" + origem + "')");
        }
        for(int a = 0; a < espaco.campos.tam; a++) {
            for(int b = a + 1; b < espaco.campos.tam; b++) {
                if(strcmp(espaco.campos[a].nome, espaco.campos[b].nome) == 0) {
                    std::string campoStr = espaco.campos[a].nome;
                    throw std::runtime_error("Espaço '" + nomeStr + "' tem o campo '" + campoStr + "' repetido");
                }
            }
        }
        espacos.definir(espaco.nome, &espaco);
    }
}

// id do espaço de nome "nome"(erro se não existir).
inline int GeradorIR::obterIdEspaco(const char* nome) {
    int id = espacos.buscar(nome);
    if(id < 0) {
        std::string nomeStr = nome;
        throw std::runtime_error("Espaço '" + nomeStr + "' não foi declarado (faltou #incluir a biblioteca correta?)");
    }
    return id;
}

// posição(em bytes, a partir do endereço base) e tipo do campo "campo" do espaço "idEspaco". Layout calculado na hora, sem guardar: cada
// campo é alinhado à própria largura(larguraDoTipo), então a posição é sempre múltipla da largura. Isso é o que permite reusar
// LEITURA_INDEXADA/ESCRITA_INDEXADA com índice imediato = pos / largura, sem instrução nova no IR nem mudança de acesso no backend.
inline void GeradorIR::posicaoDoCampo(int idEspaco, const char* campo, int* posSaida, TipoVariavel* tipoSaida) {
    const DeclaracaoEspaco& espaco = *espacos.declaracoes[idEspaco];
    int pos = 0;
    for(int i = 0; i < espaco.campos.tam; i++) {
        int largura = larguraDoTipo(espaco.campos[i].tipo);
        pos = ((pos + largura - 1) / largura) * largura;
        if(strcmp(espaco.campos[i].nome, campo) == 0) {
            *posSaida = pos;
            *tipoSaida = espaco.campos[i].tipo;
            return;
        }
        pos += largura;
    }
    std::string nomeStr = espaco.nome;
    std::string campoStr = campo;
    throw std::runtime_error("Espaço '" + nomeStr + "' não tem o campo '" + campoStr + "'");
}

// tamanho total em bytes do espaço, arredondado pro maior alinhamento entre os campos.
inline int GeradorIR::tamanhoDoEspaco(int idEspaco) {
    const DeclaracaoEspaco& espaco = *espacos.declaracoes[idEspaco];
    int pos = 0;
    int maiorLargura = 1;
    for(int i = 0; i < espaco.campos.tam; i++) {
        int largura = larguraDoTipo(espaco.campos[i].tipo);
        pos = ((pos + largura - 1) / largura) * largura + largura;
        if(largura > maiorLargura) maiorLargura = largura;
    }
    return ((pos + maiorLargura - 1) / maiorLargura) * maiorLargura;
}

// resolve "nome.campo" como valor de leitura: emite LEITURA_INDEXADA(base + pos do campo) num slot temporário novo e devolve
// REGISTRADOR com a largura e o sinal do campo. "nome" pode ser um espaço por valor ou um parâmetro/variável "Nome*": nos dois o
// slot guarda o endereço base, então o acesso é o mesmo.
inline ArgumentoIR GeradorIR::resolverAcessoCampo(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ArgumentoIR base;
    if(!ctx.variaveisLocais.buscar(expressao.esquerda->valor, &base)) {
        std::string nomeStr = expressao.esquerda->valor;
        throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
    }
    if(base.idEspaco < 0) {
        std::string nomeStr = expressao.esquerda->valor;
        throw std::runtime_error("Variável '" + nomeStr + "' não é um espaço, não tem campos");
    }
    int posCampo;
    TipoVariavel tipoCampo;
    posicaoDoCampo(base.idEspaco, expressao.valor, &posCampo, &tipoCampo);
    int largura = larguraDoTipo(tipoCampo);

    int slot = ctx.variaveisLocais.proximoSlot++;
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::LEITURA_INDEXADA;
    comandoIR.instrucaoLeituraIndexada.slotDestino = slot;
    comandoIR.instrucaoLeituraIndexada.ponteiro = copiarArgumento(base);
    comandoIR.instrucaoLeituraIndexada.ponteiro.larguraElemento = largura;
    comandoIR.instrucaoLeituraIndexada.indice = imediatoInt(posCampo / largura);
    saida.empurrar(comandoIR);

    ArgumentoIR referenciaSlot;
    referenciaSlot.tipo = ArgumentoIR::Tipo::REGISTRADOR;
    referenciaSlot.simbolo = nullptr;
    referenciaSlot.imediato = 0;
    referenciaSlot.registrador = slot;
    referenciaSlot.larguraBytes = largura;
    referenciaSlot.ehFlutuante = tipoEhFlutuante(tipoCampo);
    referenciaSlot.sinalizado = tipoEhSinalizado(tipoCampo);
    referenciaSlot.elementoSinalizado = elementoEhSinalizado(tipoCampo);
    referenciaSlot.larguraElemento = larguraElementoApontado(tipoCampo);
    return referenciaSlot;
}

// emite ESCRITA_INDEXADA de "valor" no campo(posCampo, tipoCampo) do espaço cujo endereço base está em "base". Usada pela lista de
// inicialização e por "nome.campo = expr;". Imediato é montado direto na largura do campo; registrador mantém a largura de origem.
inline void GeradorIR::emitirEscritaCampo(const ArgumentoIR& base, int posCampo, TipoVariavel tipoCampo, ArgumentoIR valor, Vetor<ComandoIR>& saida) {
    int largura = larguraDoTipo(tipoCampo);
    if(valor.tipo == ArgumentoIR::Tipo::IMEDIATO) valor.larguraBytes = largura;
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::ESCRITA_INDEXADA;
    comandoIR.instrucaoEscritaIndexada.ponteiro = copiarArgumento(base);
    comandoIR.instrucaoEscritaIndexada.ponteiro.larguraElemento = largura;
    comandoIR.instrucaoEscritaIndexada.indice = imediatoInt(posCampo / largura);
    comandoIR.instrucaoEscritaIndexada.valor = valor;
    saida.empurrar(comandoIR);
}

// "nome.campo = expr;"
inline void GeradorIR::gerarAtribuicaoCampo(const AtribuicaoCampo& atr, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ArgumentoIR base;
    if(!ctx.variaveisLocais.buscar(atr.nome, &base)) {
        std::string nomeStr = atr.nome;
        throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
    }
    if(base.idEspaco < 0) {
        std::string nomeStr = atr.nome;
        throw std::runtime_error("Variável '" + nomeStr + "' não é um espaço, não tem campos");
    }
    int posCampo;
    TipoVariavel tipoCampo;
    posicaoDoCampo(base.idEspaco, atr.campo, &posCampo, &tipoCampo);
    ArgumentoIR valor = resolverExpressao(atr.valor, saida, ctx);
    valor = converterParaTipo(valor, tipoEhFlutuante(tipoCampo), larguraDoTipo(tipoCampo), saida, ctx);
    emitirEscritaCampo(base, posCampo, tipoCampo, valor, saida);
}