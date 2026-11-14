// util/arquiteturas/arm64/olho_magico.h
#pragma once
#include <ctype.h>
#include <sstream>
#include <string>
#include "../../config.h"

// reconhece uma linha "mov DESTINO, ORIGEM" onde ORIGEM e' um
// registrador(não um imediato numérico): preenche destino/origem e
// devolve true. Usado so pela fusao abaixo; não aceita "mov x9, 8"(imediato), so "mov x9, x21"(registrador-a-registrador), que e' o único 
// caso seguro de fundir(fundir com imediato exigiria reanalisar largura w/x, fora do escopo desta otimização simples).
inline bool ehMovSimples(const std::string& linha, std::string& destino, std::string& origem) {
    if(linha.compare(0, 4, "mov ") != 0) return false;
    size_t virgula = linha.find(',');
    if(virgula == std::string::npos) return false;
    destino = linha.substr(4, virgula - 4);
    std::string resto = linha.substr(virgula + 1);
    size_t inicio = resto.find_first_not_of(' ');
    if(inicio == std::string::npos) return false;
    origem = resto.substr(inicio);
    if(origem.empty() || !(origem[0] == 'x' || origem[0] == 'w')) return false; // so registrador, não imediato
    for(size_t i = 1; i < origem.size(); i++) {
        if(!isdigit((unsigned char)origem[i])) return false;
    }
    return true;
}

// otimizacao de registradores: funde "mov A, B" seguido imediatamente de
// "mov C, A" em "mov C, B", quando C != B(evita virar um "mov X, X" inutil).
// isso elimina copias redundantes registrador-a-registrador que aparecem
// quando um valor passa por um slot temporario antes de ser gravado na
// variavel final(ex: "int total = a + b;" grava o resultado da soma num
// slot temporario, depois copia pro slot de "total" — ambos podem ganhar
// registrador fisico, e a copia vira "mov" em vez de acesso a pilha).
// so funde as DUAS linhas adjacentes(sem rotulo/instrucao no meio), pra
// não arriscar remover um "mov" que um salto possa pular por cima e
// pousar exatamente entre as duas linhas(o que quebraria a fusão).
inline std::string otimizarMovsRedundantes(const std::string& asm_) {
    VetorStr linhas;
    linhas.iniciar();
    std::istringstream entrada(asm_);
    std::string linha;
    while(std::getline(entrada, linha)) linhas.empurrar(linha.c_str(), (int)linha.size());

    std::string resultado;
    for(int i = 0; i < linhas.tam; i++) {
        int tamAtual;
        const char* textoAtual = linhas.obter(i, &tamAtual);
        std::string atual(textoAtual, tamAtual);
        if(i + 1 < linhas.tam) {
            int tamProxima;
            const char* textoProxima = linhas.obter(i + 1, &tamProxima);
            std::string proxima(textoProxima, tamProxima);
            std::string destinoA, origemB;
            std::string destinoC, origemA2;
            if(ehMovSimples(atual, destinoA, origemB) && ehMovSimples(proxima, destinoC, origemA2)) {
                if(origemA2 == destinoA && destinoC != origemB) {
                    resultado += "mov " + destinoC + ", " + origemB + "\n";
                    i++; // consome as duas linhas originais, so a fundida vai pra saida
                    continue;
                }
            }
        }
        resultado += atual + "\n";
    }
    linhas.liberar();
    return resultado;
}