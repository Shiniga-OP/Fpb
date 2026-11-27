// util/ir/geradorir.h
#pragma once
#include <string>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <initializer_list>
#include "../config.h"
#include "../ast.h"
#include "ir.h"
#include "tabelasir.h"
#include "alocadorreg.h"
#include "carregadorbiblis.h"
#include "../preprocessador.h"

// estado compartilhado por todos os métodos que geram UMA função: a
// declaração sendo gerada, a lista de dados estáticos do programa e a tabela
// de variáveis locais(vale só durante esta função). A lista de comandos
// de saída("saida") NÃO entra aqui de propósito: ela muda a cada bloco
// aninhado(corpo de "se"/"enquanto", comandos da condição), então continua
// sendo passada à parte.
struct ContextoFuncao {
    const DeclaracaoFuncao& funcao;
    Vetor<DadoEstaticoIR>& dados;
    TabelaVariaveisLocais variaveisLocais;

    Vetor<int> liberarSlot; // variáveis por valor com "liberar", na ordem de declaração(liberadas em ordem inversa)
    Vetor<int> liberarId;
    Vetor<int> tempSlot; // temporários do comando atual(resultado de operador, texto literal convertido)
    Vetor<int> tempId;
    Vetor<bool> tempHeap; // true se a casca também mora no heap(resultado de operador)

    ContextoFuncao(const DeclaracaoFuncao& funcao, Vetor<DadoEstaticoIR>& dados) : funcao(funcao), dados(dados) {
        liberarSlot.iniciar();
        liberarId.iniciar();
        tempSlot.iniciar();
        tempId.iniciar();
        tempHeap.iniciar();
    }
    void liberar() {
        liberarSlot.liberar();
        liberarId.liberar();
        tempSlot.liberar();
        tempId.liberar();
        tempHeap.liberar();
    }
};

class GeradorIR {
public:
    std::string diretorioBase;
    TabelaFuncoesUsuario funcoesUsuario; // assinaturas: programa principal + todas as bibliotecas incluidas(direta ou transitivamente)
    TabelaCorposFuncoesUsuario corposFuncoesUsuario; // nome -> AST completa(corpo), usada pra eliminação sob demanda
    TabelaEspacos espacos; // #espaco(struct) do programa principal + bibliotecas incluidas; o indice e o idEspaco
    CarregadorBiblis carregador; // dono das ASTs das bibliotecas: corposFuncoesUsuario aponta pra dentro delas, entao precisa viver até o fim de gerar()
    int contadorSimbolos = 0;
    int bytesArraysAtual = 0;
    VetorStr ligacoes;
    Vetor<VariavelGlobalIR> globais; // campos estaticos usados pelas funções geradas(sem repetir); passam pro ProgramaIR no fim de gerar()
    Vetor<const DeclaracaoVariavel*> globaisDeclarados;

    // diretorioBase: diretório a partir do qual os "#incluir" são resolvidos.
    explicit GeradorIR(const std::string& diretorioBase, Preprocessador* pre = nullptr) : diretorioBase(diretorioBase) {
        carregador.preprocessador = pre;
        funcoesUsuario.iniciar();
        corposFuncoesUsuario.iniciar();
        espacos.iniciar();
        ligacoes.iniciar();
        globais.iniciar();
        globaisDeclarados.iniciar();
    }

    ~GeradorIR() {
        funcoesUsuario.liberar();
        corposFuncoesUsuario.liberar();
        espacos.liberar();
        ligacoes.liberar();
        globais.liberar();
        globaisDeclarados.liberar();
    }

    // ===== declarações: cada método é definido(inline) no leitor indicado =====

    // === gerador_tipos.h ===
    int larguraDoTipo(TipoVariavel tipo);
    bool tipoEhFlutuante(TipoVariavel tipo);
    int larguraElementoApontado(TipoVariavel tipoPonteiro);
    bool tipoEhSinalizado(TipoVariavel tipo);
    bool elementoEhSinalizado(TipoVariavel tipoPonteiro);
    OperadorAritmeticoIR traduzirOperadorAritmetico(OperadorAritmetico op);
    OperadorComparacaoIR traduzirOperadorComparacao(OperadorComparacao op);
    ArgumentoIR imediatoInt(long valor);
    ArgumentoIR referenciaSlotInt(int slot);
    ArgumentoIR copiarArgumento(const ArgumentoIR& original);
    char* gerarNomeSimbolo(const char* textoOriginal);
    bool elementoEhFlutuante(TipoVariavel tipoPonteiro);
    ArgumentoIR constanteDoTipo(long valor, TipoVariavel tipo);
    ArgumentoIR argumentoSlot(int slot, TipoVariavel tipo);
    ArgumentoIR argumentoSlotLargura(int slot, int largura, bool flutuante = false);

    // === gerador_funcoes.h ===
    ProgramaIR gerar(const Programa& programa);
    void registrarFuncoes(const Programa& programa, const std::string& origem = "programa principal");
    void gerarFuncaoAlcancavel(const char* nome, ProgramaIR& ir, VetorStr& jaGeradas);
    FuncaoIR gerarFuncao(const DeclaracaoFuncao& funcao, Vetor<DadoEstaticoIR>& dados);
    bool comandoCrahamadaTerminaProcesso(const ComandoIR& comandoIR);
    ComandoIR gerarSairImplicito();
    void registrarLigacoes(const Programa& programa);
    void emitirConstrutorBase(const DeclaracaoFuncao& funcao, Vetor<ComandoIR>& saida);

    // === gerador_espacos.h ===
    void registrarEspacos(const Programa& programa, const std::string& origem = "programa principal");
    int obterIdEspaco(const char* nome);
    void posicaoDoCampo(int idEspaco, const char* campo, int* posSaida, TipoVariavel* tipoSaida);
    int tamDoEspaco(int idEspaco);
    ArgumentoIR resolverAcessoCampo(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void emitirEscritaCampo(const ArgumentoIR& base, int posCampo, TipoVariavel tipoCampo, ArgumentoIR valor, Vetor<ComandoIR>& saida);
    void gerarAtribuicaoCampo(const AtribuicaoCampo& atr, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    int baseDoEspaco(int idEspaco);
    int totalCampos(int idEspaco);
    const CampoEspaco& campoDoIndice(int idEspaco, int indice);
    bool existeCampo(int idEspaco, const char* campo);
    const CampoEspaco* campoRepresentante(int idEspaco);
    bool ehSubespaco(int idEspaco, int idBase);
    bool buscarMetodo(int idEspaco, const char* metodo, std::string& nomeFuncao);
    const CampoEstatico* buscarEstatico(int idEspaco, const char* campo, std::string& simbolo);
    bool localizarEstatico(const char* nomeBase, const char* campo, ContextoFuncao& ctx, std::string& simbolo, const CampoEstatico** estatico);
    void registrarGlobalUsada(const std::string& simbolo, int largura, long valorInicial);
    ArgumentoIR enderecoDeGlobal(const std::string& simbolo, const CampoEstatico& estatico, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR resolverAcessoEstatico(const std::string& simbolo, const CampoEstatico& estatico, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void validarHeranca();
    ArgumentoIR valorRepresentante(ArgumentoIR valor, TipoVariavel tipoDestino, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR valorRepresentante(ArgumentoIR valor, const ArgumentoIR& destino, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    
    // === gerador_expressoes.h ===
    ArgumentoIR resolverExpressao(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR resolverBytes(const Expressao& expressao, TabelaVariaveisLocais& variaveisLocais);
    ArgumentoIR resolverConversao(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR resolverChamadaSistemaComoValor(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR resolverRegistradorSistema(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR converterParaTipo(ArgumentoIR valor, bool destinoFlutuante, int larguraDestino, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void promoverFlutuantes(ArgumentoIR& esq, ArgumentoIR& dir, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR resolverOperadorEspaco(OperadorAritmetico op, ArgumentoIR esquerda, ArgumentoIR direita, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR resolverTernaria(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR resolverAtribuicao(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR congelarValor(ArgumentoIR valor, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR gravarEmVariavel(const char* nome, ArgumentoIR valor, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR gravarEmIndexacao(const ArgumentoIR& ponteiro, const ArgumentoIR& indice, ArgumentoIR valor, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR gravarEmCampo(const char* nome, const char* campo, ArgumentoIR valor, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR lerCampoDeBase(const ArgumentoIR& base, const char* campo, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR gravarEmCampoBase(const ArgumentoIR& base, const char* campo, ArgumentoIR valor, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    int idEspacoDoCampo(int idEspaco, const char* campo);

    // === gerador_condicoes.h ===
    void atribuirMarcacao(int slot, long valor, Vetor<ComandoIR>& saida);
    void marcarSeVerdadeira(const Condicao& cond, int slot, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void resolverCondicao(const Condicao& cond, Vetor<ComandoIR>& saida, ContextoFuncao& ctx, ArgumentoIR& esquerda, OperadorComparacaoIR& operador, ArgumentoIR& direita);

    // === gerador_comandos.h ===
    void gerarRetorne(const ComandoRetorne& comandoRetorne, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void gerarDeclaracaoVariavel(const DeclaracaoVariavel& decl, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void gerarReatribuicao(const Reatribuicao& reat, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void gerarAtribuicaoIndexada(const AtribuicaoIndexada& atr, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void gerarChamadaSistemaCrua(const ChamadaSistemaCrua& chamada, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void gerarComandoSe(const ComandoSe& comandoSe, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void gerarComandoEnquanto(const ComandoEnquanto& comandoEnquanto, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void gerarComandoPor(const ComandoPor& comandoPor, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void gerarBlocoComandos(const Vetor<Comando>& corpo, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void gerarApagar(const ComandoApagar& comando, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    
    // === gerador_chamadas.h ===
    void gerarChamadaFuncao(const ChamadaFuncao& chamada, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    std::string ocultarReceptor(const char* nome, const Expressao& receptor, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    bool desdobrarChamadaMetodo(const char* nome, const Vetor<Expressao>& argumentos, std::string& nomeFuncao, Vetor<Expressao>& novos, ContextoFuncao& ctx);
    void liberarDesdobramento(Vetor<Expressao>& novos, int totalOriginal);
    void resolverArgumentosChamada(const char* nome, const Vetor<Expressao>& argumentos, Vetor<ArgumentoIR>& destino, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    bool prepararChamadaIndireta(const char* nome, const Vetor<Expressao>& argumentos, InstrucaoChamadaFuncaoUsuario& chamada, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR resolverChamadaFuncaoComoValor(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);

    // === gerador_ponteiros.h ===
    ArgumentoIR resolverPonteiroIndexado(const char* nome, const char* campo, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR resolverPonteiroDaExpressao(const Expressao& e, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR indexarRepresentante(const ArgumentoIR& ponteiro, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR resolverIndexacao(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR resolverEndereco(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR enderecoDoElementoEspaco(const ArgumentoIR& ponteiro, const ArgumentoIR& indice, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);

    // === gerador_vida.h ===
    ArgumentoIR argumentoEspaco(int slot, int idEspaco);
    void emitirCriar(const ArgumentoIR& destino, ArgumentoIR valor, int idEspaco, Vetor<ComandoIR>& saida);
    void emitirLiberar(const ArgumentoIR& alvo, int idEspaco, Vetor<ComandoIR>& saida);
    void emitirMliberar(const ArgumentoIR& alvo, Vetor<ComandoIR>& saida);
    void copiarObjeto(const ArgumentoIR& origem, const ArgumentoIR& destino, int idEspaco, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void moverEspaco(const ArgumentoIR& temp, const ArgumentoIR& destino, int idEspaco, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void zerarEspaco(const ArgumentoIR& destino, int idEspaco, Vetor<ComandoIR>& saida);
    ArgumentoIR criarTemporarioDeValor(int idEspaco, ArgumentoIR valor, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void registrarLiberacaoAutomatica(int slot, int idEspaco, ContextoFuncao& ctx);
    void emitirLiberacoesDeEscopo(int marca, bool truncar, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void registrarTemporario(int slot, int idEspaco, bool heap, ContextoFuncao& ctx);
    void removerTemporario(int slot, ContextoFuncao& ctx);
    void emitirLiberacaoTemporarios(int marca, bool truncar, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    bool temConstrutor(int idEspaco);
    void emitirAlocar(int slot, int idEspaco, Vetor<ComandoIR>& saida);
    void emitirConstrutor(const DeclaracaoVariavel& decl, int idEspaco, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ComandoIR montarChamada(const char* nome, std::initializer_list<ArgumentoIR> args);
    void emitirChamada(const char* nome, std::initializer_list<ArgumentoIR> args, Vetor<ComandoIR>& saida);
    ArgumentoIR capturarChamada(ComandoIR& comando, TipoVariavel tipoRetorno, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void reservarNoFrame(int bytes, int slot, Vetor<ComandoIR>& saida);
    void copiarEspaco(const ArgumentoIR& origem, const ArgumentoIR& destino, int idEspaco, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR criarCopiaEspaco(const ArgumentoIR& origem, int idEspaco, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    bool buscarMetodoLista(int idEspaco, std::string& nomeMetodo);
    bool aceitaListaDeElementos(int idEspaco);
    void emitirListaElementos(const char* nome, const Vetor<Expressao>& itens, int idEspaco, const ArgumentoIR& destino, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    
    // === gerador_globais.h ===
    void registrarGlobaisDeclaradas(const Programa& programa);
    void vincularGlobais(ContextoFuncao& ctx, Vetor<ComandoIR>& saida);
    FuncaoIR gerarIniciarGlobais(Vetor<DadoEstaticoIR>& dados);
};

// definições dos métodos(cada arquivo inclui este de volta; #pragma once evita ciclo)
#include "gerador_tipos.h"
#include "gerador_funcoes.h"
#include "gerador_expressoes.h"
#include "gerador_condicoes.h"
#include "gerador_comandos.h"
#include "gerador_chamadas.h"
#include "gerador_ponteiros.h"
#include "gerador_espacos.h"
#include "gerador_vida.h"
#include "gerador_globais.h"