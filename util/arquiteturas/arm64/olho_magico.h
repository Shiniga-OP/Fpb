// util/arquiteturas/arm64/olho_magico.h
#pragma once
#include <ctype.h>
#include <sstream>
#include <string>
#include <vector>
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

// "x9"/"w9" -> "9"; devolve "" se não for x/w seguido só de dígitos
inline std::string numeroDoRegistrador(const std::string& reg) {
    if(reg.size() < 2 || (reg[0] != 'x' && reg[0] != 'w')) return "";
    for(size_t i = 1; i < reg.size(); i++) {
        if(!isdigit((unsigned char)reg[i])) return "";
    }
    return reg.substr(1);
}

// rascunho das emissões(x9/x10/x11, em x ou w)
inline bool ehRascunho(const std::string& reg) {
    std::string n = numeroDoRegistrador(reg);
    return n == "9" || n == "10" || n == "11";
}

// "ldr x9, [x9, #8]" -> {"ldr", "x9", "x9", "8"}
inline void palavrasDaLinha(const std::string& linha, std::vector<std::string>& palavras) {
    std::string atual;
    for(size_t i = 0; i <= linha.size(); i++) {
        bool alfanumerico = i < linha.size() && isalnum((unsigned char)linha[i]);
        if(alfanumerico) {
            atual += linha[i];
        } else if(!atual.empty()) {
            palavras.push_back(atual);
            atual.clear();
        }
    }
}

// true se o registrador de número "numero"(vale x e w) é escrito antes de qualquer leitura nas linhas a partir de "inicio".
// Conservador: rótulo, desvio, chamada, ret, svc ou qualquer leitura antes da escrita devolvem false.
inline bool registradorMorreApos(VetorStr& linhas, int inicio, const std::string& numero) {
    std::string nomeX = "x" + numero;
    std::string nomeW = "w" + numero;
    for(int j = inicio; j < linhas.tam; j++) {
        int tam;
        const char* texto = linhas.obter(j, &tam);
        std::string linha(texto, tam);
        size_t ini = linha.find_first_not_of(" \t");
        if(ini == std::string::npos) continue; // linha em branco
        size_t fim = linha.find_last_not_of(" \t");
        if(linha[fim] == ':') return false; // rótulo: quem salta pra cá pode ler o registrador
        std::vector<std::string> palavras;
        palavrasDaLinha(linha, palavras);
        if(palavras.empty()) continue;
        const std::string& mnemonico = palavras[0];
        if(mnemonico == "b" || mnemonico == "bl" || mnemonico == "blr" || mnemonico == "br" || mnemonico == "ret" || mnemonico == "svc" ||
           mnemonico == "cbz" || mnemonico == "cbnz" || mnemonico == "tbz" || mnemonico == "tbnz") return false;
        int mencoes = 0;
        for(size_t k = 1; k < palavras.size(); k++) {
            if(palavras[k] == nomeX || palavras[k] == nomeW) mencoes++;
        }
        if(mencoes == 0) continue;
        // única menção, como destino de uma instrução que só escreve: o valor antigo morreu
        bool soEscreve = mnemonico == "mov" || mnemonico == "movz" || mnemonico == "ldr" || mnemonico == "ldrb" || mnemonico == "ldrh" ||
                         mnemonico == "ldrsb" || mnemonico == "ldrsh" || mnemonico == "adrp" || mnemonico == "fmov";
        return mencoes == 1 && soEscreve && palavras.size() > 1 && (palavras[1] == nomeX || palavras[1] == nomeW);
    }
    return false; // fim do texto sem escrita: não prova nada
}

// otimizacao de registradores: funde "mov A, B" seguido imediatamente de
// "mov C, A" em "mov C, B", quando C != B(evita virar um "mov X, X" inutil).
// so apaga o "mov A, B" se o valor de A nao e' lido depois: ou C == A(A recebe
// o mesmo valor de qualquer jeito), ou A e' rascunho(x9/x10/x11) e as linhas
// seguintes provadamente escrevem em A antes de qualquer leitura.
// Registradores de slot(x19..x27) e de argumento(x0..x7) nunca sao apagados:
// so uma analise de vivacidade da funcao inteira provaria que morrem.
// A fusao tambem exige a mesma largura(w ou x) nos quatro operandos.
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
                bool mesmaLargura = destinoA[0] == origemB[0] && destinoA[0] == destinoC[0] && destinoA[0] == origemA2[0];
                if(mesmaLargura && origemA2 == destinoA && destinoC != origemB) {
                    // só apaga "mov A, B" se A não é lido depois: ou C reescreve A, ou A é rascunho provadamente morto
                    bool podeApagar = destinoC == destinoA || (ehRascunho(destinoA) && registradorMorreApos(linhas, i + 2, numeroDoRegistrador(destinoA)));
                    if(podeApagar) {
                        resultado += "mov " + destinoC + ", " + origemB + "\n";
                        i++; // consome as duas linhas originais, só a fundida vai pra saída
                        continue;
                    }
                }
            }
        }
        resultado += atual + "\n";
    }
    linhas.liberar();
    return resultado;
}