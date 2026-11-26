// util/ir/gerador_tipos.h
#pragma once
#include "geradorir.h"

// as funções abaixo só consultam TABELA_TIPOS(ast.h), que é o ponto único de verdade sobre tipos: tipo novo = linha nova lá.

// largura em bytes que o proprio VALOR de um tipo ocupa, usada tanto por
// resolverBytes("bytes(tipo)") quanto pra popular ArgumentoIR.larguraBytes(decide w32/x64 e a instrução de memória usada pela arquitetura).
// car=1, int=4(32 bits), longo=8(64 bits), qualquer ponteiro=8(endereco).
inline int GeradorIR::larguraDoTipo(TipoVariavel tipo) {
    return infoTipo(tipo).largura;
}

// largura em bytes de CADA ELEMENTO apontado por um tipo ponteiro,
// usada por ArgumentoIR.larguraElemento(indexacao nome[indice]).
// Nao confundir com larguraDoTipo() acima: o PONTEIRO em si sempre
// ocupa 8 bytes(endereco de 64 bits); aqui e' a largura do que ele APONTA.
inline int GeradorIR::larguraElementoApontado(TipoVariavel tipoPonteiro) {
    if(!temElementoTipado(tipoPonteiro)) return 1; // tipo não-ponteiro(ou nulo*/Nome*) nunca deveria ser indexado sem conversão antes; 1 e o valor historico/seguro
    return infoTipo(infoTipo(tipoPonteiro).elemento).largura;
}

// sinal do VALOR de um tipo: car e bool são sem sinal(zero-estendem), todos os outros com sinal. So importa pra largura 1 e 2.
inline bool GeradorIR::tipoEhSinalizado(TipoVariavel tipo) {
    return infoTipo(tipo).sinalizado;
}

// true pra flu(32 bits) e dobro(64 bits): o valor mora nos slots como o padrao de bits do float/double.
inline bool GeradorIR::tipoEhFlutuante(TipoVariavel tipo) {
    return infoTipo(tipo).flutuante;
}

// sinal de CADA ELEMENTO apontado por um tipo ponteiro: byte*, curto*, int* e longo* com sinal; car*, bool*, flu*, dobro*, nulo* e Nome* sem.
inline bool GeradorIR::elementoEhSinalizado(TipoVariavel tipoPonteiro) {
    if(!temElementoTipado(tipoPonteiro)) return false;
    const InfoTipo& elemento = infoTipo(infoTipo(tipoPonteiro).elemento);
    return elemento.sinalizado && !elemento.flutuante;
}

inline bool GeradorIR::elementoEhFlutuante(TipoVariavel tipoPonteiro) {
    return temElementoTipado(tipoPonteiro) && infoTipo(infoTipo(tipoPonteiro).elemento).flutuante;
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
    copia.ehPonteiro = original.ehPonteiro;
    copia.ehFlutuante = original.ehFlutuante;
    copia.elementoFlutuante = original.elementoFlutuante;
    copia.ehFuncao = original.ehFuncao;
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

// IMEDIATO que representa uma constante "final" do tipo dado(variável local final ou campo estatico final). Inteiros são truncados
// na largura do tipo(com extensão de sinal quando é com sinal); flu/dobro já chegam como padrão de bits na largura certa.
// Ler a constante não emite nada: o valor entra direto na instrução que usa.
inline ArgumentoIR GeradorIR::constanteDoTipo(long valor, TipoVariavel tipo) {
    int largura = larguraDoTipo(tipo);
    bool sinal = tipoEhSinalizado(tipo);
    if(!tipoEhFlutuante(tipo)) {
        if(largura == 1) valor = sinal ? (long)(signed char)valor : (valor & 0xFF);
        else if(largura == 2) valor = sinal ? (long)(short)valor : (valor & 0xFFFF);
        else if(largura == 4) valor = sinal ? (long)(int)valor : (valor & 0xFFFFFFFFL);
    }
    ArgumentoIR arg;
    arg.tipo = ArgumentoIR::Tipo::IMEDIATO;
    arg.simbolo = nullptr;
    arg.imediato = valor;
    arg.larguraBytes = largura;
    arg.sinalizado = sinal;
    arg.elementoSinalizado = elementoEhSinalizado(tipo);
    arg.larguraElemento = larguraElementoApontado(tipo);
    arg.ehPonteiro = tipoEhPonteiro(tipo);
    arg.ehFlutuante = tipoEhFlutuante(tipo);
    arg.elementoFlutuante = elementoEhFlutuante(tipo);
    return arg;
}

// REGISTRADOR de um slot com todos os atributos derivados do tipo declarado.
inline ArgumentoIR GeradorIR::argumentoSlot(int slot, TipoVariavel tipo) {
    ArgumentoIR arg;
    arg.tipo = ArgumentoIR::Tipo::REGISTRADOR;
    arg.simbolo = nullptr;
    arg.registrador = slot;
    arg.larguraBytes = larguraDoTipo(tipo);
    arg.sinalizado = tipoEhSinalizado(tipo);
    arg.elementoSinalizado = elementoEhSinalizado(tipo);
    arg.larguraElemento = larguraElementoApontado(tipo);
    arg.ehPonteiro = tipoEhPonteiro(tipo);
    arg.ehFlutuante = tipoEhFlutuante(tipo);
    arg.elementoFlutuante = elementoEhFlutuante(tipo);
    return arg;
}

// REGISTRADOR de um slot temporário que só tem largura(e talvez flutuante).
inline ArgumentoIR GeradorIR::argumentoSlotLargura(int slot, int largura, bool flutuante) {
    ArgumentoIR arg;
    arg.tipo = ArgumentoIR::Tipo::REGISTRADOR;
    arg.simbolo = nullptr;
    arg.registrador = slot;
    arg.larguraBytes = largura;
    arg.ehFlutuante = flutuante;
    return arg;
}