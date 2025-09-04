#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_TOK 512 // maximo de tolens
#define MAX_CODIGO 8192 // maximo de codhgk
#define MAX_FN 64 // maximo de funções
#define MAX_VAR 128 // maximo de variaveis
#define MAX_CONST 128 // maxmio de constantes
#define MAX_PARAMS 8 // maximo de parametros
#define MAX_TEX 128 // mqximo de textos

typedef enum {
    // tipos:
    T_ID, T_INT, T_TEX, T_CAR, T_FLU, T_DOBRO, T_COMENTARIO,
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
    T_pCAR, T_pINT, T_pFLU, T_pBOOL, T_pDOBRO,
    T_pLONGO, T_pVAZIO,
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
    double valor_d; // para constantes de ponto flutuante
    long valor_l; // para constantes inteiras grandes
    Posicao pos;
} Token;

typedef struct {
    char nome[32];
    TipoToken tipo;
    int antes;
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
    int param_antes; 
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
static Constante constantes[MAX_CONST];
static int const_cnt = 0;
static Tex texs[MAX_TEX];
static int tex_cnt = 0;
static char* arquivoAtual;

// buscar
Variavel* buscar_var(const char* nome, int escopo);
Funcao* buscar_fn(const char* nome);
// carregar
void carregar_valor(FILE* s, Variavel* var);
void carregar_const(FILE* s, int titulo);
// declaracao
void declaracao_var(FILE* s, int* antes, int escopo, int eh_parametro);
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
void verificar_stmt(FILE* s, int* antes, int escopo);
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
    if(!c) { L.tk.tipo = T_FIM; return; }

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
        if(strcmp(L.tk.lex, "incluir") == 0) { L.tk.tipo = T_INCLUIR; return; }
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
    int id = add_tex(L.tk.lex);
    fprintf(s, "  ldr x0, =%s\n", texs[id].nome);
    proximoToken();
    return T_TEX;
}

TipoToken tratar_id(FILE* s, int escopo) {
    char id[32];
    strcpy(id, L.tk.lex);
    Variavel* var = buscar_var(id, escopo);
    
    if(!var) {
        Funcao* fn = buscar_fn(id);
        if(fn) return tratar_chamada_funcao(s, escopo, id, fn);
        else fatal("variável ou função não declarada");
    }
    proximoToken();
    carregar_valor(s, var);
    return var->tipo;
}

TipoToken tratar_chamada_funcao(FILE* s, int escopo, const char* nome, Funcao* fn) {
    proximoToken();
    excessao(T_PAREN_ESQ);
    fprintf(s, "  // chamada: %s\n", nome);
    
    int arg_cnt = 0;
    while(L.tk.tipo != T_PAREN_DIR) {
        TipoToken param_tipo = expressao(s, escopo);
        
        if(param_tipo == T_pFLU && fn->retorno == T_pDOBRO) fprintf(s, "  fcvt d0, s0\n");
        else if(param_tipo == T_pDOBRO && fn->retorno == T_pFLU) fprintf(s, "  fcvt s0, d0\n");
        
        fprintf(s, "  str w0, [sp, -16]!\n");
        arg_cnt++;
        
        if(L.tk.tipo == T_VIRGULA) proximoToken();
    }
    excessao(T_PAREN_DIR);
    fprintf(s, "  bl %s\n", nome);
    fprintf(s, "  add sp, sp, %d\n", arg_cnt * 16);
    return fn->retorno;
}

TipoToken tratar_inteiro(FILE* s) {
    char num[32];
    strcpy(num, L.tk.lex);
    long l_val = L.tk.valor_l;
    proximoToken();
    
    if(l_val < 65536) fprintf(s, "  mov w0, %ld\n", l_val);
    else {
        int titulo = add_const(T_INT, num, 0.0, l_val);
        carregar_const(s, titulo);
    }
    return T_pINT;
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
    return T_pINT;
}

// [BUSCA]:
void coletar_args(FILE* s, Funcao* f) {
    f->param_antes = 32;
    while(L.tk.tipo != T_PAREN_DIR) {
        declaracao_var(s, &f->param_antes, 0, 1);
        
        if(L.tk.tipo == T_VIRGULA) proximoToken();
        else break;
    }
}

Variavel* buscar_var(const char* nome, int escopo) {
    if(fn_cnt == 0) return NULL;
    Funcao* f = &funcs[fn_cnt-1];
    for(int i = f->var_conta-1; i >= 0; i--) {
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
    excessao(T_RETORNAR);
    expressao(s, escopo);
    
    fprintf(s, "  b .epilogo_%d\n", fn_cnt-1);
    excessao(T_PONTO_VIRGULA);
}

void verificar_atribuicao(FILE* s, const char* id, int escopo) {
    Variavel* var = buscar_var(id, escopo);
    if(!var) fatal("variável não declarada");
    
    excessao(T_IGUAL);
    
    TipoToken tipo_exp = expressao(s, escopo);
    
    armazenar_valor(s, var);
}

void verificar_se(FILE* s, int escopo) {
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
}

void verificar_stmt(FILE* s, int* antes, int escopo) {
    if(escopo == 0) escopo = escopo_global;
    
    while(L.tk.tipo == T_COMENTARIO) proximoToken();
    
    if(L.tk.tipo == T_SE) {
        verificar_se(s, escopo);
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
        return;
    }
    if(L.tk.tipo == T_RETORNAR) {
        verificar_retorno(s, escopo);
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
        declaracao_var(s, antes, escopo, 0);
        excessao(T_PONTO_VIRGULA);
        return;
    }
    if(L.tk.tipo == T_ID) {
        char idn[32];
        strcpy(idn, L.tk.lex);
        proximoToken();
        
        if(L.tk.tipo == T_IGUAL) {
            verificar_atribuicao(s, idn, escopo);
            excessao(T_PONTO_VIRGULA);
            return;
        }
        excessao(T_PAREN_ESQ);
        
        if(strcmp(idn,"fim") == 0) {
            excessao(T_PAREN_DIR);
            excessao(T_PONTO_VIRGULA);
            fim(s);
            return;
        }
        if(strcmp(idn,"escrever") == 0) {
            while(1) {
                TipoToken tipo_arg = expressao(s, escopo);
                escrever_valor(s, tipo_arg);
                
                if(L.tk.tipo == T_VIRGULA) {
                    proximoToken();
                    continue;
                }
                break;
            }
            excessao(T_PAREN_DIR); 
            excessao(T_PONTO_VIRGULA);
            return;
        }
        if(strcmp(idn, "svc") == 0) {
            if(L.tk.tipo != T_INT) fatal("código de serviço esperado");
            char num[16]; strcpy(num, L.tk.lex);
            proximoToken(); 
            excessao(T_PAREN_DIR); 
            excessao(T_PONTO_VIRGULA);
            fprintf(s, "  mov x8, %s\nsvc 0\n", num);
            return;
        }
        
        Funcao* fn = buscar_fn(idn);
        if(!fn) fatal("função não declarada");
        // conta argumentos
        Lexer salvo = L;
        
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
        return;
    }
    if(L.tk.tipo == T_CHAVE_ESQ) {
        proximoToken();
        int novo_escopo = ++escopo_global;
        
        while(L.tk.tipo != T_CHAVE_DIR) verificar_stmt(s, antes, novo_escopo);
        proximoToken();
        return;
    }
    fatal("declaração inválida");
}

void verificar_fn(FILE* s) {
    TipoToken rt = L.tk.tipo;
    proximoToken();

    if(L.tk.tipo != T_ID) fatal("nome de função esperado");

    char fnome[32];
    strcpy(fnome, L.tk.lex);
    
    int eh_prototipo = 0;
    
    funcs[fn_cnt].var_conta = 0;
    funcs[fn_cnt].retorno = rt;
    funcs[fn_cnt].escopo_atual = 0;
    funcs[fn_cnt].tamanho_frame = 0;
    funcs[fn_cnt].param_antes = 16;
    strcpy(funcs[fn_cnt++].nome, fnome);
    proximoToken();

    excessao(T_PAREN_ESQ);
    coletar_args(s, &funcs[fn_cnt-1]);
    excessao(T_PAREN_DIR);

    // pré definição
    if(L.tk.tipo == T_PONTO_VIRGULA) {
        eh_prototipo = 1;
        proximoToken();
    } else excessao(T_CHAVE_ESQ);

    if(!eh_prototipo) {
    // calcula tamanho do frame antes de gerar prólogo
    int antes = 0;
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
            antes = (antes - tam - alinhamento + 1) & ~(alinhamento - 1);
            proximoToken();
            if(L.tk.tipo == T_IGUAL) {
                while (L.tk.tipo != T_PONTO_VIRGULA) 
                proximoToken();
            }
            if(L.tk.tipo == T_PONTO_VIRGULA) proximoToken();
        } else proximoToken();
    }
    L = salvo;
    
    int frame_tam = ((-antes + 15) & ~15) + 32;
    if(frame_tam < 16) frame_tam = 16;
    funcs[fn_cnt-1].tamanho_frame = frame_tam;
    
    fprintf(s, ".align 2\n");
    fprintf(s, "%s:\n", fnome);
    fprintf(s, "  stp x29, x30, [sp, -%d]!\n", frame_tam);
    fprintf(s, "  mov x29, sp\n");
    fprintf(s, "  stp x19, x20, [sp, -16]!\n");
    
    antes = 0;
    while(L.tk.tipo != T_CHAVE_DIR) verificar_stmt(s, &antes, 0);
    fprintf(s, ".epilogo_%d:\n", fn_cnt-1);
    fprintf(s, "  ldp x19, x20, [sp], 16\n");
    fprintf(s, "  mov sp, x29\n");
    fprintf(s, "  ldp x29, x30, [sp], %d\n", frame_tam);
    fprintf(s, "  ret\n");
    proximoToken();
    }
}

// [GERAÇÃO]:
void gerar_prelude(FILE* s) {
    fprintf(s,
        ".section .data\n"
        ".section .text\n"
        ".global _start\n"
         ".align 2\n"
        "_start:\n"
        "  bl inicio\n\n");
}

void gerar_texs(FILE* s) {
    if(tex_cnt == 0) return;
    fprintf(s, "  .section .rodata\n");
    for(int i = 0; i < tex_cnt; i++) {
        fprintf(s, "  .align 2\n");
        fprintf(s, "%s: .asciz \"%s\"\n", texs[i].nome, texs[i].valor);
    }
    fprintf(s, "  .section .text\n\n");
}

void gerar_consts(FILE* s) {
    if(const_cnt == 0) return;
    
    fprintf(s, "  .section .rodata\n");
    for(int i=0; i<const_cnt; i++) {
        fprintf(s, "  .align 8\n");
        fprintf(s, "const_%d:\n", i);
        if(constantes[i].tipo == T_INT) fprintf(s, "  .word %ld\n", constantes[i].l_val);
        else if(constantes[i].tipo == T_FLU) {
            float f = (float)constantes[i].d_val;
            fprintf(s, "  .float %f\n", f);
        } else if(constantes[i].tipo == T_DOBRO) fprintf(s, "  .double %f\n", constantes[i].d_val);
    }
    fprintf(s, "  .section .text\n\n");
}

void gerar_operacao(FILE* s, TipoToken op, TipoToken tipo) {
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
}

void gerar_comparacao(FILE* s, TipoToken op, TipoToken tipo) {
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
}

TipoToken converter_tipos(FILE* s, TipoToken tipo_anterior, TipoToken tipo_atual) {
    if(tipo_anterior == T_pFLU && tipo_atual == T_pDOBRO) {
        fprintf(s, "  fcvt d1, s1\n");
        fprintf(s, "  fcvt d0, s0\n");
        return T_pDOBRO;
    } else if(tipo_anterior == T_pDOBRO && tipo_atual == T_pFLU) {
        fprintf(s, "  fcvt s1, d1\n");
        fprintf(s, "  fcvt s0, d0\n");
        return T_pFLU;
    } else if(tipo_anterior == T_pINT && tipo_atual == T_pFLU) {
        fprintf(s, "  scvtf s0, w0\n");
        fprintf(s, "  fcvt d1, s1\n");
        fprintf(s, "  fcvt d0, s0\n");
        return T_pDOBRO;
    } else if(tipo_anterior == T_pINT && tipo_atual == T_pDOBRO) {
        fprintf(s, "  scvtf d0, w0\n");
        return T_pDOBRO;
    }
    // se não precisa de conversão retorna o tipo dominante
    return (tam_tipo(tipo_atual) > tam_tipo(tipo_anterior)) ? tipo_atual : tipo_anterior;
}

void escrever_valor(FILE* s, TipoToken tipo) {
    if(tipo == T_pFLU) fprintf(s, "  bl _escrever_flu\n");
    else if(tipo == T_pDOBRO) fprintf(s, "  bl _escrever_double\n");
    else if(tipo == T_pCAR) fprintf(s, "  bl _escrever_car\n");
    else if(tipo == T_pBOOL) fprintf(s, "  bl _escrever_bool\n");
    else if(tipo == T_TEX) fprintf(s, "  bl _escrever_tex\n");
    else fprintf(s, "  bl _escrever_int\n");
}

void fim(FILE* s) {
    fprintf(s,
        "  mov x8,93\n"
        "  mov x0,0\n"
        "  svc 0\n");
}

void carregar_valor(FILE* s, Variavel* var) {
    if(var->eh_parametro) {
        if(var->tipo == T_pFLU) fprintf(s, "  ldr s0, [x29, %d]\n", var->antes);
        else if(var->tipo == T_pDOBRO) fprintf(s, "  ldr d0, [x29, %d]\n", var->antes);
        else fprintf(s, "  ldr w0, [x29, %d]\n", var->antes);
    } else {
        switch(tam_tipo(var->tipo)) {
            case 1: fprintf(s, "  ldrb w0, [x29, %d]\n", var->antes);
            break;
            case 4: 
                if(var->tipo == T_pFLU) fprintf(s, "  ldr s0, [x29, %d]\n", var->antes);
                else fprintf(s, "  ldr w0, [x29, %d]\n", var->antes);
            break;
            case 8: 
                if(var->tipo == T_pDOBRO) fprintf(s, "  ldr d0, [x29, %d]\n", var->antes);
                else fprintf(s, "  ldr x0, [x29, %d]\n", var->antes);
            break;
        }
    }
}

void armazenar_valor(FILE* s, Variavel* var) {
    if(var->eh_parametro) {
        if(var->tipo == T_pFLU) fprintf(s, "  str s0, [x29, %d]\n", var->antes);
        else if(var->tipo == T_pDOBRO) fprintf(s, "  str d0, [x29, %d]\n", var->antes);
        else fprintf(s, "  str w0, [x29, %d]\n", var->antes);
    } else {
        switch(tam_tipo(var->tipo)) {
            case 1: fprintf(s, "  strb w0, [x29, %d]\n", var->antes);
            break;
            case 4: 
                if(var->tipo == T_pFLU) fprintf(s, "  str s0, [x29, %d]\n", var->antes);
                else fprintf(s, "  str w0, [x29, %d]\n", var->antes);
            break;
            case 8: 
                if(var->tipo == T_pDOBRO) fprintf(s, "  str d0, [x29, %d]\n", var->antes);
                else fprintf(s, "  str x0, [x29, %d]\n", var->antes);
            break;
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
    } else if(c->tipo == T_INT) {
        fprintf(s, "  ldr x0, = const_%d\n", titulo);
        fprintf(s, "  ldr w0, [x0]\n");
    }
}

int add_const(TipoToken tipo, const char* lex, double d_val, long l_val) {
    for(int i=0; i<const_cnt; i++) {
        if(tipo == T_FLU && constantes[i].tipo == T_FLU && fabs(constantes[i].d_val - d_val) < 1e-9)
            return constantes[i].titulo;
        if(tipo == T_DOBRO && constantes[i].tipo == T_DOBRO && fabs(constantes[i].d_val - d_val) < 1e-9)
            return constantes[i].titulo;
        if(tipo == T_INT && constantes[i].tipo == T_INT && constantes[i].l_val == l_val)
            return constantes[i].titulo;
    }
    if(const_cnt >= MAX_CONST) fatal("excesso de constantes");
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
    if(tex_cnt >= MAX_TEX) fatal("excesso de strings");
    Tex* tex = &texs[tex_cnt];
    strcpy(tex->valor, valor);
    sprintf(tex->nome, ".Lstr%d", tex_cnt);
    tex_cnt++;
    return tex_cnt - 1;
}

TipoToken fator(FILE* s, int escopo) {
    if(L.tk.tipo == T_PAREN_ESQ) {
        proximoToken();
        TipoToken tipo = expressao(s, escopo);
        excessao(T_PAREN_DIR);
        return tipo;
    }
    else if(L.tk.tipo == T_ID) return tratar_id(s, escopo);
    else if(L.tk.tipo == T_INT) return tratar_inteiro(s);
    else if(L.tk.tipo == T_FLU || L.tk.tipo == T_DOBRO) return tratar_flutuante(s);
    else if(L.tk.tipo == T_CAR) return tratar_caractere(s);
    else if(L.tk.tipo == T_TEX) return tratar_texto(s);
    else {
        fatal("fator inválido");
        return T_pINT;
    }
}

TipoToken termo(FILE* s, int escopo) {
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
    
    return tipo;
}

TipoToken expressao(FILE* s, int escopo) {
    TipoToken tipo = termo(s, escopo);
    
    while(L.tk.tipo == T_COMENTARIO) proximoToken();
    
    while(L.tk.tipo == T_MAIS || L.tk.tipo == T_MENOS) {
        TipoToken op = L.tk.tipo;
        proximoToken();
        // salva o valor atual
        if (tipo == T_pFLU) fprintf(s, "  fmov s1, s0\n");
        else if (tipo == T_pDOBRO) fprintf(s, "  fmov d1, d0\n");
        else fprintf(s, "  mov w1, w0\n");
        
        TipoToken tipo_dir = termo(s, escopo);
        // conversão de tipos
        tipo = converter_tipos(s, tipo, tipo_dir);
        
        gerar_operacao(s, op, tipo);
    }
    
    // Operadores de comparação - CORREÇÃO DA SINTAXE
    if(L.tk.tipo == T_IGUAL_IGUAL || L.tk.tipo == T_DIFERENTE || 
        L.tk.tipo == T_MAIOR || L.tk.tipo == T_MENOR ||
        L.tk.tipo == T_MAIOR_IGUAL || L.tk.tipo == T_MENOR_IGUAL) {
        
        TipoToken op = L.tk.tipo;
        proximoToken();
        
        // Salvar primeiro operando
        if (tipo == T_pFLU) fprintf(s, "  fmov s1, s0\n");
        else if (tipo == T_pDOBRO) fprintf(s, "  fmov d1, d0\n");
        else fprintf(s, "  mov w1, w0\n");
        
        // Avaliar segundo operando
        TipoToken tipo_dir = termo(s, escopo);
        
        // Converter tipos se necessário
        tipo = converter_tipos(s, tipo, tipo_dir);
        
        // Gerar comparação
        gerar_comparacao(s, op, tipo);
        
        tipo = T_pBOOL;  // Resultado é booleano
    }
    
    return tipo;
}

void declaracao_var(FILE* s, int* antes, int escopo, int eh_parametro) {
    TipoToken tipo = L.tk.tipo;
    int tam = tam_tipo(tipo);
    int alinhamento = tam_tipo(tipo);
    
    if(tam == 0) fatal("tipo inválido");
    
    if(!eh_parametro) *antes = (*antes - tam - alinhamento + 1) & ~(alinhamento - 1);
    
    proximoToken();
    
    if(L.tk.tipo != T_ID) fatal("nome de variável esperado");
    
    Funcao* f = &funcs[fn_cnt-1];
    if(f->var_conta >= MAX_VAR) fatal("excesso de variáveis");
    
    Variavel* var = &f->vars[f->var_conta];
    strcpy(var->nome, L.tk.lex);
    var->tipo = tipo;
    var->antes = *antes;
    var->escopo = escopo;
    var->eh_parametro = eh_parametro;
    f->var_conta++;
    
    if(eh_parametro) *antes += 16;
    
    proximoToken();
    
    if(!eh_parametro && L.tk.tipo == T_IGUAL) verificar_atribuicao(s, var->nome, escopo);
}

int main(int argc, char** argv) {
    if(argc < 2) {
        printf("FPB: sem arquivos de entrada\n");
        return 1;
    }
    if(strcmp(argv[1], "-v") == 0) {
        printf("[FOCA-DO ESTÚDIOS]\nFPB - v0.0.1 (alpha)\n");
        return 0;
    }
    if(strcmp(argv[1], "-c") == 0) {
        printf("[configuração]:\n");
        printf("max codigo: %i\n", MAX_CODIGO);
        printf("max variaveis: %i\n", MAX_VAR);
        printf("max constantes: %i\n", MAX_CONST);
        printf("max texs: %i\n", MAX_TEX);
        printf("max funcoes: %i\n", MAX_FN);
        printf("max parametros: %i\n", MAX_PARAMS);
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
    gerar_prelude(s);

    while(L.tk.tipo != T_FIM) {
        if(L.tk.tipo == T_INCLUIR) {
            int antes = 0;
            verificar_stmt(s, &antes, 0);
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
