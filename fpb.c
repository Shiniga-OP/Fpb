/*
* [FUNÇÃO]: Compilador.
* [IMPLEMENTAÇÃO]: @Shiniga-OP.
* [BASE]: Assembly.
* [SISTEMA OPERACIONAL]: ANDROID.
* [ARQUITETURA]: AARCH64-LINUX-ANDROID(ARM64).
* [LINGUAGEM]: Português Brasil(PT-BR).
* [DATA]: 06/07/2025.
* [ATUAL]: 02/12/2025.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>

#include "util/otimi1.h"
#include "util/otimi2.h"

#define MAX_TOK 8192 // maximo de tolens
#define MAX_CODIGO 8192 // maximo de codhgk
#define MAX_FN 252 // maximo de funções
#define MAX_VAR 252 // maximo de variaveis
#define MAX_CONST 252 // maxmio de constantes
#define MAX_PARAMS 8 // maximo de parametros
#define MAX_TEX 252 // mqximo de textos
#define MAX_DIMS 4  // máximo de dimensões para matrizes
#define MAX_MACROS 256 // maximo de macros

typedef enum {
    // mutação:
    T_FINAL,
    // tipos:
    T_ID, T_INT, T_TEX, T_CAR, T_FLU, T_BOOL, T_DOBRO,
    T_LONGO, T_BYTE,
    T_COMENTARIO,
    // simbolos:
    T_PAREN_ESQ, T_PAREN_DIR,  
    T_CHAVE_ESQ, T_CHAVE_DIR,
    T_COL_ESQ, T_COL_DIR,
    T_PONTO_VIRGULA, T_VIRGULA, 
    T_PONTO, T_LAMBDA, T_ARROBA, T_CONVERT,
    // operadores:
    T_IGUAL, T_MAIS, T_MENOS, T_VEZES, T_DIV, T_PORCEN,
    T_MAIS_MAIS, T_MENOS_MENOS,
    // condicionais:
    T_SE, T_SENAO, T_IGUAL_IGUAL, T_DIFERENTE,
    T_MAIOR, T_MENOR, T_MAIOR_IGUAL, T_MENOR_IGUAL,
    T_TAMBEM_TAMBEM, T_OU, T_OU_OU,
    // loops:
    T_POR, T_ENQ,
    // retornos:
    T_pCAR, T_pINT, T_pFLU, T_pBOOL, T_pDOBRO, 
    T_pLONGO, T_pBYTE, T_PONTEIRO,
    T_pVAZIO,
    // definições:
    T_DEF, T_FIM, T_RETORNAR, T_INCLUIR, 
    T_ESPACO, T_GLOBAL,
    // bits:
    T_MAIOR_MAIOR, T_MENOR_MENOR, T_TAMBEM, T_NAO
} TipoToken;

typedef struct {
    int linha;
    int coluna;
    const char* arquivo;
} Posicao;

typedef struct {
    TipoToken tipo;
    char lex[MAX_TOK];
    double valor_d; // pra constantes de ponto flutuante
    long valor_l; // pra constantes inteiras grandes
    Posicao pos;
} Token;

typedef struct {
    char nome[32];
    TipoToken tipo_base;
    int eh_ponteiro;
    int eh_array;
    int eh_parametro;
    int espaco_id; // 0 se não for espaço
    int eh_final;
    int num_dims; // numero de dimensões
    int dims[MAX_DIMS]; // tamanho de cada dimensão
    int bytes;
    int pos;
    int escopo;
    int valor;
    char reg[8];
} Variavel;

typedef struct {
    char nome[32];
    TipoToken retorno;
    Variavel vars[MAX_VAR];
    int var_conta;
    int escopo_atual;
    int frame_tam;
    int param_pos;
    int eh_global;
} Funcao;

typedef struct {
    const char* fonte;
    size_t pos;
    Token tk;
    int linha_atual;
    int coluna_atual;
} Lexer;

typedef struct {
    TipoToken tipo;
    char lex[32];
    double d_val;
    long l_val;
    int titulo;
} Constante;

typedef struct {
    char nome[32];
    long valor;
} Macro;

typedef struct {
    char nome[32];
    char valor[MAX_TOK];
} Tex;

static Lexer L;
static Funcao funcs[MAX_FN];
static int fn_cnt = 0;
static int escopo_global = 0;
static Constante constantes[MAX_CONST];
static int const_cnt = 0;
static Tex texs[MAX_TEX];
static int tex_cnt = 0;
static Macro macros[MAX_MACROS];
static int macro_cnt = 0;
static Variavel globais[MAX_VAR];
static int global_cnt = 0;
static char* arquivoAtual;

// buscar
Variavel* buscar_var(const char* nome, int escopo);
Funcao* buscar_fn(const char* nome);
Macro* buscar_macro(const char* nome);
// carregar
void carregar_valor(FILE* s, Variavel* var);
void carregar_const(FILE* s, int titulo);
// declaracao
void declaracao_var(FILE* s, int* pos, int escopo, int eh_parametro, int eh_final, int eh_global);
// tratar
TipoToken tratar_id(FILE* s, int escopo);
TipoToken tratar_chamada_funcao(FILE* s, int escopo, const char* nome, Funcao* fn);
TipoToken tratar_inteiro(FILE* s);
TipoToken tratar_flutuante(FILE* s);
TipoToken tratar_caractere(FILE* s);
TipoToken tratar_texto(FILE* s);
// verificar
void verificar_fn(FILE* s);
void verificar_stmt(FILE* s, int* pos, int escopo);
void verificar_retorno(FILE* s, int escopo);
void verificar_atribuicao(FILE* s, const char* id, int escopo);
void verificar_por(FILE* s, int escopo);
void verificar_enq(FILE* s, int escopo);
void verificar_matriz(FILE* s, Variavel* var, int escopo, int indices[], int nivel);
// add
int add_tex(const char* valor);
int add_const(TipoToken tipo, const char* lex, double d_val, long l_val);
// gerar
void gerar_comparacao(FILE* s, TipoToken op, TipoToken tipo);
void gerar_operacao(FILE* s, TipoToken op, TipoToken tipo);
void gerar_prelude(FILE* s);
// processar:
TipoToken processar_condicao(FILE* s, int escopo);
// etc
TipoToken expressao(FILE* s, int escopo);
TipoToken termo(FILE* s, int escopo);
TipoToken fator(FILE* s, int escopo);
TipoToken converter_tipos(FILE* s, TipoToken tipo_anterior, TipoToken tipo_atual);
void excessao(TipoToken t);
void fatal(const char* m);
void proximoToken();
int tam_tipo(TipoToken t);
void armazenar_valor(FILE* s, Variavel* var);
void escrever_valor(FILE* s, TipoToken tipo);
int calcular_pos_matriz(Variavel* var, int indices[]);

// [DEBUG]:
const char* token_str(TipoToken t) {
    switch(t) {
        case T_ID: return "identificador";
        case T_GLOBAL: return "identificador";
        case T_INT: return "inteiro";
        case T_TEX: return "texto";
        case T_CAR: return "caractere";
        case T_FLU: return "flutuante";
        case T_BOOL: return "booleano";
        case T_DOBRO: return "dobro";
        case T_BYTE: return "byte";
        case T_COMENTARIO: return "comentário";
        case T_FINAL: return "final";
        case T_PAREN_ESQ: return "(";
        case T_PAREN_DIR: return ")";
        case T_CHAVE_ESQ: return "{";
        case T_CHAVE_DIR: return "}";
        case T_COL_ESQ: return "[";
        case T_COL_DIR: return "]";
        case T_ARROBA: return "@";
        case T_LAMBDA: return "->";
        case T_PONTO_VIRGULA: return ";";
        case T_PONTO: return ".";
        case T_VIRGULA: return ",";
        case T_IGUAL: return "=";
        case T_MAIS: return "+";
        case T_MAIS_MAIS: return "++";
        case T_MENOS_MENOS: return "--";
        case T_MENOS: return "-";
        case T_VEZES: return "*";
        case T_DIV: return "/";
        case T_PORCEN: return "%";
        case T_CONVERT: return "(converter)";
        case T_IGUAL_IGUAL: return "==";
        case T_DIFERENTE: return "!=";
        case T_MAIOR: return ">";
        case T_MENOR: return "<";
        case T_MAIOR_IGUAL: return ">=";
        case T_MENOR_IGUAL: return "<=";
        case T_TAMBEM_TAMBEM: return "&&";
        case T_OU_OU: return "||";
        case T_pCAR: return "car";
        case T_pINT: return "int";
        case T_pFLU: return "flu";
        case T_pBOOL: return "bool";
        case T_pDOBRO: return "dobro";
        case T_pBYTE: return "byte";
        case T_PONTEIRO: return "ponteiro";
        case T_pVAZIO: return "vazio";
        case T_RETORNAR: return "retorne";
        case T_POR: return "por";
        case T_ENQ: return "enq";
        case T_INCLUIR: return "#incluir";
        case T_ESPACO: return "#espaco";
        case T_DEF: return "#def";
        case T_FIM: return "fim";
        case T_TAMBEM: return "&";
        case T_MAIOR_MAIOR: return ">>";
        case T_MENOR_MENOR: return "<<";
        default: return "desconhecido";
    }
}

void fatal(const char* m) {
    fprintf(stderr, "%s.fpb [ERRO] linha: %d coluna: %d, %s próximo de \"%s\"\n", arquivoAtual, L.tk.pos.linha + 1, L.tk.pos.coluna + 1, m, L.tk.lex);
    exit(1);
}

void excessao(TipoToken t) {
    if(L.tk.tipo != t) {
        char msg[100];
        sprintf(msg, "Esperado %s, encontrado %s", token_str(t), token_str(L.tk.tipo));
        fatal(msg);
    }
    proximoToken();
}
// [UTIL]:
int tam_tipo(TipoToken t) {
    switch(t) {
        case T_pBYTE: case T_pCAR: case T_pBOOL: return 1;
        case T_pINT: case T_pFLU: return 4;
        case T_pDOBRO: case T_pLONGO: case T_PONTEIRO: return 8;
        default: return 0;
    }
}

int tipos_compativeis(TipoToken tipo1, TipoToken tipo2) {
    if(tipo1 == tipo2) return 1;
    if((tipo1 == T_pBYTE && (tipo2 == T_pCAR || tipo2 == T_pINT)) || (tipo2 == T_pBYTE && (tipo1 == T_pCAR || tipo1 == T_pINT))) return 1;
    if((tipo1 == T_pCAR && tipo2 == T_pINT) || (tipo1 == T_pINT && tipo2 == T_pCAR)) return 1;
    if((tipo1 == T_pINT && tipo2 == T_pLONGO) || (tipo1 == T_pLONGO && tipo2 == T_pINT)) return 1;
    return 0;
}

int eh_tipo(TipoToken tipo) {
    TipoToken tipos[] = {
        T_pVAZIO, T_pINT, T_pFLU, T_pCAR, T_pBOOL,
        T_pDOBRO, T_pLONGO, T_PONTEIRO, T_pBYTE};
    for(int i = 0; i < sizeof(tipos)/sizeof(tipos[0]); i++) {
        if(tipo == tipos[i]) return 1;
    }
    return 0;
}

void proximoToken() {
    char c;
    int i;

    while(1) {
        c = L.fonte[L.pos];
        while(c && isspace((unsigned char)c)) {
            if(c == '\n') {
                L.linha_atual++;
                L.coluna_atual = 1;
            } else L.coluna_atual++;
            L.pos++;
            c = L.fonte[L.pos];
        }
        if(c == '/' && L.fonte[L.pos + 1] == '/') {
            L.pos += 2;
            L.coluna_atual += 2;
            while ((c = L.fonte[L.pos]) && c != '\n') {
                L.pos++;
                L.coluna_atual++;
            }
            continue;
        }
        if(c == '/' && L.fonte[L.pos + 1] == '*') {
            L.pos += 2;
            L.coluna_atual += 2;
            while(1) {
                c = L.fonte[L.pos];
                if(!c) fatal("comentario nao fechado");
                if(c == '\n') {
                    L.linha_atual++;
                    L.coluna_atual = 1;
                    L.pos++;
                    continue;
                }
                if(c == '*' && L.fonte[L.pos + 1] == '/') {
                    L.pos += 2;
                    L.coluna_atual += 2;
                    break;
                }
                L.pos++;
                L.coluna_atual++;
            }
            continue;
        }
        break;
    }
    c = L.fonte[L.pos];
    if(!c) {
        L.tk.tipo = T_FIM;
        return; 
    }
    L.tk.pos.linha = L.linha_atual;
    L.tk.pos.coluna = L.coluna_atual;

    if(c == '#') {
        L.pos++; L.coluna_atual++;
        i = 0;
        c = L.fonte[L.pos];
        while(c && isspace((unsigned char)c)) {
            if(c == '\n') {
                L.linha_atual++;
                L.coluna_atual = 1;
            } else L.coluna_atual++;
            L.pos++;
            c = L.fonte[L.pos];
        }
        while(c && isalpha((unsigned char)c)) {
            if(i < MAX_TOK - 1) L.tk.lex[i++] = c;
            L.pos++; L.coluna_atual++;
            c = L.fonte[L.pos];
        }
        L.tk.lex[i] = '\0';
        if(strcmp(L.tk.lex, "incluir") == 0) {
            L.tk.tipo = T_INCLUIR;
            return;
        }
        if(strcmp(L.tk.lex, "espaco") == 0) {
            L.tk.tipo = T_ESPACO;
            return;
        }
        if(strcmp(L.tk.lex, "def") == 0) {
            L.tk.tipo = T_DEF;
            return;
        }
        if(strcmp(L.tk.lex, "global") == 0) {
            L.tk.tipo = T_GLOBAL;
            return;
        }
        fatal("diretiva desconhecida");
    }
    if(isalpha((unsigned char)c) || c == '_') {
        i = 0;
        while((c = L.fonte[L.pos]) && (isalnum((unsigned char)c) || c == '_')) {
            if(i < MAX_TOK - 1) L.tk.lex[i++] = c;
            L.pos++; L.coluna_atual++;
        }
        L.tk.lex[i] = 0;
        // reconhece tipos e outros
        if(strcmp(L.tk.lex, "car") == 0) L.tk.tipo = T_pCAR;
        else if(strcmp(L.tk.lex, "int") == 0) L.tk.tipo = T_pINT;
        else if(strcmp(L.tk.lex, "byte") == 0) L.tk.tipo = T_pBYTE;
        else if(strcmp(L.tk.lex, "flu") == 0) L.tk.tipo = T_pFLU;
        else if(strcmp(L.tk.lex, "bool") == 0) L.tk.tipo = T_pBOOL;
        else if(strcmp(L.tk.lex, "dobro") == 0) L.tk.tipo = T_pDOBRO;
        else if(strcmp(L.tk.lex, "longo") == 0) L.tk.tipo = T_pLONGO;
        else if(strcmp(L.tk.lex, "vazio") == 0) L.tk.tipo = T_pVAZIO;
        else if(strcmp(L.tk.lex, "se") == 0) L.tk.tipo = T_SE;
        else if(strcmp(L.tk.lex, "senao") == 0) L.tk.tipo = T_SENAO;
        else if(strcmp(L.tk.lex, "por") == 0) L.tk.tipo = T_POR;
        else if(strcmp(L.tk.lex, "enq") == 0) L.tk.tipo = T_ENQ;
        else if(strcmp(L.tk.lex, "retorne") == 0) L.tk.tipo = T_RETORNAR;
        else if(strcmp(L.tk.lex, "final") == 0) L.tk.tipo = T_FINAL;
        else if(strcmp(L.tk.lex, "->") == 0) L.tk.tipo = T_LAMBDA;
        else L.tk.tipo = T_ID;
        return;
    }
    // reconhecer bytes(hexadecimal: 0xAB ou binario: 0b1010)
    if(c == '0' && (L.fonte[L.pos + 1] == 'x' || L.fonte[L.pos + 1] == 'b')) {
        int eh_hex = (L.fonte[L.pos + 1] == 'x');
        L.pos += 2; 
        L.coluna_atual += 2;
        i = 0;
        L.tk.lex[i++] = '0';
        L.tk.lex[i++] = eh_hex ? 'x' : 'b';
        
        c = L.fonte[L.pos];
        while(c && ((eh_hex && isxdigit((unsigned char)c)) || (!eh_hex && (c == '0' || c == '1')))) {
            if(i < MAX_TOK - 1) L.tk.lex[i++] = c;
            L.pos++; 
            L.coluna_atual++;
            c = L.fonte[L.pos];
        }
        L.tk.lex[i] = 0;
        // converte pra valor(garantir que é so 1 byte)
        if(eh_hex) {
            L.tk.valor_l = strtol(L.tk.lex + 2, NULL, 16) & 0xFF;
        } else {
            L.tk.valor_l = strtol(L.tk.lex + 2, NULL, 2) & 0xFF;
        }
        L.tk.tipo = T_BYTE;
        return;
    }
    if((c == '-' && isdigit((unsigned char)L.fonte[L.pos + 1])) || isdigit((unsigned char)c) || c == '.') {
        i = 0;
        int ponto = 0;
        int negativo = 0;
        int tem_sufixo = 0;
        char sufixo = 0;
        // verifica se é negativo
        if(c == '-') {
            negativo = 1;
            if(i < MAX_TOK - 1) L.tk.lex[i++] = c;
            L.pos++; L.coluna_atual++;
            c = L.fonte[L.pos];
        }
        while((c = L.fonte[L.pos]) && (isdigit((unsigned char)c) || c == '.' || c == 'f' || c == 'F' || c == 'd' || c == 'D')) {
            if(c == '.') {
                if(ponto) fatal("numero invalido");
                ponto = 1;
            }
            if(c == 'f' || c == 'F' || c == 'd' || c == 'D') {
                tem_sufixo = 1;
                sufixo = c;
                L.pos++;
                L.coluna_atual++;
                break;
            }
            if(i < MAX_TOK - 1) L.tk.lex[i++] = c;
            L.pos++;
            L.coluna_atual++;
        }
        L.tk.lex[i] = 0;
        // determina o tipo baseado no sufixo e na presença de ponto
        if(tem_sufixo && (sufixo == 'd' || sufixo == 'D')) {
            L.tk.tipo = T_DOBRO;
            L.tk.valor_d = atof(L.tk.lex);
        } else if(ponto || (tem_sufixo && (sufixo == 'f' || sufixo == 'F'))) {
            L.tk.tipo = T_FLU;
            L.tk.valor_d = atof(L.tk.lex);
        } else {
            L.tk.tipo = T_INT;
            L.tk.valor_l = atol(L.tk.lex);
        }
        return;
    }
    if(c == '"') {
        L.pos++; L.coluna_atual++;
        i = 0;
        c = L.fonte[L.pos];
        while(c && c != '"') {
            if(c == '\n') fatal("tex mal formado");
            if(c == '\\') {
                char n = L.fonte[L.pos + 1];
                if(!n) fatal("tex mal formado");
                if(i < MAX_TOK-2) { L.tk.lex[i++] = '\\'; L.tk.lex[i++] = n; }
                L.pos += 2; L.coluna_atual += 2;
                c = L.fonte[L.pos];
                continue;
            }
            if(i < MAX_TOK - 1) L.tk.lex[i++] = c;
            L.pos++; L.coluna_atual++;
            c = L.fonte[L.pos];
        }
        L.tk.lex[i] = 0;
        if(L.fonte[L.pos] == '"') {
            L.pos++;
            L.coluna_atual++;
        } else fatal("tex nao fechado");
        L.tk.tipo = T_TEX;
        return;
    }
    if(c == '\'') {
        L.pos++; L.coluna_atual++;
        if(!L.fonte[L.pos]) fatal("caractere mal formado");
        if(L.fonte[L.pos] == '\\') {
            char n = L.fonte[L.pos + 1];
            if(!n || L.fonte[L.pos + 2] != '\'') fatal("caractere mal formado");
            if(MAX_TOK > 3) sprintf(L.tk.lex, "\\%c", n);
            L.pos += 3;
            L.coluna_atual += 3;
        } else {
            char v = L.fonte[L.pos];
            if(L.fonte[L.pos + 1] != '\'') fatal("caractere mal formado");
            if(MAX_TOK > 2) sprintf(L.tk.lex, "%c", v);
            L.pos += 2;
            L.coluna_atual += 2;
        }
        L.tk.tipo = T_CAR;
        return;
    }
    // caracteres simples:
    switch(c) {
        case '(':
        // verifica se é um conversão: (tipo)expressao
        if(isalpha((unsigned char)L.fonte[L.pos + 1])) {
            // salva estado atual para verificação
            size_t salvo = L.pos + 1;
            char tipo_tex[32] = {0};
            int i = 0;
            // coleta o possivel nome de tipo
            while(isalpha((unsigned char)L.fonte[salvo]) && i < 31) {
                tipo_tex[i++] = L.fonte[salvo++];
            }
            tipo_tex[i] = '\0';
            // verifica se é um tipo valido
            if(strcmp(tipo_tex, "car") == 0 || strcmp(tipo_tex, "int") == 0 || 
            strcmp(tipo_tex, "flu") == 0 || strcmp(tipo_tex, "dobro") == 0 ||
            strcmp(tipo_tex, "longo") == 0 || strcmp(tipo_tex, "bool") == 0) {
                if(L.fonte[salvo] == ')') {
                    L.tk.tipo = T_CONVERT;
                    strcpy(L.tk.lex, tipo_tex);
                    L.pos = salvo + 1; // pula ')'
                    L.coluna_atual += (salvo - L.pos + 1);
                    return;
                }
            }
        }
        // se não for conversão, trata como parentese normal
        L.tk.tipo = T_PAREN_ESQ;
        break;
        case ')': L.tk.tipo = T_PAREN_DIR; break;
        case '{': L.tk.tipo = T_CHAVE_ESQ; break;
        case '}': L.tk.tipo = T_CHAVE_DIR; break;
        case '[': L.tk.tipo = T_COL_ESQ; break;
        case ']': L.tk.tipo = T_COL_DIR; break;
        case '@': L.tk.tipo = T_ARROBA; break;
        case ';': L.tk.tipo = T_PONTO_VIRGULA; break;
        case ',': L.tk.tipo = T_VIRGULA; break;
        case '.': L.tk.tipo = T_PONTO; break;
        case '=':
            if(L.fonte[L.pos + 1] == '=') {
                L.tk.tipo = T_IGUAL_IGUAL;
                L.pos++;
                L.coluna_atual++;
            } else L.tk.tipo = T_IGUAL;
        break;
        case '+': 
            if(L.fonte[L.pos + 1] == '+') {
                L.tk.tipo = T_MAIS_MAIS;
                L.pos++;
                L.coluna_atual++;
            } else L.tk.tipo = T_MAIS;
        break;
        case '-': 
            if(L.fonte[L.pos + 1] == '-') {
                L.tk.tipo = T_MENOS_MENOS;
                L.pos++;
                L.coluna_atual++;
            } else L.tk.tipo = T_MENOS;
        break;
        case '*': L.tk.tipo = T_VEZES; break;
        case '/': L.tk.tipo = T_DIV; break;
        case '%': L.tk.tipo = T_PORCEN; break;
        case '>':
            if(L.fonte[L.pos + 1] == '=') {
                L.tk.tipo = T_MAIOR_IGUAL;
                L.pos++;
                L.coluna_atual++;
            } else if(L.fonte[L.pos + 1] == '>') {
                L.tk.tipo = T_MAIOR_MAIOR;
                L.pos++;
                L.coluna_atual++;
            } else L.tk.tipo = T_MAIOR;
        break;
        case '<':
            if(L.fonte[L.pos + 1] == '=') {
                L.tk.tipo = T_MENOR_IGUAL;
                L.pos++;
                L.coluna_atual++;
            } else if(L.fonte[L.pos + 1] == '<') {
                L.tk.tipo = T_MENOR_MENOR;
                L.pos++;
                L.coluna_atual++;
            } else L.tk.tipo = T_MENOR;
        break;
        case '!':
            if(L.fonte[L.pos + 1] == '=') {
                L.tk.tipo = T_DIFERENTE;
                L.pos++;
                L.coluna_atual++;
            }
        break;
        case '&':
        if(L.fonte[L.pos + 1] == '&') {
            L.tk.tipo = T_TAMBEM_TAMBEM;
            L.pos++;
            L.coluna_atual++;
        } else L.tk.tipo = T_TAMBEM;
        break;
        case '|':
        if(L.fonte[L.pos + 1] == '|') {
            L.tk.tipo = T_OU_OU;
            L.pos++;
            L.coluna_atual++;
        } else L.tk.tipo = T_OU;
        break;
        default: fatal("Símbolo inválido"); break;
    }
    L.tk.lex[0] = c;
    L.tk.lex[1] = 0;
    L.pos++;
    L.coluna_atual++;
}
// [CALCULO]:
int calcular_pos_matriz(Variavel* var, int indices[]) {
    int pos = 0;
    int stride = tam_tipo(var->tipo_base);
    
    for(int i = var->num_dims - 1; i >= 0; i--) {
        if(i < var->num_dims - 1) {
            stride *= (var->dims[i + 1] > 0 ? var->dims[i + 1] : 1);
        }
        pos += indices[i] * stride;
    }
    return pos;
}
// [TRATAMENTO]:
TipoToken tratar_texto(FILE* s) {
    int id = add_tex(L.tk.lex);
    fprintf(s, "  ldr x0, = %s\n", texs[id].nome);
    proximoToken();
    return T_TEX;
}

TipoToken tratar_id(FILE* s, int escopo) {
    char id[32];
    strcpy(id, L.tk.lex);
    if(id[0] == '@') {
        char var_nome[32];
        strcpy(var_nome, id + 1); // remove o @
        
        Variavel* var = buscar_var(var_nome, escopo);
        if(!var) fatal("[tratar_id] variável não encontrada após @");
        // se for ponteiro, retorna o valor do ponteiro(endereço armazenado)
        if(var->eh_ponteiro) {
            fprintf(s, "  ldr x0, [x29, %d]\n", var->pos);
        } else {
            // se for variável normal, retorna endereço onde ta armazenada
            fprintf(s, "  add x0, x29, %d\n", var->pos);
        }
        proximoToken();
        return T_PONTEIRO;
    }
    Variavel* var = buscar_var(id, escopo);
    
    if(!var) {
        Funcao* fn = buscar_fn(id);
        if(fn) {
            proximoToken();
            excessao(T_PAREN_ESQ);
            return tratar_chamada_funcao(s, escopo, id, fn);
        } else {
            Macro* macro = buscar_macro(id);
            if(macro) {
                // trata como um literal T_INT
                L.tk.tipo = T_INT;
                L.tk.valor_l = macro->valor;
                // deixa tratar_inteiro consumir o token e gerar o codigo
                return tratar_inteiro(s);
            }
            fatal("variável, função ou macro não declarada");
        }
    }
    proximoToken();
    
    if(var->escopo == -1) { // variavel global
        if(var->eh_array && var->tipo_base == T_pCAR) {
            // array de caracteres global
            fprintf(s, "  ldr x0, = global_%s\n", var->nome);
            return T_PONTEIRO; // retorna como texto
        } else if(var->eh_ponteiro && var->tipo_base == T_pCAR) {
            // ponteiro pra caractere global
            fprintf(s, "  ldr x0, = global_%s\n", var->nome);
            fprintf(s, "  ldr x0, [x0]\n"); // carrega o ponteiro
            return T_PONTEIRO;
        } else {
            // outras variaveis globais
            carregar_valor(s, var);
            return var->tipo_base;
        }
    }
    if(L.tk.tipo == T_MAIS_MAIS || L.tk.tipo == T_MENOS_MENOS) {
        TipoToken op = L.tk.tipo;
        proximoToken();
        
        // carrega o valor atual da variavel
        carregar_valor(s, var);
        
        // incrementa ou decrementa
        if(op == T_MAIS_MAIS) {
            if(var->tipo_base == T_pFLU) {
                fprintf(s, "  fmov s1, 1.0\n");
                fprintf(s, "  fadd s0, s0, s1\n");
            } else if(var->tipo_base == T_pDOBRO) {
                fprintf(s, "  fmov d1, 1.0\n");
                fprintf(s, "  fadd d0, d0, d1\n");
            } else if(var->tipo_base == T_pLONGO) {
                fprintf(s, "  add x0, x0, 1\n");
            } else {
                fprintf(s, "  add w0, w0, 1\n");
            }
        } else { // T_MENOS_MENOS
            if(var->tipo_base == T_pFLU) {
                fprintf(s, "  fmov s1, 1.0\n");
                fprintf(s, "  fsub s0, s0, s1\n");
            } else if(var->tipo_base == T_pDOBRO) {
                fprintf(s, "  fmov d1, 1.0\n");
                fprintf(s, "  fsub d0, d0, d1\n");
            } else if(var->tipo_base == T_pLONGO) {
                fprintf(s, "  sub x0, x0, 1\n");
            } else {
                fprintf(s, "  sub w0, w0, 1\n");
            }
        }
        // armazena o novo valor de volta
        armazenar_valor(s, var);
        
        return var->tipo_base;
    }
    if(var->eh_ponteiro && var->tipo_base == T_pCAR) {
        // ponteiro pra caractere, carrega o endereço do texto
        fprintf(s, "  ldr x0, [x29, %d]\n", var->pos);
        return T_PONTEIRO;
    }
    // acesso a matriz/array
    if(L.tk.tipo == T_COL_ESQ) {
        int indices[MAX_DIMS] = {0};
        int dim_atual = 0;
        // coleta todos os indices
        while(L.tk.tipo == T_COL_ESQ && dim_atual < var->num_dims) {
            excessao(T_COL_ESQ);
            expressao(s, escopo); // resultado em w0
            fprintf(s, "  str w0, [sp, -16]!\n"); // salva indice na pilha
            indices[dim_atual] = -1; // marcador
            dim_atual++;
            excessao(T_COL_DIR);
        }
        // calcula a posicao na matriz
        if(dim_atual > 0) {
            // inicia pos
            fprintf(s, "  mov w0, 0\n"); // w0 = pos acumulado
            
            // pra cada dimensão, calcula o pos parcial
            for(int i = 0; i < dim_atual; i++) {
                fprintf(s, "  ldr w1, [sp], 16\n"); // w1 = indice atual
                // calcula stride pra essa dimensão
                int stride = tam_tipo(var->tipo_base);
                for(int j = i + 1; j < var->num_dims; j++) {
                    if(var->dims[j] > 0) {
                        stride *= var->dims[j];
                    }
                }
                // pos += indice * stride
                fprintf(s, "  mov w2, %d\n", stride);
                fprintf(s, "  mul w1, w1, w2\n");
                fprintf(s, "  add w0, w0, w1\n");
            }
            // carrega o valor
            if(var->escopo == -1) {
                fprintf(s, "  ldr x2, = global_%s\n", var->nome);
            } else {
                if(var->eh_parametro) fprintf(s, "  ldr x2, [x29, %d]\n", var->pos);
                else fprintf(s, "  add x2, x29, %d\n", var->pos);
            }
            fprintf(s, "  add x2, x2, x0\n"); // adiciona pos calculado
            
            // carrega o valor baseado no tipo
            if(var->tipo_base == T_pCAR || var->tipo_base == T_pBOOL || var->tipo_base == T_pBYTE) 
                fprintf(s, "  ldrb w0, [x2]\n");
            else if(var->tipo_base == T_pINT)
                fprintf(s, "  ldr w0, [x2]\n");
            else if(var->tipo_base == T_pFLU) 
                fprintf(s, "  ldr s0, [x2]\n");
            else if(var->tipo_base == T_pDOBRO) 
                fprintf(s, "  ldr d0, [x2]\n");
            else if(var->tipo_base == T_pLONGO)
                fprintf(s, "  ldr x0, [x2]\n");
            
            return var->tipo_base;
        }
    } else if(var->eh_array) {
        if(var->eh_parametro) fprintf(s, "  ldr x0, [x29, %d]\n", var->pos);
        else fprintf(s, "  add x0, x29, %d\n", var->pos);
        return T_PONTEIRO;
    } else if(var->eh_ponteiro) {
        fprintf(s, "  ldr x1, [x29, %d]\n", var->pos);
        // carrega endereço do ponteiro
        // agora dereferencia baseado no tipo base
        if(var->tipo_base == T_pCAR || var->tipo_base == T_pBOOL || var->tipo_base == T_pBYTE) {
            fprintf(s, "  ldrb w0, [x1]\n");
        } else if(var->tipo_base == T_pINT) {
            fprintf(s, "  ldr w0, [x1]\n"); 
        } else if(var->tipo_base == T_pFLU) {
            fprintf(s, "  ldr s0, [x1]\n");
        } else if(var->tipo_base == T_pDOBRO) {
            fprintf(s, "  ldr d0, [x1]\n");
        } else if(var->tipo_base == T_pLONGO || var->tipo_base == T_PONTEIRO) {
            fprintf(s, "  ldr x0, [x1]\n");
        }
        return var->tipo_base;  // retorna o tipo base, não T_PONTEIRO
    } else {
        carregar_valor(s, var);
        return var->tipo_base;
    }
    return var->tipo_base;
}

TipoToken tratar_chamada_funcao(FILE* s, int escopo, const char* nome, Funcao* fn) {
    if(fn == NULL) fatal("INTERNO CRITICO, FUNÇÃO INEXISTENTE!");
    int param_conta = 0;
    TipoToken param_tipos[MAX_PARAMS];
    
    if(L.tk.tipo != T_PAREN_DIR) {
        do {
            param_tipos[param_conta] = expressao(s, escopo);
            // salva resultado em temporario na pilha
            if(param_tipos[param_conta] == T_pFLU) fprintf(s, "  str s0, [sp, -16]!\n");
            else if(param_tipos[param_conta] == T_pDOBRO) fprintf(s, "  str d0, [sp, -16]!\n");
            else if(tam_tipo(param_tipos[param_conta]) <= 4) fprintf(s, "  str w0, [sp, -16]!\n");
            else fprintf(s, "  str x0, [sp, -16]!\n");
            
            param_conta++;
            if(param_conta >= MAX_PARAMS) fatal("excesso de parâmetros na chamada de função, max: 9");
        } while(L.tk.tipo == T_VIRGULA && (proximoToken(), 1));
    }
    excessao(T_PAREN_DIR); // consome ')'
    
    int int_reg = 0;
    int fp_s_reg = 0; // registradores de flutuante(s0-s7)
    int fp_d_reg = 0; // registradores de dobro(d0-d7)
    
    for(int i = 0; i < param_conta; i++) {
        int pos = (param_conta - i - 1) * 16;
        
        if(int_reg < 8 && (param_tipos[i] == T_pINT || param_tipos[i] == T_pLONGO || param_tipos[i] == T_PONTEIRO || param_tipos[i] == T_pCAR || param_tipos[i] == T_pBOOL)) {
            fprintf(s, "  ldr x%d, [sp, %d]\n", int_reg, pos);
            int_reg++;
        } else if(fp_s_reg < 8 && param_tipos[i] == T_pFLU) {
            fprintf(s, "  ldr s%d, [sp, %d]\n", fp_s_reg, pos);
            fp_s_reg++;
        } else if(fp_d_reg < 8 && param_tipos[i] == T_pDOBRO) {
            fprintf(s, "  ldr d%d, [sp, %d]\n", fp_d_reg, pos);
            fp_d_reg++;
        } else {
            // args 9+ = pilha
            if(param_tipos[i] == T_pFLU) {
                fprintf(s, "  ldr s0, [sp, %d]\n", pos);
                fprintf(s, "  str s0, [sp, -16]!\n");
            } else if(param_tipos[i] == T_pDOBRO) {
                fprintf(s, "  ldr d0, [sp, %d]\n", pos);
                fprintf(s, "  str d0, [sp, -16]!\n");
            } else if(tam_tipo(param_tipos[i]) <= 4) {
                fprintf(s, "  ldr w0, [sp, %d]\n", pos);
                fprintf(s, "  str w0, [sp, -16]!\n");
            } else {
                fprintf(s, "  ldr x0, [sp, %d]\n", pos);
                fprintf(s, "  str x0, [sp, -16]!\n");
            }
        }
    }
    // limpa temporarios da pilha
    if(param_conta > 0) fprintf(s, "  add sp, sp, %d\n", param_conta * 16);
    
    fprintf(s, "  bl %s\n", nome);
    // se a função retorna flutuante, garante que está no registrador correto
    if(fn->retorno == T_pFLU) fprintf(s, "  fmov s0, s0\n"); // garante que o retorno ta em s0
    else if(fn->retorno == T_pDOBRO) fprintf(s, "  fmov d0, d0\n"); // garante que o retorno ta em d0
    return fn->retorno;
}

TipoToken tratar_inteiro(FILE* s) {
    char num[32];
    strcpy(num, L.tk.lex);
    long l_val = L.tk.valor_l;
    proximoToken();
    // verifica se é um sufixo "L" pra longo
    if(L.tk.tipo == T_ID && (strcmp(L.tk.lex, "L") == 0 || strcmp(L.tk.lex, "l") == 0)) {
        // é um numero longo
        proximoToken();
        // pra numeros longos, sempre usa constante para valores maiores que 16 bits
        if(l_val <= 0xFFFF) {
            // numero pequeno cabe em mov imediato
            fprintf(s, "  mov x0, %ld\n", l_val);
        } else {
            // numero grande, precisa de constante
            int titulo = add_const(T_LONGO, num, 0.0, l_val);
            carregar_const(s, titulo);
        }
        return T_pLONGO;
    } else {
        // é um inteiro normal
        // pra inteiros de 32 bits, usa constante pra valores maiores que 16 bits
        if(l_val <= 0xFFFF) {
            fprintf(s, "  mov w0, %ld\n", l_val);
        } else {
            int titulo = add_const(T_INT, num, 0.0, l_val);
            carregar_const(s, titulo);
        }
        return T_pINT;
    }
}

TipoToken tratar_flutuante(FILE* s) {
    char num[32];
    strcpy(num, L.tk.lex);
    double d_val = L.tk.valor_d;
    TipoToken const_tipo = L.tk.tipo;
    proximoToken();
    
    int titulo = add_const(const_tipo, num, d_val, 0);
    carregar_const(s, titulo);
    return const_tipo == T_FLU ? T_pFLU : T_pDOBRO;
}

TipoToken tratar_caractere(FILE* s) {
    char val = L.tk.lex[0];
    proximoToken();
    fprintf(s, "  mov w0, %d\n", val);
    return T_pCAR;
}

TipoToken tratar_byte(FILE* s) {
    char num[32];
    strcpy(num, L.tk.lex);
    long byte_val = L.tk.valor_l;
    proximoToken();
    // bytes sempre usam valor imediato(são pequenos)
    fprintf(s, "  mov w0, %ld // byte: %s\n", byte_val, num);
    return T_pBYTE;
}
// [BUSCA]:
Variavel* buscar_var(const char* nome, int escopo) {
    // primeiro procura em variaveis locais
    if(fn_cnt > 0) {
        Funcao* f = &funcs[fn_cnt - 1];
        for(int i = f->var_conta - 1; i >= 0; i--) {
            if(strcmp(f->vars[i].nome, nome) == 0 && f->vars[i].escopo <= escopo) {
                return &f->vars[i];
            }
        }
    }
    // depois procura em variaveis globais
    for(int i = 0; i < global_cnt; i++) {
        if(strcmp(globais[i].nome, nome) == 0) {
            return &globais[i];
        }
    }
    return NULL;
}

Funcao* buscar_fn(const char* nome) {
    for(int i = 0; i < fn_cnt; i++) {
        if(strcmp(funcs[i].nome, nome) == 0) return &funcs[i];
    }
    return NULL;
}

Macro* buscar_macro(const char* nome) {
    for(int i = 0; i < macro_cnt; i++) {
        if(strcmp(macros[i].nome, nome) == 0) return &macros[i];
    }
    return NULL;
}
// [PROCESSAMENTO]:
void processar_args(FILE* s, Funcao* f) {
    int int_reg_idc = 0;
    int fp_reg_idc = 0;
    int pilha_pos = 0;
    Token salvo;
    f->param_pos = 0;

    while(L.tk.tipo != T_PAREN_DIR) {
        Variavel* var = &f->vars[f->var_conta];
        // arrays como parametros devem ser tratados como ponteiros(T_PONTEIRO)
        // mesmo que seu tipo base seja flutuante
        TipoToken tipo_param = L.tk.tipo;
        int eh_array_param = 0;
        // verifica se é um array
        if(L.tk.tipo == T_pFLU || L.tk.tipo == T_pINT || L.tk.tipo == T_pCAR || 
           L.tk.tipo == T_pBOOL || L.tk.tipo == T_pDOBRO || L.tk.tipo == T_pLONGO) {
            // salva estado atual
            salvo = L.tk;
            Lexer salvo_lexer = L;
            
            proximoToken();
            if(L.tk.tipo == T_COL_ESQ) {
                eh_array_param = 1;
                tipo_param = T_PONTEIRO; // arrays são passados como ponteiros
            }
            // restaura estado
            L.tk = salvo;
            L = salvo_lexer;
        }
        // arrays como parametros vão para registradores inteiros
        if(int_reg_idc < 8 && (tipo_param == T_pINT || tipo_param == T_pLONGO || tipo_param == T_PONTEIRO || 
            tipo_param == T_pCAR || tipo_param == T_pBOOL || eh_array_param)) {
            sprintf(var->reg, "x%d", int_reg_idc++);
            var->pos = -1;
            var->eh_array = eh_array_param; // marca como array
            if(eh_array_param) var->tipo_base = salvo.tipo; // mantem o tipo base
        } else if(fp_reg_idc < 8 && (tipo_param == T_pFLU || tipo_param == T_pDOBRO) && !eh_array_param) {
            sprintf(var->reg, "d%d", fp_reg_idc++);
            var->pos = -1;
        } else {
            var->reg[0] = '\0';
            var->pos = pilha_pos;
            pilha_pos += 8;
            var->eh_array = eh_array_param;
            if(eh_array_param) var->tipo_base = salvo.tipo;
        }
        declaracao_var(s, &pilha_pos, 0, 1, 0, 0);
        
        if(L.tk.tipo == T_VIRGULA) proximoToken();
        else break;
    }
}

char* processar_caminho() {
    static char caminho[512] = "";
    // tenta primeiro da variavel de ambiente FPB_DIR
    char* amb_dir = getenv("FPB_DIR");
    if(amb_dir != NULL && strlen(amb_dir) > 0) {
        strncpy(caminho, amb_dir, sizeof(caminho) - 1);
        caminho[sizeof(caminho) - 1] = '\0';
        return caminho;
    }
    // senão, usa o diretorio atual
    if(getcwd(caminho, sizeof(caminho)) != NULL) {
        return caminho;
    }
    // ultimo: diretorio vazio
    strcpy(caminho, "");
    return caminho;
}

TipoToken processar_condicao(FILE* s, int escopo) {
    TipoToken tipo_final = T_pBOOL;
    int primeiro = 1;
    int rotulo_fim = escopo_global++;
    
    while(L.tk.tipo != T_PAREN_DIR && L.tk.tipo != T_FIM) {
        TipoToken tipo_atual = expressao(s, escopo);
        
        if(!primeiro) {
            // se não é o primeiro termo, deve ser operador lógico
            if(L.tk.tipo == T_TAMBEM_TAMBEM || L.tk.tipo == T_OU_OU) {
                TipoToken op = L.tk.tipo;
                proximoToken(); // consome && ou ||
                
                int rotulo_curto = escopo_global++;
                
                if(op == T_TAMBEM_TAMBEM) {
                    // caso E: se a primeira condição for falsa, pula pra falso
                    fprintf(s, "  cmp w0, 0\n");
                    fprintf(s, "  beq .B%d\n", rotulo_curto);
                } else { // T_OU_OU
                    // caso OU: se a primeira condição for verdadeira, pula pra verdadeiro
                    fprintf(s, "  cmp w0, 1\n");
                    fprintf(s, "  beq .B%d\n", rotulo_curto);
                }
                // processa proxima expressão
                TipoToken tipo_dir = expressao(s, escopo);
                tipo_final = converter_tipos(s, tipo_final, tipo_dir);
                
                if(op == T_TAMBEM_TAMBEM) {
                    // E: ambas devem ser verdadeiras
                    fprintf(s, "  cmp w0, 0\n");
                    fprintf(s, "  beq .B%d\n", rotulo_curto);
                    fprintf(s, "  mov w0, 1\n");
                    fprintf(s, "  b .B%d\n", rotulo_fim);
                    fprintf(s, ".B%d:\n", rotulo_curto);
                    fprintf(s, "  mov w0, 0\n");
                } else { // T_OU_OU
                    // OU: pelo menos uma deve ser verdadeira
                    fprintf(s, "  cmp w0, 1\n");
                    fprintf(s, "  beq .B%d\n", rotulo_fim);
                    fprintf(s, ".B%d:\n", rotulo_curto);
                    fprintf(s, "  mov w0, 1\n");
                    fprintf(s, "  b .B%d\n", rotulo_fim);
                }
                fprintf(s, ".B%d:\n", rotulo_fim);
                tipo_final = T_pBOOL;
            } else {
                break; // não é operador lógico, termina
            }
        } else {
            // primeira expressão = avalia normalmente
            if(tipo_atual != T_pINT && tipo_atual != T_pBOOL) {
                fatal("[processar_condicao] condição deve ser inteiro ou booleano");
            }
            tipo_final = tipo_atual;
            primeiro = 0;
            
            // se a operador logico apos a primeira expressão, prepara o caso
            if(L.tk.tipo == T_TAMBEM_TAMBEM || L.tk.tipo == T_OU_OU) {
                TipoToken op = L.tk.tipo;
                proximoToken(); // consome && ou ||
                
                int rotulo_curto = escopo_global++;
                int rotulo_final = escopo_global++;
                
                if(op == T_TAMBEM_TAMBEM) {
                    // E: verifica primeira condição
                    fprintf(s, "  cmp w0, 0\n");
                    fprintf(s, "  beq .B%d\n", rotulo_curto);
                } else { // T_OU_OU
                    // OU: verifica primeira condição  
                    fprintf(s, "  cmp w0, 1\n");
                    fprintf(s, "  beq .B%d\n", rotulo_final);
                }
                // processa segunda expressão
                TipoToken tipo_dir = expressao(s, escopo);
                if(tipo_dir != T_pINT && tipo_dir != T_pBOOL) {
                    fatal("[processar_condicao] condição deve ser inteiro ou booleano");
                }
                if(op == T_TAMBEM_TAMBEM) {
                    // E: verifica segunda condição
                    fprintf(s, "  cmp w0, 0\n");
                    fprintf(s, "  beq .B%d\n", rotulo_curto);
                    fprintf(s, "  mov w0, 1\n");
                    fprintf(s, "  b .B%d\n", rotulo_final);
                    fprintf(s, ".B%d:\n", rotulo_curto);
                    fprintf(s, "  mov w0, 0\n");
                } else { // T_OU_OU
                    // OU: verifica segunda condição
                    fprintf(s, "  cmp w0, 1\n");
                    fprintf(s, "  beq .B%d\n", rotulo_final);
                    fprintf(s, ".B%d:\n", rotulo_curto);
                    fprintf(s, "  mov w0, 0\n");
                    fprintf(s, "  b .B%d\n", rotulo_final);
                    fprintf(s, "  mov w0, 1\n");
                }
                fprintf(s, ".B%d:\n", rotulo_final);
                tipo_final = T_pBOOL;
                rotulo_fim = rotulo_final;
            }
        }
    }
    return tipo_final;
}

int processar_variaveis_tam(int escopo) {
    Lexer salvo = L;
    int tam_total = 0;
    int nivel_chaves = 0;
    
    if(L.tk.tipo == T_CHAVE_ESQ) {
        proximoToken();
        nivel_chaves = 1;
    } else {
        nivel_chaves = 1; 
    }
    while(L.tk.tipo != T_FIM) {
        if(L.tk.tipo == T_CHAVE_ESQ) {
            nivel_chaves++;
            proximoToken();
            continue;
        }
        if(L.tk.tipo == T_CHAVE_DIR) {
            nivel_chaves--;
            if(nivel_chaves == 0) break;
            proximoToken();
            continue;
        }
        TipoToken tipos[] = {T_pCAR, T_pINT, T_pFLU, T_pBOOL, T_pBYTE, T_pDOBRO, T_pLONGO, T_PONTEIRO};
        int eh_tipo = 0;
        for(int i = 0; i < 7; i++) {
            if(L.tk.tipo == tipos[i]) {
                eh_tipo = 1;
                break;
            }
        }
        if(eh_tipo) {
            TipoToken tipo_base = L.tk.tipo;
            proximoToken();

            int eh_ponteiro = 0;
            if(L.tk.tipo == T_VEZES) {
                eh_ponteiro = 1;
                proximoToken();
            }
            int total_elementos = 1;
            while(L.tk.tipo == T_COL_ESQ) {
                proximoToken();
                if(L.tk.tipo == T_INT) {
                    total_elementos *= L.tk.valor_l;
                    proximoToken();
                } else if(L.tk.tipo == T_ID) {
                    Variavel* var = buscar_var(L.tk.lex, escopo);
                    if(var) {
                        total_elementos *= var->valor;
                        proximoToken();
                    } else {
                        Macro* m = buscar_macro(L.tk.lex);
                        if(m) {
                            total_elementos *= m->valor;
                            proximoToken();
                        } else {
                            while(L.tk.tipo != T_COL_DIR && L.tk.tipo != T_FIM) proximoToken();
                        }
                    }
                } else {
                    // pula expressão de tamanho desconhecido na pré-analise
                    while(L.tk.tipo != T_COL_DIR && L.tk.tipo != T_FIM) proximoToken();
                }
                if(L.tk.tipo == T_COL_DIR) proximoToken();
            }
            if(L.tk.tipo == T_ID) {
                int tam_base = eh_ponteiro ? 8 : tam_tipo(tipo_base);
                int tam_bytes = total_elementos * tam_base;
                // alinhamento
                tam_total += (tam_bytes + 15) & ~15;
                // pula resto da declaração até ; ou }
                while(L.tk.tipo != T_PONTO_VIRGULA && L.tk.tipo != T_FIM && L.tk.tipo != T_CHAVE_DIR) {
                     if(L.tk.tipo == T_CHAVE_ESQ) {
                        int sub_nivel = 1;
                        proximoToken();
                        while (sub_nivel > 0 && L.tk.tipo != T_FIM) {
                            if(L.tk.tipo == T_CHAVE_ESQ) sub_nivel++;
                            else if(L.tk.tipo == T_CHAVE_DIR) sub_nivel--;
                            proximoToken();
                        }
                     } else proximoToken();
                }
            }
        } else proximoToken();
    }
    L = salvo;
    return tam_total;
}
// [VERIFICAÇÃO]:
void verificar_global(FILE* s) {
    excessao(T_GLOBAL);
    if(eh_tipo(L.tk.tipo)) {
        // verifica se é função: tem parenteses apos o nome
        char nome[32];
        strcpy(nome, L.tk.lex);
        // processa declaração
        int pos = 0;
        declaracao_var(s, &pos, -1, 0, 0, 1);
        if(L.tk.tipo == T_PONTO_VIRGULA) excessao(T_PONTO_VIRGULA);
    } else {
        if(L.tk.tipo != T_ID) fatal("[verificar_global] nome de função esperado");
        char fnome[32];
        strcpy(fnome, L.tk.lex);
        proximoToken();
        excessao(T_PAREN_ESQ);
        excessao(T_PAREN_DIR);
        if(L.tk.tipo == T_PONTO_VIRGULA) excessao(T_PONTO_VIRGULA);
        
        fprintf(s, ".global %s\n", fnome);
        for(int i = 0; i < fn_cnt; i++) {
            if(strcmp(funcs[i].nome, fnome) == 0) {
                funcs[i].eh_global = 1;
                break;
            }
        }
    }
}

void verificar_def(void) {
    excessao(T_DEF); // consome #def
    
    if(L.tk.tipo != T_ID) fatal("nome do macro esperado");
    
    char nome_macro[32];
    strcpy(nome_macro, L.tk.lex);
    proximoToken(); // consome o nome do macro
    
    // suporta inteiros ou longos
    if(L.tk.tipo != T_INT && L.tk.tipo != T_LONGO) fatal("valor inteiro ou longo esperado para o macro");
    
    long valor = L.tk.valor_l;
    proximoToken(); // consome o valor
    
    excessao(T_PONTO_VIRGULA); // consome o ;
    
    if(macro_cnt >= MAX_MACROS) fatal("[verificar_def] excesso de macros");
    
    Macro* m = &macros[macro_cnt++];
    strcpy(m->nome, nome_macro);
    m->valor = valor;
}

void verificar_espaco(FILE* s) {
    excessao(T_ESPACO);
    
    if(L.tk.tipo != T_ID) fatal("nome do espaço esperado");
    
    char nome_espaco[32];
    strcpy(nome_espaco, L.tk.lex);
    proximoToken();
    
    excessao(T_CHAVE_ESQ);
    // ignora tudo dentro do espaço
    while(L.tk.tipo != T_CHAVE_DIR && L.tk.tipo != T_FIM) {
        proximoToken();
    }
    excessao(T_CHAVE_DIR);
}

void verificar_retorno(FILE* s, int escopo) {
    excessao(T_RETORNAR);
    if(L.tk.tipo == T_PONTO_VIRGULA) {
        fprintf(s, "  b 1f\n");
        excessao(T_PONTO_VIRGULA);
        return;
    }
    TipoToken tipo_exp = expressao(s, escopo);
    // em funções que retornam ponteiro/array espera T_PONTEIRO
    if(funcs[fn_cnt - 1].retorno == T_PONTEIRO) {
        if(tipo_exp != T_PONTEIRO) fatal("[verificar_retorno] retorno deve ser ponteiro ou endereço");
    } else if(!tipos_compativeis(funcs[fn_cnt - 1].retorno, tipo_exp)) {
        fatal("[verificar_retorno] tipo de retorno incompatível");
    }
    fprintf(s, "  b 1f\n");
    excessao(T_PONTO_VIRGULA);
}

void verificar_atribuicao(FILE* s, const char* id, int escopo) {
    if(id[0] == '@') {
        char var_nome[32];
        strcpy(var_nome, id + 1); // remove o @
        
        Variavel* var = buscar_var(var_nome, escopo);
        if(!var || !var->eh_ponteiro) {
            fatal("[verificar_atribuicao] @ só pode ser usado com ponteiros");
        }
        
        excessao(T_IGUAL);
        TipoToken tipo_exp = expressao(s, escopo);
        
        if(tipo_exp != T_pINT && tipo_exp != T_pLONGO) {
            fatal("[verificar_atribuicao] endereço deve ser inteiro ou longo");
        }
        // atribui o endereço diretamente no ponteiro
        fprintf(s, "  str x0, [x29, %d]\n", var->pos);
        return;
    }
    Variavel* var = buscar_var(id, escopo);
    if(!var) fatal("[verificar_atribuicao] variável não declarada");
    
    if(var->eh_final) fatal("[verificar_atribuicao] não é possível alterar uma variável final.");
    
    if(var->eh_array) fatal("[verificar_atribuicao] não é possível armazenar valor direto em array");
    
    excessao(T_IGUAL);
    
    TipoToken tipo_exp = expressao(s, escopo);
    
    if(var->eh_ponteiro) {
        // se a expressão da direita tambem é ponteiro = atribuição de ponteiro
        if(tipo_exp == T_PONTEIRO) {
            // atribuição de ponteiro: p = outro_ponteiro
            fprintf(s, "  str x0, [x29, %d]\n", var->pos);
        } else {
            // atribuição deferenciada: p = valor(armazena no apontado)
            fprintf(s, "  ldr x1, [x29, %d]\n", var->pos);
            
            if(var->tipo_base == T_pCAR || var->tipo_base == T_pBOOL) {
                fprintf(s, "  strb w0, [x1]\n");
            } else if(var->tipo_base == T_pINT) {
                fprintf(s, "  str w0, [x1]\n");
            } else if(var->tipo_base == T_pFLU) {
                fprintf(s, "  str s0, [x1]\n");
            } else if(var->tipo_base == T_pDOBRO) {
                fprintf(s, "  str d0, [x1]\n");
            } else if(var->tipo_base == T_pLONGO) {
                fprintf(s, "  str x0, [x1]\n");
            }
        }
    } else {
        // variavel normal
        armazenar_valor(s, var);
    }
}

void verificar_matriz(FILE* s, Variavel* var, int escopo, int indices[], int nivel) {
    int elemento_idc = 0;
    
    while(L.tk.tipo != T_CHAVE_DIR && L.tk.tipo != T_FIM) {
        if(nivel < var->num_dims - 1) {
            // sub-matriz(recursão)
            excessao(T_CHAVE_ESQ);
            indices[nivel] = elemento_idc;
            verificar_matriz(s, var, escopo, indices, nivel + 1);
            excessao(T_CHAVE_DIR);
        } else {
            // elemento final
            indices[nivel] = elemento_idc;
            int pos_bytes = calcular_pos_matriz(var, indices);
            // gera o codigo para calcular o valor(mov w0, 1)
            TipoToken tipo_valor = expressao(s, escopo);
            
            if(tipo_valor != var->tipo_base) {
                fatal("[verificar_matriz] tipo incompatível na inicialização do array");
            }
            // calcula a posição real na pilha:
            // var->pos é negativo(base do array) pos_bytes é positivo(indice)
            // resultado continua negativo(dentro do frame)
            int pos_absoluta = var->pos + pos_bytes;
            // agora tudo é negativo
            if(var->tipo_base == T_pCAR || var->tipo_base == T_pBOOL || var->tipo_base == T_pBYTE) {
                fprintf(s, "  strb w0, [x29, %d]\n", pos_absoluta);
            } else if(var->tipo_base == T_pINT) {
                fprintf(s, "  str w0, [x29, %d]\n", pos_absoluta);
            } else if(var->tipo_base == T_pFLU) {
                fprintf(s, "  str s0, [x29, %d]\n", pos_absoluta);
            } else if(var->tipo_base == T_pDOBRO) {
                fprintf(s, "  str d0, [x29, %d]\n", pos_absoluta);
            } else if(var->tipo_base == T_pLONGO || var->tipo_base == T_PONTEIRO) {
                fprintf(s, "  str x0, [x29, %d]\n", pos_absoluta);
            }
        }
        elemento_idc++;
        if(L.tk.tipo == T_VIRGULA) proximoToken();
        else break;
    }
}

void verificar_se(FILE* s, int escopo) {
    excessao(T_SE);
    excessao(T_PAREN_ESQ);
    
    TipoToken tipo_cond = processar_condicao(s, escopo);
    
    if(tipo_cond != T_pINT && tipo_cond != T_pBOOL) {
        fatal("[verificar_se] condição deve ser inteiro ou booleano");
    }
    excessao(T_PAREN_DIR);
    
    int rotulo_falso = escopo_global++;
    fprintf(s, "  cmp w0, 0\n");
    fprintf(s, "  beq .B%d\n", rotulo_falso);
    
    if(L.tk.tipo == T_CHAVE_ESQ) {
        proximoToken();
        while(L.tk.tipo != T_CHAVE_DIR) verificar_stmt(s, &funcs[fn_cnt - 1].frame_tam, escopo + 1);
        excessao(T_CHAVE_DIR);
    } else {
        verificar_stmt(s, &funcs[fn_cnt - 1].frame_tam, escopo + 1);
    }
    int rotulo_fim = escopo_global++;
    fprintf(s, "  b .B%d\n", rotulo_fim);
    fprintf(s, ".B%d:\n", rotulo_falso);
    
    if(L.tk.tipo == T_SENAO) {
        proximoToken();
        if(L.tk.tipo == T_CHAVE_ESQ) {
            proximoToken();
            while(L.tk.tipo != T_CHAVE_DIR) verificar_stmt(s, &funcs[fn_cnt - 1].frame_tam, escopo + 1);
            excessao(T_CHAVE_DIR);
        } else {
            verificar_stmt(s, &funcs[fn_cnt - 1].frame_tam, escopo + 1);
        }
    }
    fprintf(s, ".B%d:\n", rotulo_fim);
}

void verificar_por(FILE* s, int escopo) {
    excessao(T_POR);
    excessao(T_PAREN_ESQ);
    
    int novo_escopo = ++escopo_global;
    
    // processa inicialização
    if(L.tk.tipo == T_pINT || L.tk.tipo == T_pLONGO || L.tk.tipo == T_pCAR || 
       L.tk.tipo == T_pFLU || L.tk.tipo == T_pBOOL || L.tk.tipo == T_pDOBRO) {
        declaracao_var(s, &funcs[fn_cnt - 1].frame_tam, novo_escopo, 0, 0, 0);
    } else if(L.tk.tipo == T_ID) {
        char id[32];
        strcpy(id, L.tk.lex);
        proximoToken();
        if(L.tk.tipo == T_IGUAL) {
            verificar_atribuicao(s, id, escopo);
        }
    }
    // consome ponto e virgula apos iniciar
    excessao(T_PONTO_VIRGULA);
    
    int rotulo_inicio = escopo_global++;
    int rotulo_fim = escopo_global++;
    
    fprintf(s, ".B%d:\n", rotulo_inicio);
    
    // processa condição
    if(L.tk.tipo != T_PONTO_VIRGULA) {
        TipoToken tipo_cond = expressao(s, novo_escopo);
        
        if(tipo_cond != T_pINT && tipo_cond != T_pBOOL) {
            // Tenta converter para booleano
            if(tipo_cond == T_pFLU) {
                fprintf(s, "  fcmp s0, #0.0\n");
                fprintf(s, "  cset w0, ne\n");
            } else if(tipo_cond == T_pDOBRO) {
                fprintf(s, "  fcmp d0, #0.0\n");
                fprintf(s, "  cset w0, ne\n");
            } else {
                fatal("[verificar_por] condição do loop deve ser inteiro ou booleano");
            }
        }
        fprintf(s, "  cmp w0, 0\n");
        fprintf(s, "  beq .B%d\n", rotulo_fim);
    }
    // consome ponto e virgula apos condição
    excessao(T_PONTO_VIRGULA);
    
    // salva posição do incremento pra processar depois do corpo
    size_t pos_incremento = L.pos;
    int linha_incremento = L.linha_atual;
    int coluna_incremento = L.coluna_atual;
    Token tk_incremento = L.tk;
    
    // pula incremento por agora(ate encontrar PAREN_DIR)
    while(L.tk.tipo != T_PAREN_DIR && L.tk.tipo != T_FIM) {
        proximoToken();
    }
    excessao(T_PAREN_DIR);
    
    // processa corpo do loop
    if(L.tk.tipo == T_CHAVE_ESQ) {
        proximoToken();
        while(L.tk.tipo != T_CHAVE_DIR) {
            verificar_stmt(s, &funcs[fn_cnt-1].frame_tam, novo_escopo);
        }
        excessao(T_CHAVE_DIR);
    } else {
        verificar_stmt(s, &funcs[fn_cnt-1].frame_tam, novo_escopo);
    }
    // processa incremento depois do corpo
    size_t pos_atual = L.pos;
    int linha_atual = L.linha_atual;
    int coluna_atual = L.coluna_atual;
    Token tk_atual = L.tk;
    
    // restaura para processar o incremento
    L.pos = pos_incremento;
    L.linha_atual = linha_incremento;
    L.coluna_atual = coluna_incremento;
    L.tk = tk_incremento;
    // processa o incremento como um statement completo
    if(L.tk.tipo != T_PAREN_DIR) {
        // usa verificar_stmt pra processar o incremento
        verificar_stmt(s, &funcs[fn_cnt-1].frame_tam, novo_escopo);
    }
    // restaura posição atual
    L.pos = pos_atual;
    L.linha_atual = linha_atual;
    L.coluna_atual = coluna_atual;
    L.tk = tk_atual;
    
    fprintf(s, "  b .B%d\n", rotulo_inicio);
    fprintf(s, ".B%d:\n", rotulo_fim);
}

void verificar_enq(FILE* s, int escopo) {
    excessao(T_ENQ);
    excessao(T_PAREN_ESQ);
    
    int rotulo_inicio = escopo_global++;
    int rotulo_fim = escopo_global++;
    
    fprintf(s, ".B%d:\n", rotulo_inicio);
    TipoToken tipo_cond = expressao(s, escopo);
    
    if(tipo_cond != T_pINT && tipo_cond != T_pBOOL) fatal("[verificar_enq] condição do loop deve ser inteiro ou booleano");
    
    fprintf(s, "  cmp w0, 0\n");
    fprintf(s, "  beq .B%d\n", rotulo_fim);
    
    excessao(T_PAREN_DIR);
    
    if(L.tk.tipo == T_CHAVE_ESQ) {
        proximoToken();
        while(L.tk.tipo != T_CHAVE_DIR) verificar_stmt(s, &funcs[fn_cnt-1].frame_tam, escopo + 1);
        excessao(T_CHAVE_DIR);
    } else verificar_stmt(s, &funcs[fn_cnt-1].frame_tam, escopo + 1);
    
    fprintf(s, "  b .B%d\n", rotulo_inicio);
    fprintf(s, ".B%d:\n", rotulo_fim);
}

void verificar_stmt(FILE* s, int* pos, int escopo) {
    if(escopo == 0) escopo = escopo_global;
    
    while(L.tk.tipo == T_COMENTARIO) proximoToken();
    int eh_final = 0;
    if(L.tk.tipo == T_FINAL) {
        eh_final = 1;
        proximoToken();
    }
    // na parte que trata @id = expressão:
    if(L.tk.tipo == T_ARROBA) {
        proximoToken(); // consome @
        
        if(L.tk.tipo != T_ID) fatal("[verificar_stmt] identificador esperado após @");
        char id[32];
        strcpy(id, L.tk.lex);
        proximoToken(); // consome o id
        excessao(T_IGUAL);
        Variavel* var = buscar_var(id, escopo);
        
        if(!var || !var->eh_ponteiro) {
            fatal("[verificar_stmt] @ só pode ser usado com ponteiros");
        }
        TipoToken tipo_exp = expressao(s, escopo);
        // conversão baseada no tipo
        if(tipo_exp == T_pBYTE) {
            // byte -> ponteiro: estende sem sinal paa 64 bits
            fprintf(s, "  and w0, w0, 0xFF\n"); // Garante que é byte
            fprintf(s, "  uxtw x0, w0\n"); // estende sem sinal pra 64 bits
        } else if(tipo_exp == T_pINT) {
            // int -> pnteiro: estende sem sinal pra 64 bits  
            fprintf(s, "  uxtw x0, w0\n"); // Estende sem sinal para 64 bits
        } else if(tipo_exp == T_pLONGO) {
            // longo -> ponteiro: ja é 64 bits, não precisa conversão
        } else {
            fatal("[verificar_stmt] endereço deve ser byte, inteiro ou longo");
        }
        fprintf(s, "  str x0, [x29, %d]\n", var->pos);
        excessao(T_PONTO_VIRGULA);
        return;
    }
    if(L.tk.tipo == T_SE) {
        verificar_se(s, escopo);
        return;
    }
    if(L.tk.tipo == T_POR) {
        verificar_por(s, escopo);
        return;
    }
    if(L.tk.tipo == T_ENQ) {
        verificar_enq(s, escopo);
        return;
    }
    if(L.tk.tipo == T_INCLUIR) {
        proximoToken();
        
        if(L.tk.tipo != T_TEX) fatal("[verificar_stmt] caminho do arquivo esperado entre aspas");
        
        char caminho[256];
        strcpy(caminho, L.tk.lex);
        proximoToken();
        
        if(L.tk.tipo == T_PONTO_VIRGULA) excessao(T_PONTO_VIRGULA);
        
        FILE* arquivo_incluir = NULL;
        
        // tenta primeiro com o caminho relativo ao FPB_DIR
        char* base_dir = processar_caminho();
        if(strlen(base_dir) > 0) {
            char caminho_completo[512];
            snprintf(caminho_completo, sizeof(caminho_completo), 
                    "%s/%s", base_dir, caminho);
            arquivo_incluir = fopen(caminho_completo, "r");
        }
        // se não encontrou, tenta no diretorio atual
        if(!arquivo_incluir) {
            arquivo_incluir = fopen(caminho, "r");
        }
        if(!arquivo_incluir) {
            char mensagem_erro[300];
            snprintf(mensagem_erro, sizeof(mensagem_erro), 
                    "[verificar_stmt] não foi possível abrir: %s (FPB_DIR=%s)", 
                    caminho, processar_caminho());
            fatal(mensagem_erro);
        }
        fprintf(s, "\n// inicio de %s\n", caminho);
        char linha[512];
        while(fgets(linha, sizeof(linha), arquivo_incluir)) {
            if(strstr(linha, ".section .data") != NULL) {
                fputs(linha, s);
                fputs("  .align 2\n", s);
            } else if(strstr(linha, ": .asciz") != NULL) {
                fputs("  .align 2\n", s);
                fputs(linha, s);
            } else fputs(linha, s);
        }
        fprintf(s, "\n// fim de %s\n\n", caminho);
        fclose(arquivo_incluir);
        return;
    }
    if(L.tk.tipo == T_GLOBAL) {
        proximoToken(); // consome "global"
        
        // deve ser seguido por declaração de função
        if(L.tk.tipo != T_pVAZIO && 
           L.tk.tipo != T_pINT && 
           L.tk.tipo != T_pFLU && 
           L.tk.tipo != T_pCAR && 
           L.tk.tipo != T_pBOOL && 
           L.tk.tipo != T_pDOBRO && 
           L.tk.tipo != T_pLONGO && 
           L.tk.tipo != T_PONTEIRO) {
            fatal("[verificar_stmt] tipo de retorno esperado após 'global'");
        }
        // processa a função normalmente, mas marca como global
        verificar_fn(s);
        return;
    }
    if(L.tk.tipo == T_RETORNAR) {
        verificar_retorno(s, escopo);
        return;
    }
    TipoToken tipos[] = {T_pBYTE, T_pCAR, T_pINT, T_pFLU, T_pBOOL, T_pDOBRO, T_pLONGO, T_PONTEIRO};
    int eh_tipo = 0;
    for(int i=0; i<8; i++) {
        if(L.tk.tipo == tipos[i]) {
            eh_tipo = 1;
            break;
        }
    }
    if(eh_tipo || eh_final) {
        declaracao_var(s, pos, escopo, 0, eh_final, 0);
        if(L.tk.tipo == T_PONTO_VIRGULA) excessao(T_PONTO_VIRGULA);
        return;
    }
    if(L.tk.tipo == T_ID) {
        char idn[32];
        strcpy(idn, L.tk.lex);
        proximoToken();
        
        if(L.tk.tipo == T_IGUAL) {
            verificar_atribuicao(s, idn, escopo);
            if(L.tk.tipo == T_PONTO_VIRGULA) excessao(T_PONTO_VIRGULA);
            return;
        } else if(L.tk.tipo == T_MAIS_MAIS || L.tk.tipo == T_MENOS_MENOS) {
            Variavel* var = buscar_var(idn, escopo);
            if(!var) fatal("[verificar_stmt] variável não declarada");
            if(var->eh_final) fatal("[verificar_stmt] não é possível modificar variável final");
            
            TipoToken op = L.tk.tipo;
            proximoToken();
            // carrega valor atual
            carregar_valor(s, var);
            // incrementa ou decrementa
            if(op == T_MAIS_MAIS) {
                if(var->tipo_base == T_pFLU) {
                    fprintf(s, "  fmov s1, 1.0\n");
                    fprintf(s, "  fadd s0, s0, s1\n");
                } else if(var->tipo_base == T_pDOBRO) {
                    fprintf(s, "  fmov d1, 1.0\n");
                    fprintf(s, "  fadd d0, d0, d1\n");
                } else if(var->tipo_base == T_pLONGO) {
                    fprintf(s, "  add x0, x0, 1\n");
                } else {
                    fprintf(s, "  add w0, w0, 1\n");
                }
            } else { // T_MENOS_MENOS
                if(var->tipo_base == T_pFLU) {
                    fprintf(s, "  fmov s1, 1.0\n");
                    fprintf(s, "  fsub s0, s0, s1\n");
                } else if(var->tipo_base == T_pDOBRO) {
                    fprintf(s, "  fmov d1, 1.0\n");
                    fprintf(s, "  fsub d0, d0, d1\n");
                } else if(var->tipo_base == T_pLONGO) {
                    fprintf(s, "  sub x0, x0, 1\n");
                } else {
                    fprintf(s, "  sub w0, w0, 1\n");
                }
            }
            // armazena de volta
            armazenar_valor(s, var);
            
            if(L.tk.tipo == T_PONTO_VIRGULA) excessao(T_PONTO_VIRGULA);
            return;
        } else if(L.tk.tipo == T_COL_ESQ) {
            // >>>>>>ACESSO A ELEMENTO DE ARRAY<<<<<<
            Variavel* var = buscar_var(idn, escopo);
            if(!var || !var->eh_array) fatal("[verificar_stmt] não é um array");
            excessao(T_COL_ESQ);
            expressao(s, escopo); // indice(resultado em w0)
            fprintf(s, "  mov w1, w0\n"); // salva o indice em w1
            // salva o indice na pilha antes de calcular o valor
            fprintf(s, "  str w1, [sp, -16]!\n");
            excessao(T_COL_DIR);
            
            if(L.tk.tipo == T_IGUAL) {
                // atribuição a item de array
                proximoToken(); // consome '='
                TipoToken tipo_valor = expressao(s, escopo); // valor(resultado em w0)
                // restaura o indice da pilha
                fprintf(s, "  ldr w1, [sp], 16\n");
                if(!tipos_compativeis(var->tipo_base, tipo_valor)) {
                    char msg[100];
                    sprintf(msg, "[verificar_stmt] tipo incompatível: esperado %s, encontrado %s", 
                    token_str(var->tipo_base), token_str(tipo_valor));
                    fatal(msg);
                }
                // se for parametro, carrega o endereço base da pilha(é um ponteiro)
                if(var->escopo == -1) {
                    fprintf(s, "  ldr x2, = global_%s\n", var->nome);
                } else {
                    if(var->eh_parametro) fprintf(s, "  ldr x2, [x29, %d]\n", var->pos); // carrega o endereço base do array
                    else fprintf(s, "  add x2, x29, %d\n", var->pos); // endereço base do array(local)
                }
                // calcula o pos: indice * tam_elemento
                int tam_elemento = tam_tipo(var->tipo_base);
                if(tam_elemento == 1) {
                    // Para bytes, soma direta
                    fprintf(s, "  add x2, x2, x1\n");
                } else if(tam_elemento == 4) {
                    // pra inteiros(4 bytes), indice * 4
                    fprintf(s, "  add x2, x2, x1, lsl 2\n");
                } else if(tam_elemento == 8) {
                    // pra longos(8 bytes), índice * 8  
                    fprintf(s, "  add x2, x2, x1, lsl 3\n");
                }
                // armazena o valor no item do array
                if(var->tipo_base == T_pCAR || var->tipo_base == T_pBOOL || var->tipo_base == T_pBYTE) fprintf(s, "  strb w0, [x2]\n");
                else if(var->tipo_base == T_pINT) fprintf(s, "  str w0, [x2]\n");  // usa str para inteiros de 4 bytes
                else if(var->tipo_base == T_pFLU) fprintf(s, "  str s0, [x2]\n");
                else if(var->tipo_base == T_pDOBRO) fprintf(s, "  str d0, [x2]\n");
                else if(var->tipo_base == T_pLONGO) fprintf(s, "  str x0, [x2]\n");  // usa str para inteiros de 4 bytes
            } else {
                fatal("[verificar_stmt] acesso a elemento de array deve ser usado em expressão ou atribuição");
            }
            if(L.tk.tipo == T_PONTO_VIRGULA) excessao(T_PONTO_VIRGULA);
            return;
        } else if(L.tk.tipo == T_PAREN_ESQ) {
            excessao(T_PAREN_ESQ);
            if(strcmp(idn,"escrever") == 0) {
                while(1) {
                    if(L.tk.tipo == T_ID) {
                        Variavel* var = buscar_var(L.tk.lex, escopo);
                        if(var && var->eh_ponteiro) {
                            if(var->tipo_base == T_pCAR) {
                                if(var->escopo == -1) { // global    
                                    fprintf(s, "  ldr x0, = global_%s\n", var->nome);    
                                    fprintf(s, "  ldr x0, [x0]\n"); // carrega o ponteiro    
                                } else { // local    
                                    fprintf(s, "  ldr x0, [x29, %d]\n", var->pos);
                                }    
                                escrever_valor(s, T_TEX); // escrever como texto
                            } else {
                                fprintf(s, "  ldr x1, [x29, %d]\n", var->pos);
                                // carrega o valor apontado baseado no tipo base
                                if(var->tipo_base == T_pBOOL) {
                                    fprintf(s, "  ldrb w0, [x1]\n");
                                } else if(var->tipo_base == T_pINT) {
                                    fprintf(s, "  ldr w0, [x1]\n");
                                } else if(var->tipo_base == T_pFLU) {
                                    fprintf(s, "  ldr s0, [x1]\n");
                                } else if(var->tipo_base == T_pDOBRO) {
                                    fprintf(s, "  ldr d0, [x1]\n");
                                } else if(var->tipo_base == T_pLONGO) {
                                    fprintf(s, "  ldr x0, [x1]\n");
                                }
                                escrever_valor(s, var->tipo_base);
                            }
                            proximoToken();
                        } else if(var && var->eh_array && var->tipo_base == T_pCAR) {
                            if(var->escopo == -1) {
                                fprintf(s, "  ldr x0, = global_%s\n", var->nome);
                            } else {
                                if(var->eh_parametro) fprintf(s, "  ldr x0, [x29, %d]\n", var->pos);
                                else fprintf(s, "  add x0, x29, %d\n", var->pos);
                            }
                            escrever_valor(s, T_TEX);
                            proximoToken();
                        } else {
                            TipoToken tipo_arg = expressao(s, escopo);
                            escrever_valor(s, tipo_arg);
                        }
                    } else {
                        TipoToken tipo_arg = expressao(s, escopo);
                        escrever_valor(s, tipo_arg);
                    }
                    if(L.tk.tipo == T_VIRGULA) {
                        proximoToken();
                        continue;
                    }
                    break;
                }
                excessao(T_PAREN_DIR);
                if(L.tk.tipo == T_PONTO_VIRGULA) excessao(T_PONTO_VIRGULA);
                return;
            } else {
                // >>>>>CHAMADA DE FUNÇÃO NORMAL<<<<<
                Funcao* fn = buscar_fn(idn);
                if(!fn) fatal("[verificar_stmt] função não declarada");
                tratar_chamada_funcao(s, escopo, idn, fn);
                if(L.tk.tipo == T_PONTO_VIRGULA) excessao(T_PONTO_VIRGULA);
                return;
            }
        } else fatal("[verificar_stmt] declaração inválida");
    }
    if(L.tk.tipo == T_CHAVE_ESQ) {
        proximoToken();
        int novo_escopo = ++escopo_global;
        
        while(L.tk.tipo != T_CHAVE_DIR) verificar_stmt(s, pos, novo_escopo);
        proximoToken();
        return;
    }
    fatal("[verificar_stmt] declaração inválida");
}

void verificar_fn(FILE* s) {
    TipoToken rt = L.tk.tipo;
    proximoToken();

    int eh_ponteiro = 0;
    int eh_array = 0;
    int eh_prototipo = 0;
    
    if(L.tk.tipo == T_VEZES) {
        eh_ponteiro = 1;
        proximoToken();
    } else if(L.tk.tipo == T_COL_ESQ) {
        eh_array = 1;
        proximoToken();
        excessao(T_COL_DIR);
    }
    if(L.tk.tipo != T_ID) fatal("[verificar_fn] nome de função esperado");

    char fnome[32];
    strcpy(fnome, L.tk.lex);
    TipoToken tipo_real = (eh_ponteiro || eh_array) ? T_PONTEIRO : rt;
    
    funcs[fn_cnt].var_conta = 0;
    funcs[fn_cnt].retorno = tipo_real;
    funcs[fn_cnt].escopo_atual = 0;
    funcs[fn_cnt].frame_tam = 0;
    funcs[fn_cnt].param_pos = 16;
    funcs[fn_cnt].eh_global = 0;
    strcpy(funcs[fn_cnt++].nome, fnome);
    proximoToken();

    excessao(T_PAREN_ESQ);
    processar_args(s, &funcs[fn_cnt - 1]);
    excessao(T_PAREN_DIR);

    if(L.tk.tipo == T_PONTO_VIRGULA) {
        eh_prototipo = 1;
        proximoToken();
        return; 
    } else excessao(T_CHAVE_ESQ);

    if(!eh_prototipo) {
        // chama a simulação real pra saber tamanho na pilha
        int tam_vars_locais = processar_variaveis_tam(escopo_global);
        int frame_tam = tam_vars_locais;
        // espaço pra registradores salvos (x19..x22)
        if(tipo_real != T_pVAZIO) frame_tam += 32;
        // espaço pra salvar parametros da função
        for(int i = 0; i < funcs[fn_cnt-1].var_conta; i++) {
            Variavel* var = &funcs[fn_cnt-1].vars[i];
            if(var->eh_parametro && var->reg[0] != '\0') frame_tam += 8;
        }
        // margem de segurança obrigatoria para expressões
        frame_tam += 128;
        // alinhamento 16 bytes
        frame_tam = (frame_tam + 15) & ~15;
        
        funcs[fn_cnt - 1].frame_tam = frame_tam;
        // >>>>>>PROLOGO<<<<<<
        fprintf(s, "// fn: [%s] (vars: %d, total: %d)\n", fnome, tam_vars_locais, frame_tam);
            
        fprintf(s, ".align 2\n");
        fprintf(s, "%s:\n", fnome);
        fprintf(s, "  sub sp, sp, %d\n", frame_tam);
        // salva FP/LR no topo do frame
        fprintf(s, "  stp x29, x30, [sp, %d]\n", frame_tam - 16); 
        fprintf(s, "  add x29, sp, %d\n", frame_tam - 16);
        
        if(tipo_real != T_pVAZIO) {
            fprintf(s, "  stp x19, x20, [x29, -16]\n");
            fprintf(s, "  stp x21, x22, [x29, -32]\n");
        }
        int param_pos = (tipo_real == T_pVAZIO) ? 16 : 48;
        for(int i = 0; i < funcs[fn_cnt - 1].var_conta; i++) {
            Variavel* var = &funcs[fn_cnt - 1].vars[i];
            if(var->eh_parametro && var->reg[0] != '\0') {
                fprintf(s, "  str %s, [x29, -%d]  // param %s\n", 
                        var->reg, param_pos, var->nome);
                var->pos = -param_pos;
                param_pos += 8;
            }
        }
        // define onde começam as variaveis locais(abaixo dos params)
        int inicio_vars = (param_pos + 15) & ~15;
        int pos = -inicio_vars; 
        // gera o corpo
        while(L.tk.tipo != T_CHAVE_DIR) verificar_stmt(s, &pos, 0);
        
        fprintf(s, "  b 1f\n");
        // >>>>>EPILOGO<<<<<
        fprintf(s, "// epilogo\n");
        fprintf(s, "1:\n");
        if(tipo_real != T_pVAZIO) {
            fprintf(s, "  ldp x19, x20, [x29, -16]\n");
            fprintf(s, "  ldp x21, x22, [x29, -32]\n");
        }
        fprintf(s, "  ldp x29, x30, [sp, %d]\n", frame_tam - 16);
        fprintf(s, "  add sp, sp, %d\n", frame_tam);
        if(strcmp(fnome, "inicio") == 0) {
            fprintf(s, "  mov x0, 0\n");
            fprintf(s, "  mov x8, 93\n");
            fprintf(s, "  svc 0\n");
        }
        fprintf(s, "  ret\n");
        fprintf(s, "// fim: [%s]\n", fnome);
        proximoToken(); 
    }
}
// [GERAÇÃO]:
void gerar_prelude(FILE* s) {
    fprintf(s,".section .text\n");
}

void gerar_texs(FILE* s) {
    if(tex_cnt == 0) return;
    fprintf(s, ".section .rodata\n");
    fprintf(s, ".align 2\n");
    for(int i = 0; i < tex_cnt; i++) {
        fprintf(s, "%s: .asciz \"%s\"\n", texs[i].nome, texs[i].valor);
    }
    fprintf(s, ".section .text\n\n");
}

void gerar_consts(FILE* s) {
    if(const_cnt == 0) return;
    
    fprintf(s, "  .section .rodata\n");
    fprintf(s, "  .align 8\n");
    for(int i = 0; i < const_cnt; i++) {
        fprintf(s, "const_%d:\n", i);
        if(constantes[i].tipo == T_INT) fprintf(s, "  .word %ld\n", constantes[i].l_val);
        else if(constantes[i].tipo == T_FLU) {
            float f = (float)constantes[i].d_val;
            fprintf(s, "  .float %f\n", f);
        } else if(constantes[i].tipo == T_DOBRO) fprintf(s, "  .double %f\n", constantes[i].d_val);
        else if(constantes[i].tipo == T_LONGO) fprintf(s, "  .quad %ld\n", constantes[i].l_val);
    }
}

void gerar_operacao(FILE* s, TipoToken op, TipoToken tipo) {
    switch(op) {
        case T_MAIS: 
            if(tipo == T_pFLU) fprintf(s, "  fadd s0, s1, s0\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fadd d0, d1, d0\n");
            else if(tipo == T_pLONGO) fprintf(s, "  add x0, x1, x0\n");
            else if(tipo == T_PONTEIRO) {
                fprintf(s, "  lsl x0, x0, 3\n"); // multiplica por 8 "<< 3"
                fprintf(s, "  add x0, x1, x0\n");
            } else fprintf(s, "  add w0, w1, w0\n");
        break;
        case T_MENOS: 
            if(tipo == T_pFLU) fprintf(s, "  fsub s0, s1, s0\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fsub d0, d1, d0\n");
            else if(tipo == T_pLONGO) fprintf(s, "  sub x0, x1, x0\n");
            else if(tipo == T_PONTEIRO) {
                fprintf(s, "  lsl x0, x0, 3\n");
                fprintf(s, "  sub x0, x1, x0\n");
            }
            else fprintf(s, "  sub w0, w1, w0\n");
        break;
        case T_VEZES: 
            if(tipo == T_pFLU) fprintf(s, "  fmul s0, s1, s0\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fmul d0, d1, d0\n");
            else if(tipo == T_pLONGO) fprintf(s, "  mul x0, x1, x0\n");
            else if(tipo == T_PONTEIRO) {
                fprintf(s, "  lsl x0, x0, 3\n");
                fprintf(s, "  mul x0, x1, x0\n");
            }
            else fprintf(s, "  mul w0, w1, w0\n");
        break;
        case T_DIV: 
            if(tipo == T_pFLU) fprintf(s, "  fdiv s0, s1, s0\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fdiv d0, d1, d0\n");
            else if(tipo == T_pLONGO) fprintf(s, "  sdiv x0, x1, x0\n");
            else if(tipo == T_PONTEIRO) {
                fprintf(s, "  lsl x0, x0, 3\n");
                fprintf(s, "  sdiv x0, x1, x0\n");
            }
            else fprintf(s, "  sdiv w0, w1, w0\n");
        break;
        case T_PORCEN: 
            if(tipo == T_pFLU || tipo == T_pDOBRO) {
                fatal("[gerar_operacao] operador módulo não suportado para tipos flutuante");
            } else if(tipo == T_pLONGO || tipo == T_PONTEIRO) {
                fprintf(s, "  sdiv x2, x1, x0\n");
                fprintf(s, "  msub x0, x2, x0, x1\n");
            } else {
                fprintf(s, "  sdiv w2, w1, w0\n");
                fprintf(s, "  msub w0, w2, w0, w1\n");
            }
        break;
        case T_TAMBEM_TAMBEM:
            if(tipo == T_pFLU || tipo == T_pDOBRO) {
                fatal("[gerar_operacao] operador && não suportado para tipos flutuante");
            } else if(tipo == T_pLONGO || tipo == T_PONTEIRO) {
                // &&: w0 = (w1 != 0) && (w0 != 0)
                fprintf(s, "  cmp x1, 0\n");
                fprintf(s, "  cset x1, ne\n"); // w1 = (w1 != 0) ? 1 : 0
                fprintf(s, "  cmp x0, 0\n");
                fprintf(s, "  cset x0, ne\n"); // w0 = (w0 != 0) ? 1 : 0
                fprintf(s, "  and x0, x1, x0\n"); // w0 = w1 & w0
            } else {
                // &&: w0 = (w1 != 0) && (w0 != 0)
                fprintf(s, "  cmp w1, 0\n");
                fprintf(s, "  cset w1, ne\n"); // w1 = (w1 != 0) ? 1 : 0
                fprintf(s, "  cmp w0, 0\n");
                fprintf(s, "  cset w0, ne\n"); // w0 = (w0 != 0) ? 1 : 0
                fprintf(s, "  and w0, w1, w0\n"); // w0 = w1 & w0
            }
        break;
        case T_MENOR_MENOR: 
            if(tipo == T_pFLU) fprintf(s, "  flsl s0, s1, s0\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  flsl d0, d1, d0\n");
            else if(tipo == T_pLONGO) fprintf(s, "  lsl x0, x1, x0\n");
            else if(tipo == T_PONTEIRO) {
                fprintf(s, "  lsl x0, x0, 3\n");
                fprintf(s, "  lsl x0, x1, x0\n");
            }
            else fprintf(s, "  lsl w0, w1, w0\n");
        break;
        case T_MAIOR_MAIOR: 
            if(tipo == T_pFLU) fprintf(s, "  flsr s0, s1, s0\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  flsr d0, d1, d0\n");
            else if(tipo == T_pLONGO) fprintf(s, "  lsr x0, x1, x0\n");
            else if(tipo == T_PONTEIRO) {
                fprintf(s, "  lsr x0, x0, 3\n");
                fprintf(s, "  lsr x0, x1, x0\n");
            }
            else fprintf(s, "  lsr w0, w1, w0\n");
        break;
        default: fatal("[gerar_operacao] operador inválido");
    }
}

void gerar_comparacao(FILE* s, TipoToken op, TipoToken tipo) {
    switch(op) {
        case T_IGUAL_IGUAL:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, eq\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, eq\n");
            else if(tipo == T_pLONGO) fprintf(s, "  cmp x1, x0\n  cset x0, eq\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, eq\n");
        break;
        case T_DIFERENTE:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, ne\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, ne\n");
            else if(tipo == T_pLONGO) fprintf(s, "  cmp x1, x0\n  cset x0, ne\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, ne\n");
        break;
        case T_MAIOR:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, gt\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, gt\n");
            else if(tipo == T_pLONGO) fprintf(s, "  fcmp x1, x0\n  cset x0, gt\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, gt\n");
        break;
        case T_MENOR:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, lt\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, lt\n");
            else if(tipo == T_pLONGO || tipo == T_PONTEIRO) fprintf(s, "  cmp x1, x0\n  cset x0, lt\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, lt\n");
        break;
        case T_MAIOR_IGUAL:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, ge\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, ge\n");
            else if(tipo == T_pLONGO) fprintf(s, "  cmp x1, x0\n  cset x0, ge\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, ge\n");
        break;
        case T_MENOR_IGUAL:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, le\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, le\n");
            else if(tipo == T_pLONGO) fprintf(s, "  cmp x1, x0\n  cset x0, le\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, le\n");
        break;
        default: fatal("[gerar_comparacao] operador de comparação inválido");
    }
}

TipoToken converter_tipos(FILE* s, TipoToken tipo_anterior, TipoToken tipo_atual) {
    if(tipo_anterior == tipo_atual) return tipo_anterior;
    if(tipo_anterior == T_PONTEIRO && (tipo_atual == T_pINT || tipo_atual == T_pLONGO)) {
        return T_PONTEIRO;
    }
    if(tipo_atual == T_PONTEIRO && (tipo_anterior == T_pINT || tipo_anterior == T_pLONGO)) {
        return T_PONTEIRO;
    }
    // caso 1: inteiro(esq) com flutuante(dir)
    // o inteiro ta em w1(anterior), o flutuante ta em s0(atual)
    if(tipo_anterior == T_pINT && tipo_atual == T_pFLU) {
        fprintf(s, "  scvtf s1, w1\n"); // converte w1 pra s1
        return T_pFLU;
    }
    // caso 2: flutuante(esq) com inteiro(dir)
    // O flutuante ta em s1(anterior), o inteiro ta em w0(atual)
    else if(tipo_anterior == T_pFLU && tipo_atual == T_pINT) {
        fprintf(s, "  scvtf s0, w0\n"); // converte w0 pra s0
        return T_pFLU;
    }
    // caso 3: inteiro e dobro
    else if(tipo_anterior == T_pINT && tipo_atual == T_pDOBRO) {
        fprintf(s, "  scvtf d1, w1\n"); // int w1 -> dobro d1
        return T_pDOBRO;
    } else if(tipo_anterior == T_pDOBRO && tipo_atual == T_pINT) {
        fprintf(s, "  scvtf d0, w0\n"); // int w0 -> dobro d0
        return T_pDOBRO;
    }
    // caso 4: flutuante e dobro
    else if(tipo_anterior == T_pFLU && tipo_atual == T_pDOBRO) {
        fprintf(s, "  fcvt d1, s1\n"); // flu s1 -> dobro d1
        return T_pDOBRO;
    } else if(tipo_anterior == T_pDOBRO && tipo_atual == T_pFLU) {
        fprintf(s, "  fcvt d0, s0\n"); // flu s0 -> dobro d0(promoção)
        return T_pDOBRO;
    }
    // caso 5: longo e flutuante/dobro
    else if(tipo_anterior == T_pLONGO && tipo_atual == T_pFLU) {
        fprintf(s, "  scvtf s1, x1\n");
        return T_pFLU;
    } else if(tipo_anterior == T_pFLU && tipo_atual == T_pLONGO) {
        fprintf(s, "  scvtf s0, x0\n");
        return T_pFLU;
    } else if(tipo_anterior == T_pLONGO && tipo_atual == T_pDOBRO) {
        fprintf(s, "  scvtf d1, x1\n");
        return T_pDOBRO;
    } else if(tipo_anterior == T_pDOBRO && tipo_atual == T_pLONGO) {
        fprintf(s, "  scvtf d0, x0\n");
        return T_pDOBRO;
    }
    // retorno padrão(mantem o maior tipo se não tiver conversão especifica)
    return (tam_tipo(tipo_atual) > tam_tipo(tipo_anterior)) ? tipo_atual : tipo_anterior;
}

void gerar_convert(FILE* s, TipoToken tipo_origem, TipoToken tipo_destino) {
    if(tipo_origem == tipo_destino) return; // desnecessario né meu, complicado meu
    // vonversões numericas
    if(tipo_destino == T_pINT) {
        if(tipo_origem == T_pFLU) fprintf(s, "  fcvtzs w0, s0\n");
        else if(tipo_origem == T_pDOBRO) fprintf(s, "  fcvtzs w0, d0\n");
        else if(tipo_origem == T_pLONGO) fprintf(s, "  mov w0, w0\n"); // Trunca
        else if(tipo_origem == T_pCAR) fprintf(s, "  sxtb w0, w0\n");
        else if(tipo_origem == T_pBOOL) fprintf(s, "  and w0, w0, 1\n");
    } else if(tipo_destino == T_pBYTE) {
        if(tipo_origem == T_pINT || tipo_origem == T_pLONGO) {
            fprintf(s, "  and w0, w0, 0xFF  // truncar para byte\n");
        } else if(tipo_origem == T_pFLU) {
            fprintf(s, "  fcvtzs w0, s0\n");
            fprintf(s, "  and w0, w0, 0xFF\n");
        } else if(tipo_origem == T_pDOBRO) {
            fprintf(s, "  fcvtzs w0, d0\n");
            fprintf(s, "  and w0, w0, 0xFF\n");
        }
    } else if(tipo_destino == T_pLONGO) {
        if(tipo_origem == T_pINT) fprintf(s, "  sxtw x0, w0\n");
        else if(tipo_origem == T_pFLU) fprintf(s, "  fcvtzs x0, s0\n");
        else if(tipo_origem == T_pDOBRO) fprintf(s, "  fcvtzs x0, d0\n");
        else if(tipo_origem == T_pCAR) fprintf(s, "  sxtb x0, w0\n");
        else if(tipo_origem == T_pBOOL) fprintf(s, "  and x0, x0, 1\n");
    } else if(tipo_destino == T_pFLU) {
        if(tipo_origem == T_pINT) fprintf(s, "  scvtf s0, w0\n");
        else if(tipo_origem == T_pLONGO) fprintf(s, "  scvtf s0, x0\n");
        else if(tipo_origem == T_pDOBRO) fprintf(s, "  fcvt s0, d0\n");
        else if(tipo_origem == T_pCAR) {
            fprintf(s, "  sxtb w0, w0\n");
            fprintf(s, "  scvtf s0, w0\n");
        }
    } else if(tipo_destino == T_pDOBRO) {
        if(tipo_origem == T_pINT) fprintf(s, "  scvtf d0, w0\n");
        else if(tipo_origem == T_pLONGO) fprintf(s, "  scvtf d0, x0\n");
        else if(tipo_origem == T_pFLU) fprintf(s, "  fcvt d0, s0\n");
        else if(tipo_origem == T_pCAR) {
            fprintf(s, "  sxtb w0, w0\n");
            fprintf(s, "  scvtf d0, w0\n");
        }
    } else if(tipo_destino == T_pCAR) {
        if(tipo_origem == T_pINT || tipo_origem == T_pLONGO) {
            fprintf(s, "  and w0, w0, 0xFF\n");
        } else if(tipo_origem == T_pFLU) {
            fprintf(s, "  fcvtzs w0, s0\n");
            fprintf(s, "  and w0, w0, 0xFF\n");
        } else if(tipo_origem == T_pDOBRO) {
            fprintf(s, "  fcvtzs w0, d0\n");
            fprintf(s, "  and w0, w0, 0xFF\n");
        }
    } else if(tipo_destino == T_pBOOL) {
        // qualquer valor != 0 vira 1(verdade), 0 permanece 0(falso)
        if(tipo_origem == T_pINT || tipo_origem == T_pLONGO || 
           tipo_origem == T_pCAR) {
            fprintf(s, "  cmp w0, 0\n");
            fprintf(s, "  cset w0, ne\n");
        } else if(tipo_origem == T_pFLU) {
            fprintf(s, "  fcmp s0, 0.0\n");
            fprintf(s, "  cset w0, ne\n");
        } else if(tipo_origem == T_pDOBRO) {
            fprintf(s, "  fcmp d0, 0.0\n");
            fprintf(s, "  cset w0, ne\n");
        }
    }
}

void gerar_globais(FILE* s) {
    if(global_cnt == 0) return;
    
    fprintf(s, "\n.section .data\n");
    fprintf(s, ".align 3\n");
    
    for(int i = 0; i < global_cnt; i++) {
        Variavel* var = &globais[i];
        fprintf(s, "global_%s:\n", var->nome);
        
        if(var->eh_array) {
            int total_bytes = var->bytes;
            if(var->tipo_base == T_pCAR && var->valor >= 0) {
                // array de caracteres definindo com texto
                // var->valor tem o ID do texto
                const char* texto_valor = texs[var->valor].valor;
                fprintf(s, "  .asciz \"%s\"\n", texto_valor);
                // lreenche o resto com zeros se precisar
                int tam_texto = strlen(texto_valor) + 1; // +1 pra o \0
                if(tam_texto < total_bytes) {
                    fprintf(s, "  .space %d\n", total_bytes - tam_texto);
                }
            } else {
                fprintf(s, "  .space %d\n", total_bytes);
            }
        } else if(var->eh_ponteiro) {
            if(var->valor >= 0) {
                // ponteiro definido com endereço de texto
                fprintf(s, "  .quad %s\n", texs[var->valor].nome);
            } else {
                fprintf(s, "  .quad 0\n"); // ponteiros são 8 bytes
            }
        } else {
            switch(var->tipo_base) {
                case T_pBYTE:
                case T_pCAR:
                case T_pBOOL:
                    if(var->valor != 0) {
                        fprintf(s, "  .byte %d\n", var->valor);
                    } else {
                        fprintf(s, "  .byte 0\n");
                    }
                    break;
                case T_pINT:
                    if(var->valor != 0) {
                        fprintf(s, "  .word %d\n", var->valor);
                    } else {
                        fprintf(s, "  .word 0\n");
                    }
                    break;
                case T_pFLU:
                    // pra flutuante, inicializa com 0.0
                    fprintf(s, "  .float 0.0\n");
                    break;
                case T_pDOBRO:
                    fprintf(s, "  .double 0.0\n");
                    break;
                case T_pLONGO:
                    if(var->valor != 0) {
                        fprintf(s, "  .quad %ld\n", (long)var->valor);
                    } else {
                        fprintf(s, "  .quad 0\n");
                    }
                    break;
                default:
                    fprintf(s, "  .space %d\n", var->bytes);
            }
        }
    }
    fprintf(s, "\n.section .text\n");
}

void escrever_valor(FILE* s, TipoToken tipo) {
    if(tipo == T_pFLU) fprintf(s, "  bl _escrever_flu\n");
    else if(tipo == T_pCAR) fprintf(s, "  bl _escrever_car\n");
    else if(tipo == T_pBOOL) fprintf(s, "  bl _escrever_bool\n");
    else if(tipo == T_TEX) fprintf(s, "  bl _escrever_tex\n");
    else if(tipo == T_pLONGO || tipo == T_PONTEIRO) fprintf(s, "  bl _escrever_longo\n");
    else fprintf(s, "  bl _escrever_int\n");
}

void carregar_valor(FILE* s, Variavel* var) {
    if(var->escopo == -1) { // variável global
        fprintf(s, "  ldr x0, = global_%s\n", var->nome);
        
        if(var->eh_ponteiro) {
            // pra ponteiros globais, carrega o valor do ponteiro
            fprintf(s, "  ldr x0, [x0]\n");
        } else if(var->eh_array) {}
        else {
            // pra variaveis simples globais, carrega o valor
            switch(tam_tipo(var->tipo_base)) {
                case 1: 
                    fprintf(s, "  ldrb w0, [x0]\n"); 
                    break;
                case 4: 
                    if(var->tipo_base == T_pFLU) 
                        fprintf(s, "  ldr s0, [x0]\n");
                    else 
                        fprintf(s, "  ldr w0, [x0]\n");
                    break;
                case 8:
                    if(var->tipo_base == T_pDOBRO) 
                        fprintf(s, "  ldr d0, [x0]\n");
                    else 
                        fprintf(s, "  ldr x0, [x0]\n");
                    break;
            }
        }
    } else {
        if(var->eh_ponteiro) fatal("[carregar_valor] erro interno: carregar_valor chamado para ponteiro");
        else if(var->eh_array) fprintf(s, "  add x0, x29, %d\n", var->pos);
        else {
            switch(tam_tipo(var->tipo_base)) {
                case 1: 
                fprintf(s, "  ldrb w0, [x29, %d]\n", var->pos); 
                break;
                case 4: 
                if(var->tipo_base == T_pFLU) 
                fprintf(s, "  ldr s0, [x29, %d]\n", var->pos);
                else 
                fprintf(s, "  ldr w0, [x29, %d]\n", var->pos);
                break;
                case 8:
                if(var->tipo_base == T_pDOBRO) 
                fprintf(s, "  ldr d0, [x29, %d]\n", var->pos);
                else 
                fprintf(s, "  ldr x0, [x29, %d]\n", var->pos);
                break;
            }
        }
    }
}

void armazenar_valor(FILE* s, Variavel* var) {
    if(var->escopo == -1) { // variavel global
        fprintf(s, "  ldr x1, = global_%s\n", var->nome);
        
        if(var->eh_ponteiro) {
            // armazena o valor do ponteiro(x0) na variavel global
            fprintf(s, "  str x0, [x1]\n");
        } else if(var->eh_array) {
            fatal("[armazenar_valor] não é possível armazenar valor direto em array global");
        } else {
            // armazena o valor na variavel global
            switch(tam_tipo(var->tipo_base)) {
                case 1: 
                    fprintf(s, "  strb w0, [x1]\n"); 
                    break;
                case 4: 
                    if(var->tipo_base == T_pFLU) 
                        fprintf(s, "  str s0, [x1]\n");
                    else 
                        fprintf(s, "  str w0, [x1]\n");
                    break;
                case 8:
                    if(var->tipo_base == T_pDOBRO) 
                        fprintf(s, "  str d0, [x1]\n");
                    else 
                        fprintf(s, "  str x0, [x1]\n");
                    break;
            }
        }
    } else { // variavel local
        if(var->eh_ponteiro) {
            fprintf(s, "  str x0, [x29, %d]\n", var->pos);
        } else if(var->eh_array) {
            fatal("[armazenar_valor] não é possível armazenar valor direto em array");
        } else {
            switch(tam_tipo(var->tipo_base)) {
                case 1: 
                    fprintf(s, "  strb w0, [x29, %d]\n", var->pos); 
                    break;
                case 4:
                    if(var->tipo_base == T_pFLU) 
                        fprintf(s, "  str s0, [x29, %d]\n", var->pos);
                    else 
                        fprintf(s, "  str w0, [x29, %d]\n", var->pos);
                    break;
                case 8:
                    if(var->tipo_base == T_pDOBRO) 
                        fprintf(s, "  str d0, [x29, %d]\n", var->pos);
                    else 
                        fprintf(s, "  str x0, [x29, %d]\n", var->pos);
                    break;
            }
        }
    }
}

void carregar_const(FILE* s, int titulo) {
    Constante* c = &constantes[titulo];
    if(c->tipo == T_FLU) {
        fprintf(s, "  ldr x0, = const_%d\n", titulo);
        fprintf(s, "  ldr s0, [x0]\n");
    } else if(c->tipo == T_DOBRO) {
        fprintf(s, "  ldr x0, = const_%d\n", titulo);
        fprintf(s, "  ldr d0, [x0]\n");
    } else if(c->tipo == T_LONGO) {
        fprintf(s, "  ldr x0, = const_%d\n", titulo);
        fprintf(s, "  ldr x0, [x0]\n");
    } else if(c->tipo == T_INT) {
        fprintf(s, "  ldr x0, = const_%d\n", titulo);
        fprintf(s, "  ldr w0, [x0]\n");
    }
}

int add_const(TipoToken tipo, const char* lex, double d_val, long l_val) {
    for(int i = 0; i < const_cnt; i++) {
        if(tipo == T_FLU && constantes[i].tipo == T_FLU && fabs(constantes[i].d_val - d_val) < 1e-9)
            return constantes[i].titulo;
        if(tipo == T_DOBRO && constantes[i].tipo == T_DOBRO && fabs(constantes[i].d_val - d_val) < 1e-9)
            return constantes[i].titulo;
        if(tipo == T_INT && constantes[i].tipo == T_INT && constantes[i].l_val == l_val)
            return constantes[i].titulo;
        if(tipo == T_LONGO && constantes[i].tipo == T_LONGO && constantes[i].l_val == l_val)
            return constantes[i].titulo;
    }
    if(const_cnt >= MAX_CONST) fatal("[add_const] excesso de constantes");
    Constante* c = &constantes[const_cnt];
    c->tipo = tipo;
    strcpy(c->lex, lex);
    c->d_val = d_val;
    c->l_val = l_val;
    c->titulo = const_cnt;
    const_cnt++;
    return c->titulo;
}

int add_tex(const char* valor) {
    for(int i = 0; i < tex_cnt; i++) {
        if(strcmp(texs[i].valor, valor) == 0) return i;
    }
    if(tex_cnt >= MAX_TEX) fatal("[add_tex] excesso de textos");
    Tex* tex = &texs[tex_cnt];
    strcpy(tex->valor, valor);
    sprintf(tex->nome, ".tex_%d", tex_cnt);
    tex_cnt++;
    return tex_cnt - 1;
}

TipoToken fator(FILE* s, int escopo) {
    if(L.tk.tipo == T_CONVERT) {
        char* tipo_destino_str = L.tk.lex;
        TipoToken tipo_destino;
        // converte texto pra TipoToken
        if(strcmp(tipo_destino_str, "car") == 0) tipo_destino = T_pCAR;
        else if(strcmp(tipo_destino_str, "int") == 0) tipo_destino = T_pINT;
        else if(strcmp(tipo_destino_str, "flu") == 0) tipo_destino = T_pFLU;
        else if(strcmp(tipo_destino_str, "dobro") == 0) tipo_destino = T_pDOBRO;
        else if(strcmp(tipo_destino_str, "longo") == 0) tipo_destino = T_pLONGO;
        else if(strcmp(tipo_destino_str, "bool") == 0) tipo_destino = T_pBOOL;
        else if(strcmp(tipo_destino_str, "byte") == 0) tipo_destino = T_pBYTE;
        else fatal("[fator] tipo inválido no conversão");
        
        proximoToken(); // consome o token de conversão
        // processa a expressão a ser convertida
        TipoToken tipo_origem = fator(s, escopo);
        // gera codigo de conversão
        gerar_convert(s, tipo_origem, tipo_destino);
        
        return tipo_destino;
    }
    if(L.tk.tipo == T_ARROBA) {
        // operador de endereço @
        proximoToken();
        
        if(L.tk.tipo != T_ID) fatal("[fator] @ espera identificador");
        
        Variavel* var = buscar_var(L.tk.lex, escopo);
        if(!var) fatal("[fator] variável não encontrada");
        // carrega endereço da variavel
        if(var->eh_parametro && var->eh_array) fprintf(s, "  ldr x0, [x29, %d]\n", var->pos);
        else fprintf(s, "  add x0, x29, %d\n", var->pos);
        proximoToken();
        return T_PONTEIRO;
    }
    if(L.tk.tipo == T_MAIS_MAIS || L.tk.tipo == T_MENOS_MENOS) {
        TipoToken op = L.tk.tipo;
        proximoToken();
        
        if(L.tk.tipo != T_ID) fatal("[fator] identificador esperado após operador ++/--");
        
        Variavel* var = buscar_var(L.tk.lex, escopo);
        if(!var) fatal("[fator] variável não declarada");
        if(var->eh_final) fatal("[fator] não é possível modificar variável final");
        
        proximoToken();
        // carrega valor atual
        carregar_valor(s, var);
        
        // incrementa ou decrementa
        if(op == T_MAIS_MAIS) {
            if(var->tipo_base == T_pFLU) {
                fprintf(s, "  fmov s1, #1.0\n");
                fprintf(s, "  fadd s0, s0, s1\n");
            } else if(var->tipo_base == T_pDOBRO) {
                fprintf(s, "  fmov d1, #1.0\n");
                fprintf(s, "  fadd d0, d0, d1\n");
            } else if(var->tipo_base == T_pLONGO) {
                fprintf(s, "  add x0, x0, #1\n");
            } else {
                fprintf(s, "  add w0, w0, #1\n");
            }
        } else { // T_MENOS_MENOS
            if(var->tipo_base == T_pFLU) {
                fprintf(s, "  fmov s1, #1.0\n");
                fprintf(s, "  fsub s0, s0, s1\n");
            } else if(var->tipo_base == T_pDOBRO) {
                fprintf(s, "  fmov d1, #1.0\n");
                fprintf(s, "  fsub d0, d0, d1\n");
            } else if(var->tipo_base == T_pLONGO) {
                fprintf(s, "  sub x0, x0, #1\n");
            } else {
                fprintf(s, "  sub w0, w0, #1\n");
            }
        }
        // armazena o novo valor
        armazenar_valor(s, var);
        // carrega denovo pra retornar o valor(pre fixado retorna o novo valor)
        carregar_valor(s, var);
        
        return var->tipo_base;
    }
    if(L.tk.tipo == T_MENOS) {
        proximoToken();
        TipoToken tipo = fator(s, escopo);
        
        if(tipo == T_pFLU) fprintf(s, "  fneg s0, s0\n");
        else if(tipo == T_pDOBRO) fprintf(s, "  fneg d0, d0\n");
        else if(tipo == T_pLONGO) fprintf(s, "  neg x0, x0\n");
        else fprintf(s, "  neg w0, w0\n");
        
        return tipo;
    } else if(L.tk.tipo == T_PAREN_ESQ) {
        proximoToken();
        TipoToken tipo = expressao(s, escopo);
        excessao(T_PAREN_DIR);
        return tipo;
    } else if(L.tk.tipo == T_ID) return tratar_id(s, escopo);
    else if(L.tk.tipo == T_BYTE) return tratar_byte(s);
    else if(L.tk.tipo == T_INT) return tratar_inteiro(s);
    else if(L.tk.tipo == T_FLU || L.tk.tipo == T_DOBRO) return tratar_flutuante(s);
    else if(L.tk.tipo == T_CAR) return tratar_caractere(s);
    else if(L.tk.tipo == T_TEX) return tratar_texto(s);
    else {
        fatal("[fator] fator inválido");
        return T_pINT;
    }
}

TipoToken termo(FILE* s, int escopo) {
    TipoToken tipo = fator(s, escopo);
    
    while(L.tk.tipo == T_VEZES || L.tk.tipo == T_DIV || L.tk.tipo == T_PORCEN || L.tk.tipo == T_MENOR_MENOR || L.tk.tipo == T_MAIOR_MAIOR) {
        TipoToken op = L.tk.tipo;
        proximoToken();
        
        if(tipo == T_pFLU) {
            fprintf(s, "  str s0, [sp, -16]!\n");
        } else if (tipo == T_pDOBRO) {
            fprintf(s, "  str d0, [sp, -16]!\n");
        } else {
            fprintf(s, "  str w0, [sp, -16]!\n");
        }
        TipoToken tipo_dir = fator(s, escopo);
        
        if(tipo == T_pFLU) {
            fprintf(s, "  ldr s1, [sp], 16\n");
        } else if (tipo == T_pDOBRO) {
            fprintf(s, "  ldr d1, [sp], 16\n");
        } else {
            fprintf(s, "  ldr w1, [sp], 16\n");
        }
        tipo = converter_tipos(s, tipo, tipo_dir);
        gerar_operacao(s, op, tipo);
    }
    return tipo;
}

TipoToken expressao(FILE* s, int escopo) {
    if(L.tk.tipo == T_ID) {
        char id[32];
        strcpy(id, L.tk.lex);
        // dalva a posição atual
        size_t pos_salvo = L.pos;
        int linha_salvo = L.linha_atual;
        int coluna_salvo = L.coluna_atual;
        Token tk_salvo = L.tk;
        
        // avança para ver se tem '='
        proximoToken();
        if(L.tk.tipo == T_IGUAL) {
            // é uma atribuição, processa normalmente
            L.pos = pos_salvo;
            L.linha_atual = linha_salvo;
            L.coluna_atual = coluna_salvo;
            L.tk = tk_salvo;
            
            verificar_atribuicao(s, id, escopo);
            return buscar_var(id, escopo)->tipo_base;
        } else {
            // não é atribuição, volta e processa como expressão normal
            L.pos = pos_salvo;
            L.linha_atual = linha_salvo;
            L.coluna_atual = coluna_salvo;
            L.tk = tk_salvo;
        }
    }
    TipoToken tipo = termo(s, escopo);
    
    while(L.tk.tipo == T_MAIS || L.tk.tipo == T_MENOS || L.tk.tipo == T_TAMBEM_TAMBEM || L.tk.tipo == T_OU_OU || L.tk.tipo == T_MENOR_MENOR) {
        TipoToken op = L.tk.tipo;
        proximoToken();
       // salva o primeiro resultado:
       if(tipo == T_pFLU) {
           fprintf(s, "  str s0, [sp, -16]!\n");
       } else if(tipo == T_pDOBRO) {
           fprintf(s, "  str d0, [sp, -16]!\n");
       } else if(tipo == T_PONTEIRO || tipo == T_pLONGO) {
           fprintf(s, "  str x0, [sp, -16]!\n");
       } else {
           fprintf(s, "  str w0, [sp, -16]!\n");
       }
       TipoToken tipo_dir = termo(s, escopo);
        // recupera o primeiro resultado:
        if(tipo == T_pFLU) {
            fprintf(s, "  ldr s1, [sp], 16\n");
        } else if(tipo == T_pDOBRO) {
            fprintf(s, "  ldr d1, [sp], 16\n");
            
        } else if(tipo == T_PONTEIRO || tipo == T_pLONGO) {
            fprintf(s, "  ldr x1, [sp], 16\n");
        } else {
            fprintf(s, "  ldr w1, [sp], 16\n");
        }
        tipo = converter_tipos(s, tipo, tipo_dir);
        gerar_operacao(s, op, tipo);
    }
    if(L.tk.tipo == T_IGUAL_IGUAL || L.tk.tipo == T_DIFERENTE || 
        L.tk.tipo == T_MAIOR || L.tk.tipo == T_MENOR ||
        L.tk.tipo == T_MAIOR_IGUAL || L.tk.tipo == T_MENOR_IGUAL) {
        
        TipoToken op = L.tk.tipo;
        proximoToken();
        
        if(tipo == T_pFLU) {
            fprintf(s, "  str s0, [sp, -16]!\n");
        } else if(tipo == T_pDOBRO) {
            fprintf(s, "  str d0, [sp, -16]!\n");
        } else if(tipo == T_PONTEIRO || tipo == T_pLONGO) {
            fprintf(s, "  str x0, [sp, -16]!\n");
        } else {
            fprintf(s, "  str w0, [sp, -16]!\n");
        }
        TipoToken tipo_dir = termo(s, escopo);
        
        if(tipo == T_pFLU) {
            fprintf(s, "  ldr s1, [sp], 16\n");
        } else if(tipo == T_pDOBRO) {
            fprintf(s, "  ldr d1, [sp], 16\n");
        } else if(tipo == T_PONTEIRO || tipo == T_pLONGO) {
            fprintf(s, "  ldr x1, [sp], 16\n");
        } else {
            fprintf(s, "  ldr w1, [sp], 16\n");
        }
        tipo = converter_tipos(s, tipo, tipo_dir);
        gerar_comparacao(s, op, tipo);
        tipo = T_pBOOL;
    }
    return tipo;
}

void declaracao_var(FILE* s, int* pos, int escopo, int eh_parametro, int eh_final, int eh_global) {
    TipoToken tipo_base = L.tk.tipo;
    int eh_ponteiro = 0;
    int num_dims = 0;
    int dims[MAX_DIMS] = {0};

    proximoToken();

    if(L.tk.tipo == T_VEZES) {
        eh_ponteiro = 1;
        proximoToken();
    } else {
        while(L.tk.tipo == T_COL_ESQ) {
            if(num_dims >= MAX_DIMS) fatal("[declaracao_var] excesso de dimensões");
            proximoToken();
            long tam_array = 0;
            
            if(L.tk.tipo == T_INT) {
                tam_array = L.tk.valor_l;
                proximoToken();
            } else if(L.tk.tipo == T_ID) { 
                char id_tam[32];
                strcpy(id_tam, L.tk.lex);
                
                Variavel* var_tam = buscar_var(id_tam, escopo);
                if(var_tam) {
                    if(!var_tam) {
                        fatal("[declaracao_var] identificador não declarado para tamanho do array");
                    }
                    if(var_tam->tipo_base != T_pINT && var_tam->tipo_base != T_pLONGO) {
                        fatal("[declaracao_var] a variavel de tamanho deve ser do tipo 'int' ou 'longo'");
                    }
                    if(!var_tam->eh_final) {
                        fatal("[declaracao_var] o tamanho do array deve ser um literal inteiro ou uma variavel declarada como 'final'");
                    }
                    tam_array = var_tam->valor;
                    proximoToken(); // consome o T_ID
                } else {
                    Macro* ma = buscar_macro(id_tam);
                    if(ma) {
                        tam_array = ma->valor;
                        proximoToken();
                    } else {
                        fatal("[declaracao_var] identificador nao declarado (nem variavel nem macro) para tamanho do array");
                    }
                }
            }
            dims[num_dims] = (int)tam_array;
            
            excessao(T_COL_DIR);
            num_dims++;
        }
    }
    if(L.tk.tipo != T_ID) fatal("[declaracao_var] nome de variável esperado");
    
    char nome_var[32];
    strcpy(nome_var, L.tk.lex); // salva o nome da variavel
    proximoToken(); // consome o nome da variavel

    if(eh_global) {
        if(global_cnt >= MAX_VAR) fatal("[declaracao_var] excesso de variáveis globais");
        Variavel* var = &globais[global_cnt++];
        
        strcpy(var->nome, nome_var);
        var->tipo_base = tipo_base;
        var->eh_ponteiro = eh_ponteiro;
        var->eh_array = (num_dims > 0);
        var->num_dims = num_dims;
        var->eh_final = eh_final;
        var->valor = 0; // valor padrão
        memcpy(var->dims, dims, sizeof(dims));
        var->escopo = -1;
        var->eh_parametro = 0;
        
        int tam_elemento = tam_tipo(tipo_base);
        int tam_total = tam_elemento;
        for(int i = 0; i < num_dims; i++) {
            if(dims[i] > 0) tam_total *= dims[i];
        }
        if(eh_ponteiro) tam_total = 8;
        var->bytes = tam_total;
        
        if(L.tk.tipo == T_IGUAL) {
            proximoToken();
            
            if(num_dims > 0 && tipo_base == T_pCAR && L.tk.tipo == T_TEX) {
                // array de caracteres com texto literal
                const char* texto_valor = L.tk.lex;
                int tam = strlen(texto_valor);
                if(dims[0] > 0 && tam + 1 > dims[0]) {
                    fatal("[declaracao_var] texto muito longo para o array");
                }
                // guarda o texto como constante
                int id_tex = add_tex(texto_valor);
                var->valor = id_tex; // guarda o ID do texto
                proximoToken();
            } else if(num_dims > 0) {
                fatal("[declaracao_var] inicialização de array global com {} não implementada");
            } else if(eh_ponteiro && L.tk.tipo == T_TEX) {
                // ponteiro para texto literal
                int id_tex = add_tex(L.tk.lex);
                var->valor = id_tex; // guarda o ID do texto
                proximoToken();
            } else {
                // definição simples com constante
                if(L.tk.tipo == T_INT) {
                    var->valor = (int)L.tk.valor_l;
                } else if(L.tk.tipo == T_CAR && L.tk.lex[0] != 0) {
                    var->valor = (int)L.tk.lex[0];
                } else if(L.tk.tipo == T_BYTE) {
                    var->valor = (int)L.tk.valor_l;
                } else {
                    fatal("[declaracao_var] inicialização global deve ser constante");
                }
                proximoToken();
            }
        }
    } else {
        // codigo para variáveis locais
        Funcao* f = &funcs[fn_cnt - 1];
        if(f->var_conta >= MAX_VAR) fatal("[declaracao_var] excesso de variáveis");
        Variavel* var = &f->vars[f->var_conta];
        strcpy(var->nome, nome_var);
        var->tipo_base = tipo_base;
        var->eh_ponteiro = eh_ponteiro;
        var->eh_array = (num_dims > 0);
        var->num_dims = num_dims;
        var->eh_final = eh_final;
        var->valor = 0;
        memcpy(var->dims, dims, sizeof(dims));
        int tam_elemento = tam_tipo(tipo_base);
        int tam_total = tam_elemento;
        
        for(int i = 0; i < num_dims; i++) {
            if(dims[i] > 0) {
                tam_total *= dims[i];
            }
        }
        if(eh_ponteiro) tam_total = 8;
        
        if(!eh_parametro) {
            *pos = *pos - tam_total;
            *pos = *pos & ~15;
            var->pos = *pos;
        } else {
            var->pos = *pos;
        }
        var->escopo = escopo;
        var->eh_parametro = eh_parametro;
        var->bytes = tam_total;
        f->var_conta++;
        
        if(L.tk.tipo == T_IGUAL) {
            proximoToken();
            if(num_dims > 0 && tipo_base == T_pCAR && L.tk.tipo == T_TEX) {
                const char* texto_valor = L.tk.lex;
                int tam = strlen(texto_valor);
                if(dims[0] > 0 && tam + 1 > dims[0]) {
                    fatal("[declaracao_var] texto muito longo para o array");
                }
                for(int i = 0; i <= tam; i++) {
                    fprintf(s, "  mov w1, %d\n", texto_valor[i]);
                    fprintf(s, "  strb w1, [x29, %d]\n", var->pos + i);
                }
                proximoToken();
            } else if(num_dims > 0) {
                excessao(T_CHAVE_ESQ);
                int indices[MAX_DIMS] = {0};
                verificar_matriz(s, var, escopo, indices, 0);
                excessao(T_CHAVE_DIR);
            } else if(eh_ponteiro && L.tk.tipo == T_TEX) {
                int id_tex = add_tex(L.tk.lex);
                fprintf(s, "  ldr x0, = %s\n", texs[id_tex].nome);
                fprintf(s, "  str x0, [x29, %d]\n", var->pos);
                proximoToken();
            } else {
                Token token_valor = L.tk;
                TipoToken tipo_exp = expressao(s, escopo);
                if(eh_final) {
                    if(token_valor.tipo == T_INT) {
                        var->valor = (int)token_valor.valor_l;
                    } else if(token_valor.tipo == T_CAR && token_valor.lex[0] != 0) {
                        var->valor = (int)token_valor.lex[0];
                    }
                }
                if(eh_ponteiro) {
                    if(tipo_exp == T_pINT) {
                        fprintf(s, "  sxtw x0, w0\n");
                    }
                    fprintf(s, "  str x0, [x29, %d]\n", var->pos); 
                } else {
                    armazenar_valor(s, var);
                }
            }
        }
        if(eh_parametro) *pos += 8;
    }
}

int main(int argc, char** argv) {
    if(argc < 2) {
        printf("FPB: sem arquivos de entrada\n");
        printf("Use \"fpb -ajuda\" para mais informações\n");
        return 0;
    }
    // variaveis de configuração
    char arquivoEntrada[256] = "";
    char arquivoSaida[256] = "";
    int manter_asm = 0;
    int semLinkar = 0;
    int otimizar1 = 0;
    int otimizar2 = 0;
    int modoAjuda = 0;
    int modoVersao = 0;
    int modoConfig = 0;

    // processa argumentos
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-ajuda") == 0) modoAjuda = 1;
        else if(strcmp(argv[i], "-v") == 0) modoVersao = 1;
        else if(strcmp(argv[i], "-c") == 0) modoConfig = 1;
        else if(strcmp(argv[i], "-asm") == 0) manter_asm = 1;
        else if(strcmp(argv[i], "-sl") == 0) semLinkar = 1;
        else if(strcmp(argv[i], "-O1") == 0) otimizar1 = 1;
        else if(strcmp(argv[i], "-O2") == 0) otimizar2 = 1;
        else if(strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            // proximo arg é o nome do arquivo de saida
            snprintf(arquivoSaida, sizeof(arquivoSaida), "%s", argv[++i]);
        } else if(argv[i][0] != '-') {
            // é um arquivo de entrada
            if(strlen(arquivoEntrada) == 0) {
                snprintf(arquivoEntrada, sizeof(arquivoEntrada), "%s", argv[i]);
            }
        }
    }
    // modos de informação
    if(modoAjuda) {
        printf("[informação]:\n");
        printf("fpb -v : versão e o distribuidor\n");
        printf("fpb -c : configurações do compilador\n");
        printf("fpb -O1 : otimização nivel 1, eliminação de código morto\n");
        printf("fpb -O2 : otimização nivel 2, mais agressiva\n");
        printf("[compilação]:\n");
        printf("fpb exemplo.fpb : compila arquivo e gera binário\n");
        printf("fpb exemplo.fpb -s saida : compila com nome personalizado\n");
        printf("fpb exemplo.fpb -asm : mantem ASM intermediario\n");
        printf("fpb exemplo.fpb -sl : não linka o código\n");
        printf("fpb exemplo.fpb -O2 -asm -o programa : combina opções\n");
        printf("\npara definir o diretorio das bibliotecas.\nmodifique a variavel de ambiente \"$FPB_DIR\"\n");
        return 0;
    }
    if(modoVersao) {
        printf("[FOCA-DO ESTÚDIOS]\nFPB (Fácil Programação Baixo nivel) - v0.0.1 (beta)\n");
        return 0;
    }
    if(modoConfig) {
        printf("[configuração]:\n");
        printf("max codigo: %i\n", MAX_CODIGO);
        printf("max variaveis: %i\n", MAX_VAR);
        printf("max constantes: %i\n", MAX_CONST);
        printf("max texs: %i\n", MAX_TEX);
        printf("max funcoes: %i\n", MAX_FN);
        printf("max dimensões: %i\n", MAX_DIMS);
        printf("max macros: %i\n", MAX_MACROS);
        return 0;
    }
    // verificar se tem arquivo de entrada
    if(strlen(arquivoEntrada) == 0) {
        printf("FPB: [ERRO] nenhum arquivo de entrada especificado\n");
        return 2;
    }
    // processa extensão .fpb se não tiver
    char nomeArquivo[256];
    if(strstr(arquivoEntrada, ".fpb") == NULL) {
        snprintf(nomeArquivo, sizeof(nomeArquivo), "%s.fpb", arquivoEntrada);
    } else {
        snprintf(nomeArquivo, sizeof(nomeArquivo), "%s", arquivoEntrada);
    }
    // determina o nome de saida
    char asm_s[256], asm_o[256], cmd[512];
    if(strlen(arquivoSaida) > 0) {
        // usa nome sem extensão
        snprintf(asm_s, sizeof(asm_s), "%s.asm", arquivoSaida);
        snprintf(asm_o, sizeof(asm_o), "%s.o", arquivoSaida);
    } else {
        // usa o nome base do arquivo de entrada
        char nomeBase[256];
        strcpy(nomeBase, arquivoEntrada);
        char *ponto = strstr(nomeBase, ".fpb");
        if(ponto) *ponto = '\0'; // remove extensão
        
        snprintf(asm_s, sizeof(asm_s), "%s.asm", nomeBase);
        snprintf(asm_o, sizeof(asm_o), "%s.o", nomeBase);
        snprintf(arquivoSaida, sizeof(arquivoSaida), "%s", nomeBase);
    }
    // abre e compila arquivo
    arquivoAtual = arquivoEntrada;
    FILE* en = fopen(nomeArquivo, "r");
    if(!en) {
        printf("FPB: [ERRO] não foi possível abrir %s\n", nomeArquivo);
        return 2;
    }
    char* buf = malloc(MAX_CODIGO);
    size_t n = fread(buf, 1, MAX_CODIGO, en);
    buf[n] = 0;
    fclose(en);

    L.fonte = buf;
    L.pos = 0;
    proximoToken();

    FILE* s = fopen(asm_s, "w");
    gerar_prelude(s);
    
    if(!s) {
        printf("FPB: [ERRO] não foi possível criar o ASM intermediario\n");
        free(buf);
        return 1;
    }
    while(L.tk.tipo != T_FIM) {
        if(L.tk.tipo == T_INCLUIR) {
            int pos = 0;
            verificar_stmt(s, &pos, 0);
        } else if(L.tk.tipo == T_DEF) verificar_def();
        else if(L.tk.tipo == T_ESPACO) verificar_espaco(s);
        else if(L.tk.tipo == T_GLOBAL) verificar_global(s);
        else verificar_fn(s);
    }
    gerar_consts(s);
    gerar_texs(s);
    gerar_globais(s);
    fclose(s);
    // aplica otimizações
    if(otimizar1) otimizarO1(asm_s);
    if(otimizar2) otimizarO2(asm_s);
    // montagem do objeto
    snprintf(cmd, sizeof(cmd), "as %s -o %s", asm_s, asm_o);
    if(system(cmd)) {
        free(buf);
        return 3;
    }
    // linkar(se não for -sl)
    if(!semLinkar) {
        snprintf(cmd, sizeof(cmd), "ld -e inicio %s -o %s", asm_o, arquivoSaida);
        if(system(cmd)) {
            free(buf);
            return 4;
        }
    }
    // limpeza
    if(!manter_asm) remove(asm_s);
    remove(asm_o);
    free(buf);
    return 0;
}