// util/analise/analise_funcoes.h
#pragma once
#include "../analisador.h"

// analisa os parâmetros "(...)" e o corpo "{ ... }" de uma função, a partir do '('. Usada por analisarFuncao e pelos métodos de #classe.
inline void Analisador::analisarParametrosECorpo(DeclaracaoFuncao& funcao) {
    consumir(TipoToken::ABRE_PARENTESE, "esperado '(' após nome da função");
    while(!verificar(TipoToken::FECHA_PARENTESE)) {
        if(funcao.ehVariadica) erro("'...' só pode ser o último parâmetro");
        Parametro parametro = analisarParametro();
        if(parametro.ehVariadico) funcao.ehVariadica = true;
        funcao.parametros.empurrar(parametro);
        if(verificar(TipoToken::VIRGULA)) pos++;
    }
    consumir(TipoToken::FECHA_PARENTESE, "esperado ')' apos parâmetros");
    
    if(!verificar(TipoToken::ABRE_CHAVE) || verificar(TipoToken::PONTO_VIRGULA)) {
        pos++;
        funcao.ehExterna = true;
        return;
    }
    consumir(TipoToken::ABRE_CHAVE, "esperado '{' para iniciar corpo da função");
    while(!verificar(TipoToken::FECHA_CHAVE)) {
        funcao.corpo.empurrar(analisarComando());
    }
    consumir(TipoToken::FECHA_CHAVE, "esperado '}' para fechar corpo da função");
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
    analisarParametrosECorpo(funcao);
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
    if(verificar(TipoToken::IDENTIFICADOR)) {
        // "Nome*": ponteiro pra espaco(struct). O nome do espaço fica em nomeEspaco, pra cadeia a.b.c saber o tipo do retorno.
        tr.nomeEspaco = strdup(atual().valor);
        pos++;
        consumir(TipoToken::VEZES, "espaço só pode ser retornado por ponteiro: use 'Nome*'");
        tr.ehNulo = false;
        tr.tipo = TipoVariavel::ESPACO_PONTEIRO;
        return tr;
    }
    tr.ehNulo = false;
    tr.tipo = analisarTipoValor("esperado 'nulo', 'int', 'longo', 'car', 'bool', 'byte' ou 'curto' no tipo de retorno da função");
    return tr;
}

// analisa um parâmetro formal: Nome nome(espaço por valor), Nome* nome(ponteiro pra espaço), int nome, int* nome(ponteiro de 8 bytes), car nome(caractere), car* nome(ponteiro de texto), nulo* nome(ponteiro generico) ou longo... nome(variádico, só como último)
inline Parametro Analisador::analisarParametro() {
    Parametro par;
    if(verificar(TipoToken::NULO)) {
        pos++;
        consumir(TipoToken::VEZES, "esperado '*' apos 'nulo' no tipo do parametro(nulo sozinho nao e' um tipo de valor valido)");
        par.tipo = TipoVariavel::NULO_PONTEIRO;
    } else if(verificar(TipoToken::IDENTIFICADOR)) {
        par.nomeEspaco = strdup(atual().valor);
        pos++;
        // "Nome nome" = espaço por valor(o chamador passa uma cópia); "Nome* nome" = ponteiro pra espaço.
        if(verificar(TipoToken::VEZES)) {
            pos++;
            par.tipo = TipoVariavel::ESPACO_PONTEIRO;
        } else {
            par.tipo = TipoVariavel::ESPACO;
        }
    } else {
        par.tipo = analisarTipoValor("esperado 'int', 'longo', 'car', 'bool', 'byte' ou 'curto' no tipo do parametro");
    }
    par.ehVariadico = false;
    if(verificar(TipoToken::RETICENCIAS)) {
        // "longo... nome": cada argumento extra da chamada ocupa 8 bytes num array, e dentro da função "nome" é um longo*
        if(par.tipo != TipoVariavel::LONGO) erro("'...' só aceita o tipo 'longo' (cada argumento extra ocupa 8 bytes), ex: longo... args");
        pos++;
        par.ehVariadico = true;
        par.tipo = TipoVariavel::LONGO_PONTEIRO;
    }
    const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome do parametro");
    par.nome = strdup(nome.valor);
    return par;
}