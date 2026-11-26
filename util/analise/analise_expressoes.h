// util/analise/analise_expressoes.h
#pragma once
#include "../analisador.h"

inline Expressao montarNumeroLiteral(const char* valor) {
    Expressao e;
    e.tipo = Expressao::Tipo::NUMERO_LITERAL;
    e.valor = strdup(valor);
    return e;
}

// operador binario do nivel dado, se o token atual pertencer a ele. Niveis, do mais fraco ao mais forte:
// 0 "|", 1 "^", 2 "&", 3 "<<" ">>", 4 "+" "-", 5 "*" "/" "%". O nivel 6 e a expressao primaria.
inline bool Analisador::operadorDoNivel(int nivel, OperadorAritmetico& op) {
    TipoToken t = atual().tipo;
    if(nivel == 0 && t == TipoToken::OU) op = OperadorAritmetico::OU_BIT;
    else if(nivel == 1 && t == TipoToken::CIRCUNFLEXO) op = OperadorAritmetico::XOU;
    else if(nivel == 2 && t == TipoToken::TAMBEM) op = OperadorAritmetico::E_BIT;
    else if(nivel == 3 && t == TipoToken::MENOR_MENOR) op = OperadorAritmetico::DESL_ESQ;
    else if(nivel == 3 && t == TipoToken::MAIOR_MAIOR) op = OperadorAritmetico::DESL_DIR;
    else if(nivel == 4 && t == TipoToken::MAIS) op = OperadorAritmetico::SOMA;
    else if(nivel == 4 && t == TipoToken::MENOS) op = OperadorAritmetico::SUBTRACAO;
    else if(nivel == 5 && t == TipoToken::VEZES) op = OperadorAritmetico::MULTIPLICACAO;
    else if(nivel == 5 && t == TipoToken::DIVIDIR) op = OperadorAritmetico::DIVISAO;
    else if(nivel == 5 && t == TipoToken::PORCENTAGEM) op = OperadorAritmetico::MODULO;
    else return false;
    return true;
}

// analisa uma expressao binaria no nivel de precedencia dado: cada nivel chama o proximo mais forte pros
// operandos e empilha pra esquerda a cada operador do proprio nivel (associativo a esquerda).
inline Expressao Analisador::analisarBinaria(int nivel) {
    if(nivel == 6) return analisarExpressaoPrimaria();
    Expressao esquerda = analisarBinaria(nivel + 1);
    OperadorAritmetico op = OperadorAritmetico::SOMA;
    while(operadorDoNivel(nivel, op)) {
        pos++;
        Expressao direita = analisarBinaria(nivel + 1);
        
        if(op == OperadorAritmetico::SOMA && esquerda.tipo == Expressao::Tipo::TEXTO_LITERAL && direita.tipo == Expressao::Tipo::TEXTO_LITERAL) {
            size_t a = strlen(esquerda.valor);
            size_t b = strlen(direita.valor);
            char* junto = (char*)malloc(a + b + 1);
            memcpy(junto, esquerda.valor, a);
            memcpy(junto + a, direita.valor, b + 1);
            free(esquerda.valor);
            free(direita.valor);
            esquerda.valor = junto;
            continue;
        }
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

// destino valido de atribuição: variavel, "nome.campo" ou "nome[indice]" / "nome.campo[indice]".
inline bool ehDestinoAtribuicao(const Expressao& e) {
    return e.tipo == Expressao::Tipo::IDENTIFICADOR || e.tipo == Expressao::Tipo::ACESSO_CAMPO || e.tipo == Expressao::Tipo::INDEXACAO;
}

// true se avaliar "e" duas vezes dá o mesmo resultado (sem chamada de função, atribuição ou chamada de sistema).
inline bool ehExpressaoPura(const Expressao& e) {
    Expressao::Tipo t = e.tipo;
    if(t == Expressao::Tipo::NUMERO_LITERAL || t == Expressao::Tipo::DECIMAL_LITERAL || t == Expressao::Tipo::TEXTO_LITERAL || t == Expressao::Tipo::IDENTIFICADOR || t == Expressao::Tipo::BYTES) return true;
    if(t == Expressao::Tipo::ACESSO_CAMPO || t == Expressao::Tipo::CONVERSAO) return ehExpressaoPura(*e.esquerda);
    if(t == Expressao::Tipo::BINARIA || t == Expressao::Tipo::INDEXACAO) return ehExpressaoPura(*e.esquerda) && ehExpressaoPura(*e.direita);
    return false;
}

// cópia profunda de uma expressão pura (só chamar depois de ehExpressaoPura).
inline Expressao clonarExpressaoPura(const Expressao& e) {
    Expressao c = e;
    Expressao::Tipo t = e.tipo;
    if(t == Expressao::Tipo::BYTES) {
        if(e.bytesNomeVariavel) c.bytesNomeVariavel = strdup(e.bytesNomeVariavel);
    } else if(t == Expressao::Tipo::NUMERO_LITERAL || t == Expressao::Tipo::DECIMAL_LITERAL || t == Expressao::Tipo::TEXTO_LITERAL || t == Expressao::Tipo::IDENTIFICADOR || t == Expressao::Tipo::ACESSO_CAMPO) {
        c.valor = strdup(e.valor);
    }
    if(t == Expressao::Tipo::ACESSO_CAMPO || t == Expressao::Tipo::CONVERSAO || t == Expressao::Tipo::BINARIA || t == Expressao::Tipo::INDEXACAO) {
        c.esquerda = new Expressao(clonarExpressaoPura(*e.esquerda));
    }
    if(t == Expressao::Tipo::BINARIA || t == Expressao::Tipo::INDEXACAO) {
        c.direita = new Expressao(clonarExpressaoPura(*e.direita));
    }
    return c;
}

// monta "destino = destino OP 1" (nó ATRIBUICAO, que devolve o valor NOVO).
inline Expressao Analisador::montarIncremento(const Expressao& destino, OperadorAritmetico op) {
    if(!ehExpressaoPura(destino)) erro("'++'/'--' não aceita destino com efeito colateral (ex: a[f()]++ ou a[i++]++): use uma variável auxiliar");
    Expressao nova;
    nova.tipo = Expressao::Tipo::BINARIA;
    nova.valor = nullptr;
    nova.operadorAritmetico = op;
    nova.esquerda = new Expressao(clonarExpressaoPura(destino));
    nova.direita = new Expressao(montarNumeroLiteral("1"));
    Expressao atrib;
    atrib.tipo = Expressao::Tipo::ATRIBUICAO;
    atrib.valor = nullptr;
    atrib.esquerda = new Expressao(destino);
    atrib.direita = new Expressao(nova);
    return atrib;
}

// "x++" / "x--" pós-fixo: devolve o valor ANTIGO, ou seja, "(x = x + 1) - 1".
inline Expressao Analisador::analisarPosfixo(Expressao base) {
    if((verificar(TipoToken::MAIS_MAIS) || verificar(TipoToken::MENOS_MENOS)) && ehDestinoAtribuicao(base)) {
        bool soma = verificar(TipoToken::MAIS_MAIS);
        pos++;
        Expressao atrib = montarIncremento(base, soma ? OperadorAritmetico::SOMA : OperadorAritmetico::SUBTRACAO);
        Expressao antigo;
        antigo.tipo = Expressao::Tipo::BINARIA;
        antigo.valor = nullptr;
        antigo.operadorAritmetico = soma ? OperadorAritmetico::SUBTRACAO : OperadorAritmetico::SOMA;
        antigo.esquerda = new Expressao(atrib);
        antigo.direita = new Expressao(montarNumeroLiteral("1"));
        return antigo;
    }
    return base;
}

// ponto de entrada das expressões primárias: primária + "++"/"--" pós-fixo opcional.
inline Expressao Analisador::analisarExpressaoPrimaria() {
    return analisarPosfixo(analisarPrimariaSemPosfixo());
}

// ponto de entrada de qualquer expressao: operadores aritmeticos e de bit, e "cond ? a : b" no nivel mais fraco(associativo a direita).
// le primeiro como expressao comum; se logo depois vem comparacao, "&&", "||" ou "?", era a condicao de um ternario: volta ao inicio e le como condicao.
// se a leitura comum falhar(ex: "(a == 1 || b == 2) ? x : y", onde o parentese tem uma condicao), tambem tenta como condicao.
inline Expressao Analisador::analisarExpressaoAditiva() {
    size_t inicio = pos;
    Expressao base;
    bool leu = true;
    try {
        base = analisarBinaria(0);
    } catch(const std::runtime_error&) {
        leu = false;
    }
    if(leu && verificar(TipoToken::IGUAL) && ehDestinoAtribuicao(base)) {
        // "destino = expr": associativo a direita. "a = b = c = 0" vira a = (b = (c = 0)).
        pos++;
        Expressao e;
        e.tipo = Expressao::Tipo::ATRIBUICAO;
        e.valor = nullptr;
        e.esquerda = new Expressao(base);
        e.direita = new Expressao(analisarExpressaoAditiva());
        return e;
    }
    if(leu && !ehTokenComparacao(atual().tipo) && !verificar(TipoToken::OU_OU) && !verificar(TipoToken::TAMBEM_TAMBEM) && !verificar(TipoToken::INTERROGACAO)) {
        return base;
    }
    if(leu) base.liberar();
    pos = inicio;
    Expressao e;
    e.tipo = Expressao::Tipo::TERNARIA;
    e.valor = nullptr;
    e.condicaoTernaria = new Condicao(analisarCondicao("?"));
    consumir(TipoToken::INTERROGACAO, "esperado '?' depois da condição do operador ternário");
    e.esquerda = new Expressao(analisarExpressaoAditiva());
    consumir(TipoToken::DOIS_PONTOS, "esperado ':' no operador ternário");
    e.direita = new Expressao(analisarExpressaoAditiva());
    return e;
}

// expressao sem ternario: usada pelos dois lados de uma comparacao em analisarCondicaoBase(senao "a < b ? x : y" entraria em loop).
inline Expressao Analisador::analisarExpressaoSemTernario() {
    return analisarBinaria(0);
}

// analisa uma conversão de tipo(conversão): (car)expr, (int)expr, (car*)expr
// analisa uma conversão de tipo(conversão): (car)expr, (int)expr, (car*)expr, (nulo*)expr
// ou (Nome*)expr(ponteiro pra espaço). So chamada quando já detectamos "(" seguido de
// um tipo(distingue de uma expressão parentesizada comum, ver
// analisarExpressaoParentesizada logo abaixo).
inline Expressao Analisador::analisarConversao() {
    consumir(TipoToken::ABRE_PARENTESE, "esperado '(' no inicio da conversao de tipo");
    TipoVariavel tipoAlvo;
    if(verificar(TipoToken::NULO)) {
        pos++;
        consumir(TipoToken::VEZES, "esperado '*' apos 'nulo' na conversao de tipo(nulo sozinho nao e' um tipo de valor valido)");
        tipoAlvo = TipoVariavel::NULO_PONTEIRO;
    } else if(verificar(TipoToken::IDENTIFICADOR)) {
        // "(Nome*)expr": ponteiro pra espaço. Em tempo de execução é só um endereço de 8 bytes, igual a "(nulo*)expr"
        pos++;
        consumir(TipoToken::VEZES, "esperado '*' apos o nome do espaço na conversao de tipo");
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
        const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado tipo(int/longo/car/car*/longo*), espaço ou variavel dentro de 'bytes(...)'");
        e.bytesEhTipoLiteral = false;
        e.bytesNomeVariavel = strdup(nome.valor);
    }
    consumir(TipoToken::FECHA_PARENTESE, "esperado ')' apos argumento de 'bytes'");
    return e;
}

inline Expressao Analisador::analisarPrimariaSemPosfixo() {
    if(verificar(TipoToken::CHAMADA_SISTEMA)) {
        Expressao e;
        e.tipo = Expressao::Tipo::CHAMADA_SISTEMA;
        e.valor = nullptr;
        e.chamadaSistema = new ChamadaSistemaCrua(analisarNucleoChamadaSistema());
        return e;
    }
    if(verificar(TipoToken::ARROBA)) {
        // "@(expr)": o valor de expr é tratado como endereço. "@nome": endereço da variável "nome".
        pos++;
        Expressao e;
        e.tipo = Expressao::Tipo::ENDERECO;
        e.valor = nullptr;
        e.direita = nullptr;
        if(verificar(TipoToken::ABRE_PARENTESE)) {
            e.enderecoDeValor = true;
            e.esquerda = new Expressao(analisarExpressaoParentesizada());
        } else {
            e.enderecoDeValor = false;
            const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome da variável ou '(' após '@'");
            if(verificar(TipoToken::PONTO)) {
                pos++; // consome '.'
                const Token& campo = consumir(TipoToken::IDENTIFICADOR, "esperado nome do campo após '.'");
                e.esquerda = new Expressao(montarAcessoCampo(nome.valor, campo.valor));
            } else {
                Expressao variavel;
                variavel.tipo = Expressao::Tipo::IDENTIFICADOR;
                variavel.valor = strdup(nome.valor);
                e.esquerda = new Expressao(variavel);
            }
        }
        return e;
    }
    if(verificar(TipoToken::MAIS_MAIS) || verificar(TipoToken::MENOS_MENOS)) {
        // "++x" / "--x" pré-fixo: devolve o valor NOVO, ou seja, "x = x + 1".
        bool soma = verificar(TipoToken::MAIS_MAIS);
        pos++;
        Expressao alvo = analisarPrimariaSemPosfixo();
        if(!ehDestinoAtribuicao(alvo)) erro("'++'/'--' só vale em variável, campo ou posição de array");
        return montarIncremento(alvo, soma ? OperadorAritmetico::SOMA : OperadorAritmetico::SUBTRACAO);
    }
    if(verificar(TipoToken::TIL)) {
        // "~x" vira "x ^ -1" (-1 tem todos os bits em 1): sem nó novo na AST nem instrução nova no IR.
        pos++;
        Expressao menosUm;
        menosUm.tipo = Expressao::Tipo::NUMERO_LITERAL;
        menosUm.valor = strdup("-1");
        Expressao e;
        e.tipo = Expressao::Tipo::BINARIA;
        e.valor = nullptr;
        e.operadorAritmetico = OperadorAritmetico::XOU;
        e.esquerda = new Expressao(analisarExpressaoPrimaria());
        e.direita = new Expressao(menosUm);
        return e;
    }
    if(verificar(TipoToken::MENOS) && (tokens[pos + 1].tipo == TipoToken::NUMERO || tokens[pos + 1].tipo == TipoToken::DECIMAL)) {
        pos++; // consome '-'
        const Token& t = tokens[pos++];
        Expressao e;
        e.tipo = (t.tipo == TipoToken::DECIMAL) ? Expressao::Tipo::DECIMAL_LITERAL : Expressao::Tipo::NUMERO_LITERAL;
        std::string valorNegativo = std::string("-") + t.valor;
        e.valor = strdup(valorNegativo.c_str());
        return e;
    }
    if(verificar(TipoToken::MENOS)) {
        // "-x" vira "0 - x": sem nó novo na AST nem instrução nova no IR
        pos++;
        Expressao e;
        e.tipo = Expressao::Tipo::BINARIA;
        e.valor = nullptr;
        e.operadorAritmetico = OperadorAritmetico::SUBTRACAO;
        e.esquerda = new Expressao(montarNumeroLiteral("0"));
        e.direita = new Expressao(analisarExpressaoPrimaria());
        return e;
    }
    if(verificar(TipoToken::EXCLAMACAO)) {
        // "!x" como valor vira "x == 0 ? 1 : 0". Dentro de condição, quem trata o "!" é analisarCondicaoBase.
        pos++;
        Expressao e;
        e.tipo = Expressao::Tipo::TERNARIA;
        e.valor = nullptr;
        e.condicaoTernaria = new Condicao();
        e.condicaoTernaria->tipo = Condicao::Tipo::COMPARACAO;
        e.condicaoTernaria->esquerda = analisarExpressaoPrimaria();
        e.condicaoTernaria->operador = OperadorComparacao::IGUAL;
        e.condicaoTernaria->direita = montarNumeroLiteral("0");
        e.esquerda = new Expressao(montarNumeroLiteral("1"));
        e.direita = new Expressao(montarNumeroLiteral("0"));
        return e;
    }
    if(verificar(TipoToken::NULO)) {
        pos++;
        Expressao e;
        e.tipo = Expressao::Tipo::NUMERO_LITERAL;
        e.valor = strdup("0");
        return e;
    }
    bool ehConversaoEspaco = pos + 3 < tokens.tam && tokens[pos + 1].tipo == TipoToken::IDENTIFICADOR && tokens[pos + 2].tipo == TipoToken::VEZES && tokens[pos + 3].tipo == TipoToken::FECHA_PARENTESE;
    if(verificar(TipoToken::ABRE_PARENTESE) && (ehTokenTipoValor(tokens[pos + 1].tipo) || tokens[pos + 1].tipo == TipoToken::NULO || ehConversaoEspaco)) {
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
    if(verificar(TipoToken::DECIMAL)) {
        const Token& t = tokens[pos++];
        Expressao e;
        e.tipo = Expressao::Tipo::DECIMAL_LITERAL;
        e.valor = strdup(t.valor);
        return e;
    }
    if(ehChamadaMetodo()) {
        return analisarChamadaMetodoComoExpressao();
    }
    if(verificar(TipoToken::IDENTIFICADOR) && tokens[pos + 1].tipo == TipoToken::PONTO) {
        const Token& nome = tokens[pos++];
        pos++; // consome '.'
        const Token& campo = consumir(TipoToken::IDENTIFICADOR, "esperado nome do campo após '.'");
        Expressao acesso = montarAcessoCampo(nome.valor, campo.valor);
        if(!verificar(TipoToken::ABRE_COLCHETE)) return acesso;
        // "nome.campo[indice]": indexa o ponteiro guardado no campo
        pos++;
        Expressao indice = analisarExpressaoAditiva();
        consumir(TipoToken::FECHA_COLCHETE, "esperado ']' apos indice");
        Expressao e;
        e.tipo = Expressao::Tipo::INDEXACAO;
        e.valor = nullptr;
        e.esquerda = new Expressao(acesso);
        e.direita = new Expressao(indice);
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
        e.argumentosFuncao->empurrar(analisarExpressaoAditiva());
        if(verificar(TipoToken::VIRGULA)) pos++;
    }
    consumir(TipoToken::FECHA_PARENTESE, "esperado ')' após argumentos");
    return e;
}

// analisa "nome.metodo(args)" como expressao: CHAMADA_FUNCAO de nome "nome.metodo"(ver analisarChamadaMetodo).
inline Expressao Analisador::analisarChamadaMetodoComoExpressao() {
    const Token& base = consumir(TipoToken::IDENTIFICADOR, "esperado nome da variável ou da classe");
    std::string nomeCompleto = base.valor;
    consumir(TipoToken::PONTO, "esperado '.' após o nome");
    const Token& metodo = consumir(TipoToken::IDENTIFICADOR, "esperado nome do método após '.'");
    nomeCompleto = nomeCompleto + "." + metodo.valor;
    Expressao e;
    e.tipo = Expressao::Tipo::CHAMADA_FUNCAO;
    e.valor = nullptr;
    e.nomeFuncao = strdup(nomeCompleto.c_str());
    e.argumentosFuncao = new Vetor<Expressao>();
    e.argumentosFuncao->iniciar();
    consumir(TipoToken::ABRE_PARENTESE, "esperado '(' após nome do método");
    while(!verificar(TipoToken::FECHA_PARENTESE)) {
        e.argumentosFuncao->empurrar(analisarExpressaoAditiva());
        if(verificar(TipoToken::VIRGULA)) pos++;
    }
    consumir(TipoToken::FECHA_PARENTESE, "esperado ')' após argumentos");
    return e;
}

// monta "nome.campo" como expressao: "esquerda" guarda a variavel(identificador), "valor" guarda o nome do campo.
inline Expressao Analisador::montarAcessoCampo(const char* nome, const char* campo) {
    Expressao base;
    base.tipo = Expressao::Tipo::IDENTIFICADOR;
    base.valor = strdup(nome);
    Expressao e;
    e.tipo = Expressao::Tipo::ACESSO_CAMPO;
    e.valor = strdup(campo);
    e.esquerda = new Expressao(base);
    e.direita = nullptr;
    return e;
}