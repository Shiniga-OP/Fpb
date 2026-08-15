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

    void iniciar() {
        nomes.iniciar();
        totaisParametros.iniciar();
        tiposRetorno.iniciar();
    }
    void liberar() {
        nomes.liberar();
        totaisParametros.liberar();
        tiposRetorno.liberar();
    }
    void definir(const char* nome, int totalParametros, TipoRetorno tipoRetorno) {
        nomes.empurrar(nome);
        totaisParametros.empurrar(totalParametros);
        tiposRetorno.empurrar(tipoRetorno);
    }
    bool buscar(const char* nome, int* saida) const {
        int tamNome = (int)strlen(nome);
        for(int i = 0; i < nomes.tam; i++) {
            int tamCandidato;
            const char* candidato = nomes.obter(i, &tamCandidato);
            if(tamCandidato == tamNome && memcmp(candidato, nome, tamNome) == 0) {
                *saida = totaisParametros[i];
                return true;
            }
        }
        return false;
    }
    // retorna true e preenche *saida com o tipo de retorno se encontrar; false caso contrario.
    bool buscarTipoRetorno(const char* nome, TipoRetorno* saida) const {
        int tamNome = (int)strlen(nome);
        for(int i = 0; i < nomes.tam; i++) {
            int tamCandidato;
            const char* candidato = nomes.obter(i, &tamCandidato);
            if(tamCandidato == tamNome && memcmp(candidato, nome, tamNome) == 0) {
                *saida = tiposRetorno[i];
                return true;
            }
        }
        return false;
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
        int tamNome = (int)strlen(nome);
        for(int i = 0; i < nomes.tam; i++) {
            int tamCandidato;
            const char* candidato = nomes.obter(i, &tamCandidato);
            if(tamCandidato == tamNome && memcmp(candidato, nome, tamNome) == 0) {
                *saida = corpos[i];
                return true;
            }
        }
        return false;
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
    // usado por declaração de variável: aloca o próximo slot livre e o retorna.
    int definir(const char* nome, ArgumentoIR valor) {
        int slot = proximoSlot++;
        definirComSlot(nome, valor, slot);
        return slot;
    }
    // retorna true e preenche *saida(copia rasa: símbolo continua sendo dono da tabela) e *saidaSlot se encontrar; false caso contrário.
    bool buscar(const char* nome, ArgumentoIR* saida, int* saidaSlot = nullptr) const {
        int tamNome = (int)strlen(nome);
        for(int i = 0; i < nomes.tam; i++) {
            int tamCandidato;
            const char* candidato = nomes.obter(i, &tamCandidato);
            if(tamCandidato == tamNome && memcmp(candidato, nome, tamNome) == 0) {
                *saida = valores[i];
                if(saidaSlot) *saidaSlot = slots[i];
                return true;
            }
        }
        return false;
    }
};