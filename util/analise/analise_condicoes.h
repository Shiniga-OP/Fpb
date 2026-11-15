// util/analise/analise_condicoes.h
#pragma once
#include "../analisador.h"

// analisa um dos seis operadores de comparacao aceitos na condicao de
// "se"/"enquanto": ==, !=, >, <, >=, <=. Extraida numa funcao unica pra
// nao duplicar a mesma lista em analisarSe e analisarEnquanto.
inline OperadorComparacao Analisador::analisarOperadorComparacao(const char* nomeConstrucao) {
    if(verificar(TipoToken::IGUAL_IGUAL)) {
        pos++;
        return OperadorComparacao::IGUAL;
    }
    if(verificar(TipoToken::DIFERENTE)) {
        pos++;
        return OperadorComparacao::DIFERENTE;
    }
    if(verificar(TipoToken::MAIOR_IGUAL)) {
        pos++;
        return OperadorComparacao::MAIOR_IGUAL;
    }
    if(verificar(TipoToken::MENOR_IGUAL)) {
        pos++;
        return OperadorComparacao::MENOR_IGUAL;
    }
    if(verificar(TipoToken::MAIOR)) {
        pos++;
        return OperadorComparacao::MAIOR;
    }
    if(verificar(TipoToken::MENOR)) {
        pos++;
        return OperadorComparacao::MENOR;
    }
    std::string msg = std::string("esperado '==', '!=', '>', '<', '>=' ou '<=' na condição do '") + nomeConstrucao + "'";
    erro(msg.c_str());
}

inline bool Analisador::ehTokenComparacao(TipoToken tipo) {
    return tipo == TipoToken::IGUAL_IGUAL || tipo == TipoToken::DIFERENTE || tipo == TipoToken::MAIOR || tipo == TipoToken::MENOR || tipo == TipoToken::MAIOR_IGUAL || tipo == TipoToken::MENOR_IGUAL;
}

// "(" no início de uma condição: agrupamento de condição, "(a == 1 || b == 2)", ou início de uma expressão, "(c % d) == 0" / "(int)x == 0"?
// Conversão de tipo é sempre expressão. Senão, acha o ")" correspondente: se logo depois vem um operador de comparação, é expressão.
inline bool Analisador::ehCondicaoAgrupada() {
    TipoToken seguinte = tokens[pos + 1].tipo;
    if(seguinte == TipoToken::NULO || ehTokenTipoValor(seguinte)) return false;
    int profundidade = 0;
    size_t i = pos;
    while(tokens[i].tipo != TipoToken::FIM_ARQUIVO) {
        if(tokens[i].tipo == TipoToken::ABRE_PARENTESE) profundidade++;
        else if(tokens[i].tipo == TipoToken::FECHA_PARENTESE) {
            profundidade--;
            if(profundidade == 0) break;
        }
        i++;
    }
    if(tokens[i].tipo == TipoToken::FIM_ARQUIVO) return false;
    return !ehTokenComparacao(tokens[i + 1].tipo);
}

inline Condicao Analisador::montarCondicaoLogica(Condicao::Tipo tipo, const Condicao& esq, const Condicao& dir) {
    Condicao c;
    c.tipo = tipo;
    c.condEsquerda = new Condicao(esq);
    c.condDireita = new Condicao(dir);
    return c;
}

// condicao := e ("||" e)*
inline Condicao Analisador::analisarCondicao(const char* nomeConstrucao) {
    Condicao esq = analisarCondicaoE(nomeConstrucao);
    while(verificar(TipoToken::OU_OU)) {
        pos++;
        Condicao dir = analisarCondicaoE(nomeConstrucao);
        esq = montarCondicaoLogica(Condicao::Tipo::OU, esq, dir);
    }
    return esq;
}

// e := base ("&&" base)*
inline Condicao Analisador::analisarCondicaoE(const char* nomeConstrucao) {
    Condicao esq = analisarCondicaoBase(nomeConstrucao);
    while(verificar(TipoToken::TAMBEM_TAMBEM)) {
        pos++;
        Condicao dir = analisarCondicaoBase(nomeConstrucao);
        esq = montarCondicaoLogica(Condicao::Tipo::E, esq, dir);
    }
    return esq;
}

// base := "(" condicao ")" | expressao OP expressao | expressao
inline Condicao Analisador::analisarCondicaoBase(const char* nomeConstrucao) {
    if(verificar(TipoToken::ABRE_PARENTESE) && ehCondicaoAgrupada()) {
        pos++;
        Condicao interna = analisarCondicao(nomeConstrucao);
        consumir(TipoToken::FECHA_PARENTESE, "esperado ')' para fechar a condição agrupada");
        return interna;
    }
    Condicao c;
    c.tipo = Condicao::Tipo::COMPARACAO;
    c.esquerda = analisarExpressaoAditiva();
    if(!ehTokenComparacao(atual().tipo)) {
        // sem operador: "se(sim())" vale "se(sim() != 0)"(qualquer valor diferente de zero é verdadeiro)
        c.operador = OperadorComparacao::DIFERENTE;
        c.direita.tipo = Expressao::Tipo::NUMERO_LITERAL;
        c.direita.valor = strdup("0");
        return c;
    }
    c.operador = analisarOperadorComparacao(nomeConstrucao);
    c.direita = analisarExpressaoAditiva();
    return c;
}