// util/arquiteturas/arm64/arm64.h
#pragma once
#include <sstream>
#include <string>
#include <stdexcept>
#include <string.h>
#include "../arq.h"
#include "olho_magico.h"

// traduz a IR genérica para assembly ARM64, convenção de
// chamada do Linux(numero da chamada em x8, args em x0-x2, svc 0).
class ARM64 : public Arquitetura {
public:
    std::string nome() const override { return "arm64"; }
    int contadorRotulos = 0; // usado pra gerar labels unicos(ex: loops de strlen)
    int baseArrays = 0;
    int baseSlots = 0;

    // funcao IR sendo emitida no momento: setada no inicio de cada função 
    // em gerarAssembly() e consultada por emitirCarregarSlot/emitirGravarSlot/
    // emitirCarregarParametro pra saber se um slot tem registrador fisico
    // atribuido(liberacao por ultimo uso) ou se ainda mora na pilha.
    const FuncaoIR* funcaoAtual = nullptr;

    // ===== declarações: cada método é definido(inline) no leitor indicado =====

    // === arm64_programa.h ===
    std::string gerarAssembly(const ProgramaIR& programa) override;
    long numSistema(ChamadaSistemaId id);
    void emitirBlocoComandos(std::ostringstream& saida, const Vetor<ComandoIR>& bloco, const ProgramaIR& programa);

    // === arm64_slots.h ===
    static const char* nomeRegistradorFisico(int indice);
    void calcularRegistradoresUsados(const FuncaoIR& funcao, int* saida, int* totalUsados);
    const char* registradorFisicoDoSlot(int slot);
    void emitirPuxarPegarSalvo(std::ostringstream& saida, const int* registradoresUsados, int totalRegistradoresUsados, int basePos, bool salvar);
    void emitirCarregarParametro(std::ostringstream& saida, const char* registradorDestino, int indiceParametro);
    std::string paraRegistrador32(const char* registradorX);
    void emitirCarregarSlot(std::ostringstream& saida, const char* registradorDestino, int slot, int larguraBytes, bool sinalizado = true);
    void emitirGravarSlot(std::ostringstream& saida, const char* registradorOrigem, int slot, int larguraBytes);
    void emitirMovImediato(std::ostringstream& saida, const char* registradorDestino, long valor, bool usaW);
    void emitirCarregarValorEm(std::ostringstream& saida, const char* registradorDestino, const ArgumentoIR& arg);
    void emitirCarregarValorEmComLargura(std::ostringstream& saida, const char* registradorDestino, const ArgumentoIR& arg, int larguraOperacao);
    void emitirReatribuicao(std::ostringstream& saida, const InstrucaoReatribuicao& instrucao);

    // === arm64_aritmetica.h ===
    void emitirOperacaoAritmetica(std::ostringstream& saida, const InstrucaoOperacaoAritmetica& instrucao);
    void emitirChecagemDivisorZero(std::ostringstream& saida);

    // === arm64_memoria.h ===
    void emitirTexTam(std::ostringstream& saida, const ArgumentoIR& argumentoPonteiro);
    void emitirLeituraIndexada(std::ostringstream& saida, const InstrucaoLeituraIndexada& instrucao);
    void emitirEscritaIndexada(std::ostringstream& saida, const InstrucaoEscritaIndexada& instrucao);
    void emitirEnderecoFrame(std::ostringstream& saida, const InstrucaoEnderecoFrame& instrucao);

    // === arm64_fluxo.h ===
    const char* mnemonicoSaltoSeFalso(OperadorComparacaoIR operador);
    void emitirSe(std::ostringstream& saida, const InstrucaoSe& instrucao, const ProgramaIR& programa);
    void emitirEnquanto(std::ostringstream& saida, const InstrucaoEnquanto& instrucao, const ProgramaIR& programa);
    void emitirRetorne(std::ostringstream& saida, const InstrucaoRetorne& instrucao);

    // === arm64_chamadas.h ===
    void emitirChamadaSistemaCrua(std::ostringstream& saida, const InstrucaoChamadaSistemaCrua& instrucao);
    void emitirChamadaFuncaoUsuario(std::ostringstream& saida, const InstrucaoChamadaFuncaoUsuario& instrucao);
};

// definições dos métodos(cada arquivo inclui este de volta; #pragma once evita ciclo)
#include "arm64_programa.h"
#include "arm64_slots.h"
#include "arm64_aritmetica.h"
#include "arm64_memoria.h"
#include "arm64_fluxo.h"
#include "arm64_chamadas.h"