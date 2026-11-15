// util/ir/geradorir.h
#pragma once
#include <string>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../config.h"
#include "../ast.h"
#include "ir.h"
#include "tabelasir.h"
#include "alocadorreg.h"
#include "carregadorbiblis.h"

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

    ContextoFuncao(const DeclaracaoFuncao& funcao, Vetor<DadoEstaticoIR>& dados) : funcao(funcao), dados(dados) {}
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

    // diretorioBase: diretório a partir do qual os "#incluir" são resolvidos.
    explicit GeradorIR(const std::string& diretorioBase) : diretorioBase(diretorioBase) {
        funcoesUsuario.iniciar();
        corposFuncoesUsuario.iniciar();
        espacos.iniciar();
    }

    ~GeradorIR() {
        funcoesUsuario.liberar();
        corposFuncoesUsuario.liberar();
        espacos.liberar();
    }

    // ===== declarações: cada método é definido(inline) no leitor indicado =====

    // === gerador_tipos.h ===
    int larguraDoTipo(TipoVariavel tipo);
    int larguraElementoApontado(TipoVariavel tipoPonteiro);
    bool tipoEhSinalizado(TipoVariavel tipo);
    bool elementoEhSinalizado(TipoVariavel tipoPonteiro);
    OperadorAritmeticoIR traduzirOperadorAritmetico(OperadorAritmetico op);
    OperadorComparacaoIR traduzirOperadorComparacao(OperadorComparacao op);
    ArgumentoIR imediatoInt(long valor);
    ArgumentoIR referenciaSlotInt(int slot);
    ArgumentoIR copiarArgumento(const ArgumentoIR& original);
    char* gerarNomeSimbolo(const char* textoOriginal);

    // === gerador_funcoes.h ===
    ProgramaIR gerar(const Programa& programa);
    void registrarFuncoes(const Programa& programa, const std::string& origem = "programa principal");
    void gerarFuncaoAlcancavel(const char* nome, ProgramaIR& ir, VetorStr& jaGeradas);
    void coletarChamadasUsuario(const Vetor<ComandoIR>& bloco, VetorStr& saida);
    FuncaoIR gerarFuncao(const DeclaracaoFuncao& funcao, Vetor<DadoEstaticoIR>& dados);
    bool comandoCrahamadaTerminaProcesso(const ComandoIR& comandoIR);
    ComandoIR gerarSairImplicito();

    // === gerador_espacos.h ===
    void registrarEspacos(const Programa& programa, const std::string& origem = "programa principal");
    int obterIdEspaco(const char* nome);
    void posicaoDoCampo(int idEspaco, const char* campo, int* posSaida, TipoVariavel* tipoSaida);
    int tamanhoDoEspaco(int idEspaco);
    ArgumentoIR resolverAcessoCampo(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void emitirEscritaCampo(const ArgumentoIR& base, int posCampo, TipoVariavel tipoCampo, ArgumentoIR valor, Vetor<ComandoIR>& saida);
    void gerarAtribuicaoCampo(const AtribuicaoCampo& atr, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);

    // === gerador_expressoes.h ===
    ArgumentoIR resolverExpressao(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR resolverBytes(const Expressao& expressao, TabelaVariaveisLocais& variaveisLocais);
    ArgumentoIR resolverIndexacao(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR resolverConversao(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR resolverChamadaSistemaComoValor(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR resolverChamadaFuncaoComoValor(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR resolverEndereco(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR resolverOperadorEspaco(OperadorAritmetico op, ArgumentoIR esquerda, ArgumentoIR direita, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);

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
    void gerarChamadaFuncao(const ChamadaFuncao& chamada, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    void resolverArgumentosChamada(const char* nome, const Vetor<Expressao>& argumentos, Vetor<ArgumentoIR>& destino, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    bool prepararChamadaIndireta(const char* nome, const Vetor<Expressao>& argumentos, InstrucaoChamadaFuncaoUsuario& chamada, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    bool desdobrarChamadaMetodo(const char* nome, const Vetor<Expressao>& argumentos, std::string& nomeFuncao, Vetor<Expressao>& novos, ContextoFuncao& ctx);
    void liberarDesdobramento(Vetor<Expressao>& novos, int totalOriginal);
    void copiarEspaco(const ArgumentoIR& origem, const ArgumentoIR& destino, int idEspaco, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
    ArgumentoIR criarCopiaEspaco(const ArgumentoIR& origem, int idEspaco, Vetor<ComandoIR>& saida, ContextoFuncao& ctx);
};

// definições dos métodos(cada arquivo inclui este de volta; #pragma once evita ciclo)
#include "gerador_tipos.h"
#include "gerador_funcoes.h"
#include "gerador_expressoes.h"
#include "gerador_condicoes.h"
#include "gerador_comandos.h"
#include "gerador_espacos.h"