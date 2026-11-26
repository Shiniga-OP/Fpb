// util/ir/tabelasir.h
#pragma once
#include <string.h>
#include "../config.h"
#include "../ast.h"
#include "ir.h"

// tabela simples nome->total de parâmetros de funcoes de usuario(definidas com corpo normal, não #chamada_sistema). busca linear, mesmo raciocínio 
// das outras tabelas deste arquivo: poucas funções por programa.
struct TabelaFuncoesUsuario {
    VetorStr nomes;
    Vetor<int> totaisParametros;
    Vetor<TipoRetorno> tiposRetorno;
    Vetor<bool> variadicas; // paralelo a nomes: true se o último parâmetro é "longo... nome"(totaisParametros conta ele)

    void iniciar() {
        nomes.iniciar();
        totaisParametros.iniciar();
        tiposRetorno.iniciar();
        variadicas.iniciar();
    }
    void liberar() {
        nomes.liberar();
        totaisParametros.liberar();
        tiposRetorno.liberar();
        variadicas.liberar();
    }
    void definir(const char* nome, int totalParametros, TipoRetorno tipoRetorno, bool ehVariadica = false) {
        nomes.empurrar(nome);
        totaisParametros.empurrar(totalParametros);
        tiposRetorno.empurrar(tipoRetorno);
        variadicas.empurrar(ehVariadica);
    }
    bool ehVariadica(const char* nome) const {
        int i = nomes.buscar(nome);
        return i >= 0 && variadicas[i];
    }
    
    bool buscar(const char* nome, int* saida) const {
        int i = nomes.buscar(nome);
        if(i < 0) return false;
        *saida = totaisParametros[i];
        return true;
    }
    // retorna true e preenche *saida com o tipo de retorno se encontrar; false caso contrario.
    bool buscarTipoRetorno(const char* nome, TipoRetorno* saida) const {
        int i = nomes.buscar(nome);
        if(i < 0) return false;
        *saida = tiposRetorno[i];
        return true;
    }
};

// tabela simples nome->declaracao completa(AST) de funcao de usuario,
// incluindo as vindas de bibliotecas incluidas via #incluir. Usada pelo
// "tree shaking": guarda o corpo pra poder gerar FuncaoIR sob demanda, so
// pras funções de fato alcancaveis a partir do programa principal.
struct TabelaCorposFuncoesUsuario {
    VetorStr nomes;
    Vetor<const DeclaracaoFuncao*> corpos; // não possuí: dono é quem chamou definir()

    void iniciar() {
        nomes.iniciar();
        corpos.iniciar();
    }
    void liberar() {
        nomes.liberar();
        corpos.liberar();
    }
    void definir(const char* nome, const DeclaracaoFuncao* corpo) {
        nomes.empurrar(nome);
        corpos.empurrar(corpo);
    }
    
    bool buscar(const char* nome, const DeclaracaoFuncao** saida) const {
        int i = nomes.buscar(nome);
        if(i < 0) return false;
        *saida = corpos[i];
        return true;
    }
};

// tabela simples nome->ArgumentoIR de variaveis locais de uma funcao. busca
// linear, mesmo raciocínio das outras tabelas deste arquivo: poucas variáveis por
// funcao, entao O(n) nao pesa aqui. Guarda o ArgumentoIR ja resolvido(imediato ou simbolo de texto) pra reusar na hora de montar o #chamada_sistema,
// alem do slot(índice de 16 bytes no frame) onde a variavel mora: e nesse
// slot que uma reatribuição futura escreve.
struct TabelaVariaveisLocais {
    VetorStr nomes;
    Vetor<ArgumentoIR> valores;
    Vetor<int> slots;
    int proximoSlot = 0; // parâmetros ocupam 0..totalParametros-1; variáveis continuam daqui

    void iniciar(int totalParametros) {
        nomes.iniciar();
        valores.iniciar();
        slots.iniciar();
        proximoSlot = totalParametros;
    }
    void liberar() {
        nomes.liberar();
        for(int i = 0; i < valores.tam; i++) valores[i].liberar();
        valores.liberar();
        slots.liberar();
    }
    // usado pros parâmetros(0..totalParametros-1): slot == ind índice do próprio parâmetro.
    void definirComSlot(const char* nome, ArgumentoIR valor, int slot) {
        nomes.empurrar(nome);
        valores.empurrar(valor);
        slots.empurrar(slot);
    }
    // escopo de bloco("se"/"senao"/"enquanto"/"por"): abrirEscopo() devolve uma marca, e
    // fecharEscopo(marca) esquece todas as variáveis declaradas depois dela. proximoSlot NÃO
    // volta atrás de propósito: slots nunca são reaproveitados, então o alocador de
    // registradores continua enxergando um slot por variável, sem aliasing entre blocos.
    int abrirEscopo() const { return nomes.tam; }
    void fecharEscopo(int marca) {
        for(int i = marca; i < valores.tam; i++) valores[i].liberar();
        nomes.tam = marca;
        valores.tam = marca;
        slots.tam = marca;
    }
    // retorna true e preenche *saida(copia rasa: símbolo continua sendo dono da tabela) e *saidaSlot se encontrar; false caso contrário.
    // devolve a declaração MAIS RECENTE com esse nome(varre de trás pra frente): uma variável de
    // bloco interno sombreia a de mesmo nome de um bloco externo enquanto o escopo interno existir.
    bool buscar(const char* nome, ArgumentoIR* saida, int* saidaSlot = nullptr) const {
        int i = nomes.buscarUltimo(nome);
        if(i < 0) return false;
        *saida = valores[i];
        if(saidaSlot) *saidaSlot = slots[i];
        return true;
    }
};

// tabela nome->declaracao de #espaco(struct), incluindo as vindas de bibliotecas. O indice devolvido por buscar() e o "id" do espaco
// (ArgumentoIR::idEspaco). Busca linear: poucos espacos por programa.
struct TabelaEspacos {
    VetorStr nomes;
    Vetor<const DeclaracaoEspaco*> declaracoes; // não possui: dono é o Programa que contem a declaracao

    void iniciar() {
        nomes.iniciar();
        declaracoes.iniciar();
    }
    void liberar() {
        nomes.liberar();
        declaracoes.liberar();
    }
    void definir(const char* nome, const DeclaracaoEspaco* declaracao) {
        nomes.empurrar(nome);
        declaracoes.empurrar(declaracao);
    }
    // devolve o id(indice) do espaco, ou -1 se nao existir.
    int buscar(const char* nome) const {
        return nomes.buscar(nome);
    }
};