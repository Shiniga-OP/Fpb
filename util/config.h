// util/config.h
#pragma once
#include <string.h>
#include <stdlib.h>

enum class TipoToken {
    // pré processadores:
    INCLUIR, CHAMADA_SISTEMA, MACRO, MACROFIM, ESPACO,
    CLASSE, LIGAR, ALVO, PRE_SE, PRE_SENAOSE, PRE_SENAO,
    PRE_FIM,
    // tipagem:
    NULO, INT, LONGO, CAR, // int(32 bits, 4 bytes), longo(64 bits, 8 bytes, mesma largura de qualquer ponteiro), car(usado como "car*")
    BOOL, BYTE, CURTO, FLUTUANTE, DOBRO,
    IDENTIFICADOR, TEXTO, NUMERO, DECIMAL,
    // estrutura:
    ABRE_PARENTESE, FECHA_PARENTESE, ABRE_CHAVE,
    FECHA_CHAVE, ABRE_COLCHETE, FECHA_COLCHETE,
    // pontuação:
    VIRGULA, PONTO_VIRGULA, RETICENCIAS, PONTO,
    ARROBA, INTERROGACAO, DOIS_PONTOS,
    // operadores:
    IGUAL, MAIS, MENOS, VEZES, DIVIDIR, PORCENTAGEM,
    // operadores compostos:
    MAIS_MAIS, MENOS_MENOS, MAIS_IGUAL, MENOS_IGUAL,
    VEZES_IGUAL, DIVIDIR_IGUAL, PORCENTAGEM_IGUAL,
    // operadores de bit:
    MAIOR_MAIOR, MENOR_MENOR, OU, TAMBEM, CIRCUNFLEXO,
    TIL,
    // operadores de bit compostos:
    TAMBEM_IGUAL, OU_IGUAL, CIRCUNFLEXO_IGUAL,
    MENOR_MENOR_IGUAL, MAIOR_MAIOR_IGUAL,
    // condição:
    SE, SENAO, IGUAL_IGUAL, EXCLAMACAO, DIFERENTE,
    OU_OU, TAMBEM_TAMBEM, MAIOR, MENOR, MAIOR_IGUAL,
    MENOR_IGUAL,
    // laço:
    ENQUANTO, POR, APAGAR,
    // definição:
    RETORNE, SEM_SINAL, HERDA, ESTATICO, FINAL,
    NOVO, RESOLVEC, REPRESENTANTE,
    // assembly:
    LER_REG, ESCREVER_REG,
    // fim:
    FIM_ARQUIVO
};

struct Token {
    TipoToken tipo;
    char* valor; // dono: quem criou o token(strdup). liberar() quando descartar.
    int linha;
    int coluna = 0;
    int tam = 0;
    int arquivo = -1;

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
    
    int buscar(const char* s, int sTam) const {
        for(int i = 0; i < tam; i++) {
            if(tams[i] == sTam && memcmp(buf + pos[i], s, sTam) == 0) return i;
        }
        return -1;
    }
    int buscar(const char* s) const {
        return buscar(s, (int)strlen(s));
    }
    int buscarUltimo(const char* s) const {
        int sTam = (int)strlen(s);
        for(int i = tam - 1; i >= 0; i--) {
            if(tams[i] == sTam && memcmp(buf + pos[i], s, sTam) == 0) return i;
        }
        return -1;
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