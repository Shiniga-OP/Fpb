// util/analisador.h
#pragma once
#include <stdexcept>
#include <string>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "diagnostico.h"
#include "ast.h"

class Analisador {
public:
    explicit Analisador(const Vetor<Token>& tokens) : tokens(tokens) {}

    Vetor<Token> tokens; // não possuí: dono e quem chamou o Analisador
    size_t pos = 0;

    // ===== declarações: cada método é definido(inline) no leitor indicado =====

    // === analise_base.h ===
    Programa analisar();
    const Token& atual();
    bool verificar(TipoToken tipo);
    const Token& consumir(TipoToken tipo, const char* mensagemErro);
    [[noreturn]] void erro(const char* mensagem);
    Inclusao analisarInclusao(TipoToken diretiva = TipoToken::INCLUIR);
    bool ehTokenTipoValor(TipoToken tipo);
    TipoVariavel analisarTipoValor(const char* mensagemErro);

    // === analise_funcoes.h ===
    DeclaracaoFuncao analisarFuncao();
    void analisarParametrosECorpo(DeclaracaoFuncao& funcao);
    TipoRetorno analisarTipoRetorno();
    Parametro analisarParametro();

    // === analise_espacos.h ===
    DeclaracaoEspaco analisarEspaco();
    void analisarClasse(Programa& programa);
    bool ehInicioDeclaracaoEspaco();

    // === analisador_comandos.h ===
    Comando analisarComando();
    AtribuicaoIndexada analisarAtribuicaoIndexada();
    AtribuicaoCampo analisarAtribuicaoCampo();
    Reatribuicao analisarReatribuicao();
    ComandoRetorne analisarRetorne();
    ComandoSe analisarSe();
    ComandoEnquanto analisarEnquanto();
    void analisarBloco(Vetor<Comando>& corpo);
    bool ehTokenReatribuicao(TipoToken tipo);
    void analisarPartePor(Vetor<Comando>& destino, bool ehInicio);
    ComandoPor analisarPor();
    DeclaracaoVariavel analisarDeclaracaoVariavel();
    ChamadaSistemaCrua analisarNucleoChamadaSistema();
    ChamadaSistemaCrua analisarChamadaSistemaCrua();
    ChamadaSistemaCrua analisarChamadaSistemaComDestino();
    ChamadaFuncao analisarChamadaFuncao();
    bool ehChamadaMetodo();
    ChamadaFuncao analisarChamadaMetodo();
    ComandoApagar analisarApagar();

    // === analisador_condicoes.h ===
    OperadorComparacao analisarOperadorComparacao(const char* nomeConstrucao);
    bool ehTokenComparacao(TipoToken tipo);
    bool ehCondicaoAgrupada();
    Condicao montarCondicaoLogica(Condicao::Tipo tipo, const Condicao& esq, const Condicao& dir);
    Condicao analisarCondicao(const char* nomeConstrucao);
    Condicao analisarCondicaoE(const char* nomeConstrucao);
    Condicao analisarCondicaoBase(const char* nomeConstrucao);

    // === analisador_expressoes.h ===
    bool operadorDoNivel(int nivel, OperadorAritmetico& op);
    Expressao analisarBinaria(int nivel);
    Expressao analisarExpressaoAditiva();
    Expressao analisarConversao();
    Expressao analisarExpressaoParentesizada();
    Expressao analisarBytes();
    Expressao analisarExpressaoPrimaria();
    Expressao analisarIndexacao();
    Expressao montarAcessoCampo(const char* nome, const char* campo);
    Expressao analisarChamadaFuncaoComoExpressao();
    Expressao analisarChamadaMetodoComoExpressao();
    Expressao analisarExpressaoSemTernario();
    Expressao analisarPosfixo(Expressao base);
    Expressao montarIncremento(const Expressao& destino, OperadorAritmetico op);
    Expressao analisarPrimariaSemPosfixo();
    Expressao continuarCadeia(Expressao base);
    Comando analisarComandoCadeia();
};

// definições dos métodos (cada arquivo inclui este de volta; #pragma once evita ciclo)
#include "analise/analise_base.h"
#include "analise/analise_funcoes.h"
#include "analise/analise_espacos.h"
#include "analise/analise_comandos.h"
#include "analise/analise_condicoes.h"
#include "analise/analise_expressoes.h"