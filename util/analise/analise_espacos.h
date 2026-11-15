// util/analise/analise_espacos.h
#pragma once
#include "../analisador.h"

// true se o comando/parte do "por" comeca com "Nome nome" ou "Nome* nome": dois identificadores seguidos(ou identificador, '*', identificador) nunca
// formam uma expressao valida no inicio de um comando, entao e sempre uma declaracao de espaco(struct). Nao precisa conhecer os nomes dos #espaco aqui:
// quem confere se o espaco existe é o GeradorIR(que ja enxerga os das bibliotecas incluidas).
inline bool Analisador::ehInicioDeclaracaoEspaco() {
    if(!verificar(TipoToken::IDENTIFICADOR)) return false;
    TipoToken proximo = tokens[pos + 1].tipo;
    if(proximo == TipoToken::IDENTIFICADOR) return true;
    return proximo == TipoToken::VEZES && tokens[pos + 2].tipo == TipoToken::IDENTIFICADOR;
}

// analisa: #espaco Nome { tipo campo; tipo campo; ... }
// campos: int/longo/car/bool/byte/curto, seus ponteiros, "nulo*" e "Outro*"(ponteiro pra outro espaco). Espaco dentro de espaco so por ponteiro.
inline DeclaracaoEspaco Analisador::analisarEspaco() {
    consumir(TipoToken::ESPACO, "esperado #espaco");
    const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome do espaço após #espaco");
    DeclaracaoEspaco espaco;
    espaco.iniciar();
    espaco.nome = strdup(nome.valor);
    consumir(TipoToken::ABRE_CHAVE, "esperado '{' após nome do espaço");
    while(!verificar(TipoToken::FECHA_CHAVE)) {
        CampoEspaco campo;
        if(verificar(TipoToken::NULO)) {
            pos++;
            consumir(TipoToken::VEZES, "esperado '*' apos 'nulo' no campo(nulo sozinho nao e' um tipo de valor valido)");
            campo.tipo = TipoVariavel::NULO_PONTEIRO;
        } else if(verificar(TipoToken::IDENTIFICADOR)) {
            pos++;
            consumir(TipoToken::VEZES, "espaço dentro de espaço só por ponteiro: use 'Nome* campo'");
            campo.tipo = TipoVariavel::ESPACO_PONTEIRO;
        } else {
            campo.tipo = analisarTipoValor("esperado tipo do campo");
        }
        const Token& nomeCampo = consumir(TipoToken::IDENTIFICADOR, "esperado nome do campo");
        campo.nome = strdup(nomeCampo.valor);
        espaco.campos.empurrar(campo);
        consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após campo");
    }
    consumir(TipoToken::FECHA_CHAVE, "esperado '}' para fechar o espaço");
    return espaco;
}

// analisa: #classe Nome { campos e métodos }
// vira um #espaco Nome(só os campos) + uma função "Nome_metodo" por método, com "este"(Nome*) como parâmetro 0, exceto nos "estatico".
// Campo: "tipo nome;". Método: "[estatico] tipoRetorno nome(params) { corpo }". Empurra o espaço e as funções direto em "programa".
inline void Analisador::analisarClasse(Programa& programa) {
    consumir(TipoToken::CLASSE, "esperado #classe");
    const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome da classe após #classe");
    std::string nomeClasse = nome.valor;
    DeclaracaoEspaco espaco;
    espaco.iniciar();
    espaco.nome = strdup(nomeClasse.c_str());
    consumir(TipoToken::ABRE_CHAVE, "esperado '{' após nome da classe");
    while(!verificar(TipoToken::FECHA_CHAVE)) {
        bool ehEstatico = false;
        if(verificar(TipoToken::IDENTIFICADOR) && strcmp(atual().valor, "estatico") == 0) {
            pos++;
            ehEstatico = true;
        }
        TipoRetorno tipo = analisarTipoRetorno();
        const Token& nomeMembro = consumir(TipoToken::IDENTIFICADOR, "esperado nome do campo ou do método");
        if(verificar(TipoToken::ABRE_PARENTESE)) {
            DeclaracaoFuncao funcao;
            funcao.iniciar();
            std::string nomeFuncao = nomeClasse + "_" + nomeMembro.valor;
            funcao.nome = strdup(nomeFuncao.c_str());
            funcao.tipoRetorno = tipo;
            if(!ehEstatico) {
                Parametro este;
                este.tipo = TipoVariavel::ESPACO_PONTEIRO;
                este.nome = strdup("este");
                este.ehVariadico = false;
                este.nomeEspaco = strdup(nomeClasse.c_str());
                funcao.parametros.empurrar(este);
            }
            analisarParametrosECorpo(funcao);
            programa.funcoes.empurrar(funcao);
        } else {
            if(tipo.ehNulo) erro("campo não pode ser 'nulo' (use 'nulo*')");
            if(ehEstatico) erro("'estatico' só vale para método");
            CampoEspaco campo;
            campo.tipo = tipo.tipo;
            campo.nome = strdup(nomeMembro.valor);
            espaco.campos.empurrar(campo);
            consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após campo");
        }
    }
    consumir(TipoToken::FECHA_CHAVE, "esperado '}' para fechar a classe");
    programa.espacos.empurrar(espaco);
}