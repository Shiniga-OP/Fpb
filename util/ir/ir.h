// util/ir/ir.h
#pragma once
#include <stdlib.h>
#include "../config.h"

// IDs abstratos de chamada de sistema. Cada arquitetura traduz esses IDs para o número real de chamada da plataforma.
enum class ChamadaSistemaId {
    ESCREVER = 1,
    SAIR = 2,
    ALOCAR = 3, LIBERAR = 4,
    ALEATORIO = 5,
    ABRIR = 6, FECHAR = 7, LER = 8, POSICIONAR = 9,
    CLONE = 10, FUTEX = 11
};

// uma instrução IR genérica: "chame o sistema X com estes argumentos".
// os argumentos podem ser um literal de texto(referencia a um simbolo)
// ou um valor imediato(numero).
struct ArgumentoIR {
    enum class Tipo {
        SIMBOLO_TEXTO, IMEDIATO, REGISTRADOR
    } tipo;
    char* simbolo; // dono: quem criou o argumento(strdup). usado quando tipo == SIMBOLO_TEXTO
    long imediato = 0; // usado quando tipo == IMEDIATO
    int registrador = -1; // usado quando tipo == REGISTRADOR: indice x0..x5 (posição do parâmetro)
    int larguraBytes = 8; // largura em bytes do VALOR em si(não confundir com larguraElemento, que e' a largura do que um PONTEIRO aponta): 1 pra car, 4 pra int(32 bits), 8 pra longo/ponteiro(64 bits). Decide o registrador usado pela arquitetura(w de 32 bits pra 1 e 4, x de 64 bits pra 8) e a instrucao de memoria(strb/ldrb pra 1, str/ldr de 32 bits pra 4, str/ldr de 64 bits pra 8) na hora de materializar/gravar o valor
    bool sinalizado = true; // so importa pra larguraBytes 1 e 2: decide entre ldrsb/ldrsh/sxtb/sxth(true) e ldrb/ldrh/uxtb/uxth(false) ao carregar o valor. car e bool são sem sinal, byte e curto com sinal
    bool elementoSinalizado = false; // so quando este ArgumentoIR e um PONTEIRO: sinal do elemento apontado, aplicado ao resultado de "nome[indice]"
    int larguraElemento = 1; // usado so quando este ArgumentoIR representa um PONTEIRO sendo indexado(nome[indice]): tamanho em bytes de CADA elemento apontado(1 pra car*, 8 pra int*, ou qualquer outro valor se novos tipos forem adicionados no futuro). indice sempre conta em ELEMENTOS(igual C): endereco = ponteiro + indice*larguraElemento

    int idEspaco = -1; // indice na TabelaEspacos do GeradorIR se esta variavel e um espaco(struct) ou ponteiro pra um; -1 se nao
    bool espacoPorValor = false; // true so pra variavel declarada "Nome var = {...}"(o corpo mora no frame): proibe "a = b"
    bool ehPonteiro = false; // true se o slot guarda um endereço(variável/parâmetro ponteiro, array, ou resultado de "@"): "@nome" devolve o conteúdo do slot em vez do endereço do slot
    bool ehFlutuante = false; // true pra flu(4 bytes) e dobro(8 bytes): o valor(ou o imediato) guarda o padrão de bits do float/double, e a arquitetura opera nos registradores de ponto flutuante
    bool elementoFlutuante = false; // so quando este ArgumentoIR e um PONTEIRO: true se o elemento apontado é flu/dobro, aplicado ao resultado de "nome[indice]"
    bool ehTemporario = false; // true se é o resultado de um operador de espaço(Nome* de heap ainda sem dono): quem recebe move o conteúdo e devolve a casca
    bool ehFuncao = false; // true se tipo == SIMBOLO_TEXTO guarda o símbolo de uma função("@funcao"): o endereço vem pela GOT, não por adrp/add direto
    
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
    bool resultadoFlutuante = false; // true se a função devolve flu/dobro(valor em s0/d0 em vez de x0), valido so se capturaResultado == true
    bool indireta = false; // true se a chamada é por endereço("f();" com "nulo* f = @(endereco);"): "nome" fica nullptr e a arquitetura emite "blr" no endereço guardado em enderecoAlvo
    ArgumentoIR enderecoAlvo; // valido so se indireta == true: REGISTRADOR, slot da variável que guarda o endereço da função

    void iniciar() {
        nome = nullptr;
        argumentos.iniciar();
        capturaResultado = false;
        slotDestino = -1;
        resultadoLargura = 8;
        resultadoFlutuante = false;
        indireta = false;
    }
    void liberar() {
        free(nome);
        nome = nullptr;
        for(int i = 0; i < argumentos.tam; i++) argumentos[i].liberar();
        argumentos.liberar();
        if(indireta) enderecoAlvo.liberar();
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
enum class OperadorAritmeticoIR { SOMA, SUBTRACAO, MULTIPLICACAO, DIVISAO, MODULO, E_BIT, OU_BIT, XOU, DESL_ESQ, DESL_DIR };

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

struct InstrucaoEnderecoFrame {
    int slotDestino;
    int posArray; // pos dentro da área de arrays do frame
    void liberar() {}
};

// slotDestino = endereço(na pilha) do slot slotOrigem. Usada por "@nome" quando "nome" é escalar. O alocador de registradores
// deixa slotOrigem sempre na pilha, pois registrador físico não tem endereço.
struct InstrucaoEnderecoSlot {
    int slotDestino;
    int slotOrigem;
    void liberar() {}
};

// slotDestino = (tipo destino)origem entre inteiro e flutuante: scvtf(int -> flu/dobro), fcvtzs(flu/dobro -> int) e fcvt(flu <-> dobro).
// "larguraDestino" é a largura em bytes do valor gravado no slot e "destinoFlutuante" diz se o destino é flu/dobro ou inteiro.
struct InstrucaoConversaoNumerica {
    int slotDestino;
    ArgumentoIR origem;
    int larguraDestino;
    bool destinoFlutuante;

    void liberar() {
        origem.liberar();
    }
};

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
    ArgumentoIR valor; // largura e sinal de ORIGEM: o valor e carregado com eles(extensão correta) e depois gravado com larguraDestino
    int larguraDestino = 8; // largura em bytes da gravação no slot(tipo da variável de destino)

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

// registrador de sistema da arquitetura(cada arquitetura traduz pro registrador real: TLS = tpidr_el0 no ARM64).
enum class RegistradorSistemaId { TLS };

// le o registrador de sistema pro slotDestino, ou escreve "valor" nele(escrever == true).
struct InstrucaoRegistradorSistema {
    RegistradorSistemaId registrador;
    bool escrever = false;
    int slotDestino = -1; // so na leitura
    ArgumentoIR valor; // so na escrita

    void liberar() {
        if(escrever) valor.liberar();
    }
};

// um comando no nivel de IR: chamada de sistema crua(#chamada_sistema direto no corpo), chamada a uma função de usuario(bl), desvio condicional(se), laço(enquanto), reatribuição de variável, operação aritméticas, leitura/escrita indexada(ponteiro[indice]) ou conversão numérica.
struct ComandoIR {
    enum class Tipo {
        CHAMADA_SISTEMA_CRUA, CHAMADA_FUNCAO_USUARIO,
        SE, ENQUANTO, REATRIBUICAO, OPERACAO_ARITMETICA,
        RETORNE, LEITURA_INDEXADA, ESCRITA_INDEXADA,
        ENDERECO_FRAME, ENDERECO_SLOT, CONVERSAO_NUMERICA,
        REGISTRADOR_SISTEMA
    } tipo;
    InstrucaoChamadaSistemaCrua chamadaSistemaCrua;
    InstrucaoChamadaFuncaoUsuario chamadaFuncaoUsuario;
    InstrucaoSe instrucaoSe;
    InstrucaoEnquanto instrucaoEnquanto;
    InstrucaoReatribuicao instrucaoReatribuicao;
    InstrucaoOperacaoAritmetica instrucaoOperacaoAritmetica;
    InstrucaoRetorne instrucaoRetorne;
    InstrucaoLeituraIndexada instrucaoLeituraIndexada;
    InstrucaoEscritaIndexada instrucaoEscritaIndexada;
    InstrucaoEnderecoFrame instrucaoEnderecoFrame;
    InstrucaoEnderecoSlot instrucaoEnderecoSlot;
    InstrucaoConversaoNumerica instrucaoConversaoNumerica;
    InstrucaoRegistradorSistema instrucaoRegistradorSistema;

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
        else if(tipo == Tipo::ENDERECO_FRAME) instrucaoEnderecoFrame.liberar();
        else if(tipo == Tipo::ENDERECO_SLOT) instrucaoEnderecoSlot.liberar();
        else if(tipo == Tipo::CONVERSAO_NUMERICA) instrucaoConversaoNumerica.liberar();
        else if(tipo == Tipo::REGISTRADOR_SISTEMA) instrucaoRegistradorSistema.liberar();
    }
};

// libera cada comando de um bloco alocado com new, o próprio bloco, e zera o ponteiro. Aceita nullptr(senão ausente).
inline void liberarBloco(Vetor<ComandoIR>*& bloco) {
    if(!bloco) return;
    for(int i = 0; i < bloco->tam; i++) (*bloco)[i].liberar();
    bloco->liberar();
    delete bloco;
    bloco = nullptr;
}

inline void InstrucaoSe::liberar() {
    liberarBloco(entao);
    liberarBloco(senaoCorpo);
    esquerda.liberar();
    direita.liberar();
}

inline void InstrucaoEnquanto::liberar() {
    liberarBloco(comandosCondicao);
    liberarBloco(corpo);
    esquerda.liberar();
    direita.liberar();
}

// ===== percorrer a IR =====
// quem precisa descer em SE/ENQUANTO escreve só o tratamento de UM bloco e passa a si mesmo como "fn":
// paraCadaFilho chama fn(bloco, ctx) pra cada bloco aninhado do comando, na ordem de emissão
// (SE: então, senão; ENQUANTO: comandosCondicao, corpo). Comando sem bloco aninhado não chama nada.
template<typename T>
inline void paraCadaFilho(const ComandoIR& c, void (*fn)(const Vetor<ComandoIR>&, T&), T& ctx) {
    if(c.tipo == ComandoIR::Tipo::SE) {
        fn(*c.instrucaoSe.entao, ctx);
        if(c.instrucaoSe.temSenao) fn(*c.instrucaoSe.senaoCorpo, ctx);
    } else if(c.tipo == ComandoIR::Tipo::ENQUANTO) {
        fn(*c.instrucaoEnquanto.comandosCondicao, ctx);
        fn(*c.instrucaoEnquanto.corpo, ctx);
    }
}

// chama fn(arg, ctx) pra cada ArgumentoIR LIDO pelo próprio comando(não desce nos blocos aninhados: ver paraCadaFilho).
// SE/ENQUANTO: os dois lados da comparação. O slot gravado não entra aqui: ver slotDefinidoPor.
template<typename T>
inline void paraCadaOperando(const ComandoIR& c, void (*fn)(const ArgumentoIR&, T&), T& ctx) {
    if(c.tipo == ComandoIR::Tipo::REATRIBUICAO) {
        fn(c.instrucaoReatribuicao.valor, ctx);
    } else if(c.tipo == ComandoIR::Tipo::OPERACAO_ARITMETICA) {
        fn(c.instrucaoOperacaoAritmetica.esquerda, ctx);
        fn(c.instrucaoOperacaoAritmetica.direita, ctx);
    } else if(c.tipo == ComandoIR::Tipo::CHAMADA_SISTEMA_CRUA) {
        for(int j = 0; j < c.chamadaSistemaCrua.argumentos.tam; j++) fn(c.chamadaSistemaCrua.argumentos[j], ctx);
    } else if(c.tipo == ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO) {
        for(int j = 0; j < c.chamadaFuncaoUsuario.argumentos.tam; j++) fn(c.chamadaFuncaoUsuario.argumentos[j], ctx);
        if(c.chamadaFuncaoUsuario.indireta) fn(c.chamadaFuncaoUsuario.enderecoAlvo, ctx);
    } else if(c.tipo == ComandoIR::Tipo::RETORNE) {
        if(c.instrucaoRetorne.temValor) fn(c.instrucaoRetorne.valor, ctx);
    } else if(c.tipo == ComandoIR::Tipo::LEITURA_INDEXADA) {
        fn(c.instrucaoLeituraIndexada.ponteiro, ctx);
        fn(c.instrucaoLeituraIndexada.indice, ctx);
    } else if(c.tipo == ComandoIR::Tipo::ESCRITA_INDEXADA) {
        fn(c.instrucaoEscritaIndexada.ponteiro, ctx);
        fn(c.instrucaoEscritaIndexada.indice, ctx);
        fn(c.instrucaoEscritaIndexada.valor, ctx);
    } else if(c.tipo == ComandoIR::Tipo::SE) {
        fn(c.instrucaoSe.esquerda, ctx);
        fn(c.instrucaoSe.direita, ctx);
    } else if(c.tipo == ComandoIR::Tipo::ENQUANTO) {
        fn(c.instrucaoEnquanto.esquerda, ctx);
        fn(c.instrucaoEnquanto.direita, ctx);
    } else if(c.tipo == ComandoIR::Tipo::CONVERSAO_NUMERICA) {
        fn(c.instrucaoConversaoNumerica.origem, ctx);
    } else if(c.tipo == ComandoIR::Tipo::REGISTRADOR_SISTEMA) {
        if(c.instrucaoRegistradorSistema.escrever) fn(c.instrucaoRegistradorSistema.valor, ctx);
    }
}

// slot que o comando grava, ou -1 se não grava nenhum(ou se só grava quando capturaResultado é false).
inline int slotDefinidoPor(const ComandoIR& c) {
    if(c.tipo == ComandoIR::Tipo::REATRIBUICAO) return c.instrucaoReatribuicao.slotDestino;
    if(c.tipo == ComandoIR::Tipo::OPERACAO_ARITMETICA) return c.instrucaoOperacaoAritmetica.slotDestino;
    if(c.tipo == ComandoIR::Tipo::CHAMADA_SISTEMA_CRUA) return c.chamadaSistemaCrua.capturaResultado ? c.chamadaSistemaCrua.slotDestino : -1;
    if(c.tipo == ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO) return c.chamadaFuncaoUsuario.capturaResultado ? c.chamadaFuncaoUsuario.slotDestino : -1;
    if(c.tipo == ComandoIR::Tipo::LEITURA_INDEXADA) return c.instrucaoLeituraIndexada.slotDestino;
    if(c.tipo == ComandoIR::Tipo::ENDERECO_FRAME) return c.instrucaoEnderecoFrame.slotDestino;
    if(c.tipo == ComandoIR::Tipo::ENDERECO_SLOT) return c.instrucaoEnderecoSlot.slotDestino;
    if(c.tipo == ComandoIR::Tipo::CONVERSAO_NUMERICA) return c.instrucaoConversaoNumerica.slotDestino;
    if(c.tipo == ComandoIR::Tipo::REGISTRADOR_SISTEMA) return c.instrucaoRegistradorSistema.escrever ? -1 : c.instrucaoRegistradorSistema.slotDestino;
    return -1;
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
    Vetor<bool> parametrosFlutuante; // paralelo a parametrosLargura: true se o parâmetro é flu/dobro(chega em s0..s7/d0..d7 em vez de x0..x7)
    bool retornoFlutuante = false; // true se a função devolve flu/dobro(valor em s0/d0)
    int retornoLargura = 8; // largura em bytes do valor devolvido(4 pra flu, 8 pra dobro), usado quando retornoFlutuante == true

    // mapa slot -> indice de registrador fisico(0..TOTAL_REGISTRADORES_FISICOS-1,
    // correspondendo a x19..x27, todos salvos: ver comentario de
    // TOTAL_REGISTRADORES_FISICOS acima) preenchido pelo alocador de "liberação por ultimo uso" em GeradorIR::alocarRegistradores(). DONO_SLOT_NENHUM
    // significa que o slot não ganhou registrador físico e continua sendo
    // acessado via pilha(ldr/str), igual ao comportamento antigo. Tamanho
    // igual a totalSlots; indice by slot.
    Vetor<int> registradorDoSlot;

    int bytesArrays = 0;
    
    bool ehExportada = false;
    bool ehIniciador = false; // true só pro _iniciar_globais de uma biblioteca: vai pra .init_array e o carregador dinâmico chama ao carregar a .so(num executável quem chama é o "inicio")

    void iniciar() {
        nome = nullptr;
        instrucoes.iniciar();
        totalParametros = 0;
        totalSlots = 0;
        bytesArrays = 0;
        ehInicio = false;
        ehIniciador = false;
        parametrosLargura.iniciar();
        parametrosFlutuante.iniciar();
        retornoFlutuante = false;
        retornoLargura = 8;
        registradorDoSlot.iniciar();
    }
    void liberar() {
        free(nome);
        nome = nullptr;
        for(int i = 0; i < instrucoes.tam; i++) instrucoes[i].liberar();
        instrucoes.liberar();
        parametrosLargura.liberar();
        parametrosFlutuante.liberar();
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

// uma variável global(campo "estatico" de uma #classe): vai pra seção .data com "valorInicial", "largura" bytes
struct VariavelGlobalIR {
    char* simbolo; // dono: quem criou a variável(strdup)
    int largura;
    long valorInicial;

    void liberar() {
        free(simbolo);
        simbolo = nullptr;
    }
};

struct ProgramaIR {
    Vetor<DadoEstaticoIR> dados;
    Vetor<VariavelGlobalIR> globais;
    Vetor<FuncaoIR> funcoes;
    bool ehBibli = false;

    void iniciar() {
        dados.iniciar();
        globais.iniciar();
        funcoes.iniciar();
    }
    void liberar() {
        for(int i = 0; i < dados.tam; i++) dados[i].liberar();
        dados.liberar();
        for(int i = 0; i < globais.tam; i++) globais[i].liberar();
        globais.liberar();
        for(int i = 0; i < funcoes.tam; i++) funcoes[i].liberar();
        funcoes.liberar();
    }
};