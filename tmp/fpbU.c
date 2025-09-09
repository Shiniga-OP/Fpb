/*
* [INFO]:
* [IMPLEMENTAÇÃO]: @Shiniga-OP.
* [BASE]: Assembly.
* [SISTEMA OPERACIONAL]: ANDROID.
* [ARQUITETURA]: AARCH64-LINUX-ANDROID(ARM64).
* [LINGUAGEM]: Português Brasil(PT-BR).
* [ESTADO]: Alpha.
* [DATA]: 06/07/2025.
* [ATUAL]: 07/09/2025.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_TOK 512 // maximo de tolens
#define MAX_CODIGO 8192 // maximo de codhgk
#define MAX_FN 64 // maximo de funções
#define MAX_VAR 128 // maximo de variaveis
#define MAX_CONST 128 // maxmio de constpos
#define MAX_PARAMS 8 // maximo de parametros
#define MAX_TEX 128 // mqximo de textos

typedef enum {
    // tipos:
    T_ID, T_INT, T_TEX, T_CAR, T_FLU, T_DOBRO, T_LONGO,
    T_COMENTARIO,
    // simbolos:
    T_PAREN_ESQ, T_PAREN_DIR,  
    T_CHAVE_ESQ, T_CHAVE_DIR,
    T_COL_ESQ, T_COL_DIR,
    T_PONTO_VIRGULA, T_VIRGULA,  
    // operadores:
    T_IGUAL, T_MAIS, T_MENOS, T_VEZES, T_DIV,
    // condicionais:
    T_SE, T_SENAO, T_IGUAL_IGUAL, T_DIFERENTE,
    T_MAIOR, T_MENOR, T_MAIOR_IGUAL, T_MENOR_IGUAL,
    // retornos:
    T_pCAR, T_pINT, T_pFLU, T_pBOOL, T_pDOBRO, T_pLONGO,
    T_pVAZIO,
    // definições:
    T_DEF, T_REG, T_FIM, T_RETORNAR, T_INCLUIR
} TipoToken;

typedef struct {
    int linha;
    int coluna;
    const char* arquivo;
} Posicao;

typedef struct {
    TipoToken tipo;
    char lex[MAX_TOK];
    double valor_d; // para constpos de ponto flutuante
    long valor_l; // para constpos inteiras grandes
    Posicao pos;
} Token;

typedef struct {
    char nome[32];
    TipoToken tipo_base;
    int eh_ponteiro;
    int eh_array;
    int tam_array;
    int pos;
    int escopo;
    int eh_parametro;
} Variavel;

typedef struct {
    char nome[32];
    TipoToken retorno;
    Variavel vars[MAX_VAR];
    int var_conta;
    int escopo_atual;
    int tamanho_frame;
    int param_pos; 
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
    char valor[MAX_TOK];
} Tex;

static Lexer L;
static Funcao funcs[MAX_FN];
static int fn_cnt = 0;
static int escopo_global = 0;
static Constante constpos[MAX_CONST];
static int const_cnt = 0;
static Tex texs[MAX_TEX];
static int tex_cnt = 0;
static char* arquivoAtual;
static int debug = 0;

// buscar
Variavel* buscar_var(const char* nome, int escopo);
Funcao* buscar_fn(const char* nome);
// carregar
void carregar_valor(FILE* s, Variavel* var);
void carregar_const(FILE* s, int titulo);
// declaracao
void declaracao_var(FILE* s, int* pos, int escopo, int eh_parametro);
// tratar
TipoToken tratar_id(FILE* s, int escopo);
TipoToken tratar_chamada_funcao(FILE* s, int escopo, const char* nome, Funcao* fn);
TipoToken tratar_inteiro(FILE* s);
TipoToken tratar_flutuante(FILE* s);
TipoToken tratar_caractere(FILE* s);
TipoToken tratar_texto(FILE* s);
// verificar
void verificar_fn(FILE* s);
void verificar_parenteses_extras();
void verificar_stmt(FILE* s, int* pos, int escopo);
void verificar_retorno(FILE* s, int escopo);
void verificar_atribuicao(FILE* s, const char* id, int escopo);
// add
int add_tex(const char* valor);
int add_const(TipoToken tipo, const char* lex, double d_val, long l_val);
// gerar
void gerar_comparacao(FILE* s, TipoToken op, TipoToken tipo);
void gerar_operacao(FILE* s, TipoToken op, TipoToken tipo);
void gerar_prelude(FILE* s);
// etc
TipoToken expressao(FILE* s, int escopo);
TipoToken termo(FILE* s, int escopo);
TipoToken fator(FILE* s, int escopo);
TipoToken converter_tipos(FILE* s, TipoToken tipo_anterior, TipoToken tipo_atual);
void excessao(TipoToken t);
void fatal(const char* m);
void proximoToken();
int tam_tipo(TipoToken t);
void fim(FILE* s);
void armazenar_valor(FILE* s, Variavel* var);
void escrever_valor(FILE* s, TipoToken tipo);

// [DEBUG]:
const char* token_str(TipoToken t) {
    switch(t) {
        case T_ID: return "identificador";
        case T_INT: return "inteiro";
        case T_TEX: return "texto";
        case T_CAR: return "caractere";
        case T_FLU: return "flutuante";
        case T_DOBRO: return "dobro";
        case T_COMENTARIO: return "comentário";
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
        case T_IGUAL_IGUAL: return "==";
        case T_DIFERENTE: return "!=";
        case T_MAIOR: return ">";
        case T_MENOR: return "<";
        case T_MAIOR_IGUAL: return ">=";
        case T_MENOR_IGUAL: return "<=";
        case T_pCAR: return "car";
        case T_pINT: return "int";
        case T_pFLU: return "flu";
        case T_pBOOL: return "bool";
        case T_pDOBRO: return "dobro";
        case T_pLONGO: return "longo";
        case T_pVAZIO: return "vazio";
        case T_DEF: return "def";
        case T_REG: return "reg";
        case T_RETORNAR: return "retorne";
        case T_INCLUIR: return "incluir";
        case T_FIM: return "fim";
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

void verificar_parenteses_extras() {
    if(L.tk.tipo == T_PAREN_DIR) fatal("Parêntese extra encontrado");
}
// [UTIL]:
int tam_tipo(TipoToken t) {
    switch(t) {
        case T_pCAR: case T_pBOOL: return 1;
        case T_pINT: case T_pFLU: return 4;
        case T_pDOBRO: case T_pLONGO: return 8;
        default: return 0;
    }
}

int tipos_compativeis(TipoToken tipo1, TipoToken tipo2) {
    if(tipo1 == tipo2) return 1;
    if((tipo1 == T_pCAR && tipo2 == T_pINT) || (tipo1 == T_pINT && tipo2 == T_pCAR)) return 1;
    return 0;
}

void proximoToken() {
    char c;
    int i;

    for(;;) {
        c = L.fonte[L.pos];
        while(c && isspace((unsigned char)c)) {
            if(c == '\n') { L.linha_atual++; L.coluna_atual = 1; }
            else L.coluna_atual++;
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
            for(;;) {
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
            }
            else L.coluna_atual++;
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
        fatal("diretiva desconhecida");
    }
    if(isalpha((unsigned char)c) || c == '_') {
        i = 0;
        while((c = L.fonte[L.pos]) && (isalnum((unsigned char)c) || c == '_')) {
            if(i < MAX_TOK - 1) L.tk.lex[i++] = c;
            L.pos++; L.coluna_atual++;
        }
        L.tk.lex[i] = 0;
        // reconhece tipos
        if(strcmp(L.tk.lex, "car") == 0) L.tk.tipo = T_pCAR;
        else if(strcmp(L.tk.lex, "int") == 0) L.tk.tipo = T_pINT;
        else if(strcmp(L.tk.lex, "flu") == 0) L.tk.tipo = T_pFLU;
        else if(strcmp(L.tk.lex, "bool") == 0) L.tk.tipo = T_pBOOL;
        else if(strcmp(L.tk.lex, "dobro") == 0) L.tk.tipo = T_pDOBRO;
        else if(strcmp(L.tk.lex, "longo") == 0) L.tk.tipo = T_pLONGO;
        else if(strcmp(L.tk.lex, "vazio") == 0) L.tk.tipo = T_pVAZIO;
        else if(strcmp(L.tk.lex, "se") == 0) L.tk.tipo = T_SE;
        else if(strcmp(L.tk.lex, "senao") == 0) L.tk.tipo = T_SENAO;
        else if(strcmp(L.tk.lex, "retorne") == 0) L.tk.tipo = T_RETORNAR;
        else if(strcmp(L.tk.lex, "retornar") == 0) L.tk.tipo = T_RETORNAR;
        else L.tk.tipo = T_ID;
        return;
    }
    if(isdigit((unsigned char)c) || c == '.') {
        i = 0;
        int ponto = 0;
        while((c = L.fonte[L.pos]) && (isdigit((unsigned char)c) || c == '.')) {
            if(c == '.') {
                if(ponto) fatal("numero invalido");
                ponto = 1;
            }
            if(i < MAX_TOK - 1) L.tk.lex[i++] = c;
            L.pos++; L.coluna_atual++;
        }
        L.tk.lex[i] = 0;
        if(ponto) {
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
        }
        else fatal("tex nao fechado");
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
        case '(': L.tk.tipo = T_PAREN_ESQ; break;
        case ')': L.tk.tipo = T_PAREN_DIR; break;
        case '{': L.tk.tipo = T_CHAVE_ESQ; break;
        case '}': L.tk.tipo = T_CHAVE_DIR; break;
        case '[': L.tk.tipo = T_COL_ESQ; break;
        case ']': L.tk.tipo = T_COL_DIR; break;
        case ';': L.tk.tipo = T_PONTO_VIRGULA; break;
        case ',': L.tk.tipo = T_VIRGULA; break;
        case '=':
            if(L.fonte[L.pos + 1] == '=') {
                L.tk.tipo = T_IGUAL_IGUAL;
                L.pos++;
                L.coluna_atual++;
            } else L.tk.tipo = T_IGUAL;
        break;
        case '+': L.tk.tipo = T_MAIS; break;
        case '-': L.tk.tipo = T_MENOS; break;
        case '*': L.tk.tipo = T_VEZES; break;
        case '/': L.tk.tipo = T_DIV; break;
        case '>':
            if(L.fonte[L.pos + 1] == '=') {
                L.tk.tipo = T_MAIOR_IGUAL;
                L.pos++;
                L.coluna_atual++;
            } else L.tk.tipo = T_MAIOR;
        break;
        case '<':
            if(L.fonte[L.pos + 1] == '=') {
                L.tk.tipo = T_MENOR_IGUAL;
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
        default: fatal("simbolo invalido"); break;
    }
    L.tk.lex[0] = c;
    L.tk.lex[1] = 0;
    L.pos++;
    L.coluna_atual++;
}

// [TRATAMENTO]:
TipoToken tratar_texto(FILE* s) {
    if(debug) fprintf(s, "// <[tratar_texto]>\n");
    int id = add_tex(L.tk.lex);
    fprintf(s, "  ldr x0, = %s\n", texs[id].nome);
    proximoToken();
    if(debug) fprintf(s, "// </[tratar_texto]>\n");
    return T_TEX;
}

TipoToken tratar_id(FILE* s, int escopo) {
    if(debug) fprintf(s, "// <[tratar_id]>\n");
    char id[32];
    strcpy(id, L.tk.lex);
    Variavel* var = buscar_var(id, escopo);
    
    if(!var) {
        Funcao* fn = buscar_fn(id);
        if(fn) {
            if(debug) fprintf(s, "// </[tratar_id]>\n");
            return tratar_chamada_funcao(s, escopo, id, fn);
        } else fatal("variável ou função não declarada");
    }
    proximoToken();
    
    if(var->eh_array) {
        fprintf(s, "  add x0, x29, %d\n", var->pos);
        if(debug) fprintf(s, "// </[tratar_id]>\n");
        return T_pLONGO;
    } else if(var->eh_ponteiro) {
        fprintf(s, "  ldr x0, [x29, %d]\n", var->pos);
        if(debug) fprintf(s, "// </[tratar_id]>\n");
        return T_pLONGO;
    } else {
        carregar_valor(s, var);
        if(debug) fprintf(s, "// </[tratar_id]>\n");
        return var->tipo_base;
    }
}

TipoToken tratar_chamada_funcao(FILE* s, int escopo, const char* nome, Funcao* fn) {
    if(debug) fprintf(s, "// <[tratar_chamada_funcao]>\n");
    proximoToken();
    excessao(T_PAREN_ESQ);
    fprintf(s, "  // chamada: %s\n", nome);
    
    int arg_cnt = 0;
    while(L.tk.tipo != T_PAREN_DIR) {
        TipoToken param_tipo = expressao(s, escopo);
        
        if(param_tipo == T_pFLU && fn->retorno == T_pDOBRO) fprintf(s, "  fcvt d0, s0\n");
        else if(param_tipo == T_pDOBRO && fn->retorno == T_pFLU) fprintf(s, "  fcvt s0, d0\n");
        
        if(param_tipo == T_pFLU) fprintf(s, "  str s0, [sp, -16]!\n");
        else if(param_tipo == T_pDOBRO) fprintf(s, "  str d0, [sp, -16]!\n");
        else if(param_tipo == T_pINT || param_tipo == T_pBOOL || param_tipo == T_pCAR) fprintf(s, "  str w0, [sp, -16]!\n");
        else if(param_tipo == T_pLONGO) fprintf(s, "  str x0, [sp, -16]!\n");
        else fprintf(s, "  str x0, [sp, -16]!\n");
        arg_cnt++;
        
        if(L.tk.tipo == T_VIRGULA) proximoToken();
    }
    excessao(T_PAREN_DIR);
    fprintf(s, "  bl %s\n", nome);
    fprintf(s, "  add sp, sp, %d\n", arg_cnt * 16);
    if(debug) fprintf(s, "// </[tratar_chamada_funcao]>\n");
    return fn->retorno;
}

TipoToken tratar_inteiro(FILE* s) {
    if(debug) fprintf(s, "// <[tratar_inteiro]>\n");
    char num[32];
    strcpy(num, L.tk.lex);
    long l_val = L.tk.valor_l;
    proximoToken();
    
    if(l_val < 65536) fprintf(s, "  mov w0, %ld\n", l_val);
    else {
        int titulo = add_const(T_INT, num, 0.0, l_val);
        carregar_const(s, titulo);
    }
    if(debug) fprintf(s, "// </[tratar_inteiro]>\n");
    return T_pINT;
}

TipoToken tratar_flutuante(FILE* s) {
    if(debug) fprintf(s, "// <[tratar_flutuante]>\n");
    char num[32];
    strcpy(num, L.tk.lex);
    double d_val = L.tk.valor_d;
    TipoToken const_tipo = L.tk.tipo;
    proximoToken();
    
    int titulo = add_const(const_tipo, num, d_val, 0);
    carregar_const(s, titulo);
    if(debug) fprintf(s, "// </[tratar_flutuante]>\n");
    return const_tipo == T_FLU ? T_pFLU : T_pDOBRO;
}

TipoToken tratar_caractere(FILE* s) {
    if(debug) fprintf(s, "// <[tratar_caractere]>\n");
    char val = L.tk.lex[0];
    proximoToken();
    fprintf(s, "  mov w0, %d\n", val);
    if(debug) fprintf(s, "// </[tratar_caractere]>\n");
    return T_pCAR;
}

// [BUSCA]:
void coletar_args(FILE* s, Funcao* f) {
    if(debug) fprintf(s, "// <[coletar_args]>\n");
    f->param_pos = 32;
    while(L.tk.tipo != T_PAREN_DIR) {
        declaracao_var(s, &f->param_pos, 0, 1);
        
        if(L.tk.tipo == T_VIRGULA) proximoToken();
        else {
            if(debug) fprintf(s, "// </[coletar_args]>\n");
            break;
        }
    }
}

Variavel* buscar_var(const char* nome, int escopo) {
    if(fn_cnt == 0) return NULL;
    Funcao* f = &funcs[fn_cnt - 1];
    for(int i = f->var_conta - 1; i >= 0; i--) {
        if(strcmp(f->vars[i].nome, nome) == 0 && f->vars[i].escopo <= escopo) {
            return &f->vars[i];
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

void verificar_retorno(FILE* s, int escopo) {
    if(debug) fprintf(s, "// <[verificar_retorno]>\n");
    excessao(T_RETORNAR);
    TipoToken tipo_exp = expressao(s, escopo);
    // em funções que retornam ponteiro/array espera T_pLONGO
    if(funcs[fn_cnt-1].retorno == T_pLONGO) {
        if(tipo_exp != T_pLONGO) fatal("retorno deve ser ponteiro ou endereço");
    } else if (!tipos_compativeis(funcs[fn_cnt-1].retorno, tipo_exp)) {
        char msg[100];
        sprintf(msg, "tipo de retorno incompatível");
        fatal(msg);
    }
    fprintf(s, "  b .epilogo_%d\n", fn_cnt-1);
    excessao(T_PONTO_VIRGULA);
    if(debug) fprintf(s, "// </[verificar_retorno]>\n");
}

void verificar_atribuicao(FILE* s, const char* id, int escopo) {
    if(debug) fprintf(s, "// <[verificar_atribuicao]>\n");
    Variavel* var = buscar_var(id, escopo);
    if(!var) fatal("variável não declarada");
    
    if(var->eh_array) fatal("não é possível armazenar valor direto em array");
    
    excessao(T_IGUAL);
    TipoToken tipo_exp = expressao(s, escopo);
    armazenar_valor(s, var);
    if(debug) fprintf(s, "// </[verificar_atribuicao]>\n");
}

void verificar_se(FILE* s, int escopo) {
    if(debug) fprintf(s, "// <[verificar_se]>\n");
    excessao(T_SE);
    excessao(T_PAREN_ESQ);
    
    TipoToken tipo_cond = expressao(s, escopo);
    
    if(tipo_cond != T_pINT && tipo_cond != T_pBOOL) fatal("condição deve ser inteiro ou booleano");
    
    excessao(T_PAREN_DIR);
    
    int rotulo_falso = escopo_global++;
    fprintf(s, "  cmp w0, 0\n");
    fprintf(s, "  beq .L%d\n", rotulo_falso);
    
    if(L.tk.tipo == T_CHAVE_ESQ) {
        proximoToken();
        while(L.tk.tipo != T_CHAVE_DIR) verificar_stmt(s, &funcs[fn_cnt-1].tamanho_frame, escopo + 1);
        excessao(T_CHAVE_DIR);
    } else verificar_stmt(s, &funcs[fn_cnt-1].tamanho_frame, escopo + 1);
    
    int rotulo_fim = escopo_global++;
    fprintf(s, "  b .L%d\n", rotulo_fim);
    fprintf(s, ".L%d:\n", rotulo_falso);
    
    if(L.tk.tipo == T_SENAO) {
        proximoToken();
        if(L.tk.tipo == T_CHAVE_ESQ) {
            proximoToken();
            while(L.tk.tipo != T_CHAVE_DIR) verificar_stmt(s, &funcs[fn_cnt-1].tamanho_frame, escopo + 1);
            excessao(T_CHAVE_DIR);
        } else verificar_stmt(s, &funcs[fn_cnt-1].tamanho_frame, escopo + 1);
    }
    fprintf(s, ".L%d:\n", rotulo_fim);
    if(debug) fprintf(s, "// </[verificar_se]>\n");
}

void verificar_stmt(FILE* s, int* pos, int escopo) {
    if(debug) fprintf(s, "// <[verificar_stmt]>\n");
    if(escopo == 0) escopo = escopo_global;
    
    while(L.tk.tipo == T_COMENTARIO) proximoToken();
    
    if(L.tk.tipo == T_SE) {
        verificar_se(s, escopo);
        if(debug) fprintf(s, "// </[verificar_stmt]>\n");
        return;
    }
    if(L.tk.tipo == T_INCLUIR) {
        proximoToken();
        
        if(L.tk.tipo != T_TEX) fatal("caminho do arquivo esperado entre aspas");
        
        char caminho[256];
        strcpy(caminho, L.tk.lex);
        proximoToken();
        
        if(L.tk.tipo != T_PONTO_VIRGULA) fatal("ponto e vírgula esperado após o caminho do arquivo");
        
        proximoToken();
        
        FILE* arquivo_include = fopen(caminho, "r");
        if(!arquivo_include) {
            char mensagem_erro[300];
            snprintf(mensagem_erro, sizeof(mensagem_erro), "não foi possível abrir: %s", caminho);
            fatal(mensagem_erro);
        }
        fprintf(s, "\n// início de %s\n", caminho);
        char linha[512];
        while(fgets(linha, sizeof(linha), arquivo_include)) {
            if(strstr(linha, ".section .data") != NULL) {
                fputs(linha, s);
                fputs("  .align 2\n", s);
            } else if(strstr(linha, ": .asciz") != NULL) {
                fputs("  .align 2\n", s);
                fputs(linha, s);
            } else fputs(linha, s);
        }
        fprintf(s, "// fim de %s\n\n", caminho);
        fclose(arquivo_include);
        if(debug) fprintf(s, "// </[verificar_stmt]>\n");
        return;
    }
    if(L.tk.tipo == T_DEF) {
        proximoToken();
        if(L.tk.tipo != T_REG) fatal("registrador esperado");
        char reg[16]; strcpy(reg, L.tk.lex);
        proximoToken(); excessao(T_IGUAL);
        if(L.tk.tipo != T_INT && L.tk.tipo != T_CAR) 
            fatal("valor inteiro ou caractere esperado");
        char val[16]; strcpy(val, L.tk.lex);
        proximoToken(); excessao(T_PONTO_VIRGULA);
        fprintf(s, "  mov %s, %s\n", reg, val);
        if(debug) fprintf(s, "// </[verificar_stmt]>\n");
        return;
    }
    if(L.tk.tipo == T_RETORNAR) {
        verificar_retorno(s, escopo);
        if(debug) fprintf(s, "// </[verificar_stmt]>\n");
        return;
    }
    TipoToken tipos[] = {T_pCAR, T_pINT, T_pFLU, T_pBOOL, T_pDOBRO, T_pLONGO};
    int eh_tipo = 0;
    for(int i=0; i<6; i++) {
        if(L.tk.tipo == tipos[i]) {
            eh_tipo = 1;
            break;
        }
    }
    if(eh_tipo) {
        declaracao_var(s, pos, escopo, 0);
        excessao(T_PONTO_VIRGULA);
        if(debug) fprintf(s, "// </[verificar_stmt]>\n");
        return;
    }
    if(L.tk.tipo == T_ID) {
        char idn[32];
        strcpy(idn, L.tk.lex);
        proximoToken();
        
        if(L.tk.tipo == T_IGUAL) {
            verificar_atribuicao(s, idn, escopo);
            excessao(T_PONTO_VIRGULA);
            if(debug) fprintf(s, "// </[verificar_stmt]>\n");
            return;
        } else if(L.tk.tipo == T_COL_ESQ) {
            Variavel* var = buscar_var(idn, escopo);
            if(!var || !var->eh_array) fatal("não é um array");
            
            excessao(T_COL_ESQ);
            expressao(s, escopo); // índice (resultado em w0)
            fprintf(s, "  mov w1, w0\n"); // salva o índice em w1
            excessao(T_COL_DIR);
            
            if(L.tk.tipo == T_IGUAL) {
                // atribuição a elemento de array
                proximoToken(); // consome '='
                TipoToken tipo_valor = expressao(s, escopo); // valor (resultado em w0)
                if(!tipos_compativeis(var->tipo_base, tipo_valor)) {
                    char msg[100];
                    sprintf(msg, "tipo incompatível: esperado %s, encontrado %s", token_str(var->tipo_base), token_str(tipo_valor));
                    fatal(msg);
                }
                fprintf(s, "  add x2, x29, %d\n", var->pos); // endereço base
                fprintf(s, "  mov x3, %d\n", tam_tipo(var->tipo_base)); // tamanho elemento
                fprintf(s, "  mul w1, w1, w3\n"); // índice * tamanho
                if(tam_tipo(var->tipo_base) == 1) fprintf(s, "  add x2, x2, x1\n"); // bytes: soma direta
                else if(tam_tipo(var->tipo_base) == 4)  fprintf(s, "  add x2, x2, x1, lsl 2\n"); // inteiros: multiplica por 4
                else if(tam_tipo(var->tipo_base) == 8) fprintf(s, "  add x2, x2, x1, lsl 3\n"); // longos: multiplica por 8
                if(var->tipo_base == T_pCAR || var->tipo_base == T_pINT || var->tipo_base == T_pBOOL) fprintf(s, "  strb w0, [x2]\n");
                else if(var->tipo_base == T_pFLU) fprintf(s, "  str s0, [x2]\n");
                else if(var->tipo_base == T_pDOBRO) fprintf(s, "  str d0, [x2]\n");
            } else fatal("acesso a array sem atribuição não é uma declaração válida");
            excessao(T_PONTO_VIRGULA);
            if(debug) fprintf(s, "// </[verificar_stmt]>\n");
            return;
        } else if(L.tk.tipo == T_PAREN_ESQ) {
            excessao(T_PAREN_ESQ);
            
            if(strcmp(idn,"escrever") == 0) {
                while(1) {
                    if(L.tk.tipo == T_ID) {
                        Variavel* var = buscar_var(L.tk.lex, escopo);
                        if(var && var->eh_ponteiro) {
                            if(var->eh_parametro) fprintf(s, "  ldr x0, [x29, %d]\n", var->pos);
                            else fprintf(s, "  ldr x0, [x29, %d]\n", var->pos);
                            escrever_valor(s, T_TEX);
                            proximoToken();
                        } else if(var && var->eh_array && var->tipo_base == T_pCAR) {
                            fprintf(s, "  add x0, x29, %d\n", var->pos);
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
                excessao(T_PONTO_VIRGULA);
                if(debug) fprintf(s, "// </[verificar_stmt]>\n");
                return;
            } else {
                // chamada de função normal
                Funcao* fn = buscar_fn(idn);
                if(!fn) fatal("função não declarada");
                
                int arg_cnt = 0;
                while(L.tk.tipo != T_PAREN_DIR) {
                    expressao(s, escopo);
                    
                    fprintf(s, "  str w0, [sp, -16]!\n");
                    arg_cnt++;
                    
                    if(L.tk.tipo == T_VIRGULA) proximoToken();
                }
                excessao(T_PAREN_DIR);
                excessao(T_PONTO_VIRGULA);
                
                fprintf(s, "  bl %s\n", idn);
                fprintf(s, "  add sp, sp, %d\n", arg_cnt * 16);
                if(debug) fprintf(s, "// </[verificar_stmt]>\n");
                return;
            }
        } else fatal("declaração inválida");
    }
    if(L.tk.tipo == T_CHAVE_ESQ) {
        proximoToken();
        int novo_escopo = ++escopo_global;
        
        while(L.tk.tipo != T_CHAVE_DIR) verificar_stmt(s, pos, novo_escopo);
        proximoToken();
        if(debug) fprintf(s, "// </[verificar_stmt]>\n");
        return;
    }
    fatal("declaração inválida");
}

void verificar_fn(FILE* s) {
    if(debug) fprintf(s, "// <[verificar_fn]>\n");
    TipoToken rt = L.tk.tipo;
    proximoToken();

    int eh_ponteiro = 0;
    int eh_array = 0;
    int eh_prototipo = 0;
    
    if(L.tk.tipo == T_VEZES) {
        eh_ponteiro = 1;
        proximoToken();
    } else if (L.tk.tipo == T_COL_ESQ) {
        eh_array = 1;
        proximoToken();
        excessao(T_COL_DIR);
    }

    if(L.tk.tipo != T_ID) fatal("nome de função esperado");

    char fnome[32];
    strcpy(fnome, L.tk.lex);
    // se é ponteiro ou array, o tipo é T_pLONGO(endereço)
    TipoToken tipo_real = (eh_ponteiro || eh_array) ? T_pLONGO : rt;
    
    funcs[fn_cnt].var_conta = 0;
    funcs[fn_cnt].retorno = tipo_real;
    funcs[fn_cnt].escopo_atual = 0;
    funcs[fn_cnt].tamanho_frame = 0;
    funcs[fn_cnt].param_pos = 16;
    strcpy(funcs[fn_cnt++].nome, fnome);
    proximoToken();

    excessao(T_PAREN_ESQ);
    coletar_args(s, &funcs[fn_cnt - 1]);
    excessao(T_PAREN_DIR);
    // pré definição
    if(L.tk.tipo == T_PONTO_VIRGULA) {
        eh_prototipo = 1;
        proximoToken();
    } else excessao(T_CHAVE_ESQ);

    if(!eh_prototipo) {

    int pos = -16;
    Lexer salvo = L;
    while(L.tk.tipo != T_CHAVE_DIR) {
        if(L.tk.tipo == T_pCAR || L.tk.tipo == T_pINT || L.tk.tipo == T_pFLU || 
            L.tk.tipo == T_pBOOL || L.tk.tipo == T_pDOBRO || L.tk.tipo == T_pLONGO) {
            TipoToken tipo = L.tk.tipo;
            int tam = tam_tipo(tipo);
            int alinhamento = tam_tipo(tipo);
            proximoToken();
            if(L.tk.tipo != T_ID) {
                L = salvo;
                break;
            }
            pos = (pos - tam - alinhamento + 1) & ~(alinhamento - 1);
            proximoToken();
            if(L.tk.tipo == T_IGUAL) {
                while(L.tk.tipo != T_PONTO_VIRGULA) proximoToken();
            }
            if(L.tk.tipo == T_PONTO_VIRGULA) proximoToken();
        } else proximoToken();
    }
    L = salvo;
    
    int frame_tam = ((-pos + 15) & ~15) + 16;
    if(frame_tam < 32) frame_tam = 32;
    funcs[fn_cnt - 1].tamanho_frame = frame_tam;
    
    fprintf(s, ".align 2\n");
    fprintf(s, "%s:\n", fnome);
    fprintf(s, "  stp x29, x30, [sp, -%d]!\n", frame_tam);
    fprintf(s, "  mov x29, sp\n");
    fprintf(s, "  stp x19, x20, [x29, 16]\n");
    
    while(L.tk.tipo != T_CHAVE_DIR) verificar_stmt(s, &pos, 0);
    
    if(funcs[fn_cnt - 1].retorno == T_pVAZIO) fprintf(s, "  mov x0, 0\n");
    fprintf(s, "  b .epilogo_%d\n", fn_cnt - 1);
    
    fprintf(s, ".epilogo_%d:\n", fn_cnt - 1);
    fprintf(s, "  ldp x19, x20, [x29, 16]\n");
    fprintf(s, "  mov sp, x29\n");
    fprintf(s, "  ldp x29, x30, [sp], %d\n", frame_tam);
    fprintf(s, "  ret\n");
    proximoToken();
    }
    if(debug) fprintf(s, "// </[verificar_fn]>\n");
}

// [GERAÇÃO]:
void gerar_prelude(FILE* s) {
    if(debug) fprintf(s, "// <[gerar_prelude]>\n");
    fprintf(s,
        ".section .data\n"
        ".section .text\n"
        ".global _start\n"
         ".align 2\n"
        "_start:\n"
        "  mov x0, 0\n"
        "  mov x1, 0\n"
        "  mov x2, 0\n"
        "  bl inicio\n"
        "  mov x0, 0\n"
        "  mov x8, 93\n"
        "  svc 0\n");
    if(debug) fprintf(s, "// </[gerar_prelude]>\n");
}

void gerar_texs(FILE* s) {
    if(debug) fprintf(s, "// <[gerar_texs]>\n");
    if(tex_cnt == 0) return;
    fprintf(s, ".section .rodata\n");
    fprintf(s, ".align 2\n");
    for(int i = 0; i < tex_cnt; i++) {
        fprintf(s, "%s: .asciz \"%s\"\n", texs[i].nome, texs[i].valor);
    }
    fprintf(s, ".section .text\n\n");
    if(debug) fprintf(s, "// </[gerar_texs]>\n");
}

void gerar_consts(FILE* s) {
    if(debug) fprintf(s, "// <[gerar_consts]>\n");
    if(const_cnt == 0) return;
    
    fprintf(s, "  .section .rodata\n");
    for(int i = 0; i < const_cnt; i++) {
        fprintf(s, "  .align 8\n");
        fprintf(s, "const_%d:\n", i);
        if(constpos[i].tipo == T_INT) fprintf(s, "  .word %ld\n", constpos[i].l_val);
        else if(constpos[i].tipo == T_FLU) {
            float f = (float)constpos[i].d_val;
            fprintf(s, "  .float %f\n", f);
        } else if(constpos[i].tipo == T_DOBRO) fprintf(s, "  .double %f\n", constpos[i].d_val);
    }
    fprintf(s, "  .section .text\n\n");
    if(debug) fprintf(s, "// </[gerar_consts]>\n");
}

void gerar_operacao(FILE* s, TipoToken op, TipoToken tipo) {
    if(debug) fprintf(s, "// <[gerar_operacao]>\n");
    switch(op) {
        case T_MAIS: 
            if(tipo == T_pFLU) fprintf(s, "  fadd s0, s1, s0\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fadd d0, d1, d0\n");
            else fprintf(s, "  add w0, w1, w0\n");
        break;
        case T_MENOS: 
            if(tipo == T_pFLU) fprintf(s, "  fsub s0, s1, s0\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fsub d0, d1, d0\n");
            else fprintf(s, "  sub w0, w1, w0\n");
        break;
        case T_VEZES: 
            if(tipo == T_pFLU) fprintf(s, "  fmul s0, s1, s0\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fmul d0, d1, d0\n");
            else fprintf(s, "  mul w0, w1, w0\n");
        break;
        case T_DIV: 
            if(tipo == T_pFLU) fprintf(s, "  fdiv s0, s1, s0\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fdiv d0, d1, d0\n");
            else fprintf(s, "  sdiv w0, w1, w0\n");
        break;
        default: fatal("operador inválido");
    }
    if(debug) fprintf(s, "// </[gerar_operacao]>\n");
}

void gerar_comparacao(FILE* s, TipoToken op, TipoToken tipo) {
    if(debug) fprintf(s, "// <[gerar_comparacao]>\n");
    switch(op) {
        case T_IGUAL_IGUAL:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, eq\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, eq\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, eq\n");
        break;
        case T_DIFERENTE:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, ne\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, ne\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, ne\n");
        break;
        case T_MAIOR:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, gt\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, gt\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, gt\n");
        break;
        case T_MENOR:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, lt\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, lt\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, lt\n");
        break;
        case T_MAIOR_IGUAL:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, ge\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, ge\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, ge\n");
        break;
        case T_MENOR_IGUAL:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, le\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, le\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, le\n");
        break;
        default: fatal("operador de comparação inválido");
    }
    if(debug) fprintf(s, "// </[gerar_comparacao]>\n");
}

TipoToken converter_tipos(FILE* s, TipoToken tipo_anterior, TipoToken tipo_atual) {
    if(debug) fprintf(s, "// <[converter_tipos]>\n");
    if(tipo_anterior == T_pFLU && tipo_atual == T_pDOBRO) {
        fprintf(s, "  fcvt d1, s1\n");
        fprintf(s, "  fcvt d0, s0\n");
        if(debug) fprintf(s, "// </[converter_tipos]>\n");
        return T_pDOBRO;
    } else if(tipo_anterior == T_pDOBRO && tipo_atual == T_pFLU) {
        fprintf(s, "  fcvt s1, d1\n");
        fprintf(s, "  fcvt s0, d0\n");
        if(debug) fprintf(s, "// </[converter_tipos]>\n");
        return T_pFLU;
    } else if(tipo_anterior == T_pINT && tipo_atual == T_pFLU) {
        fprintf(s, "  scvtf s0, w0\n");
        fprintf(s, "  fcvt d1, s1\n");
        fprintf(s, "  fcvt d0, s0\n");
        if(debug) fprintf(s, "// </[converter_tipos]>\n");
        return T_pDOBRO;
    } else if(tipo_anterior == T_pINT && tipo_atual == T_pDOBRO) {
        fprintf(s, "  scvtf d0, w0\n");
        if(debug) fprintf(s, "// </[converter_tipos]>\n");
        return T_pDOBRO;
    }
    if(debug) fprintf(s, "// </[converter_tipos]>\n");
    // se não precisa de conversão retorna o tipo dominante
    return (tam_tipo(tipo_atual) > tam_tipo(tipo_anterior)) ? tipo_atual : tipo_anterior;
}

void escrever_valor(FILE* s, TipoToken tipo) {
    if(debug) fprintf(s, "// <[escrever_valor]>\n");
    if(tipo == T_pFLU) fprintf(s, "  bl _escrever_flu\n");
    else if(tipo == T_pDOBRO) fprintf(s, "  bl _escrever_double\n");
    else if(tipo == T_pCAR) fprintf(s, "  bl _escrever_car\n");
    else if(tipo == T_pBOOL) fprintf(s, "  bl _escrever_bool\n");
    else if(tipo == T_TEX) fprintf(s, "  bl _escrever_tex\n");
    else fprintf(s, "  bl _escrever_int\n");
    if(debug) fprintf(s, "// </[escrever_valor]>\n");
}

void fim(FILE* s) {
    if(debug) fprintf(s, "// <[fim]>\n");
    fprintf(s,
        "  mov x8,93\n"
        "  mov x0,0\n"
        "  svc 0\n");
    if(debug) fprintf(s, "// </[fim]>\n");
}

void carregar_valor(FILE* s, Variavel* var) {
    if(debug) fprintf(s, "// <[carregar_valor]>\n");
    if(var->eh_ponteiro) fatal("erro interno: carregar_valor chamado para ponteiro");
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
    if(debug) fprintf(s, "// </[carregar_valor]>\n");
}

void armazenar_valor(FILE* s, Variavel* var) {
    if(debug) fprintf(s, "// <[armazenar_valor]>\n");
    if(var->eh_ponteiro) fprintf(s, "  ldr x1, [x29, %d]\n", var->pos);
    else if(var->eh_array) fatal("não é possível armazenar valor direto em array");
    else {
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
    if(debug) fprintf(s, "// </[armazenar_valor]>\n");
}

void carregar_const(FILE* s, int titulo) {
    if(debug) fprintf(s, "// <[carregar_const]>\n");
    Constante* c = &constpos[titulo];
    if(c->tipo == T_FLU) {
        fprintf(s, "  ldr x0, = const_%d\n", titulo);
        fprintf(s, "  ldr s0, [x0]\n");
    } else if(c->tipo == T_DOBRO) {
        fprintf(s, "  ldr x0, = const_%d\n", titulo);
        fprintf(s, "  ldr d0, [x0]\n");
    } else if(c->tipo == T_INT) {
        fprintf(s, "  ldr x0, = const_%d\n", titulo);
        fprintf(s, "  ldr w0, [x0]\n");
    }
    if(debug) fprintf(s, "// </[carregar_const]>\n");
}

int add_const(TipoToken tipo, const char* lex, double d_val, long l_val) {
    for(int i = 0; i < const_cnt; i++) {
        if(tipo == T_FLU && constpos[i].tipo == T_FLU && fabs(constpos[i].d_val - d_val) < 1e-9)
            return constpos[i].titulo;
        if(tipo == T_DOBRO && constpos[i].tipo == T_DOBRO && fabs(constpos[i].d_val - d_val) < 1e-9)
            return constpos[i].titulo;
        if(tipo == T_INT && constpos[i].tipo == T_INT && constpos[i].l_val == l_val)
            return constpos[i].titulo;
    }
    if(const_cnt >= MAX_CONST) fatal("excesso de constpos");
    Constante* c = &constpos[const_cnt];
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
    if(tex_cnt >= MAX_TEX) fatal("excesso de textos");
    Tex* tex = &texs[tex_cnt];
    strcpy(tex->valor, valor);
    sprintf(tex->nome, ".Lstr%d", tex_cnt);
    tex_cnt++;
    return tex_cnt - 1;
}

void empurrar_arg(FILE* s, TipoToken tipo) {
    if(debug) fprintf(s, "// <[empurrar_arg]>\n");
    if(tipo == T_pFLU) fprintf(s, "  str s0, [sp, -16]!\n");
    else if(tipo == T_pDOBRO) fprintf(s, "  str d0, [sp, -16]!\n");
    else {
        int tam = tam_tipo(tipo);
        if(tam <= 4) fprintf(s, "  str w0, [sp, -16]!\n");
        else fprintf(s, "  str x0, [sp, -16]!\n");
    }
    if(debug) fprintf(s, "// </[empurrar_arg]>\n");
}

TipoToken fator(FILE* s, int escopo) {
    if(debug) fprintf(s, "// <[fator]>\n");
    if(L.tk.tipo == T_PAREN_ESQ) {
        proximoToken();
        TipoToken tipo = expressao(s, escopo);
        excessao(T_PAREN_DIR);
        if(debug) fprintf(s, "// </[fator]>\n");
        return tipo;
    } else if(L.tk.tipo == T_ID) {
        if(debug) fprintf(s, "// </[fator]>\n");
        return tratar_id(s, escopo);
    } else if(L.tk.tipo == T_INT) {
        if(debug) fprintf(s, "// </[fator]>\n");
        return tratar_inteiro(s);
    } else if(L.tk.tipo == T_FLU || L.tk.tipo == T_DOBRO) {
        if(debug) fprintf(s, "// </[fator]>\n");
        return tratar_flutuante(s);
    } else if(L.tk.tipo == T_CAR) {
        if(debug) fprintf(s, "// </[fator]>\n");
        return tratar_caractere(s);
    } else if(L.tk.tipo == T_TEX) {
        if(debug) fprintf(s, "// </[fator]>\n");
        return tratar_texto(s);
    } else {
        fatal("fator inválido");
        return T_pINT;
    }
}

TipoToken termo(FILE* s, int escopo) {
    if(debug) fprintf(s, "// <[termo]>\n");
    TipoToken tipo = fator(s, escopo);
    
    while(L.tk.tipo == T_VEZES || L.tk.tipo == T_DIV) {
        TipoToken op = L.tk.tipo;
        proximoToken();
        
        if(tipo == T_pFLU) fprintf(s, "  fmov s1, s0\n");
        else if (tipo == T_pDOBRO) fprintf(s, "  fmov d1, d0\n");
        else fprintf(s, "  mov w1, w0\n");
        
        TipoToken tipo_dir = fator(s, escopo);
        
        tipo = converter_tipos(s, tipo, tipo_dir);
        
        gerar_operacao(s, op, tipo);
    }
    if(debug) fprintf(s, "// </[termo]>\n");
    return tipo;
}

TipoToken expressao(FILE* s, int escopo) {
    if(debug) fprintf(s, "// <[expressao]>\n");
    TipoToken tipo = termo(s, escopo);
    
    while(L.tk.tipo == T_COMENTARIO) proximoToken();
    
    while(L.tk.tipo == T_MAIS || L.tk.tipo == T_MENOS) {
        TipoToken op = L.tk.tipo;
        proximoToken();
        // salva o valor atual
        if(tipo == T_pFLU) fprintf(s, "  fmov s1, s0\n");
        else if(tipo == T_pDOBRO) fprintf(s, "  fmov d1, d0\n");
        else fprintf(s, "  mov w1, w0\n");
        
        TipoToken tipo_dir = termo(s, escopo);
        // conversão de tipos
        tipo = converter_tipos(s, tipo, tipo_dir);
        
        gerar_operacao(s, op, tipo);
    }
    if(L.tk.tipo == T_IGUAL_IGUAL || L.tk.tipo == T_DIFERENTE || 
        L.tk.tipo == T_MAIOR || L.tk.tipo == T_MENOR ||
        L.tk.tipo == T_MAIOR_IGUAL || L.tk.tipo == T_MENOR_IGUAL) {
        
        TipoToken op = L.tk.tipo;
        proximoToken();
        // salva primeiro operando
        if(tipo == T_pFLU) fprintf(s, "  fmov s1, s0\n");
        else if (tipo == T_pDOBRO) fprintf(s, "  fmov d1, d0\n");
        else fprintf(s, "  mov w1, w0\n");
        //  segundo operando
        TipoToken tipo_dir = termo(s, escopo);
        // converte tipos se prexisar
        tipo = converter_tipos(s, tipo, tipo_dir);
        
        gerar_comparacao(s, op, tipo);
        tipo = T_pBOOL;
    }
    if(debug) fprintf(s, "// </[expressao]>\n");
    return tipo;
}

void declaracao_var(FILE* s, int* pos, int escopo, int eh_parametro) {
    if(debug) fprintf(s, "// <[declaracao_var]>\n");
    TipoToken tipo_base = L.tk.tipo;
    int eh_ponteiro = 0;
    int eh_array = 0;
    int tam_array = 0;

    proximoToken();

    if(L.tk.tipo == T_VEZES) {
        eh_ponteiro = 1;
        proximoToken();
    } else if(L.tk.tipo == T_COL_ESQ) {
        eh_array = 1;
        proximoToken();
        if(L.tk.tipo == T_INT) {
            tam_array = L.tk.valor_l;
            proximoToken();
        }
        excessao(T_COL_DIR);
    }
    
    int tam = eh_ponteiro ? 8 : (eh_array ? 
        (tam_array == 0 ? tam_tipo(tipo_base) : tam_array * tam_tipo(tipo_base)) : 
        tam_tipo(tipo_base));
    
    int alinhamento = eh_ponteiro ? 8 : tam_tipo(tipo_base);

    if(!eh_parametro) {
        *pos = *pos - tam;
        *pos = *pos & ~15;
    }

    if(L.tk.tipo != T_ID) fatal("nome de variável esperado");

    Funcao* f = &funcs[fn_cnt - 1];
    if(f->var_conta >= MAX_VAR) fatal("excesso de variáveis");

    Variavel* var = &f->vars[f->var_conta];
    strcpy(var->nome, L.tk.lex);
    var->tipo_base = tipo_base;
    var->eh_ponteiro = eh_ponteiro;
    var->eh_array = eh_array;
    var->tam_array = tam_array;
    var->pos = *pos;
    var->escopo = escopo;
    var->eh_parametro = eh_parametro;
    f->var_conta++;

    proximoToken();

    if(L.tk.tipo == T_IGUAL) {
        proximoToken();
        
        if(eh_array && tipo_base == T_pCAR && L.tk.tipo == T_TEX) {
            const char* texto_valor = L.tk.lex;
            fprintf(s, "  add x0, x29, %d\n", var->pos);
            for(int i = 0; i <= strlen(texto_valor); i++) {
                fprintf(s, "  mov w1, %d\n", texto_valor[i]);
                fprintf(s, "  strb w1, [x0, %d]\n", i);
            }
            proximoToken();
        } else if(eh_array) {
            excessao(T_CHAVE_ESQ);
            int i = 0;
            while(L.tk.tipo != T_CHAVE_DIR) {
                if(tam_array > 0 && i >= tam_array) fatal("excesso de elementos");
                TipoToken tipo_valor = expressao(s, escopo);
                if(tipo_valor != tipo_base) fatal("tipo incompatível");
                int pos = var->pos + i * tam_tipo(tipo_base);
                if(pos >= 0) {
                    if(tipo_base == T_pCAR || tipo_base == T_pINT || tipo_base == T_pBOOL) fprintf(s, "  strb w0, [x29, %d]\n", pos);
                    else if(tipo_base == T_pFLU) fprintf(s, "  str s0, [x29, %d]\n", pos);
                    else if(tipo_base == T_pDOBRO) fprintf(s, "  str d0, [x29, %d]\n", pos);
                } else {
                    fprintf(s, "  mov x1, %d\n", pos);
                    if(tipo_base == T_pCAR || tipo_base == T_pINT || tipo_base == T_pBOOL) fprintf(s, "  strb w0, [x29, x1]\n");
                    else if(tipo_base == T_pFLU) fprintf(s, "  str s0, [x29, x1]\n");
                    else if(tipo_base == T_pDOBRO) fprintf(s, "  str d0, [x29, x1]\n");
                }
                i++;
                if(L.tk.tipo == T_VIRGULA) proximoToken();
            }
            excessao(T_CHAVE_DIR);
            if(var->tam_array == 0) var->tam_array = i;
        } else if(eh_ponteiro && L.tk.tipo == T_TEX) {
            int id_tex = add_tex(L.tk.lex);
            fprintf(s, "  ldr x0, = %s\n", texs[id_tex].nome);
            if(var->pos >= 0) fprintf(s, "  str x0, [x29, %d]\n", var->pos);
            else {
                fprintf(s, "  mov x1, %d\n", var->pos);
                fprintf(s, "  str x0, [x29, x1]\n");
            }
            proximoToken();
        } else if(eh_ponteiro) fatal("esperado texto para ponteiro");
        else {
            TipoToken tipo_exp = expressao(s, escopo);
            armazenar_valor(s, var);
        }
    }
    if(debug) fprintf(s, "// </[declaracao_var]>\n");
    if(eh_parametro) *pos += 16;
}


int main(int argc, char** argv) {
    if(argc < 2) {
        printf("FPB: sem arquivos de entrada\n");
        return 1;
    }
    if((strcmp(argv[3], "-debug") == 0 && argc >= 4) || (strcmp(argv[5], "-debug") == 0 && argc >= 6)) {
        debug = 1;
        printf("[DEBUG]: ativado\n\n");
    }
    if(strcmp(argv[1], "-ajuda") == 0) {
        printf("[informação]:\n");
        printf("fpb -v : versão e o distribuidor\n\n");
        printf("fpb -c : configurações do compilador\n\n");
        printf("[depuração]:\n");
        printf("fpb ola -asm -debug : compila na pasta atual e ativa marcações no codigo assembly gerado\n\n");
        printf("fpb ola -s pasta/ola -asm -debug : compila e ativa marcações no codigo assembly gerado\n\n");
        printf("[compilação]:\n");
        printf("fpb exemplo : compila um arquivo.fpb e gera o binário na pasta atual\n\n");
        printf("fpb exemplo -s pasta/exemplo : compila um arquivo.fpb e cria um arquivo em um caminho personalizavel\n\n");
        printf("fpb exemplo -asm : compila mantendo o ASM intermediario na pasta atual\n\n");
        printf("fpb exemplo -s pasta/exemplo -asm : compila mantendo o ASM intermediario na pasta do binário\n");
        return 0;
    }
    if(strcmp(argv[1], "-v") == 0) {
        printf("[FOCA-DO ESTÚDIOS]\nFPB (Fácil Programação Baixo nivel) - v0.0.1 (alpha)\n");
        return 0;
    }
    if(strcmp(argv[1], "-c") == 0) {
        printf("[configuração]:\n");
        printf("max codigo: %i\n", MAX_CODIGO);
        printf("max variaveis: %i\n", MAX_VAR);
        printf("max constpos: %i\n", MAX_CONST);
        printf("max texs: %i\n", MAX_TEX);
        printf("max funcoes: %i\n", MAX_FN);
        printf("max parametros: %i\n", MAX_PARAMS);
        printf("arquitetura: AARCH64(ARM64)");
        printf("sistema: LINUX(ANDROID)");
        return 0;
    }

    char asm_s[128], asm_o[128], cmd[256], nomeArquivo[256];
    arquivoAtual = argv[1];
    
    int manter_asm = ((argc >= 3 && strcmp(argv[2], "-asm") == 0) || (argc >= 5 && strcmp(argv[4], "-asm") == 0));

    snprintf(nomeArquivo, sizeof(nomeArquivo), "%s.fpb", argv[1]);
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

    if(argc >= 4 && strcmp(argv[2], "-s") == 0) snprintf(asm_s, sizeof(asm_s), "%s.asm", argv[3]);
    else snprintf(asm_s, sizeof(asm_s), "%s.asm", argv[1]);

    FILE* s = fopen(asm_s, "w");
    if(debug) fprintf(s, "// ! AARCH64(ARM64) LINUX(ANDROID) !\n");
    gerar_prelude(s);

    while(L.tk.tipo != T_FIM) {
        if(L.tk.tipo == T_INCLUIR) {
            int pos = 0;
            verificar_stmt(s, &pos, 0);
        } else verificar_fn(s);
    }
    
    gerar_consts(s);
    gerar_texs(s);

    fclose(s);

    snprintf(asm_o, sizeof(asm_o), "%s.o", argv[1]);
    snprintf(cmd, sizeof(cmd), "as %s -o %s", asm_s, asm_o);
    if(system(cmd)) return 3;

    if(argc >= 4 && strcmp(argv[2], "-s") == 0) snprintf(cmd, sizeof(cmd), "ld %s -o %s", asm_o, argv[3]);
    else snprintf(cmd, sizeof(cmd), "ld %s -o %s", asm_o, argv[1]);
    if(system(cmd)) return 4;

    if(!manter_asm) remove(asm_s);
    remove(asm_o);
    free(buf);
    return 0;
}
