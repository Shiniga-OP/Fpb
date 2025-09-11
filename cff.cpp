/*
* [INFO]:
* [AUTOR]: @Shiniga-OP.
* [EMPRESA]: @Foca-do Estúdios.
* [BASE]: Assembly ARM64(AARCH64).
* [SISTEMA OPERACIONAL]: ANDROID.
* [ARQUITETURA]: LINUX-ANDROID.
* [COMPILADOR]: C++.
* [LINGUAGEM]: Português Brasil(PT-BR).
* [COMPILA]: FPB(Facíl Programação Baixo nível).
* [DATA]: 10/09/2025.
* [ATUAL]: 10/09/2025.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <iostream>
#include <vector>

typedef enum {
    // tipos:
    T_ID, T_BOOL, T_INT, T_TEX, T_CAR, T_FLU, T_DOBRO, 
    T_LONGO, T_VAZIO, T_ARRAY, T_PONTEIRO,T_COMENTARIO,
    // simbolos:
    T_PAREN_ESQ, T_PAREN_DIR, T_CHAVE_ESQ,
    T_CHAVE_DIR, T_COL_ESQ, T_COL_DIR, T_PONTO_VIRGULA,
    T_VIRGULA, T_NAO, T_PERGUNTA,
    // operadores:
    T_IGUAL, T_MAIS, T_MENOS, T_VEZES, T_DIV,
    T_MAIS_MAIS, T_MENOS_MENOS, T_MAIS_IGUAL, T_MENOS_IGUAL,
    T_VEZES_IGUAL, T_DIV_IGUAL,
    // condicionais:
    T_SE, T_SENAO, T_IGUAL_IGUAL, T_DIFERENTE,
    T_MAIOR, T_MENOR, T_MAIOR_IGUAL, T_MENOR_IGUAL,
    // loops:
    T_POR, T_ENQ,
    // definições:
    T_DEF, T_REG, T_FIM, T_RETORNE, T_INCLUIR
} TipoToken;

// [UTIL]:
void fatal(const char* msg) {
    // \033[1;31m = vermelho
    // \033[0m = volta pra cor normal
    fprintf(stderr, "[\033[1;31mERRO\033[0m]: %s\n", msg);
    exit(1);
}

void aviso(const char* msg) {
    // \033[1;33m = amarelo
    fprintf(stderr, "[\033[1;33mAVISO\033[0m]: %s\n", msg);
}

// [ANALISE LEXICA]:
class Lexico {
    public:
    const char* fonte;
    size_t pos;
    TipoToken tk;
    int lin_atual;
    int col_atual;
    
    const char* token_str(TipoToken t) {
        switch(t) {
            case T_ID: return "identificador";
            case T_INT: return "inteiro";
            case T_TEX: return "texto";
            case T_CAR: return "caractere";
            case T_FLU: return "flutuante";
            case T_DOBRO: return "dobro";
            case T_LONGO: return "longo";
            case T_VAZIO: return "vazio";
            case T_ARRAY: return "array";
            case T_PONTEIRO: return "ponteiro";
            case T_COMENTARIO: return "comentário";
            case T_RETORNE: return "retorne";
            case T_POR: return "por";
            case T_ENQ: return "enq";
            case T_INCLUIR: return "incluir";
            case T_PAREN_ESQ: return "(";
            case T_PAREN_DIR: return ")";
            case T_CHAVE_ESQ: return "{";
            case T_CHAVE_DIR: return "}";
            case T_COL_ESQ: return "[";
            case T_COL_DIR: return "]";
            case T_PONTO_VIRGULA: return ";";
            case T_VIRGULA: return ",";
            case T_IGUAL: return "=";
            case T_MAIS: return "+";
            case T_MENOS: return "-";
            case T_VEZES: return "*";
            case T_DIV: return "/";
            case T_MAIS_IGUAL: return "+=";
            case T_MENOS_IGUAL: return "-=";
            case T_VEZES_IGUAL: return "*=";
            case T_DIV_IGUAL: return "/=";
            case T_IGUAL_IGUAL: return "==";
            case T_DIFERENTE: return "!=";
            case T_MAIOR: return ">";
            case T_MENOR: return "<";
            case T_MAIOR_IGUAL: return ">=";
            case T_MENOR_IGUAL: return "<=";
            case T_NAO: return "!";
            case T_PERGUNTA: return "?";
            case T_FIM: return "fim";
            // comunicação assembly:
            case T_DEF: return "def";
            case T_REG: return "registrador";
            default: return "desconhecido";
        }
    }
    
    const char* olhar(int pos) {
        // ...
        return "nulo";
    }
    
    void consumir() {
        // ...
    }
    
    void excessao(TipoToken t) {
        if(tk != t) {
            char msg[100];
            sprintf(msg, "Esperado %s, encontrado %s", token_str(t), token_str(tk));
            fatal(msg);
        }
        consumir();
    }
    
    int tam_tipo(TipoToken t) {
        switch(t) {
            case T_CAR: case T_BOOL: return 1;
            case T_INT: case T_FLU: return 4;
            case T_DOBRO: case T_LONGO: return 8;
            default: return 0;
        }
    }
    
    int compativeis(TipoToken tipo1, TipoToken tipo2) {
        if(tipo1 == tipo2) return 1;
        if((tipo1 == T_CAR && tipo2 == T_INT) || (tipo1 == T_INT && tipo2 == T_CAR)) return 1;
        return 0;
    }
};

// [ANALISE SINTATICA]:
class No {
    public:
    TipoToken tipo;
    No* esquerdo;
    No* direito;
    const char* valor;

    No(TipoToken t, const char* v = NULL) : tipo(t), valor(v), esquerdo(NULL), direito(NULL) {}
};

class Variavel : public No {
    public:
    std::string nome;
    char reg[3]; // x0, x1, e etc
    int pos; // posição na pilha
};

class Funcao : public No {
    public:
    char* nome;
    std::vector<Variavel> variaveis;
    std::vector<char*> chamadas; // chamadas de funções
};

class Sintatico : public Lexico {
    public:
    No* expressao() {
        No* no = termo();
        while(tk == T_MAIS || tk == T_MENOS) {
            TipoToken op = tk;
            consumir();
            No* direita = termo();
            No* novo = new No(op);
            novo->esquerdo = no;
            novo->direito = direita;
            no = novo;
        }
        return no;
    }

    No* termo() {
        No* no = fator();
        while(tk == T_VEZES || tk == T_DIV) {
            TipoToken op = tk;
            consumir();
            No* direita = fator();
            No* novo = new No(op);
            novo->esquerdo = no;
            novo->direito = direita;
            no = novo;
        }
        return no;
    }

    No* fator() {
        if(tk == T_INT) {
            No* no = new No(T_INT, "valor");
            consumir();
            return no;
        } else if(tk == T_PAREN_ESQ) {
            consumir();
            No* no = expressao();
            excessao(T_PAREN_DIR);
            return no;
        } else {
            fatal("fator inválido");
            return NULL;
        }
    }
};

int main(int argc, char** argv) {
    if(argc < 2) {
        printf("CFF: sem arquivos de entrada\n");
        return 0;
    }
    if(strcmp(argv[1], "-ajuda") == 0) {
        printf("[informação]:\n");
        printf("cff -v : versão e o distribuidor\n");
        printf("[compilação]:\n");
        printf("cff exemplo : compila um arquivo.fpb e gera o binário na pasta atual\n");
        printf("cff exemplo -s pasta/exemplo : compila um arquivo.fpb e cria um arquivo em um caminho personalizavel\n");
        printf("cff exemplo -asm : compila mantendo o ASM intermediario na pasta atual\n");
        printf("cff exemplo -s pasta/exemplo -asm : compila mantendo o ASM intermediario na pasta do binário\n");
        return 0;
    }
    if(strcmp(argv[1], "-v") == 0) {
        printf("CFF (Compilador Foca-do em FPB)\n\n[v0.0.1] (alpha)\n");
        return 0;
    }
    // implementar...
    return 0;
}
