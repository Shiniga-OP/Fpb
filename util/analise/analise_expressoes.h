// util/analise/analise_expressoes.h
#pragma once
#include "../analisador.h"

// analisa: primaria (("+"|"-"|"/"|"%") primária)*  - associativo a esquerda.
// usada so em declaração de variavel e reatribuicao(unicos lugares que
// aceitam aritmetica por enquanto). Monta a AST binaria empilhando pra
// esquerda a cada operador encontrado.
inline Expressao Analisador::analisarExpressaoAditiva() {
    Expressao esquerda = analisarExpressaoPrimaria();
    while(verificar(TipoToken::MAIS) || verificar(TipoToken::MENOS) || verificar(TipoToken::VEZES) || verificar(TipoToken::DIVIDIR) || verificar(TipoToken::PORCENTAGEM)) {
        OperadorAritmetico op;
        if(verificar(TipoToken::MAIS)) op = OperadorAritmetico::SOMA;
        else if(verificar(TipoToken::MENOS)) op = OperadorAritmetico::SUBTRACAO;
        else if(verificar(TipoToken::VEZES)) op = OperadorAritmetico::MULTIPLICACAO;
        else if(verificar(TipoToken::DIVIDIR)) op = OperadorAritmetico::DIVISAO;
        else op = OperadorAritmetico::MODULO;
        pos++;
        Expressao direita = analisarExpressaoPrimaria();

        Expressao binaria;
        binaria.tipo = Expressao::Tipo::BINARIA;
        binaria.valor = nullptr;
        binaria.operadorAritmetico = op;
        binaria.esquerda = new Expressao(esquerda);
        binaria.direita = new Expressao(direita);
        esquerda = binaria;
    }
    return esquerda;
}

// analisa uma conversão de tipo(conversão): (car)expr, (int)expr, (car*)expr
// ou (int*)expr. So chamada quando já detectamos "(" seguido de
// "int/car"(distingue de uma expressão parentesizada comum, ver
// analisarExpressaoParentesizada logo abaixo).
inline Expressao Analisador::analisarConversao() {
    consumir(TipoToken::ABRE_PARENTESE, "esperado '(' no inicio da conversao de tipo");
    TipoVariavel tipoAlvo;
    if(verificar(TipoToken::NULO)) {
        pos++;
        consumir(TipoToken::VEZES, "esperado '*' apos 'nulo' na conversao de tipo(nulo sozinho nao e' um tipo de valor valido)");
        tipoAlvo = TipoVariavel::NULO_PONTEIRO;
    } else {
        tipoAlvo = analisarTipoValor("esperado 'int', 'longo', 'car', 'bool', 'byte', 'curto' ou 'nulo' na conversao de tipo");
    }
    consumir(TipoToken::FECHA_PARENTESE, "esperado ')' apos tipo da conversao");

    Expressao e;
    e.tipo = Expressao::Tipo::CONVERSAO;
    e.valor = nullptr;
    e.tipoConversao = tipoAlvo;
    e.esquerda = new Expressao(analisarExpressaoPrimaria());
    e.direita = nullptr;
    return e;
}

// analisa "(expr)": parenteses de agrupamento, usados pra forcar
// precedencia(ex: "se((c % d) == 0)") ou so por clareza. So chamada
// quando ja descartamos conversao de tipo(analisarExpressaoPrimaria
// checa "(" seguido de "int/car" primeiro). O parentese em si nao
// gera no novo na AST: "expr" pode ser qualquer expressao aditiva
// (aceita +, -, /, %encadeados), e o resultado e devolvido direto,
// exatamente como se os parenteses nunca tivessem existido.
inline Expressao Analisador::analisarExpressaoParentesizada() {
    consumir(TipoToken::ABRE_PARENTESE, "esperado '(' no inicio da expressao parentesizada");
    Expressao e = analisarExpressaoAditiva();
    consumir(TipoToken::FECHA_PARENTESE, "esperado ')' para fechar a expressao parentesizada");
    return e;
}

// analisa "bytes(int|car|car*|variavel)": tamanho em tempo de compilação,
// resolvido de verdade no GeradorIR(que ja sabe o tipo de cada variável).
// aqui só monta o no da AST guardando se veio de um tipo literal ou de
// um identificador.
inline Expressao Analisador::analisarBytes() {
    pos++; // consome "bytes"
    consumir(TipoToken::ABRE_PARENTESE, "esperado '(' apos 'bytes'");

    Expressao e;
    e.tipo = Expressao::Tipo::BYTES;
    e.valor = nullptr;
    e.bytesNomeVariavel = nullptr;

    if(ehTokenTipoValor(atual().tipo)) {
        e.bytesEhTipoLiteral = true;
        e.bytesTipoLiteral = analisarTipoValor("esperado tipo dentro de 'bytes(...)'");
    } else {
        const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado tipo(int/longo/car/car*/longo*) ou variavel dentro de 'bytes(...)'");
        e.bytesEhTipoLiteral = false;
        e.bytesNomeVariavel = strdup(nome.valor);
    }
    consumir(TipoToken::FECHA_PARENTESE, "esperado ')' apos argumento de 'bytes'");
    return e;
}

inline Expressao Analisador::analisarExpressaoPrimaria() {
    if(verificar(TipoToken::CHAMADA_SISTEMA)) {
        Expressao e;
        e.tipo = Expressao::Tipo::CHAMADA_SISTEMA;
        e.valor = nullptr;
        e.chamadaSistema = new ChamadaSistemaCrua(analisarNucleoChamadaSistema());
        return e;
    }
    if(verificar(TipoToken::MENOS) && tokens[pos + 1].tipo == TipoToken::NUMERO) {
        pos++; // consome '-'
        const Token& t = tokens[pos++];
        Expressao e;
        e.tipo = Expressao::Tipo::NUMERO_LITERAL;
        std::string valorNegativo = std::string("-") + t.valor;
        e.valor = strdup(valorNegativo.c_str());
        return e;
    }
    if(verificar(TipoToken::NULO)) {
        pos++;
        Expressao e;
        e.tipo = Expressao::Tipo::NUMERO_LITERAL;
        e.valor = strdup("0");
        return e;
    }
    if(verificar(TipoToken::ABRE_PARENTESE) && (ehTokenTipoValor(tokens[pos + 1].tipo) || tokens[pos + 1].tipo == TipoToken::NULO)) {
        return analisarConversao();
    }
    if(verificar(TipoToken::ABRE_PARENTESE)) {
        return analisarExpressaoParentesizada();
    }
    if(verificar(TipoToken::IDENTIFICADOR) && strcmp(atual().valor, "bytes") == 0 && tokens[pos + 1].tipo == TipoToken::ABRE_PARENTESE) {
        return analisarBytes();
    }
    if(verificar(TipoToken::TEXTO)) {
        const Token& t = tokens[pos++];
        Expressao e;
        e.tipo = Expressao::Tipo::TEXTO_LITERAL;
        e.valor = strdup(t.valor);
        return e;
    }
    if(verificar(TipoToken::NUMERO)) {
        const Token& t = tokens[pos++];
        Expressao e;
        e.tipo = Expressao::Tipo::NUMERO_LITERAL;
        e.valor = strdup(t.valor);
        return e;
    }
    if(verificar(TipoToken::IDENTIFICADOR) && tokens[pos + 1].tipo == TipoToken::ABRE_PARENTESE) {
        return analisarChamadaFuncaoComoExpressao();
    }
    if(verificar(TipoToken::IDENTIFICADOR) && tokens[pos + 1].tipo == TipoToken::ABRE_COLCHETE) {
        return analisarIndexacao();
    }
    if(verificar(TipoToken::IDENTIFICADOR)) {
        const Token& t = tokens[pos++];
        Expressao e;
        e.tipo = Expressao::Tipo::IDENTIFICADOR;
        e.valor = strdup(t.valor);
        return e;
    }
    erro("esperado expressão (texto, identificador, número ou chamada de função)");
}

// analisa "nome[indice]" como expressao de leitura(ex: "int b = txt[i];"
// ou "se(txt[i] == 0)"). "esquerda" guarda o ponteiro(identificador),
// "direita" guarda a expressao de indice.
inline Expressao Analisador::analisarIndexacao() {
    const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome do ponteiro");
    Expressao ponteiro;
    ponteiro.tipo = Expressao::Tipo::IDENTIFICADOR;
    ponteiro.valor = strdup(nome.valor);

    consumir(TipoToken::ABRE_COLCHETE, "esperado '[' apos nome do ponteiro");
    Expressao indice = analisarExpressaoAditiva();
    consumir(TipoToken::FECHA_COLCHETE, "esperado ']' apos indice");

    Expressao e;
    e.tipo = Expressao::Tipo::INDEXACAO;
    e.valor = nullptr;
    e.esquerda = new Expressao(ponteiro);
    e.direita = new Expressao(indice);
    return e;
}

// analisa "nome(arg1, arg2, ...)" como expressao(nao consome ';': quem
// chamou a expressao decide o que vem depois). Usado em declaracao,
// reatribuicao, retorne, condicao de "se"/"enquanto" e operandos de "+"/"-"".
inline Expressao Analisador::analisarChamadaFuncaoComoExpressao() {
    const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome da função chamada");
    Expressao e;
    e.tipo = Expressao::Tipo::CHAMADA_FUNCAO;
    e.valor = nullptr;
    e.nomeFuncao = strdup(nome.valor);
    e.argumentosFuncao = new Vetor<Expressao>();
    e.argumentosFuncao->iniciar();

    consumir(TipoToken::ABRE_PARENTESE, "esperado '(' após nome da função chamada");
    while(!verificar(TipoToken::FECHA_PARENTESE)) {
        e.argumentosFuncao->empurrar(analisarExpressaoPrimaria());
        if(verificar(TipoToken::VIRGULA)) pos++;
    }
    consumir(TipoToken::FECHA_PARENTESE, "esperado ')' após argumentos");
    return e;
}