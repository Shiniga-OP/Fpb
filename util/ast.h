// util/ast.h
#pragma once
#include <stdlib.h>
#include "config.h"

// operador aritmético ou de bit usado numa expressão binária(a + b, a - b, a / b, a % b, a & b, a | b, a ^ b, a << b, a >> b).
enum class OperadorAritmetico { SOMA, SUBTRACAO, MULTIPLICACAO, DIVISAO, MODULO, E_BIT, OU_BIT, XOU, DESL_ESQ, DESL_DIR };

// tipo declarado de uma variável: int(32 bits, 4 bytes), car(caractere/byte de 8 bits), longo(64 bits, 8 bytes) ou os respectivos ponteiros(car*, int*, longo*, sempre 8 bytes de endereco, mas indexando elementos de largura diferente). Declarado aqui em cima pois CONVERSAO referencia o tipo alvo.
enum class TipoVariavel { INT, CAR, CAR_PONTEIRO, LONGO, LONGO_PONTEIRO, INT_PONTEIRO, NULO_PONTEIRO, BOOL, BOOL_PONTEIRO, BYTE, BYTE_PONTEIRO, CURTO, CURTO_PONTEIRO, ESPACO, ESPACO_PONTEIRO, FLUTUANTE, FLUTUANTE_PONTEIRO, DOBRO, DOBRO_PONTEIRO };

// devolve o ponteiro do tipo de valor dado(ex: INT -> INT_PONTEIRO). Tipo que não tem ponteiro correspondente(ou já é ponteiro) volta igual.
inline TipoVariavel ponteiroDoTipo(TipoVariavel tipo) {
    if(tipo == TipoVariavel::INT) return TipoVariavel::INT_PONTEIRO;
    if(tipo == TipoVariavel::CAR) return TipoVariavel::CAR_PONTEIRO;
    if(tipo == TipoVariavel::LONGO) return TipoVariavel::LONGO_PONTEIRO;
    if(tipo == TipoVariavel::BOOL) return TipoVariavel::BOOL_PONTEIRO;
    if(tipo == TipoVariavel::BYTE) return TipoVariavel::BYTE_PONTEIRO;
    if(tipo == TipoVariavel::CURTO) return TipoVariavel::CURTO_PONTEIRO;
    if(tipo == TipoVariavel::FLUTUANTE) return TipoVariavel::FLUTUANTE_PONTEIRO;
    if(tipo == TipoVariavel::DOBRO) return TipoVariavel::DOBRO_PONTEIRO;
    return tipo;
}

// true se "tipo" é um ponteiro(car*, int*, ..., nulo*, Nome*). Arrays declarados já chegam aqui com o tipo ponteiro.
inline bool tipoEhPonteiro(TipoVariavel tipo) {
    return tipo == TipoVariavel::CAR_PONTEIRO || tipo == TipoVariavel::LONGO_PONTEIRO || tipo == TipoVariavel::INT_PONTEIRO || tipo == TipoVariavel::NULO_PONTEIRO || tipo == TipoVariavel::BOOL_PONTEIRO || tipo == TipoVariavel::BYTE_PONTEIRO || tipo == TipoVariavel::CURTO_PONTEIRO || tipo == TipoVariavel::ESPACO_PONTEIRO || tipo == TipoVariavel::FLUTUANTE_PONTEIRO || tipo == TipoVariavel::DOBRO_PONTEIRO;
}

// tipo de retorno de uma função: "nulo" continua significando "sem retorno"(mesma palavra chave usada hoje pra declarar função), os demais reusam TipoVariavel pra não duplicar o mesmo espaco de tipos.
struct TipoRetorno {
    bool ehNulo;
    TipoVariavel tipo; // valido so se ehNulo == false
};

// propagação: Expressao e ela mesma(recursiva) numa BINARIA ou CONVERSAO.
// esquerda/direita sao guardados por ponteiro(alocados no heap) pra quebrar
// a recursão de tamanho, mesma solução usada em ComandoSe/InstrucaoSe. Em
// CONVERSAO(conversão, ex: (car)45), só "esquerda" e usado: guarda a subexpressao
// sendo convertida, e "tipoConversao" guarda o tipo alvo da conversão.
// propagação: Expressao precisa de ChamadaSistemaCrua*(usada quando um
// "#chamada_sistema" aparece embutido dentro de outra expressao, ex:
// "retorne #chamada_sistema ...;"), mas ChamadaSistemaCrua guarda um
// Vetor<Expressao> dela mesma. Forward declaration aqui quebra o ciclo:
// Expressao só precisa do PONTEIRO(alocado com new), a definição completa
// de ChamadaSistemaCrua continua vindo depois, ja que só ai ela pode usar Expressao por valor dentro do Vetor.
struct ChamadaSistemaCrua;
struct Condicao;

struct Expressao {
    enum class Tipo {
        TEXTO_LITERAL, IDENTIFICADOR,
        NUMERO_LITERAL, DECIMAL_LITERAL,
        BINARIA, CONVERSAO,
        CHAMADA_FUNCAO, BYTES,
        INDEXACAO, CHAMADA_SISTEMA,
        ACESSO_CAMPO, ENDERECO,
        TERNARIA
    } tipo;
    char* valor; // dono: quem criou a expressao(strdup). usado quando tipo != BINARIA e tipo != CONVERSAO e tipo != CHAMADA_FUNCAO e tipo != BYTES e tipo != INDEXACAO e tipo != CHAMADA_SISTEMA
    OperadorAritmetico operadorAritmetico; // usado quando tipo == BINARIA
    TipoVariavel tipoConversao; // usado quando tipo == CONVERSAO: tipo alvo da conversão, ex: (car) ou (int)
    Expressao* esquerda; // dono: quem criou a expressao(alocado com new). valido se tipo == BINARIA(operando esquerdo), CONVERSAO(subexpressao convertida) ou INDEXACAO(ponteiro sendo indexado, ex: "txt" em txt[i])
    Expressao* direita; // dono: idem. válido se tipo == BINARIA ou INDEXACAO(indice, ex: "i" em txt[i])
    char* nomeFuncao; // dono: quem criou a expressao(strdup). usado quando tipo == CHAMADA_FUNCAO
    Vetor<Expressao>* argumentosFuncao; // dono: quem criou a expressao(alocado com new). usado quando tipo == CHAMADA_FUNCAO
    bool bytesEhTipoLiteral; // usado quando tipo == BYTES: true se veio de bytes(int/car/car*), false se veio de bytes(variavel)
    TipoVariavel bytesTipoLiteral; // usado quando tipo == BYTES e bytesEhTipoLiteral == true
    char* bytesNomeVariavel; // dono: quem criou a expressao(strdup). usado quando tipo == BYTES e bytesEhTipoLiteral == false
    bool enderecoDeValor; // usado quando tipo == ENDERECO: true em "@(expr)"(o valor de expr é tratado como endereço), false em "@nome"(endereço da variável "nome"). "esquerda" guarda expr ou o IDENTIFICADOR
    ChamadaSistemaCrua* chamadaSistema; // dono: quem criou a expressao(alocado com new). usado quando tipo == CHAMADA_SISTEMA: "#chamada_sistema id, arg1, ...;" usada como EXPRESSAO(ex: "retorne #chamada_sistema ...;", "escrever(#chamada_sistema ...)", dentro de "se(#chamada_sistema ... == 0)"). O valor da expressao e' o retorno da syscall(x0), igual "nome = #chamada_sistema ..." na declaracao/reatribuicao, so que aqui embutido dentro de outra expressao maior em vez de sozinho num comando.
    Condicao* condicaoTernaria; // dono: quem criou a expressao(alocado com new). usado quando tipo == TERNARIA: "esquerda" é o valor se verdadeira e "direita" o valor se falsa
    
    void liberar() {
        free(valor);
        valor = nullptr;
        if(tipo == Tipo::BINARIA) {
            esquerda->liberar();
            delete esquerda;
            esquerda = nullptr;
            direita->liberar();
            delete direita;
            direita = nullptr;
        } else if(tipo == Tipo::CONVERSAO || tipo == Tipo::ACESSO_CAMPO || tipo == Tipo::ENDERECO) { // ACESSO_CAMPO: esquerda = base(IDENTIFICADOR), valor = nome do campo(strdup)
            esquerda->liberar();
            delete esquerda;
            esquerda = nullptr;
        } else if(tipo == Tipo::INDEXACAO) {
            esquerda->liberar();
            delete esquerda;
            esquerda = nullptr;
            direita->liberar();
            delete direita;
            direita = nullptr;
        } else if(tipo == Tipo::CHAMADA_FUNCAO) {
            free(nomeFuncao);
            nomeFuncao = nullptr;
            for(int i = 0; i < argumentosFuncao->tam; i++) (*argumentosFuncao)[i].liberar();
            argumentosFuncao->liberar();
            delete argumentosFuncao;
            argumentosFuncao = nullptr;
        } else if(tipo == Tipo::BYTES) {
            free(bytesNomeVariavel);
            bytesNomeVariavel = nullptr;
        } else if(tipo == Tipo::CHAMADA_SISTEMA) {
            liberarChamadaSistemaDeExpressao();
        } else if(tipo == Tipo::TERNARIA) {
            liberarCondicaoTernaria();
            esquerda->liberar();
            delete esquerda;
            esquerda = nullptr;
            direita->liberar();
            delete direita;
            direita = nullptr;
        }
    }
    void liberarChamadaSistemaDeExpressao(); // implementada apos ChamadaSistemaCrua estar definida por completo(ver mais abaixo no arquivo)
    void liberarCondicaoTernaria(); // implementada apos Condicao estar definida por completo
};

// uma chamada de funcao dentro de um corpo: escrever("Ola\n");
struct ChamadaFuncao {
    char* nome; // dono: quem criou a chamada(strdup)
    Vetor<Expressao> argumentos;

    void iniciar() {
        nome = nullptr;
        argumentos.iniciar();
    }
    void liberar() {
        free(nome);
        nome = nullptr;
        for(int i = 0; i < argumentos.tam; i++) argumentos[i].liberar();
        argumentos.liberar();
    }
};

// chamada de sistema crua dentro do corpo: #chamada_sistema número, saida, msg, tamanho_msg;
// ou, com captura de retorno: nome = #chamada_sistema número, arg1, arg2, ...;
// o primeiro identificador apos o "id" e o numero da chamada de sistema, os demais sao os argumentos em ordem, na mesma ordem da syscall real.
struct ChamadaSistemaCrua {
    Vetor<Expressao> identificadores; // dono: quem criou a chamada
    bool temDestino; // true se a forma "nome = #chamada_sistema ..." foi usada
    char* nomeDestino; // dono: quem criou a chamada(strdup). valido so se temDestino == true

    void iniciar() {
        identificadores.iniciar();
        temDestino = false;
        nomeDestino = nullptr;
    }
    void liberar() {
        for(int i = 0; i < identificadores.tam; i++) identificadores[i].liberar();
        identificadores.liberar();
        free(nomeDestino);
        nomeDestino = nullptr;
    }
};

// definida so agora, pois precisa de ChamadaSistemaCrua completa(ver
// forward declaration e comentario junto de Expressao::chamadaSistema no
// topo do arquivo: quebra a dependencia circular Expressao<->ChamadaSistemaCrua).
inline void Expressao::liberarChamadaSistemaDeExpressao() {
    chamadaSistema->liberar();
    delete chamadaSistema;
    chamadaSistema = nullptr;
}

// declaracao de variável com valor inicial: int numero = 1; ou car* msg = "ola"; ou car letra = (car)65;
// ou, com valor vindo de chamada de sistema: nulo* p = #chamada_sistema 3, ...;(ver ehChamadaSistema)
struct DeclaracaoVariavel {
    TipoVariavel tipo;
    char* nome; // dono: quem criou a declaracao(strdup)
    Expressao valorInicial; // valido so se ehChamadaSistema == false
    bool ehChamadaSistema; // true se "tipo nome = #chamada_sistema ...;" foi usada(valor inicial vem do retorno da syscall, x0, em vez de uma expressao normal)
    ChamadaSistemaCrua chamadaSistemaInicial; // valido so se ehChamadaSistema == true

    bool ehArray;
    int tamArray;
    char* nomeEspaco; // dono: quem criou a declaracao(strdup). nome do #espaco quando tipo == ESPACO ou ESPACO_PONTEIRO, senao nullptr
    Vetor<Expressao> listaInicial; // usado so quando tipo == ESPACO e temLista == true: "Nome var = { e1, e2 };"
    bool temLista;
    
    void iniciar() {
        nome = nullptr;
        ehChamadaSistema = false;
        ehArray = false;
        tamArray = 0;
        nomeEspaco = nullptr;
        listaInicial.iniciar();
        temLista = false;
    }
    
    void liberar() {
        free(nome);
        nome = nullptr;
        free(nomeEspaco);
        nomeEspaco = nullptr;
        if(tipo == TipoVariavel::ESPACO) {
            for(int i = 0; i < listaInicial.tam; i++) listaInicial[i].liberar();
            listaInicial.liberar();
        } else if(ehChamadaSistema) chamadaSistemaInicial.liberar();
        else if(!ehArray) valorInicial.liberar();
    }
};

// operador de comparacao usado na condição de um "se".
enum class OperadorComparacao { IGUAL, DIFERENTE, MAIOR, MENOR, MAIOR_IGUAL, MENOR_IGUAL };

// condicao de um "se"/"enquanto". COMPARACAO: esquerda OP direita, ex: num == num2.
// E / OU: combinacao de duas condicoes("&&" / "||"), guardadas por ponteiro(alocadas no heap) pra quebrar a recursao de tamanho.
struct Condicao {
    enum class Tipo { COMPARACAO, E, OU } tipo;
    Expressao esquerda; // so COMPARACAO
    OperadorComparacao operador; // so COMPARACAO
    Expressao direita; // so COMPARACAO
    Condicao* condEsquerda = nullptr; // dono: quem criou a condicao(alocado com new). so E/OU
    Condicao* condDireita = nullptr; // dono: idem. so E/OU

    void liberar() {
        if(tipo == Tipo::COMPARACAO) {
            esquerda.liberar();
            direita.liberar();
        } else {
            condEsquerda->liberar();
            delete condEsquerda;
            condEsquerda = nullptr;
            condDireita->liberar();
            delete condDireita;
            condDireita = nullptr;
        }
    }
};

inline void Expressao::liberarCondicaoTernaria() {
    condicaoTernaria->liberar();
    delete condicaoTernaria;
    condicaoTernaria = nullptr;
}

// reatribuicao de uma variável já declarada: nome = expr;
struct Reatribuicao {
    char* nome; // dono: quem criou a reatribuição(strdup)
    Expressao valor;

    void iniciar() {
        nome = nullptr;
    }
    void liberar() {
        free(nome);
        nome = nullptr;
        valor.liberar();
    }
};

// escrita indexada num ponteiro ja declarado: nome[indice] = expr;
// ex: txt[i] = 65; grava 1 byte(car) no endereco nome+indice.
struct AtribuicaoIndexada {
    char* nome; // dono: quem criou a atribuicao(strdup). nome do ponteiro(car*) sendo indexado
    Expressao indice;
    Expressao valor;

    void iniciar() {
        nome = nullptr;
    }
    void liberar() {
        free(nome);
        nome = nullptr;
        indice.liberar();
        valor.liberar();
    }
};

// escrita num campo de um espaco(struct) ja declarado: nome.campo = expr;
// "x.c += e" e "x.c++" viram "x.c = x.c + e" no analisador, igual Reatribuicao.
struct AtribuicaoCampo {
    char* nome; // dono: quem criou a atribuicao(strdup). nome da variavel do espaco
    char* campo; // dono: idem. nome do campo
    Expressao valor;

    void iniciar() {
        nome = nullptr;
        campo = nullptr;
    }
    void liberar() {
        free(nome);
        nome = nullptr;
        free(campo);
        campo = nullptr;
        valor.liberar();
    }
};

// propagação: Comando e ComandoSe se referenciam mutuamente. Os corpos de
// ComandoSe sao guardados por ponteiro(alocados no heap) pra quebrar a
// recursao de tamanho: Vetor<Comando> dentro de Comando não compila,
// pois Comando ainda estaria incompleto no momento da definicao.
struct Comando;

// comando "se(condicao) { entao } senao { senaoCorpo }". "senao" e
// opcional(temSenao == false). "senao se" e representado com senaoCorpo
// tendo um unico Comando do tipo SE(aninhado).
struct ComandoSe {
    Condicao condicao;
    Vetor<Comando>* entao; // dono: quem criou o ComandoSe(alocado com new)
    Vetor<Comando>* senaoCorpo; // dono: idem. só valido se temSenao == true
    bool temSenao = false;

    void iniciar() {
        entao = new Vetor<Comando>();
        entao->iniciar();
        senaoCorpo = nullptr;
        temSenao = false;
    }
    void liberar();
};

// comando "enquanto(condicao) { corpo }": repete o corpo enquanto a
// condição for verdadeira. Mesma forma de ComandoSe(só sem "senao").
struct ComandoEnquanto {
    Condicao condicao;
    Vetor<Comando>* corpo; // dono: quem criou o ComandoEnquanto(alocado com new)

    void iniciar() {
        corpo = new Vetor<Comando>();
        corpo->iniciar();
    }
    void liberar();
};

// comando "por(inicio; condicao; passo) { corpo }". "inicio" e "passo" têm 0 ou 1 Comando
// (declaração/reatribuição e reatribuição, respectivamente). O GeradorIR desdobra em:
// inicio; enquanto(condicao) { corpo; passo; }
struct ComandoPor {
    Vetor<Comando>* inicio; // dono: quem criou o ComandoPor(alocado com new)
    Condicao condicao;
    Vetor<Comando>* passo; // dono: idem
    Vetor<Comando>* corpo; // dono: idem

    void iniciar() {
        inicio = new Vetor<Comando>();
        inicio->iniciar();
        passo = new Vetor<Comando>();
        passo->iniciar();
        corpo = new Vetor<Comando>();
        corpo->iniciar();
    }
    void liberar();
};

// comando "retorne expr;"(funcao tipada) ou "retorne;"(funcao nulo, sem valor).
struct ComandoRetorne {
    bool temValor;
    Expressao valor; // valido so se temValor == true

    void liberar() {
        if(temValor) valor.liberar();
    }
};

// um comando dentro do corpo de uma função: declaracao de variável,
// chamada de função normal, chamada de sistema crua, "se", "enquanto", "retorne" ou atribuição indexada(txt[i] = valor).
struct Comando {
    enum class Tipo {
        DECLARACAO_VARIAVEL, REATRIBUICAO,
        CHAMADA_FUNCAO, CHAMADA_SISTEMA_CRUA,
        SE, ENQUANTO, POR, RETORNE, ATRIBUICAO_INDEXADA,
        ATRIBUICAO_CAMPO
    } tipo;
    DeclaracaoVariavel declaracaoVariavel;
    Reatribuicao reatribuicao;
    ChamadaFuncao chamadaFuncao;
    ChamadaSistemaCrua chamadaSistemaCrua;
    ComandoSe comandoSe;
    ComandoEnquanto comandoEnquanto;
    ComandoPor comandoPor;
    ComandoRetorne comandoRetorne;
    AtribuicaoIndexada atribuicaoIndexada;
    AtribuicaoCampo atribuicaoCampo;

    void liberar() {
        if(tipo == Tipo::DECLARACAO_VARIAVEL) declaracaoVariavel.liberar();
        else if(tipo == Tipo::REATRIBUICAO) reatribuicao.liberar();
        else if(tipo == Tipo::CHAMADA_FUNCAO) chamadaFuncao.liberar();
        else if(tipo == Tipo::CHAMADA_SISTEMA_CRUA) chamadaSistemaCrua.liberar();
        else if(tipo == Tipo::SE) comandoSe.liberar();
        else if(tipo == Tipo::ENQUANTO) comandoEnquanto.liberar();
        else if(tipo == Tipo::POR) comandoPor.liberar();
        else if(tipo == Tipo::RETORNE) comandoRetorne.liberar();
        else if(tipo == Tipo::ATRIBUICAO_INDEXADA) atribuicaoIndexada.liberar();
        else if(tipo == Tipo::ATRIBUICAO_CAMPO) atribuicaoCampo.liberar();
    }
};

// definida só agora, pois precisa de Comando completo(pra chamar liberar()
// de cada elemento dos vetores "entao"/"senaoCorpo").
inline void ComandoSe::liberar() {
    for(int i = 0; i < entao->tam; i++) (*entao)[i].liberar();
    entao->liberar();
    delete entao;
    entao = nullptr;
    if(senaoCorpo) {
        for(int i = 0; i < senaoCorpo->tam; i++) (*senaoCorpo)[i].liberar();
        senaoCorpo->liberar();
        delete senaoCorpo;
        senaoCorpo = nullptr;
    }
    condicao.liberar();
}

// definida só agora, pelo mesmo motivo de ComandoSe::liberar() acima.
inline void ComandoEnquanto::liberar() {
    for(int i = 0; i < corpo->tam; i++) (*corpo)[i].liberar();
    corpo->liberar();
    delete corpo;
    corpo = nullptr;
    condicao.liberar();
}

inline void liberarBlocoComandos(Vetor<Comando>*& bloco) {
    for(int i = 0; i < bloco->tam; i++) (*bloco)[i].liberar();
    bloco->liberar();
    delete bloco;
    bloco = nullptr;
}

inline void ComandoPor::liberar() {
    liberarBlocoComandos(inicio);
    liberarBlocoComandos(passo);
    liberarBlocoComandos(corpo);
    condicao.liberar();
}

// parâmetro formal de uma função: int nome ou car* nome
struct Parametro {
    TipoVariavel tipo;
    char* nome; // dono: quem criou o parâmetro(strdup)
    bool ehVariadico = false; // true se declarado como "longo... nome": o tipo guardado já é longo*, e a chamada empacota os argumentos extras num array de longo
    char* nomeEspaco = nullptr; // dono: quem criou o parametro(strdup). nome do #espaco quando tipo == ESPACO_PONTEIRO

    void liberar() {
        free(nome);
        nome = nullptr;
        free(nomeEspaco);
        nomeEspaco = nullptr;
    }
};

// declaração de função com corpo normal(ex: inicio)
struct DeclaracaoFuncao {
    char* nome; // dono: quem criou a declaracao(strdup)
    Vetor<Parametro> parametros;
    Vetor<Comando> corpo;
    TipoRetorno tipoRetorno; // "nulo" ou int/car/car*
    bool ehVariadica = false; // true se o último parâmetro é "longo... nome"
    bool ehExterna = false;
    
    void iniciar() {
        nome = nullptr;
        parametros.iniciar();
        corpo.iniciar();
        tipoRetorno.ehNulo = true;
        ehVariadica = false;
        ehExterna = false;
    }
    void liberar() {
        free(nome);
        nome = nullptr;
        for(int i = 0; i < parametros.tam; i++) parametros[i].liberar();
        parametros.liberar();
        for(int i = 0; i < corpo.tam; i++) corpo[i].liberar();
        corpo.liberar();
    }
};

// campo de um #espaco: "int x;" ou "car* ptr;". A posicao em bytes nao fica aqui: o GeradorIR calcula o layout(precisa de larguraDoTipo).
struct CampoEspaco {
    TipoVariavel tipo;
    char* nome; // dono: quem criou o campo(strdup)

    void liberar() {
        free(nome);
        nome = nullptr;
    }
};

// "#espaco Nome { int x; car* ptr; }": tipo composto(struct).
struct DeclaracaoEspaco {
    char* nome; // dono: quem criou a declaracao(strdup)
    Vetor<CampoEspaco> campos;

    void iniciar() {
        nome = nullptr;
        campos.iniciar();
    }
    void liberar() {
        free(nome);
        nome = nullptr;
        for(int i = 0; i < campos.tam; i++) campos[i].liberar();
        campos.liberar();
    }
};

struct Inclusao {
    char* caminho; // dono: quem criou a inclusao(strdup)

    void liberar() {
        free(caminho);
        caminho = nullptr;
    }
};

struct Programa {
    Vetor<Inclusao> inclusoes;
    Vetor<DeclaracaoEspaco> espacos;
    Vetor<DeclaracaoFuncao> funcoes;
    Vetor<Inclusao> ligacoes;
    char* alvo;

    void iniciar() {
        inclusoes.iniciar();
        espacos.iniciar();
        funcoes.iniciar();
        ligacoes.iniciar();
        alvo = nullptr;
    }
    void liberar() {
        for(int i = 0; i < inclusoes.tam; i++) inclusoes[i].liberar();
        inclusoes.liberar();
        for(int i = 0; i < espacos.tam; i++) espacos[i].liberar();
        espacos.liberar();
        for(int i = 0; i < funcoes.tam; i++) funcoes[i].liberar();
        funcoes.liberar();
        for(int i = 0; i < ligacoes.tam; i++) ligacoes[i].liberar();
        ligacoes.liberar();
        free(alvo);
    }
};