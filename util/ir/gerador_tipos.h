// util/ir/gerador_tipos.h
#pragma once
#include "geradorir.h"

// largura em bytes que o proprio VALOR de um tipo ocupa, usada tanto por
// resolverBytes("bytes(tipo)") quanto pra popular ArgumentoIR.larguraBytes(decide w32/x64 e a instrução de memória usada pela arquitetura). Ponto
// UNICO de verdade sobre "quantos bytes este tipo ocupa": adicionar um
// tipo novo no futuro exige mudar só aqui.
// car=1, int=4(32 bits), longo=8(64 bits), qualquer ponteiro=8(endereco).
inline int GeradorIR::larguraDoTipo(TipoVariavel tipo) {
    if(tipo == TipoVariavel::CAR || tipo == TipoVariavel::BOOL || tipo == TipoVariavel::BYTE) return 1;
    if(tipo == TipoVariavel::CURTO) return 2;
    if(tipo == TipoVariavel::INT) return 4;
    return 8; // LONGO, CAR_PONTEIRO, LONGO_PONTEIRO, NULO_PONTEIRO
}

// largura em bytes de CADA ELEMENTO apontado por um tipo ponteiro,
// usada por ArgumentoIR.larguraElemento(indexacao nome[indice]). Ponto
// UNICO de verdade sobre "o que um ponteiro deste tipo aponta": adicionar
// um tipo ponteiro novo no futuro exige mudar só aqui, não espalhar
// "if"/"switch" por resolverIndexacao/gerarAtribuicaoIndexada/arm64.h.
// Nao confundir com larguraDoTipo() acima: o PONTEIRO em si sempre
// ocupa 8 bytes(endereco de 64 bits); aqui e' a largura do que ele APONTA.
inline int GeradorIR::larguraElementoApontado(TipoVariavel tipoPonteiro) {
    if(tipoPonteiro == TipoVariavel::CAR_PONTEIRO || tipoPonteiro == TipoVariavel::BOOL_PONTEIRO || tipoPonteiro == TipoVariavel::BYTE_PONTEIRO) return 1;
    if(tipoPonteiro == TipoVariavel::CURTO_PONTEIRO) return 2;
    if(tipoPonteiro == TipoVariavel::INT_PONTEIRO) return 4;
    if(tipoPonteiro == TipoVariavel::LONGO_PONTEIRO) return 8;
    return 1; // tipo não-ponteiro(ou NULO_PONTEIRO, generico) nunca deveria ser indexado sem conversão antes; 1 e o valor historico/seguro
}

// sinal do VALOR de um tipo: car e bool são sem sinal(zero-estendem), todos os outros com sinal. So importa pra largura 1 e 2.
inline bool GeradorIR::tipoEhSinalizado(TipoVariavel tipo) {
    return !(tipo == TipoVariavel::CAR || tipo == TipoVariavel::BOOL);
}

// sinal de CADA ELEMENTO apontado por um tipo ponteiro: car*, bool* e nulo* são sem sinal, os demais com sinal.
inline bool GeradorIR::elementoEhSinalizado(TipoVariavel tipoPonteiro) {
    return tipoPonteiro == TipoVariavel::BYTE_PONTEIRO || tipoPonteiro == TipoVariavel::CURTO_PONTEIRO || tipoPonteiro == TipoVariavel::INT_PONTEIRO || tipoPonteiro == TipoVariavel::LONGO_PONTEIRO;
}

// traduz o operador aritmético da AST(OperadorAritmetico) pro equivalente
// no IR(OperadorAritmeticoIR): mesmos nomes, só pra o IR não depender de ast.h.
inline OperadorAritmeticoIR GeradorIR::traduzirOperadorAritmetico(OperadorAritmetico op) {
    switch(op) {
        case OperadorAritmetico::SOMA: return OperadorAritmeticoIR::SOMA;
        case OperadorAritmetico::SUBTRACAO: return OperadorAritmeticoIR::SUBTRACAO;
        case OperadorAritmetico::MULTIPLICACAO: return OperadorAritmeticoIR::MULTIPLICACAO;
        case OperadorAritmetico::DIVISAO: return OperadorAritmeticoIR::DIVISAO;
        case OperadorAritmetico::MODULO: return OperadorAritmeticoIR::MODULO;
        case OperadorAritmetico::E_BIT: return OperadorAritmeticoIR::E_BIT;
        case OperadorAritmetico::OU_BIT: return OperadorAritmeticoIR::OU_BIT;
        case OperadorAritmetico::XOU: return OperadorAritmeticoIR::XOU;
        case OperadorAritmetico::DESL_ESQ: return OperadorAritmeticoIR::DESL_ESQ;
        case OperadorAritmetico::DESL_DIR: return OperadorAritmeticoIR::DESL_DIR;
        default: return OperadorAritmeticoIR::MODULO;
    }
}

// traduz o operador de comparação da AST(OperadorComparacao) pro
// equivalente no IR(OperadorComparacaoIR): mesmos nomes, só pra o IR
// não depender de ast.h. Usada por gerarComandoSe e gerarComandoEnquanto,
// evita duplicar o mapeamento em cada uma.
inline OperadorComparacaoIR GeradorIR::traduzirOperadorComparacao(OperadorComparacao op) {
    switch(op) {
        case OperadorComparacao::IGUAL: return OperadorComparacaoIR::IGUAL;
        case OperadorComparacao::DIFERENTE: return OperadorComparacaoIR::DIFERENTE;
        case OperadorComparacao::MAIOR: return OperadorComparacaoIR::MAIOR;
        case OperadorComparacao::MENOR: return OperadorComparacaoIR::MENOR;
        case OperadorComparacao::MAIOR_IGUAL: return OperadorComparacaoIR::MAIOR_IGUAL;
        default: return OperadorComparacaoIR::MENOR_IGUAL;
    }
}

inline ArgumentoIR GeradorIR::imediatoInt(long valor) {
    ArgumentoIR arg;
    arg.tipo = ArgumentoIR::Tipo::IMEDIATO;
    arg.simbolo = nullptr;
    arg.imediato = valor;
    arg.larguraBytes = 4;
    return arg;
}

inline ArgumentoIR GeradorIR::referenciaSlotInt(int slot) {
    ArgumentoIR arg;
    arg.tipo = ArgumentoIR::Tipo::REGISTRADOR;
    arg.simbolo = nullptr;
    arg.imediato = 0;
    arg.registrador = slot;
    arg.larguraBytes = 4;
    return arg;
}

// cópia independente de um ArgumentoIR(duplica "simbolo" se houver),
// usada quando o mesmo valor resolvido precisa viver em dois lugares
// donos diferentes(ex: na tabela de variáveis e no ComandoIR emitido).
inline ArgumentoIR GeradorIR::copiarArgumento(const ArgumentoIR& original) {
    ArgumentoIR copia;
    copia.tipo = original.tipo;
    copia.imediato = original.imediato;
    copia.registrador = original.registrador;
    copia.simbolo = original.simbolo ? strdup(original.simbolo) : nullptr;
    copia.larguraBytes = original.larguraBytes;
    copia.larguraElemento = original.larguraElemento;
    copia.sinalizado = original.sinalizado;
    copia.elementoSinalizado = original.elementoSinalizado;
    copia.idEspaco = original.idEspaco;
    copia.espacoPorValor = original.espacoPorValor;
    return copia;
}

inline char* GeradorIR::gerarNomeSimbolo(const char* textoOriginal) {
    // gera um nome de símbolo legível a partir do conteúdo do texto,
    // ex: "Ola Mundo\n" -> "_ola_mundo_0"
    BufAcumulador base;
    base.iniciar();
    for(const char* p = textoOriginal; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if(isalnum(c)) {
            base.empurrar((char)tolower(c));
        } else if(c == ' ' && base.tam > 0 && base.dados[base.tam - 1] != '_') {
            base.empurrar('_');
        }
    }
    while(base.tam > 0 && base.dados[base.tam - 1] == '_') {
        base.tam--;
        base.dados[base.tam] = '\0';
    }
    if(base.tam == 0) {
        base.empurrar('s'); base.empurrar('t'); base.empurrar('r');
    }
    std::string resultado = "_" + std::string(base.dados) + "_" + std::to_string(contadorSimbolos++);
    base.liberar();
    return strdup(resultado.c_str());
}