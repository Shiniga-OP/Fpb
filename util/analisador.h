// util/analisador.h
#pragma once
#include <stdexcept>
#include <string>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "ast.h"

class Analisador {
public:
    explicit Analisador(const Vetor<Token>& tokens) : tokens(tokens) {}

    Programa analisar() {
        Programa programa;
        programa.iniciar();
        while(!verificar(TipoToken::FIM_ARQUIVO)) {
            if(verificar(TipoToken::INCLUIR)) {
                programa.inclusoes.empurrar(analisarInclusao());
            } else if(verificar(TipoToken::NULO) || verificar(TipoToken::INT) || verificar(TipoToken::LONGO) || verificar(TipoToken::CAR)) {
                programa.funcoes.empurrar(analisarFuncao());
            } else {
                erro("Esperado '#incluir' ou declaração de função");
            }
        }
        return programa;
    }
    Vetor<Token> tokens; // não possuí: dono e quem chamou o Analisador
    size_t pos = 0;

    const Token& atual() {
        return tokens[pos];
    }
    bool verificar(TipoToken tipo) {
        return atual().tipo == tipo;
    }

    const Token& consumir(TipoToken tipo, const char* mensagemErro) {
        if(!verificar(tipo)) erro(mensagemErro);
        return tokens[pos++];
    }

    [[noreturn]] void erro(const char* mensagem) {
        std::string msg = "Erro de sintaxe na linha " + std::to_string(atual().linha) + ": " + mensagem + " (encontrado '" + atual().valor + "')";
        throw std::runtime_error(msg);
    }

    Inclusao analisarInclusao() {
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
    DeclaracaoFuncao analisarFuncao() {
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
    TipoRetorno analisarTipoRetorno() {
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
        if(verificar(TipoToken::INT)) {
            pos++;
            if(verificar(TipoToken::VEZES)) {
                pos++;
                tr.tipo = TipoVariavel::INT_PONTEIRO;
            } else {
                tr.tipo = TipoVariavel::INT;
            }
        } else if(verificar(TipoToken::LONGO)) {
            pos++;
            if(verificar(TipoToken::VEZES)) {
                pos++;
                tr.tipo = TipoVariavel::LONGO_PONTEIRO;
            } else {
                tr.tipo = TipoVariavel::LONGO;
            }
        } else {
            consumir(TipoToken::CAR, "esperado 'nulo', 'int', 'longo' ou 'car' no tipo de retorno da função");
            if(verificar(TipoToken::VEZES)) {
                pos++;
                tr.tipo = TipoVariavel::CAR_PONTEIRO;
            } else {
                tr.tipo = TipoVariavel::CAR;
            }
        }
        return tr;
    }

    // analisa um parâmetro formal: int nome, int* nome(ponteiro de 8 bytes), car nome(caractere), car* nome(ponteiro de texto) ou nulo* nome(ponteiro generico)
    Parametro analisarParametro() {
        Parametro par;
        if(verificar(TipoToken::NULO)) {
            pos++;
            consumir(TipoToken::VEZES, "esperado '*' apos 'nulo' no tipo do parametro(nulo sozinho nao e' um tipo de valor valido)");
            par.tipo = TipoVariavel::NULO_PONTEIRO;
        } else if(verificar(TipoToken::INT)) {
            pos++;
            if(verificar(TipoToken::VEZES)) {
                pos++;
                par.tipo = TipoVariavel::INT_PONTEIRO;
            } else {
                par.tipo = TipoVariavel::INT;
            }
        } else if(verificar(TipoToken::LONGO)) {
            pos++;
            if(verificar(TipoToken::VEZES)) {
                pos++;
                par.tipo = TipoVariavel::LONGO_PONTEIRO;
            } else {
                par.tipo = TipoVariavel::LONGO;
            }
        } else {
            consumir(TipoToken::CAR, "esperado 'int', 'longo' ou 'car' no tipo do parametro");
            if(verificar(TipoToken::VEZES)) {
                pos++;
                par.tipo = TipoVariavel::CAR_PONTEIRO;
            } else {
                par.tipo = TipoVariavel::CAR;
            }
        }
        const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome do parametro");
        par.nome = strdup(nome.valor);
        return par;
    }

    // analisa um comando dentro do corpo de uma função: declaração de
    // variável(int/car*/nulo*), chamada de sistema crua(#chamada_sistema)
    // ou chamada de função normal(nome(...)).
    Comando analisarComando() {
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
        if(verificar(TipoToken::IDENTIFICADOR) && tokens[pos + 1].tipo == TipoToken::IGUAL) {
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
    AtribuicaoIndexada analisarAtribuicaoIndexada() {
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
    Reatribuicao analisarReatribuicao() {
        Reatribuicao reat;
        reat.iniciar();
        const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome da variável");
        reat.nome = strdup(nome.valor);
        consumir(TipoToken::IGUAL, "esperado '=' na reatribuição");
        reat.valor = analisarExpressaoAditiva();
        if(verificar(TipoToken::PONTO_VIRGULA)) {
            consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após reatribuição");
        }
        return reat;
    }

    // analisa: "retorne;"(sem valor) ou "retorne expr;"(com valor). A
    // checagem de qual forma bate com o tipo de retorno da função(nulo vs
    // tipado) fica pro GeradorIR, que ja tem acesso ao tipo da função atual.
    ComandoRetorne analisarRetorne() {
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

    // analisa um dos seis operadores de comparacao aceitos na condicao de
    // "se"/"enquanto": ==, !=, >, <, >=, <=. Extraida numa funcao unica pra
    // nao duplicar a mesma lista em analisarSe e analisarEnquanto.
    OperadorComparacao analisarOperadorComparacao(const char* nomeConstrucao) {
        if(verificar(TipoToken::IGUAL_IGUAL)) {
            pos++;
            return OperadorComparacao::IGUAL;
        }
        if(verificar(TipoToken::DIFERENTE)) {
            pos++;
            return OperadorComparacao::DIFERENTE;
        }
        if(verificar(TipoToken::MAIOR_IGUAL)) {
            pos++;
            return OperadorComparacao::MAIOR_IGUAL;
        }
        if(verificar(TipoToken::MENOR_IGUAL)) {
            pos++;
            return OperadorComparacao::MENOR_IGUAL;
        }
        if(verificar(TipoToken::MAIOR)) {
            pos++;
            return OperadorComparacao::MAIOR;
        }
        if(verificar(TipoToken::MENOR)) {
            pos++;
            return OperadorComparacao::MENOR;
        }
        std::string msg = std::string("esperado '==', '!=', '>', '<', '>=' ou '<=' na condição do '") + nomeConstrucao + "'";
        erro(msg.c_str());
    }

    // analisa: se(esquerda OP direita) BLOCO [senao (se(...) BLOCO | BLOCO)]
    ComandoSe analisarSe() {
        consumir(TipoToken::SE, "esperado 'se'");
        consumir(TipoToken::ABRE_PARENTESE, "esperado '(' após 'se'");

        ComandoSe comandoSe;
        comandoSe.iniciar();
        comandoSe.condicao.esquerda = analisarExpressaoPrimaria();
        comandoSe.condicao.operador = analisarOperadorComparacao("se");
        comandoSe.condicao.direita = analisarExpressaoPrimaria();
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

    // analisa: enquanto(esquerda OP direita) BLOCO
    ComandoEnquanto analisarEnquanto() {
        consumir(TipoToken::ENQUANTO, "esperado 'enquanto'");
        consumir(TipoToken::ABRE_PARENTESE, "esperado '(' apos 'enquanto'");

        ComandoEnquanto comandoEnquanto;
        comandoEnquanto.iniciar();
        comandoEnquanto.condicao.esquerda = analisarExpressaoPrimaria();
        comandoEnquanto.condicao.operador = analisarOperadorComparacao("enquanto");
        comandoEnquanto.condicao.direita = analisarExpressaoPrimaria();
        consumir(TipoToken::FECHA_PARENTESE, "esperado ')' apos condicao do 'enquanto'");

        analisarBloco(*comandoEnquanto.corpo);
        return comandoEnquanto;
    }


    void analisarBloco(Vetor<Comando>& corpo) {
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
    DeclaracaoVariavel analisarDeclaracaoVariavel() {
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
        const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome da variável");
        decl.nome = strdup(nome.valor);

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

    // analisa: #chamada_sistema id, arg1, arg2, ...;
    // analisa so os argumentos de "#chamada_sistema id, arg1, arg2, ...", SEM
    // consumir o ';' final: usada tanto por analisarChamadaSistemaCrua(que
    // consome o ';' logo em seguida, pro caso de comando de topo) quanto por
    // analisarExpressaoPrimaria(que NAO deve consumir ';', pois a chamada de
    // sistema esta embutida dentro de outra construção maior, ex: "retorne
    // #chamada_sistema ...;" — quem fecha com ';' ali e' o "retorne", nao a
    // chamada de sistema em si).
    ChamadaSistemaCrua analisarNucleoChamadaSistema() {
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
    ChamadaSistemaCrua analisarChamadaSistemaCrua() {
        ChamadaSistemaCrua chamada = analisarNucleoChamadaSistema();
        consumir(TipoToken::PONTO_VIRGULA, "esperado ';' após #chamada_sistema");
        return chamada;
    }

    // analisa: nome = #chamada_sistema id, arg1, arg2, ...;
    // captura o valor devolvido em x0(apos o "svc") na variavel "nome", ja declarada antes. Usada por chamadas de sistema que retornam
    // um valor de verdade, ex: o ponteiro alocado por mmap. Mesma gramatica de argumentos de analisarChamadaSistemaCrua, so com o destino na frente.
    ChamadaSistemaCrua analisarChamadaSistemaComDestino() {
        const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado nome da variável de destino");
        consumir(TipoToken::IGUAL, "esperado '=' antes de #chamada_sistema");
        ChamadaSistemaCrua chamada = analisarChamadaSistemaCrua();
        chamada.temDestino = true;
        chamada.nomeDestino = strdup(nome.valor);
        return chamada;
    }

    ChamadaFuncao analisarChamadaFuncao() {
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

    // analisa: primaria (("+"|"-"|"/"|"%") primária)*  - associativo a esquerda.
    // usada so em declaração de variavel e reatribuicao(unicos lugares que
    // aceitam aritmetica por enquanto). Monta a AST binaria empilhando pra
    // esquerda a cada operador encontrado.
    Expressao analisarExpressaoAditiva() {
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
    Expressao analisarConversao() {
        consumir(TipoToken::ABRE_PARENTESE, "esperado '(' no inicio da conversao de tipo");
        TipoVariavel tipoAlvo;
        if(verificar(TipoToken::NULO)) {
            pos++;
            consumir(TipoToken::VEZES, "esperado '*' apos 'nulo' na conversao de tipo(nulo sozinho nao e' um tipo de valor valido)");
            tipoAlvo = TipoVariavel::NULO_PONTEIRO;
        } else if(verificar(TipoToken::INT)) {
            pos++;
            if(verificar(TipoToken::VEZES)) {
                pos++;
                tipoAlvo = TipoVariavel::INT_PONTEIRO;
            } else {
                tipoAlvo = TipoVariavel::INT;
            }
        } else if(verificar(TipoToken::LONGO)) {
            pos++;
            if(verificar(TipoToken::VEZES)) {
                pos++;
                tipoAlvo = TipoVariavel::LONGO_PONTEIRO;
            } else {
                tipoAlvo = TipoVariavel::LONGO;
            }
        } else {
            consumir(TipoToken::CAR, "esperado 'int', 'longo', 'car' ou 'nulo' na conversao de tipo");
            if(verificar(TipoToken::VEZES)) {
                pos++;
                tipoAlvo = TipoVariavel::CAR_PONTEIRO;
            } else {
                tipoAlvo = TipoVariavel::CAR;
            }
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
    Expressao analisarExpressaoParentesizada() {
        consumir(TipoToken::ABRE_PARENTESE, "esperado '(' no inicio da expressao parentesizada");
        Expressao e = analisarExpressaoAditiva();
        consumir(TipoToken::FECHA_PARENTESE, "esperado ')' para fechar a expressao parentesizada");
        return e;
    }

    // analisa "bytes(int|car|car*|variavel)": tamanho em tempo de compilação,
    // resolvido de verdade no GeradorIR(que ja sabe o tipo de cada variável).
    // aqui só monta o no da AST guardando se veio de um tipo literal ou de
    // um identificador.
    Expressao analisarBytes() {
        pos++; // consome "bytes"
        consumir(TipoToken::ABRE_PARENTESE, "esperado '(' apos 'bytes'");

        Expressao e;
        e.tipo = Expressao::Tipo::BYTES;
        e.valor = nullptr;
        e.bytesNomeVariavel = nullptr;

        if(verificar(TipoToken::INT)) {
            pos++;
            e.bytesEhTipoLiteral = true;
            e.bytesTipoLiteral = TipoVariavel::INT;
        } else if(verificar(TipoToken::LONGO)) {
            pos++;
            if(verificar(TipoToken::VEZES)) {
                pos++;
                e.bytesEhTipoLiteral = true;
                e.bytesTipoLiteral = TipoVariavel::LONGO_PONTEIRO;
            } else {
                e.bytesEhTipoLiteral = true;
                e.bytesTipoLiteral = TipoVariavel::LONGO;
            }
        } else if(verificar(TipoToken::CAR)) {
            pos++;
            if(verificar(TipoToken::VEZES)) {
                pos++;
                e.bytesEhTipoLiteral = true;
                e.bytesTipoLiteral = TipoVariavel::CAR_PONTEIRO;
            } else {
                e.bytesEhTipoLiteral = true;
                e.bytesTipoLiteral = TipoVariavel::CAR;
            }
        } else {
            const Token& nome = consumir(TipoToken::IDENTIFICADOR, "esperado tipo(int/longo/car/car*/longo*) ou variavel dentro de 'bytes(...)'");
            e.bytesEhTipoLiteral = false;
            e.bytesNomeVariavel = strdup(nome.valor);
        }
        consumir(TipoToken::FECHA_PARENTESE, "esperado ')' apos argumento de 'bytes'");
        return e;
    }

    Expressao analisarExpressaoPrimaria() {
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
        if(verificar(TipoToken::ABRE_PARENTESE) && (tokens[pos + 1].tipo == TipoToken::INT || tokens[pos + 1].tipo == TipoToken::LONGO || tokens[pos + 1].tipo == TipoToken::CAR || tokens[pos + 1].tipo == TipoToken::NULO)) {
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
    Expressao analisarIndexacao() {
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
    Expressao analisarChamadaFuncaoComoExpressao() {
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
};