// util/analise/analise_base.h
#pragma once
#include "../analisador.h"

inline Programa Analisador::analisar() {
    Programa programa;
    programa.iniciar();
    while(!verificar(TipoToken::FIM_ARQUIVO)) {
        if(verificar(TipoToken::INCLUIR)) {
            programa.inclusoes.empurrar(analisarInclusao());
        } else if(verificar(TipoToken::ESPACO)) {
            programa.espacos.empurrar(analisarEspaco());
        } else if(verificar(TipoToken::CLASSE)) {
            analisarClasse(programa);
        } else if(verificar(TipoToken::NULO) || ehTokenTipoValor(atual().tipo) || (verificar(TipoToken::IDENTIFICADOR) && tokens[pos + 1].tipo == TipoToken::VEZES)) {
            programa.funcoes.empurrar(analisarFuncao());
        } else {
            erro("Esperado '#incluir' ou declaração de função");
        }
    }
    return programa;
}

inline const Token& Analisador::atual() {
    return tokens[pos];
}

inline bool Analisador::verificar(TipoToken tipo) {
    return atual().tipo == tipo;
}

inline const Token& Analisador::consumir(TipoToken tipo, const char* mensagemErro) {
    if(!verificar(tipo)) erro(mensagemErro);
    return tokens[pos++];
}

inline void Analisador::erro(const char* mensagem) {
    std::string msg = "Erro de sintaxe na linha " + std::to_string(atual().linha) + ": " + mensagem + " (encontrado '" + atual().valor + "')";
    throw std::runtime_error(msg);
}

inline Inclusao Analisador::analisarInclusao() {
    consumir(TipoToken::INCLUIR, "esperado #incluir");
    const Token& caminho = consumir(TipoToken::TEXTO, "esperado caminho entre aspas após #incluir");
    if(verificar(TipoToken::PONTO_VIRGULA)) {
        consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após declaração #incluir");
    }
    Inclusao inc;
    inc.caminho = strdup(caminho.valor); // copia propria: tokens serão liberados a parte
    return inc;
}

inline bool Analisador::ehTokenTipoValor(TipoToken tipo) {
    return tipo == TipoToken::INT || tipo == TipoToken::LONGO || tipo == TipoToken::CAR || tipo == TipoToken::BOOL || tipo == TipoToken::BYTE || tipo == TipoToken::CURTO;
}

// consome um tipo de valor(int, longo, car, bool, byte, curto) seguido de "*" opcional, e devolve o TipoVariavel correspondente. "nulo" não entra aqui: cada chamador trata antes.
inline TipoVariavel Analisador::analisarTipoValor(const char* mensagemErro) {
    TipoToken base = atual().tipo;
    if(!ehTokenTipoValor(base)) erro(mensagemErro);
    pos++;
    TipoVariavel tipo;
    if(base == TipoToken::INT) tipo = TipoVariavel::INT;
    else if(base == TipoToken::LONGO) tipo = TipoVariavel::LONGO;
    else if(base == TipoToken::CAR) tipo = TipoVariavel::CAR;
    else if(base == TipoToken::BOOL) tipo = TipoVariavel::BOOL;
    else if(base == TipoToken::BYTE) tipo = TipoVariavel::BYTE;
    else tipo = TipoVariavel::CURTO;
    if(verificar(TipoToken::VEZES)) {
        pos++;
        return ponteiroDoTipo(tipo);
    }
    return tipo;
}