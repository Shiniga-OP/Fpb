// util/ir/ir.h
#pragma once
#include <stdlib.h>
#include "../config.h"

// IDs abstratos de chamada de sistema. Cada arquitetura traduz esses IDs para o número real de chamada da plataforma.
enum class ChamadaSistemaId {
    ESCREVER = 1,
    SAIR = 2,
    ALOCAR = 3,
    LIBERAR = 4
};

// uma instrução IR genérica: "chame o sistema X com estes argumentos".
// os argumentos podem ser um literal de texto(referencia a um simbolo)
// ou um valor imediato(numero).
struct ArgumentoIR {
    enum class Tipo { SIMBOLO_TEXTO, IMEDIATO, REGISTRADOR } tipo;
    char* simbolo; // dono: quem criou o argumento(strdup). usado quando tipo == SIMBOLO_TEXTO
    long imediato = 0; // usado quando tipo == IMEDIATO
    int registrador = -1; // usado quando tipo == REGISTRADOR: indice x0..x5 (posição do parâmetro)
    int larguraBytes = 8; // largura em bytes do VALOR em si(não confundir com larguraElemento, que e' a largura do que um PONTEIRO aponta): 1 pra car, 4 pra int(32 bits), 8 pra longo/ponteiro(64 bits). Decide o registrador usado pela arquitetura(w de 32 bits pra 1 e 4, x de 64 bits pra 8) e a instrucao de memoria(strb/ldrb pra 1, str/ldr de 32 bits pra 4, str/ldr de 64 bits pra 8) na hora de materializar/gravar o valor
    int larguraElemento = 1; // usado so quando este ArgumentoIR representa um PONTEIRO sendo indexado(nome[indice]): tamanho em bytes de CADA elemento apontado(1 pra car*, 8 pra int*, ou qualquer outro valor se novos tipos forem adicionados no futuro). indice sempre conta em ELEMENTOS(igual C): endereco = ponteiro + indice*larguraElemento

    void liberar() {
        free(simbolo);
        simbolo = nullptr;
    }
};

// instrução IR de chamada de sistema crua: primeiro argumento e o ID
// ABSTRATO da chamada de sistema(ChamadaSistemaId, ex: ESCREVER=1, SAIR=2).
// cada arquitetura traduz esse ID pro numero real de chamada de sistema da plataforma(posicao x8), e os demais argumentos são posicionais (x0, x1, x2, ...),
// na mesma ordem em que foram escritos na linguagem.
struct InstrucaoChamadaSistemaCrua {
    Vetor<ArgumentoIR> argumentos; // argumentos[0] = ID abstrato da chamada de sistema
    bool capturaResultado; // true se "nome = #chamada_sistema ..." foi usada: grava x0(retorno da chamada de sistema, apos o "svc") num slot
    int slotDestino; // valido so se capturaResultado == true

    void iniciar() {
        argumentos.iniciar();
        capturaResultado = false;
        slotDestino = -1;
    }
    void liberar() {
        for(int i = 0; i < argumentos.tam; i++) argumentos[i].liberar();
        argumentos.liberar();
    }
};

// instrução IR de chamada a uma função de usuário(bl nome): argumentos
// posicionais vão para x0, x1, x2... na ordem em que foram escritos.
struct InstrucaoChamadaFuncaoUsuario {
    char* nome; // dono: quem criou a instrução(strdup). nome da funcao de usuário chamada
    Vetor<ArgumentoIR> argumentos;
    bool capturaResultado = false; // true se o valor de retorno(x0) precisa ser gravado num slot(chamada usada como expressao, ex: "int x = f();")
    int slotDestino = -1; // valido so se capturaResultado == true
    int resultadoLargura = 8; // largura em bytes da gravacao no slot(1=car, 4=int, 8=longo/ponteiro), valido so se capturaResultado == true

    void iniciar() {
        nome = nullptr;
        argumentos.iniciar();
        capturaResultado = false;
        slotDestino = -1;
        resultadoLargura = 8;
    }
    void liberar() {
        free(nome);
        nome = nullptr;
        for(int i = 0; i < argumentos.tam; i++) argumentos[i].liberar();
        argumentos.liberar();
    }
};

// comando "retorne expr;" ou "retorne;": materializa(se houver) o valor em
// x0 e desvia pro epilogo o único da função(rotulo "_fim_<nome>").
struct InstrucaoRetorne {
    bool temValor;
    ArgumentoIR valor; // válido so se temValor == true

    void liberar() {
        if(temValor) valor.liberar();
    }
};

// mesmo operador da AST, copiado aqui pra o IR não depender de ast.h.
enum class OperadorComparacaoIR { IGUAL, DIFERENTE, MAIOR, MENOR, MAIOR_IGUAL, MENOR_IGUAL };

// mesmo operador aritmetico da AST, copiado aqui pra o IR não depender de ast.h.
// mesmo operador aritmetico da AST, copiado aqui pra o IR não depender de ast.h.
enum class OperadorAritmeticoIR { SOMA, SUBTRACAO, MULTIPLICACAO, DIVISAO, MODULO };

// calcula esquerda OP direita e grava o resultado no slot destino(mesmo
// espaço de slots de InstrucaoReatribuicao: 0..5 são parâmetros, dai em
// diante são variáveis/temporários do frame da função).
struct InstrucaoOperacaoAritmetica {
    int slotDestino;
    ArgumentoIR esquerda;
    OperadorAritmeticoIR operador;
    ArgumentoIR direita;

    void liberar() {
        esquerda.liberar();
        direita.liberar();
    }
};

// propagação: ComandoIR e InstrucaoSe se referenciam mutuamente. Os blocos
// então/senão são guardados por ponteiro(alocados no heap) pra quebrar a
// recursão de tamanho, mesma solução usada em ast.h(ComandoSe).
struct ComandoIR;

// desvio condicional no nível de IR: compara esquerda OP direita e executa
// o bloco "então" ou "senaoCorpo"(se existir).
struct InstrucaoSe {
    ArgumentoIR esquerda;
    OperadorComparacaoIR operador;
    ArgumentoIR direita;
    Vetor<ComandoIR>* entao; // dono: quem criou a instrucao(alocado com new)
    Vetor<ComandoIR>* senaoCorpo; // dono: idem. só valido se temSenao == true
    bool temSenao = false;

    void iniciar() {
        entao = new Vetor<ComandoIR>();
        entao->iniciar();
        senaoCorpo = nullptr;
        temSenao = false;
    }
    void liberar();
};

// laço no nível de IR: enquanto esquerda OP direita for verdadeiro,
// repete "corpo". Mesma forma de InstrucaoSe(só sem "senao").
struct InstrucaoEnquanto {
    Vetor<ComandoIR>* comandosCondicao; // dono: quem criou a instrução(alocado com new). instruções auxiliares pra materializar esquerda/direita(ex: LEITURA_INDEXADA de "txt[i]"), reemitidas no TOPO do laço a cada iteracao(não uma unica vez antes dele), pois a condição pode depender de estado que muda dentro do corpo
    ArgumentoIR esquerda;
    OperadorComparacaoIR operador;
    ArgumentoIR direita;
    Vetor<ComandoIR>* corpo; // dono: quem criou a instrucao(alocado com new)

    void iniciar() {
        comandosCondicao = new Vetor<ComandoIR>();
        comandosCondicao->iniciar();
        corpo = new Vetor<ComandoIR>();
        corpo->iniciar();
    }
    void liberar();
};

// escreve "valor" no slot(pilha) de uma variavel ja declarada. "slotDestino"
// e o mesmo indice usado pelos parâmetros(ArgumentoIR::registrador): 0..5
// caem nos registradores de parâmetro salvos pelo prologo, e dai em diante
// e so mais um slot de 16 bytes no frame da função.
struct InstrucaoReatribuicao {
    int slotDestino;
    ArgumentoIR valor;

    void liberar() {
        valor.liberar();
    }
};

// le "larguraElemento" bytes(1, 8, ou qualquer largura futura) do endereco
// ponteiro+(indice*larguraElemento) e grava o resultado no slot destino
// (mesmo espaço de slots de InstrucaoReatribuicao). Usada pra "int b = txt[i];"
// ou qualquer outro uso de "ponteiro[indice]" como valor. "ponteiro.larguraElemento"
// decide a largura de leitura; indice sempre conta em ELEMENTOS.
struct InstrucaoLeituraIndexada {
    int slotDestino;
    ArgumentoIR ponteiro; // REGISTRADOR: slot(parâmetro ou variável) do ponteiro sendo indexado
    ArgumentoIR indice;

    void liberar() {
        ponteiro.liberar();
        indice.liberar();
    }
};

// grava "larguraElemento" bytes(1, 8, ou qualquer largura futura) no endereco
// ponteiro+(indice*larguraElemento). Usada pra "txt[i] = valor;". Mesma
// regra de largura de InstrucaoLeituraIndexada acima.
struct InstrucaoEscritaIndexada {
    ArgumentoIR ponteiro; // REGISTRADOR: slot(parâmetro ou variável) do ponteiro sendo indexado
    ArgumentoIR indice;
    ArgumentoIR valor;

    void liberar() {
        ponteiro.liberar();
        indice.liberar();
        valor.liberar();
    }
};

// um comando no nivel de IR: chamada de sistema crua(#chamada_sistema direto no corpo), chamada a uma função de usuario(bl), desvio condicional(se), laço(enquanto), reatribuição de variável, operação aritméticas, ou leitura/escrita indexada(ponteiro[indice]).
struct ComandoIR {
    enum class Tipo { CHAMADA_SISTEMA_CRUA, CHAMADA_FUNCAO_USUARIO, SE, ENQUANTO, REATRIBUICAO, OPERACAO_ARITMETICA, RETORNE, LEITURA_INDEXADA, ESCRITA_INDEXADA } tipo;
    InstrucaoChamadaSistemaCrua chamadaSistemaCrua;
    InstrucaoChamadaFuncaoUsuario chamadaFuncaoUsuario;
    InstrucaoSe instrucaoSe;
    InstrucaoEnquanto instrucaoEnquanto;
    InstrucaoReatribuicao instrucaoReatribuicao;
    InstrucaoOperacaoAritmetica instrucaoOperacaoAritmetica;
    InstrucaoRetorne instrucaoRetorne;
    InstrucaoLeituraIndexada instrucaoLeituraIndexada;
    InstrucaoEscritaIndexada instrucaoEscritaIndexada;

    void liberar() {
        if(tipo == Tipo::CHAMADA_SISTEMA_CRUA) chamadaSistemaCrua.liberar();
        else if(tipo == Tipo::CHAMADA_FUNCAO_USUARIO) chamadaFuncaoUsuario.liberar();
        else if(tipo == Tipo::SE) instrucaoSe.liberar();
        else if(tipo == Tipo::ENQUANTO) instrucaoEnquanto.liberar();
        else if(tipo == Tipo::REATRIBUICAO) instrucaoReatribuicao.liberar();
        else if(tipo == Tipo::OPERACAO_ARITMETICA) instrucaoOperacaoAritmetica.liberar();
        else if(tipo == Tipo::RETORNE) instrucaoRetorne.liberar();
        else if(tipo == Tipo::LEITURA_INDEXADA) instrucaoLeituraIndexada.liberar();
        else if(tipo == Tipo::ESCRITA_INDEXADA) instrucaoEscritaIndexada.liberar();
    }
};

inline void InstrucaoSe::liberar() {
    for(int i = 0; i < entao->tam; i++) (*entao)[i].liberar();
    entao->liberar();
    delete entao;
    entao = nullptr;
    if(senaoCorpo) {
        for(int i = 0; i < senaoCorpo->tam; i++) {
            (*senaoCorpo)[i].liberar();
        }
        senaoCorpo->liberar();
        delete senaoCorpo;
        senaoCorpo = nullptr;
    }
    esquerda.liberar();
    direita.liberar();
}

inline void InstrucaoEnquanto::liberar() {
    for(int i = 0; i < comandosCondicao->tam; i++) (*comandosCondicao)[i].liberar();
    comandosCondicao->liberar();
    delete comandosCondicao;
    comandosCondicao = nullptr;
    for(int i = 0; i < corpo->tam; i++) (*corpo)[i].liberar();
    corpo->liberar();
    delete corpo;
    corpo = nullptr;
    esquerda.liberar();
    direita.liberar();
}

// número de registradores fisicos disponíveis pro alocador de "liberação por
// ultimo uso"(x19..x27, 9 registradores). Restrito a essa faixa de proposito:
// AAPCS64(convenção de chamada ARM64) define x9..x18 como salvado(uma
// função chamada via "bl" pode sujar livremente, sem restaurar) e x19..x28
// como salvado(a função que os usa e' responsavel por salvar no
// prologo e restaurar no epilogo). Um slot que atravessa uma CHAMADA_FUNCAO_USUARIO(ex: "int q = a/b; escrever(...); int r = a%b;", onde 'a'/'b'
// continuam vivos depois do "escrever(...)") só pode ficar num registrador
// fisico se esse registrador sobreviver a chamada — por isso o reuso nao
// pode incluir x11..x18(bug ja visto na prática: variavel sobrevivendo a
// uma chamada de função perdia seu valor, pois a função chamada reusava o
// mesmo registrador fisico pros PRÓPRIOS slots dela, sem nenhuma obrigação
// de preservar). x28 fica de fora do reuso tambem(não por causa de
// chamado/salvado, mas reservado como rascunho extra da arquitetura,
// usado so por MODULO em emitirOperacaoAritmetica). Fora dessa faixa:
// x0-x5(parametros/args), x8(número de chamada de sistema), x9/x10(rascunho
// da arquitetura), x28(rascunho extra, ver acima), x29/x30(frame). A
// arquitetura(arm64.h) e' responsavel por emitir puxar/pegar desses
// registradores no prologo/epilogo de qualquer função. Ver DONO_SLOT_NENHUM abaixo pra quando um
// slot nao ganha registrador fisico(estoura pra pilha, comportamento antigo).
#define TOTAL_REGISTRADORES_FISICOS 9
#define DONO_SLOT_NENHUM (-1)

// uma função no nivel de IR: uma sequência de comandos.
struct FuncaoIR {
    char* nome; // dono: quem criou a função IR (strdup)
    Vetor<ComandoIR> instrucoes;
    int totalParametros = 0; // quantos x0..x5 esta função recebe
    int totalSlots = 0; // parâmetros + variaveis declaradas no corpo; dimensiona o frame
    bool ehInicio = false; // caso especial: nunca emite "ret"
    bool temRetornoTipado = false; // true se a funcao foi declarada com int/car/car*(exige "retorne" em vez do "sair implicito"); usado so pra decidir se emite o rotulo de fim(toda funcao com "retorne" no meio do corpo precisa dele, independente do tipo, mas mantido explicito aqui pra clareza)
    Vetor<int> parametrosLargura; // paralelo aos primeiros totalParametros slots: largura em bytes do tipo do parametro correspondente(1=car, 4=int, 8=longo/ponteiro), usado pelo prologo da arquitetura pra gravar com a largura certa(strb/str32/str64)

    // mapa slot -> indice de registrador fisico(0..TOTAL_REGISTRADORES_FISICOS-1,
    // correspondendo a x19..x27, todos salvos: ver comentario de
    // TOTAL_REGISTRADORES_FISICOS acima) preenchido pelo alocador de "liberação por ultimo uso" em GeradorIR::alocarRegistradores(). DONO_SLOT_NENHUM
    // significa que o slot não ganhou registrador físico e continua sendo
    // acessado via pilha(ldr/str), igual ao comportamento antigo. Tamanho
    // igual a totalSlots; indice by slot.
    Vetor<int> registradorDoSlot;

    void iniciar() {
        nome = nullptr;
        instrucoes.iniciar();
        totalParametros = 0;
        totalSlots = 0;
        ehInicio = false;
        parametrosLargura.iniciar();
        registradorDoSlot.iniciar();
    }
    void liberar() {
        free(nome);
        nome = nullptr;
        for(int i = 0; i < instrucoes.tam; i++) instrucoes[i].liberar();
        instrucoes.liberar();
        parametrosLargura.liberar();
        registradorDoSlot.liberar();
    }
};

// um dado estático(texto literal) que precisa ir pra secao .rodata
struct DadoEstaticoIR {
    char* simbolo; // dono: quem criou o dado(strdup). ex: _ola_mundo
    char* valor; // dono: quem criou o dado(strdup). conteudo cru da texto
    int tamanho; // tamanho de "valor" em bytes(não depende de strlen)

    void liberar() {
        free(simbolo);
        free(valor);
        simbolo = nullptr;
        valor = nullptr;
    }
};

struct ProgramaIR {
    Vetor<DadoEstaticoIR> dados;
    Vetor<FuncaoIR> funcoes;

    void iniciar() {
        dados.iniciar();
        funcoes.iniciar();
    }
    void liberar() {
        for(int i = 0; i < dados.tam; i++) dados[i].liberar();
        dados.liberar();
        for(int i = 0; i < funcoes.tam; i++) funcoes[i].liberar();
        funcoes.liberar();
    }
};