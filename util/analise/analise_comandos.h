// util/analise/analise_comandos.h
#pragma once
#include "../analisador.h"

// analisa um comando dentro do corpo de uma função: declaração de
// variável(int/car*/nulo*), chamada de sistema crua(#chamada_sistema)
// ou chamada de função normal(nome(...)).
inline Comando Analisador::analisarComando() {
    Comando comando;
    bool inicioDeclaracaoNuloPonteiro = verificar(TipoToken::NULO) && tokens[pos + 1].tipo == TipoToken::VEZES;
    if(verificar(TipoToken::INT) || verificar(TipoToken::LONGO) || verificar(TipoToken::CAR) || inicioDeclaracaoNuloPonteiro) {
        comando.tipo = Comando::Tipo::DECLARACAO_VARIAVEL;
        comando.declaracaoVariavel = analisarDeclaracaoVariavel();
        return comando;
    }
    if(verificar(TipoToken::CHAMADA_SISTEMA)) {
        comando.tipo = Comando::Tipo::CHAMADA_SISTEMA_CRUA;
        comando.chamadaSistemaCrua = analisarChamadaSistemaCrua();
        return comando;
    }
    if(verificar(TipoToken::SE)) {
        comando.tipo = Comando::Tipo::SE;
        comando.comandoSe = analisarSe();
        return comando;
    }
    if(verificar(TipoToken::ENQUANTO)) {
        comando.tipo = Comando::Tipo::ENQUANTO;
        comando.comandoEnquanto = analisarEnquanto();
        return comando;
    }
    if(verificar(TipoToken::POR)) {
        comando.tipo = Comando::Tipo::POR;
        comando.comandoPor = analisarPor();
        return comando;
    }
    if(verificar(TipoToken::RETORNE)) {
        comando.tipo = Comando::Tipo::RETORNE;
        comando.comandoRetorne = analisarRetorne();
        return comando;
    }
    if(verificar(TipoToken::IDENTIFICADOR) && tokens[pos + 1].tipo == TipoToken::IGUAL && tokens[pos + 2].tipo == TipoToken::CHAMADA_SISTEMA) {
        comando.tipo = Comando::Tipo::CHAMADA_SISTEMA_CRUA;
        comando.chamadaSistemaCrua = analisarChamadaSistemaComDestino();
        return comando;
    }
    if(verificar(TipoToken::IDENTIFICADOR) && ehTokenReatribuicao(tokens[pos + 1].tipo)) {
        comando.tipo = Comando::Tipo::REATRIBUICAO;
        comando.reatribuicao = analisarReatribuicao();
        return comando;
    }
    if(verificar(TipoToken::IDENTIFICADOR) && tokens[pos + 1].tipo == TipoToken::ABRE_COLCHETE) {
        comando.tipo = Comando::Tipo::ATRIBUICAO_INDEXADA;
        comando.atribuicaoIndexada = analisarAtribuicaoIndexada();
        return comando;
    }
    comando.tipo = Comando::Tipo::CHAMADA_FUNCAO;
    comando.chamadaFuncao = analisarChamadaFuncao();
    return comando;
}

// analisa: nome[indice] = expressao;
inline AtribuicaoIndexada Analisador::analisarAtribuicaoIndexada() {
    AtribuicaoIndexada atr;
    atr.iniciar();
    const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome do ponteiro");
    atr.nome = strdup(nome.valor);
    consumir(TipoToken::ABRE_COLCHETE, "esperado '[' após nome do ponteiro");
    atr.indice = analisarExpressaoAditiva();
    consumir(TipoToken::FECHA_COLCHETE, "esperado ']' após índice");
    consumir(TipoToken::IGUAL, "esperado '=' na atribuicao indexada");
    atr.valor = analisarExpressaoAditiva();
    if(verificar(TipoToken::PONTO_VIRGULA)) {
        consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após atribuição indexada");
    }
    return atr;
}

// analisa: nome = expressao;
inline Reatribuicao Analisador::analisarReatribuicao() {
    Reatribuicao reat;
    reat.iniciar();
    const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome da variável");
    reat.nome = strdup(nome.valor);
    TipoToken op = atual().tipo;
    if(op == TipoToken::IGUAL) {
        pos++;
        reat.valor = analisarExpressaoAditiva();
    } else if(op == TipoToken::MAIS_IGUAL || op == TipoToken::MENOS_IGUAL || op == TipoToken::MAIS_MAIS || op == TipoToken::MENOS_MENOS) {
        // "x += e" vira "x = x + e"; "x++" vira "x = x + 1" (idem pra -= e --): sem nó novo na AST.
        pos++;
        bool soma = (op == TipoToken::MAIS_IGUAL || op == TipoToken::MAIS_MAIS);
        Expressao variavel;
        variavel.tipo = Expressao::Tipo::IDENTIFICADOR;
        variavel.valor = strdup(reat.nome);
        Expressao operando;
        if(op == TipoToken::MAIS_MAIS || op == TipoToken::MENOS_MENOS) {
            operando.tipo = Expressao::Tipo::NUMERO_LITERAL;
            operando.valor = strdup("1");
        } else {
            operando = analisarExpressaoAditiva();
        }
        Expressao binaria;
        binaria.tipo = Expressao::Tipo::BINARIA;
        binaria.valor = nullptr;
        binaria.operadorAritmetico = soma ? OperadorAritmetico::SOMA : OperadorAritmetico::SUBTRACAO;
        binaria.esquerda = new Expressao(variavel);
        binaria.direita = new Expressao(operando);
        reat.valor = binaria;
    } else {
        erro("esperado '=', '+=', '-=', '++' ou '--' na reatribuição");
    }
    if(verificar(TipoToken::PONTO_VIRGULA)) {
        consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após reatribuição");
    }
    return reat;
}

// analisa: "retorne;"(sem valor) ou "retorne expr;"(com valor). A
// checagem de qual forma bate com o tipo de retorno da função(nulo vs
// tipado) fica pro GeradorIR, que ja tem acesso ao tipo da função atual.
inline ComandoRetorne Analisador::analisarRetorne() {
    consumir(TipoToken::RETORNE, "esperado 'retorne'");
    ComandoRetorne cr;
    if(verificar(TipoToken::PONTO_VIRGULA)) {
        cr.temValor = false;
        consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após 'retorne'");
        return cr;
    }
    cr.temValor = true;
    cr.valor = analisarExpressaoAditiva();
    if(verificar(TipoToken::PONTO_VIRGULA)) {
        consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após 'retorne'");
    }
    return cr;
}

// analisa: se(condicao) BLOCO [senao (se(...) BLOCO | BLOCO)]
inline ComandoSe Analisador::analisarSe() {
    consumir(TipoToken::SE, "esperado 'se'");
    consumir(TipoToken::ABRE_PARENTESE, "esperado '(' após 'se'");

    ComandoSe comandoSe;
    comandoSe.iniciar();
    comandoSe.condicao = analisarCondicao("se");
    consumir(TipoToken::FECHA_PARENTESE, "esperado ')' após condição do 'se'");

    analisarBloco(*comandoSe.entao);

    if(verificar(TipoToken::SENAO)) {
        pos++;
        comandoSe.temSenao = true;
        comandoSe.senaoCorpo = new Vetor<Comando>();
        comandoSe.senaoCorpo->iniciar();
        if(verificar(TipoToken::SE)) {
            // "senao se": encadeia como um unico Comando SE dentro do corpo do senao.
            Comando comandoAninhado;
            comandoAninhado.tipo = Comando::Tipo::SE;
            comandoAninhado.comandoSe = analisarSe();
            comandoSe.senaoCorpo->empurrar(comandoAninhado);
        } else {
            analisarBloco(*comandoSe.senaoCorpo);
        }
    }
    return comandoSe;
}

// analisa: enquanto(condicao) BLOCO
inline ComandoEnquanto Analisador::analisarEnquanto() {
    consumir(TipoToken::ENQUANTO, "esperado 'enquanto'");
    consumir(TipoToken::ABRE_PARENTESE, "esperado '(' apos 'enquanto'");

    ComandoEnquanto comandoEnquanto;
    comandoEnquanto.iniciar();
    comandoEnquanto.condicao = analisarCondicao("enquanto");
    consumir(TipoToken::FECHA_PARENTESE, "esperado ')' apos condicao do 'enquanto'");

    analisarBloco(*comandoEnquanto.corpo);
    return comandoEnquanto;
}

inline void Analisador::analisarBloco(Vetor<Comando>& corpo) {
    if(verificar(TipoToken::ABRE_CHAVE)) {
        pos++;
        while(!verificar(TipoToken::FECHA_CHAVE)) {
            corpo.empurrar(analisarComando());
        }
        consumir(TipoToken::FECHA_CHAVE, "esperado '}' para fechar bloco");
    } else {
        corpo.empurrar(analisarComando());
    }
}

// analisa: int nome = expressao; ou car nome = expressao;(caractere) ou car* nome = expressao;(ponteiro de texto)
inline DeclaracaoVariavel Analisador::analisarDeclaracaoVariavel() {
    DeclaracaoVariavel decl;
    decl.iniciar();

    if(verificar(TipoToken::NULO)) {
        pos++;
        consumir(TipoToken::VEZES, "esperado '*' apos 'nulo' na declaração de variável(nulo sozinho nao e' um tipo de valor valido)");
        decl.tipo = TipoVariavel::NULO_PONTEIRO;
    } else if(verificar(TipoToken::INT)) {
        pos++;
        if(verificar(TipoToken::VEZES)) {
            pos++;
            decl.tipo = TipoVariavel::INT_PONTEIRO;
        } else {
            decl.tipo = TipoVariavel::INT;
        }
    } else if(verificar(TipoToken::LONGO)) {
        pos++;
        if(verificar(TipoToken::VEZES)) {
            pos++;
            decl.tipo = TipoVariavel::LONGO_PONTEIRO;
        } else {
            decl.tipo = TipoVariavel::LONGO;
        }
    } else {
        consumir(TipoToken::CAR, "esperado 'int', 'longo' ou 'car' no início da declaração de variável");
        if(verificar(TipoToken::VEZES)) {
            pos++;
            decl.tipo = TipoVariavel::CAR_PONTEIRO;
        } else {
            decl.tipo = TipoVariavel::CAR;
        }
    }
    bool ehArray = false;
    int tamArray = 0;
    if(verificar(TipoToken::ABRE_COLCHETE)) {
        pos++;
        const Token& tam = consumir(TipoToken::NUMERO, "esperado tamanho do array");
        tamArray = atoi(tam.valor);
        consumir(TipoToken::FECHA_COLCHETE, "esperado ']' após tamanho do array");
        ehArray = true;
    }
    const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome da variável");
    decl.nome = strdup(nome.valor);

    if(ehArray) {
        decl.tamArray = tamArray;
        if(verificar(TipoToken::PONTO_VIRGULA)) pos++;
        if(decl.tipo == TipoVariavel::CAR) decl.tipo = TipoVariavel::CAR_PONTEIRO;
        else if(decl.tipo == TipoVariavel::INT) decl.tipo = TipoVariavel::INT_PONTEIRO;
        else if(decl.tipo == TipoVariavel::LONGO) decl.tipo = TipoVariavel::LONGO_PONTEIRO;
        else erro("array só aceita car, int ou longo");
        decl.ehArray = true;
        return decl;
    }
    consumir(TipoToken::IGUAL, "esperado '=' após nome da variável");
    if(verificar(TipoToken::CHAMADA_SISTEMA)) {
        decl.ehChamadaSistema = true;
        decl.chamadaSistemaInicial = analisarChamadaSistemaCrua();
        return decl; // analisarChamadaSistemaCrua ja consome o ';' final
    }
    decl.valorInicial = analisarExpressaoAditiva();
    if(verificar(TipoToken::PONTO_VIRGULA)) {
        consumir(TipoToken::PONTO_VIRGULA, "esperado ';' apos declaração de variável");
    }
    return decl;
}

// analisa os argumentos de "#chamada_sistema id, arg1, arg2, ..." SEM consumir o ';' final: usada por
// analisarChamadaSistemaCrua (comando de topo, consome o ';') e por analisarExpressaoPrimaria (chamada embutida em
// outra construção, ex: "retorne #chamada_sistema ...;", onde quem fecha com ';' é o "retorne").
inline ChamadaSistemaCrua Analisador::analisarNucleoChamadaSistema() {
    consumir(TipoToken::CHAMADA_SISTEMA, "esperado #chamada_sistema");
    ChamadaSistemaCrua chamada;
    chamada.iniciar();

    chamada.identificadores.empurrar(analisarExpressaoPrimaria());
    while(verificar(TipoToken::VIRGULA)) {
        pos++;
        chamada.identificadores.empurrar(analisarExpressaoPrimaria());
    }
    return chamada;
}

// analisa: #chamada_sistema id, arg1, arg2, ...; COMO COMANDO DE TOPO(consome o ';' final). Ver analisarNucleoChamadaSistema() pra
// versão sem ';', usada quando a chamada de sistema aparece embutida
// dentro de outra expressao.
inline ChamadaSistemaCrua Analisador::analisarChamadaSistemaCrua() {
    ChamadaSistemaCrua chamada = analisarNucleoChamadaSistema();
    consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após #chamada_sistema");
    return chamada;
}

// analisa: nome = #chamada_sistema id, arg1, arg2, ...;
// captura o valor devolvido em x0(apos o "svc") na variavel "nome", ja declarada antes. Usada por chamadas de sistema que retornam
// um valor de verdade, ex: o ponteiro alocado por mmap. Mesma gramatica de argumentos de analisarChamadaSistemaCrua, so com o destino na frente.
inline ChamadaSistemaCrua Analisador::analisarChamadaSistemaComDestino() {
    const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome da variável de destino");
    consumir(TipoToken::IGUAL, "esperado '=' antes de #chamada_sistema");
    ChamadaSistemaCrua chamada = analisarChamadaSistemaCrua();
    chamada.temDestino = true;
    chamada.nomeDestino = strdup(nome.valor);
    return chamada;
}

inline ChamadaFuncao Analisador::analisarChamadaFuncao() {
    const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado chamada de função");
    ChamadaFuncao chamada;
    chamada.iniciar();
    chamada.nome = strdup(nome.valor);

    consumir(TipoToken::ABRE_PARENTESE, "esperado '(' após nome da função chamada");
    while(!verificar(TipoToken::FECHA_PARENTESE)) {
        chamada.argumentos.empurrar(analisarExpressaoPrimaria());
        if(verificar(TipoToken::VIRGULA)) pos++;
    }
    consumir(TipoToken::FECHA_PARENTESE, "esperado ')' após argumentos");
    if(verificar(TipoToken::PONTO_VIRGULA)) {
        consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após chamada de função");
    }
    return chamada;
}

// true se "tipo" pode vir logo depois do nome numa reatribuição: =, +=, -=, ++, --.
inline bool Analisador::ehTokenReatribuicao(TipoToken tipo) {
    return tipo == TipoToken::IGUAL || tipo == TipoToken::MAIS_IGUAL || tipo == TipoToken::MENOS_IGUAL || tipo == TipoToken::MAIS_MAIS || tipo == TipoToken::MENOS_MENOS;
}

// uma parte do cabeçalho do "por", empurrada em "destino": inicialização (declaração ou
// reatribuição) quando ehInicio, ou passo (só reatribuição: "i = i + 1", "i += 1", "i++").
inline void Analisador::analisarPartePor(Vetor<Comando>& destino, bool ehInicio) {
    Comando comando;
    bool comecaComTipo = verificar(TipoToken::INT) || verificar(TipoToken::LONGO) || verificar(TipoToken::CAR) || (verificar(TipoToken::NULO) && tokens[pos + 1].tipo == TipoToken::VEZES);
    if(ehInicio && comecaComTipo) {
        comando.tipo = Comando::Tipo::DECLARACAO_VARIAVEL;
        comando.declaracaoVariavel = analisarDeclaracaoVariavel();
    } else if(verificar(TipoToken::IDENTIFICADOR) && ehTokenReatribuicao(tokens[pos + 1].tipo)) {
        comando.tipo = Comando::Tipo::REATRIBUICAO;
        comando.reatribuicao = analisarReatribuicao();
    } else if(ehInicio) {
        erro("esperado declaração ou reatribuição na inicialização do 'por'");
    } else {
        erro("esperado reatribuição (i = i + 1, i += 1, i++) no passo do 'por'");
    }
    destino.empurrar(comando);
}

// analisa: por(inicio; condicao; passo) BLOCO. Inicio e passo podem ser vazios.
inline ComandoPor Analisador::analisarPor() {
    consumir(TipoToken::POR, "esperado 'por'");
    consumir(TipoToken::ABRE_PARENTESE, "esperado '(' após 'por'");

    ComandoPor comandoPor;
    comandoPor.iniciar();
    if(!verificar(TipoToken::PONTO_VIRGULA)) analisarPartePor(*comandoPor.inicio, true);
    // declaração/reatribuição já consomem o ';' sozinhas(é opcional nelas): só exige se não veio
    if(tokens[pos - 1].tipo != TipoToken::PONTO_VIRGULA) consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após a inicialização do 'por'");

    comandoPor.condicao = analisarCondicao("por");
    consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após a condição do 'por'");

    if(!verificar(TipoToken::FECHA_PARENTESE)) analisarPartePor(*comandoPor.passo, false);
    consumir(TipoToken::FECHA_PARENTESE, "esperado ')' após o passo do 'por'");

    analisarBloco(*comandoPor.corpo);
    return comandoPor;
}