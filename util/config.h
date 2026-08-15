// util/config.h
#pragma once
#include <string.h>
#include <stdlib.h>

enum class TipoToken {
    // pré processadores:
    INCLUIR, CHAMADA_SISTEMA,
    // tipagem:
    NULO, INT, LONGO, CAR, // int(32 bits, 4 bytes), longo(64 bits, 8 bytes, mesma largura de qualquer ponteiro), car(usado como "car*")
    IDENTIFICADOR, TEXTO, NUMERO,
    // estrutura:
    ABRE_PARENTESE, FECHA_PARENTESE, ABRE_CHAVE, FECHA_CHAVE, ABRE_COLCHETE, FECHA_COLCHETE,
    // pontuação:
    VIRGULA, PONTO_VIRGULA,
    // operadores:
    IGUAL, MAIS, MENOS, VEZES, DIVIDIR, PORCENTAGEM, OU, TAMBEM,
    // operadores compostos:
    MAIS_IGUAL, MENOS_IGUAL, VEZES_IGUAL, DIVIDIR_IGUAL, PORCENTAGEM_IGUAL,
    // condição:
    SE, SENAO, IGUAL_IGUAL, EXCLAMACAO, DIFERENTE, OU_OU, TAMBEM_TAMBEM,
    MAIOR, MENOR, MAIOR_IGUAL, MENOR_IGUAL,
    // laço:
    ENQUANTO, POR,
    // definição:
    RETORNE,
    // fim:
    FIM_ARQUIVO
};

struct Token {
    TipoToken tipo;
    char* valor; // dono: quem criou o token(strdup). liberar() quando descartar.
    int linha;

    void liberar() {
        free(valor);
        valor = nullptr;
    }
};

// array dinamico simples(T sem dono de memoria propria, ex: structs planas)
template <typename T>
struct Vetor {
    T* dados;
    int tam;
    int cap;

    void iniciar() {
        dados = nullptr;
        tam = cap = 0;
    }
    void liberar() {
        free(dados);
        iniciar();
    }
    void limpar() {
        tam = 0;
    }

    void empurrar(T v) {
        if(tam == cap) {
            cap = cap ? cap * 2 : 8;
            dados = (T*)realloc(dados, cap * sizeof(T));
        }
        dados[tam++] = v;
    }
    T& operator[](int i) { return dados[i]; }
    const T& operator[](int i) const { return dados[i]; }
};

// buffer plano de strings
struct VetorStr {
    char* buf;
    int* pos;
    int* tams;
    int tam;
    int capE;
    int capB;

    void iniciar() {
        buf = nullptr;
        pos = nullptr;
        tams = nullptr;
        tam = 0;
        capE = 0;
        capB = 0;
    }

    void liberar() {
        free(buf);
        free(pos);
        free(tams);
        iniciar();
    }

    void limpar() { tam = 0; }

    const char* obter(int i, int* saidaTam) const {
        *saidaTam = tams[i];
        return buf + pos[i];
    }

    void empurrar(const char* s, int sLen) {
        if(tam == capE) {
            capE = capE ? capE * 2 : 8;
            pos = (int*)realloc(pos,  capE * sizeof(int));
            tams = (int*)realloc(tams, capE * sizeof(int));
        }
        int usadoBuf = tam > 0 ? pos[tam-1] + tams[tam-1] : 0;
        while(usadoBuf + sLen > capB) {
            capB = capB ? capB * 2 : 64;
            buf = (char*)realloc(buf, capB);
        }
        memcpy(buf + usadoBuf, s, sLen);
        pos[tam] = usadoBuf;
        tams[tam] = sLen;
        tam++;
    }

    void empurrar(const char* s) {
        empurrar(s, (int)strlen(s));
    }
};