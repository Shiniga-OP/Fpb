// util/analise/analise_comandos.h
#pragma once
#include "../analisador.h"

// true se "tipo" e um operador composto de bit: &=, |=, ^=, <<=, >>=.
inline bool ehAtribuicaoComposta(TipoToken tipo) {
    return tipo == TipoToken::TAMBEM_IGUAL || tipo == TipoToken::OU_IGUAL || tipo == TipoToken::CIRCUNFLEXO_IGUAL || tipo == TipoToken::MENOR_MENOR_IGUAL || tipo == TipoToken::MAIOR_MAIOR_IGUAL || tipo == TipoToken::VEZES_IGUAL || tipo == TipoToken::DIVIDIR_IGUAL || tipo == TipoToken::PORCENTAGEM_IGUAL;
}

// operador que um operador composto representa: "x op= e" vira "x = x op e" (e "x++" / "x--" viram soma / subtração de 1).
inline OperadorAritmetico operadorDaAtribuicaoComposta(TipoToken tipo) {
    if(tipo == TipoToken::MAIS_IGUAL || tipo == TipoToken::MAIS_MAIS) return OperadorAritmetico::SOMA;
    if(tipo == TipoToken::MENOS_IGUAL || tipo == TipoToken::MENOS_MENOS) return OperadorAritmetico::SUBTRACAO;
    if(tipo == TipoToken::TAMBEM_IGUAL) return OperadorAritmetico::E_BIT;
    if(tipo == TipoToken::OU_IGUAL) return OperadorAritmetico::OU_BIT;
    if(tipo == TipoToken::CIRCUNFLEXO_IGUAL) return OperadorAritmetico::XOU;
    if(tipo == TipoToken::MENOR_MENOR_IGUAL) return OperadorAritmetico::DESL_ESQ;
    if(tipo == TipoToken::VEZES_IGUAL) return OperadorAritmetico::MULTIPLICACAO;
    if(tipo == TipoToken::DIVIDIR_IGUAL) return OperadorAritmetico::DIVISAO;
    if(tipo == TipoToken::PORCENTAGEM_IGUAL) return OperadorAritmetico::MODULO;
    return OperadorAritmetico::DESL_DIR;
}

// analisa um comando dentro do corpo de uma função: declaração de
// variável(int/car*/nulo*), chamada de sistema crua(#chamada_sistema)
// ou chamada de função normal(nome(...)).
inline Comando Analisador::analisarComando() {
    Comando comando;
    bool inicioDeclaracaoNuloPonteiro = verificar(TipoToken::NULO) && tokens[pos + 1].tipo == TipoToken::VEZES;
    if(verificar(TipoToken::FINAL) || ehTokenTipoValor(atual().tipo) || inicioDeclaracaoNuloPonteiro || ehInicioDeclaracaoEspaco()) {
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
    if(verificar(TipoToken::APAGAR)) {
        comando.tipo = Comando::Tipo::APAGAR;
        comando.comandoApagar = analisarApagar();
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
    if(ehChamadaMetodo()) {
        comando.tipo = Comando::Tipo::CHAMADA_FUNCAO;
        comando.chamadaFuncao = analisarChamadaMetodo();
        return comando;
    }
    if(verificar(TipoToken::IDENTIFICADOR) && tokens[pos + 1].tipo == TipoToken::PONTO) {
        comando.tipo = Comando::Tipo::ATRIBUICAO_CAMPO;
        comando.atribuicaoCampo = analisarAtribuicaoCampo();
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

// analisa: nome.campo = expressao;(ou +=, -=, ++, --, &=, |=, ^=, <<=, >>=, desdobrados como em analisarReatribuicao)
inline AtribuicaoCampo Analisador::analisarAtribuicaoCampo() {
    AtribuicaoCampo atr;
    atr.iniciar();
    const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome da variável");
    atr.nome = strdup(nome.valor);
    consumir(TipoToken::PONTO, "esperado '.' após nome da variável");
    const Token& campo = consumir(TipoToken::IDENTIFICADOR, "esperado nome do campo após '.'");
    atr.campo = strdup(campo.valor);
    TipoToken op = atual().tipo;
    if(op == TipoToken::IGUAL) {
        pos++;
        atr.valor = analisarExpressaoAditiva();
    } else if(ehTokenReatribuicao(op)) {
        pos++;
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
        binaria.operadorAritmetico = operadorDaAtribuicaoComposta(op);
        binaria.esquerda = new Expressao(montarAcessoCampo(atr.nome, atr.campo));
        binaria.direita = new Expressao(operando);
        atr.valor = binaria;
    } else {
        erro("esperado '=', '+=', '-=', '*=', '%=', '/=', '++', '--' ou operador de bit composto na atribuição de campo");
    }
    if(verificar(TipoToken::PONTO_VIRGULA)) {
        consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após atribuição de campo");
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
    } else if(op == TipoToken::MAIS_IGUAL || op == TipoToken::MENOS_IGUAL || op == TipoToken::MAIS_MAIS || op == TipoToken::MENOS_MENOS || ehAtribuicaoComposta(op)) {
        // "x += e" vira "x = x + e"; "x++" vira "x = x + 1" (idem pra -=, -- e os de bit: &=, |=, ^=, <<=, >>=): sem nó novo na AST.
        pos++;
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
        binaria.operadorAritmetico = operadorDaAtribuicaoComposta(op);
        binaria.esquerda = new Expressao(variavel);
        binaria.direita = new Expressao(operando);
        reat.valor = binaria;
    } else {
        erro("esperado '=', '+=', '*=', '%=', '/=', '-=', '&=', '|=', '^=', '<<=', '>>=', '++' ou '--' na reatribuição");
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
    if(verificar(TipoToken::FINAL)) {
        pos++;
        decl.ehFinal = true;
    }

    if(verificar(TipoToken::NULO)) {
        pos++;
        consumir(TipoToken::VEZES, "esperado '*' apos 'nulo' na declaração de variável(nulo sozinho nao e' um tipo de valor valido)");
        decl.tipo = TipoVariavel::NULO_PONTEIRO;
    } else if(verificar(TipoToken::IDENTIFICADOR)) {
        // "Nome nome" = espaço por valor(corpo no frame); "Nome* nome" = ponteiro pra espaço.
        decl.nomeEspaco = strdup(atual().valor);
        pos++;
        if(verificar(TipoToken::VEZES)) {
            pos++;
            decl.tipo = TipoVariavel::ESPACO_PONTEIRO;
        } else {
            decl.tipo = TipoVariavel::ESPACO;
        }
    } else {
        decl.tipo = analisarTipoValor("esperado 'int', 'longo', 'car', 'bool', 'byte' ou 'curto' no início da declaração de variável");
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
    if(decl.ehFinal) {
        if(ehArray) erro("'final' não vale para array");
        if(decl.tipo == TipoVariavel::ESPACO || decl.tipo == TipoVariavel::ESPACO_PONTEIRO) erro("'final' só vale para tipos de valor e ponteiros de valor");
    }

    if(ehArray) {
        decl.tamArray = tamArray;
        if(verificar(TipoToken::PONTO_VIRGULA)) pos++;
        TipoVariavel tipoArray = ponteiroDoTipo(decl.tipo);
        if(tipoArray == decl.tipo) erro("array só aceita int, longo, car, bool, byte ou curto");
        decl.tipo = tipoArray;
        decl.ehArray = true;
        return decl;
    }
    if(decl.tipo == TipoVariavel::ESPACO) {
        // espaço por valor: "Nome nome;"(campos sem valor) ou "Nome nome = { e1, e2, ... };"
        if(!verificar(TipoToken::IGUAL)) {
            if(verificar(TipoToken::PONTO_VIRGULA)) pos++;
            return decl;
        }
        pos++;
        if(!verificar(TipoToken::ABRE_CHAVE)) {
            // "Nome nome = expr;": a expressão fica sozinha em listaInicial(temLista continua false). O GeradorIR chama o construtor "Nome_criar" ou copia, se expr for outro espaço do mesmo tipo.
            decl.listaInicial.empurrar(analisarExpressaoAditiva());
            if(verificar(TipoToken::PONTO_VIRGULA)) pos++;
            return decl;
        }
        consumir(TipoToken::ABRE_CHAVE, "espaço só pode ser inicializado com uma lista: { valor1, valor2, ... } ou uma expressão");
        while(!verificar(TipoToken::FECHA_CHAVE)) {
            decl.listaInicial.empurrar(analisarExpressaoAditiva());
            if(verificar(TipoToken::VIRGULA)) pos++;
        }
        consumir(TipoToken::FECHA_CHAVE, "esperado '}' para fechar a lista de inicialização");
        decl.temLista = true;
        if(verificar(TipoToken::PONTO_VIRGULA)) pos++;
        return decl;
    }
    if(decl.tipo == TipoVariavel::ESPACO_PONTEIRO && verificar(TipoToken::IGUAL) && tokens[pos + 1].tipo == TipoToken::ABRE_CHAVE) {
        pos += 2;
        while(!verificar(TipoToken::FECHA_CHAVE)) {
            decl.listaInicial.empurrar(analisarExpressaoAditiva());
            if(verificar(TipoToken::VIRGULA)) pos++;
        }
        consumir(TipoToken::FECHA_CHAVE, "esperado '}' para fechar a lista de inicialização");
        decl.temLista = true;
        if(verificar(TipoToken::PONTO_VIRGULA)) pos++;
        return decl;
    }
    consumir(TipoToken::IGUAL, "esperado '=' após nome da variável ('final' exige valor inicial)");
    if(decl.ehFinal && verificar(TipoToken::CHAMADA_SISTEMA)) erro("'final' não aceita #chamada_sistema: o valor precisa ser constante");
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

    chamada.identificadores.empurrar(analisarExpressaoAditiva());
    while(verificar(TipoToken::VIRGULA)) {
        pos++;
        chamada.identificadores.empurrar(analisarExpressaoAditiva());
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
        chamada.argumentos.empurrar(analisarExpressaoAditiva());
        if(verificar(TipoToken::VIRGULA)) pos++;
    }
    consumir(TipoToken::FECHA_PARENTESE, "esperado ')' após argumentos");
    if(verificar(TipoToken::PONTO_VIRGULA)) {
        consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após chamada de função");
    }
    return chamada;
}

// true se o comando/expressão atual é "nome.metodo(": identificador, '.', identificador e '('.
inline bool Analisador::ehChamadaMetodo() {
    if(pos + 3 >= (size_t)tokens.tam) return false;
    return verificar(TipoToken::IDENTIFICADOR) && tokens[pos + 1].tipo == TipoToken::PONTO && tokens[pos + 2].tipo == TipoToken::IDENTIFICADOR && tokens[pos + 3].tipo == TipoToken::ABRE_PARENTESE;
}

// analisa: nome.metodo(args); como comando. Vira uma ChamadaFuncao de nome "nome.metodo": o GeradorIR decide se "nome" é
// variável(chama Classe_metodo(nome, args)) ou o nome de uma classe(chama Classe_metodo(args), método estático).
inline ChamadaFuncao Analisador::analisarChamadaMetodo() {
    const Token& base = consumir(TipoToken::IDENTIFICADOR, "esperado nome da variável ou da classe");
    std::string nomeCompleto = base.valor;
    consumir(TipoToken::PONTO, "esperado '.' após o nome");
    const Token& metodo = consumir(TipoToken::IDENTIFICADOR, "esperado nome do método após '.'");
    nomeCompleto = nomeCompleto + "." + metodo.valor;
    ChamadaFuncao chamada;
    chamada.iniciar();
    chamada.nome = strdup(nomeCompleto.c_str());
    consumir(TipoToken::ABRE_PARENTESE, "esperado '(' após nome do método");
    while(!verificar(TipoToken::FECHA_PARENTESE)) {
        chamada.argumentos.empurrar(analisarExpressaoAditiva());
        if(verificar(TipoToken::VIRGULA)) pos++;
    }
    consumir(TipoToken::FECHA_PARENTESE, "esperado ')' após argumentos");
    if(verificar(TipoToken::PONTO_VIRGULA)) {
        consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após chamada de método");
    }
    return chamada;
}

// true se "tipo" pode vir logo depois do nome numa reatribuição: =, +=, -=, ++, --, &=, |=, ^=, <<=, >>=.
inline bool Analisador::ehTokenReatribuicao(TipoToken tipo) {
    return tipo == TipoToken::IGUAL || tipo == TipoToken::MAIS_IGUAL || tipo == TipoToken::MENOS_IGUAL || tipo == TipoToken::MAIS_MAIS || tipo == TipoToken::MENOS_MENOS || ehAtribuicaoComposta(tipo);
}

// uma parte do cabeçalho do "por", empurrada em "destino": inicialização (declaração ou
// reatribuição) quando ehInicio, ou passo (só reatribuição: "i = i + 1", "i += 1", "i++").
inline void Analisador::analisarPartePor(Vetor<Comando>& destino, bool ehInicio) {
    Comando comando;
    bool comecaComTipo = ehTokenTipoValor(atual().tipo) || (verificar(TipoToken::NULO) && tokens[pos + 1].tipo == TipoToken::VEZES) || ehInicioDeclaracaoEspaco();
    if(ehInicio && comecaComTipo) {
        comando.tipo = Comando::Tipo::DECLARACAO_VARIAVEL;
        comando.declaracaoVariavel = analisarDeclaracaoVariavel();
    } else if(verificar(TipoToken::IDENTIFICADOR) && ehTokenReatribuicao(tokens[pos + 1].tipo)) {
        comando.tipo = Comando::Tipo::REATRIBUICAO;
        comando.reatribuicao = analisarReatribuicao();
    } else if(verificar(TipoToken::IDENTIFICADOR) && tokens[pos + 1].tipo == TipoToken::PONTO) {
        comando.tipo = Comando::Tipo::ATRIBUICAO_CAMPO;
        comando.atribuicaoCampo = analisarAtribuicaoCampo();
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

// analisa: apagar expressao;
inline ComandoApagar Analisador::analisarApagar() {
    consumir(TipoToken::APAGAR, "esperado 'apagar'");
    ComandoApagar ca;
    ca.alvo = analisarExpressaoAditiva();
    if(verificar(TipoToken::PONTO_VIRGULA)) {
        consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após 'apagar'");
    }
    return ca;
}