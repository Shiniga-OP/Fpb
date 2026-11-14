// util/analise/analise_base.h
#pragma once
#include "../analisador.h"

inline Programa Analisador::analisar() {
    Programa programa;
    programa.iniciar();
    while(!verificar(TipoToken::FIM_ARQUIVO)) {
        if(verificar(TipoToken::INCLUIR)) {
            programa.inclusoes.empurrar(analisarInclusao());
        } else if(verificar(TipoToken::NULO) || ehTokenTipoValor(atual().tipo)) {
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

// analisa: nulo nome(...) { corpo }
// ou: int/car/car* nome(...) { corpo }(funcao tipada, exige 'retorne' no corpo)
// corpo pode conter #chamada_sistema cru(direto no corpo) pra chamar o sistema.
inline DeclaracaoFuncao Analisador::analisarFuncao() {
    TipoRetorno tipoRetorno = analisarTipoRetorno();
    const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome da função");

    DeclaracaoFuncao funcao;
    funcao.iniciar();
    funcao.nome = strdup(nome.valor);
    funcao.tipoRetorno = tipoRetorno;

    consumir(TipoToken::ABRE_PARENTESE, "esperado '(' após nome da função");
    while(!verificar(TipoToken::FECHA_PARENTESE)) {
        funcao.parametros.empurrar(analisarParametro());
        if(verificar(TipoToken::VIRGULA)) pos++;
    }
    consumir(TipoToken::FECHA_PARENTESE, "esperado ')' apos parâmetros");

    consumir(TipoToken::ABRE_CHAVE, "esperado '{' para iniciar corpo da função");
    while(!verificar(TipoToken::FECHA_CHAVE)) {
        funcao.corpo.empurrar(analisarComando());
    }
    consumir(TipoToken::FECHA_CHAVE, "esperado '}' para fechar corpo da função");

    return funcao;
}

// analisa o tipo de retorno no início de uma declaração de função:
// "nulo"(sem retorno), "nulo*"(ponteiro generico), int/int*(32 bits, elemento de 4 bytes), longo/longo*(64 bits, elemento de 8 bytes) ou car/car*(1 byte).
inline TipoRetorno Analisador::analisarTipoRetorno() {
    TipoRetorno tr;
    if(verificar(TipoToken::NULO)) {
        pos++;
        if(verificar(TipoToken::VEZES)) {
            pos++;
            tr.ehNulo = false;
            tr.tipo = TipoVariavel::NULO_PONTEIRO;
            return tr;
        }
        tr.ehNulo = true;
        tr.tipo = TipoVariavel::INT; // valor irrelevante, ehNulo == true
        return tr;
    }
    tr.ehNulo = false;
    tr.tipo = analisarTipoValor("esperado 'nulo', 'int', 'longo', 'car', 'bool', 'byte' ou 'curto' no tipo de retorno da função");
    return tr;
}

// analisa um parâmetro formal: int nome, int* nome(ponteiro de 8 bytes), car nome(caractere), car* nome(ponteiro de texto) ou nulo* nome(ponteiro generico)
inline Parametro Analisador::analisarParametro() {
    Parametro par;
    if(verificar(TipoToken::NULO)) {
        pos++;
        consumir(TipoToken::VEZES, "esperado '*' apos 'nulo' no tipo do parametro(nulo sozinho nao e' um tipo de valor valido)");
        par.tipo = TipoVariavel::NULO_PONTEIRO;
    } else {
        par.tipo = analisarTipoValor("esperado 'int', 'longo', 'car', 'bool', 'byte' ou 'curto' no tipo do parametro");
    }
    const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome do parametro");
    par.nome = strdup(nome.valor);
    return par;
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