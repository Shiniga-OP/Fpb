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
            campo.nomeEspaco = strdup(atual().valor);
            pos++;
            consumir(TipoToken::VEZES, "espaço dentro de espaço só por ponteiro: use 'Nome* campo'");
            campo.tipo = TipoVariavel::ESPACO_PONTEIRO;
        } else {
            campo.tipo = analisarTipoValor("esperado tipo do campo");
        }
        const Token& nomeCampo = consumir(TipoToken::IDENTIFICADOR, "esperado nome do campo");
        campo.nome = strdup(nomeCampo.valor);
        espaco.campos.empurrar(campo);
        
        while(verificar(TipoToken::VIRGULA)) {
            pos++;
            const Token& outroNome = consumir(TipoToken::IDENTIFICADOR, "esperado nome do campo após ','");
            CampoEspaco outro;
            outro.tipo = campo.tipo;
            outro.nomeEspaco = campo.nomeEspaco ? strdup(campo.nomeEspaco) : nullptr;
            outro.nome = strdup(outroNome.valor);
            espaco.campos.empurrar(outro);
        }
        consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após campo");
    }
    consumir(TipoToken::FECHA_CHAVE, "esperado '}' para fechar o espaço");
    return espaco;
}

// analisa: #classe Nome [herda Base] { campos e métodos }
// vira um #espaco Nome(só os campos) + uma função "Nome_metodo" por método, com "este"(Nome*) como parâmetro 0, exceto nos "estatico".
// Campo: "tipo nome;". Campo estático: "estatico tipo nome [= constante];"(variável global, não entra no espaço). Método: "[@sobrepor] [estatico] tipoRetorno nome(params) { corpo }".
// Construtor: "Nome(params) { corpo }" vira o método ".criar". Destrutor: "~Nome() { corpo }" vira o método ".liberar".
// Nomes com '#' não podem ser escritos no fonte, então nunca conflitam com um método do usuário.
// Empurra o espaço e as funções direto em "programa".
inline void Analisador::analisarClasse(Programa& programa) {
    consumir(TipoToken::CLASSE, "esperado #classe");
    const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome da classe após #classe");
    std::string nomeClasse = nome.valor;
    DeclaracaoEspaco espaco;
    espaco.iniciar();
    espaco.nome = strdup(nomeClasse.c_str());
    if(verificar(TipoToken::HERDA)) {
        pos++;
        const Token& base = consumir(TipoToken::IDENTIFICADOR, "esperado nome da classe base após 'herda'");
        espaco.base = strdup(base.valor);
    }
    consumir(TipoToken::ABRE_CHAVE, "esperado '{' após nome da classe");
    while(!verificar(TipoToken::FECHA_CHAVE)) {
        bool ehEstatico = false;
        bool ehSobrepor = false;
        bool ehLista = false;
        bool ehFinal = false;
        bool ehRepresentante = false;
        while(true) {
            if(verificar(TipoToken::ARROBA) && tokens[pos + 1].tipo == TipoToken::IDENTIFICADOR && strcmp(tokens[pos + 1].valor, "sobrepor") == 0) {
                pos += 2;
                ehSobrepor = true;
            } else if(verificar(TipoToken::ARROBA) && tokens[pos + 1].tipo == TipoToken::IDENTIFICADOR && strcmp(tokens[pos + 1].valor, "lista") == 0) {
                pos += 2;
                ehLista = true;
            } else if(verificar(TipoToken::ESTATICO)) {
                pos++;
                ehEstatico = true;
            } else if(verificar(TipoToken::FINAL)) {
                pos++;
                ehFinal = true;
            } else if(verificar(TipoToken::REPRESENTANTE)) {
                pos++;
                ehRepresentante = true;
            } else {
                break;
            }
        }
        TipoRetorno tipo;
        tipo.ehNulo = false;
        tipo.tipo = TipoVariavel::INT;
        tipo.nomeEspaco = nullptr;
        std::string nomeMembro;
        bool ehConstrutor = false;
        bool ehDestrutor = false;
        if(verificar(TipoToken::IDENTIFICADOR) && strcmp(atual().valor, nomeClasse.c_str()) == 0 && tokens[pos + 1].tipo == TipoToken::ABRE_PARENTESE) {
            // "Nome(...)": construtor, vira o método ".criar"
            pos++;
            ehConstrutor = true;
            tipo.ehNulo = true;
            nomeMembro = ".criar";
        } else if(verificar(TipoToken::TIL) && tokens[pos + 1].tipo == TipoToken::IDENTIFICADOR && strcmp(tokens[pos + 1].valor, nomeClasse.c_str()) == 0 && tokens[pos + 2].tipo == TipoToken::ABRE_PARENTESE) {
            // "~Nome(...)": destrutor, vira o método ".liberar"
            pos += 2;
            ehDestrutor = true;
            tipo.ehNulo = true;
            nomeMembro = ".liberar";
        } else {
            tipo = analisarTipoRetorno();
            nomeMembro = consumir(TipoToken::IDENTIFICADOR, "esperado nome do campo ou do método").valor;
        }
        if(verificar(TipoToken::ABRE_PARENTESE)) {
            if((ehConstrutor || ehDestrutor) && (ehEstatico || ehSobrepor || ehFinal)) erro("construtor e destrutor não aceitam 'estatico', 'final' nem '@sobrepor'");
            if(ehSobrepor && ehEstatico) erro("'@sobrepor' não vale para método estatico");
            if(ehLista && (ehConstrutor || ehDestrutor || ehEstatico)) erro("'@lista' só vale para método comum(não estatico, construtor nem destrutor)");
            if(ehFinal) erro("'final' não vale para método");
            if(ehRepresentante) erro("'representante' só vale para campo");
            if(!ehEstatico) {
                MetodoDeclarado metodo;
                metodo.nome = strdup(nomeMembro.c_str());
                metodo.ehSobrepor = ehSobrepor;
                metodo.ehLista = ehLista;
                espaco.metodos.empurrar(metodo);
            }
            DeclaracaoFuncao funcao;
            funcao.iniciar();
            std::string nomeFuncao = nomeClasse + "_" + nomeMembro;
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
            if(ehLista && funcao.parametros.tam != 2) erro("'@lista' exige exatamente 1 parâmetro além de 'este'");
            if(ehDestrutor && funcao.parametros.tam != 1) erro("destrutor não recebe parâmetros");
            programa.funcoes.empurrar(funcao);
        } else if(ehEstatico) {
            if(ehSobrepor || ehLista) erro("'@sobrepor' e '@lista' só valem para método");
            if(ehRepresentante) erro("'representante' não vale para campo estatico");
            if(tipo.ehNulo) erro("campo estatico não pode ser 'nulo'");
            bool ehFlu = tipo.tipo == TipoVariavel::FLUTUANTE || tipo.tipo == TipoVariavel::DOBRO;
            if(ehFlu && !ehFinal) erro("campo estatico só aceita flu/dobro com 'final'");
            CampoEstatico estatico;
            estatico.tipo = tipo.tipo;
            estatico.nome = strdup(nomeMembro.c_str());
            estatico.valorInicial = 0;
            estatico.ehFinal = ehFinal;
            if(ehFinal && !verificar(TipoToken::IGUAL)) erro("campo estatico final precisa de valor inicial");
            if(verificar(TipoToken::IGUAL)) {
                pos++;
                bool negativo = false;
                if(verificar(TipoToken::MENOS)) {
                    pos++;
                    negativo = true;
                }
                if(ehFlu) {
                    if(!verificar(TipoToken::NUMERO) && !verificar(TipoToken::DECIMAL)) erro("campo estatico final flu/dobro só aceita valor numérico constante");
                    double d = strtod(atual().valor, nullptr);
                    pos++;
                    if(negativo) d = -d;
                    if(tipo.tipo == TipoVariavel::FLUTUANTE) {
                        float f = (float)d;
                        unsigned int bits;
                        memcpy(&bits, &f, 4);
                        estatico.valorInicial = (long)bits;
                    } else {
                        memcpy(&estatico.valorInicial, &d, 8);
                    }
                } else {
                    const Token& numero = consumir(TipoToken::NUMERO, "campo estatico só aceita valor inicial inteiro constante");
                    estatico.valorInicial = atol(numero.valor);
                    if(negativo) estatico.valorInicial = -estatico.valorInicial;
                }
            }
            espaco.estaticos.empurrar(estatico);
            consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após campo estatico");
        } else {
            if(ehSobrepor || ehLista) erro("'@sobrepor' e '@lista' só valem para método");
            if(ehFinal) erro("'final' em campo de instância não é suportado: use 'estatico final'");
            CampoEspaco campo;
            campo.tipo = tipo.tipo;
            campo.nomeEspaco = tipo.nomeEspaco;
            campo.nome = strdup(nomeMembro.c_str());
            if(ehRepresentante) {
                if(!tipoEhPonteiro(tipo.tipo)) erro("'representante' exige um campo ponteiro(T*)");
                for(int i = 0; i < espaco.campos.tam; i++) {
                    if(espaco.campos[i].ehRepresentante) erro("a classe já tem um campo 'representante'");
                }
                campo.ehRepresentante = true;
            }
            espaco.campos.empurrar(campo);

            while(verificar(TipoToken::VIRGULA)) {
                pos++;
                const Token& outroNome = consumir(TipoToken::IDENTIFICADOR, "esperado nome do campo após ','");
                CampoEspaco outro;
                outro.tipo = tipo.tipo;
                outro.nomeEspaco = tipo.nomeEspaco ? strdup(tipo.nomeEspaco) : nullptr;
                outro.nome = strdup(outroNome.valor);
                espaco.campos.empurrar(outro);
            }
            consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após campo");
        }
    }
    consumir(TipoToken::FECHA_CHAVE, "esperado '}' para fechar a classe");
    programa.espacos.empurrar(espaco);
}