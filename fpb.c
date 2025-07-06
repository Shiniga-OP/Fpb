#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_TOK 512
#define MAX_SRC 8192
#define MAX_FN  64
#define MAX_VAR 128
#define MAX_CONST 128
#define MAX_PARAMS 8

typedef enum {
  T_ID, T_INT, T_TEX, T_CAR, T_FLU, T_DOBRO,
  T_PAREN_ESQ, T_PAREN_DIR,  
  T_COL_ESQ, T_COL_DIR,  
  T_PONTO_VIRGULA, T_VIRGULA,  
  T_IGUAL, T_MAIS, T_MENOS, T_VEZES, T_DIV,  
  T_pCAR, T_pINT, T_pFLU, T_pBOOL, T_pDOBRO, T_pLONGO, T_pVAZIO,
  T_DEF, T_REG, T_FIM, T_RETORNAR
} TipoToken;

typedef struct {
    TipoToken tipo;
    char lex[MAX_TOK];
    double valor_d; // Para constantes de ponto flutuante
    long valor_l;   // Para constantes inteiras grandes
} Token;

typedef struct {
    char nome[32];
    TipoToken tipo;
    int offset;
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
    int param_offset;
} Funcao;

typedef struct {
    const char *src;
    size_t pos;
    Token tk;
} Lexer;

typedef struct {
    TipoToken tipo;
    char lex[32];
    double d_val;
    long l_val;
    int label;
} Constante;

static Lexer L;
static Funcao funcs[MAX_FN];
static int fn_cnt = 0;
static int escopo_global = 0;
static Constante constantes[MAX_CONST];
static int const_cnt = 0;

int tamanho_tipo(TipoToken t) {
    switch(t) {
        case T_pCAR: case T_pBOOL: return 1;
        case T_pINT: case T_pFLU: return 4;
        case T_pDOBRO: case T_pLONGO: return 8;
        default: return 0;
    }
}

int alinhamento_tipo(TipoToken t) {
    switch(t) {
        case T_pCAR: case T_pBOOL: return 1;
        case T_pINT: case T_pFLU: return 4;
        case T_pDOBRO: case T_pLONGO: return 8;
        default: return 8;
    }
}

void fatal(const char *m) {
    fprintf(stderr, "Erro: %s próximo de \"%s\"\n", m, L.tk.lex);
    exit(1);
}

void proximoToken() {
    char c;
    while(isspace(c = L.src[L.pos])) L.pos++;
    if(!c) {
        L.tk.tipo = T_FIM;
        return;
    }
    if(isalpha(c) || c == '_') {
        int i = 0;
        while(isalnum(c = L.src[L.pos]) || c == '_') {
            L.tk.lex[i++] = c;
            L.pos++;
        }
        L.tk.lex[i] = 0;
        if(strcmp(L.tk.lex, "car") == 0) L.tk.tipo = T_pCAR;
        else if(strcmp(L.tk.lex, "int") == 0) L.tk.tipo = T_pINT;
        else if(strcmp(L.tk.lex, "flu") == 0) L.tk.tipo = T_pFLU;
        else if(strcmp(L.tk.lex, "bool") == 0) L.tk.tipo = T_pBOOL;
        else if(strcmp(L.tk.lex, "dobro") == 0) L.tk.tipo = T_pDOBRO;
        else if(strcmp(L.tk.lex, "longo") == 0) L.tk.tipo = T_pLONGO;
        else if(strcmp(L.tk.lex, "vazio") == 0) L.tk.tipo = T_pVAZIO;
        else if(strcmp(L.tk.lex, "def") == 0) L.tk.tipo = T_DEF;
        else if(strcmp(L.tk.lex, "retornar") == 0) L.tk.tipo = T_RETORNAR;
        else if((L.tk.lex[0]=='x' || L.tk.lex[0]=='w') && isdigit(L.tk.lex[1])) {
            int ok=1;
            for(int j=1; L.tk.lex[j]; j++) 
                if(!isdigit(L.tk.lex[j])) { ok=0; break; }
            if(ok) L.tk.tipo = T_REG;
        }
        else L.tk.tipo = T_ID;
        return;
    }
    if(isdigit(c) || c == '.') {
        int i=0;
        int ponto = 0;
        while(isdigit(c = L.src[L.pos]) || c == '.') {
            if(c == '.') {
                if(ponto) fatal("número inválido");
                ponto = 1;
            }
            L.tk.lex[i++] = c;
            L.pos++;
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
    if(c == '\"') {
        L.pos++;
        int i=0;
        while((c = L.src[L.pos]) != '\"' && c) {
            if(c == '\\' && L.src[L.pos+1] == 'n') {
                L.tk.lex[i++] = '\\';
                L.tk.lex[i++] = 'n';
                L.pos += 2;
            } else {
                L.tk.lex[i++] = c;
                L.pos++;
            }
        }
        L.tk.lex[i] = 0;
        if(L.src[L.pos] == '\"') L.pos++;
        L.tk.tipo = T_TEX;
        return;
    }
    if(c == '\'') {
        L.pos++;
        char v = L.src[L.pos++];
        if(L.src[L.pos] != '\'') fatal("char mal formado");
        L.pos++;
        L.tk.tipo = T_CAR;
        sprintf(L.tk.lex, "%c", v);
        return;
    }
    switch(c) {
        case '(': L.tk.tipo = T_PAREN_ESQ; break;
        case ')': L.tk.tipo = T_PAREN_DIR; break;
        case '{': L.tk.tipo = T_COL_ESQ; break;
        case '}': L.tk.tipo = T_COL_DIR; break;
        case ';': L.tk.tipo = T_PONTO_VIRGULA; break;
        case ',': L.tk.tipo = T_VIRGULA; break;
        case '=': L.tk.tipo = T_IGUAL; break;
        case '+': L.tk.tipo = T_MAIS; break;
        case '-': L.tk.tipo = T_MENOS; break;
        case '*': L.tk.tipo = T_VEZES; break;
        case '/': L.tk.tipo = T_DIV; break;
        default: fatal("Símbolo inválido"); break;
    }
    L.tk.lex[0] = c;
    L.tk.lex[1] = 0;
    L.pos++;
}

void excessao(TipoToken t) {
    if(L.tk.tipo != t) fatal("erro de sintaxe");
    proximoToken();
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
        if(strcmp(funcs[i].nome, nome) == 0) {
            return &funcs[i];
        }
    }
    return NULL;
}

int adicionar_constante(TipoToken tipo, const char* lex, double d_val, long l_val) {
    for(int i=0; i<const_cnt; i++) {
        if(tipo == T_FLU && constantes[i].tipo == T_FLU && fabs(constantes[i].d_val - d_val) < 1e-9)
            return constantes[i].label;
        if(tipo == T_DOBRO && constantes[i].tipo == T_DOBRO && fabs(constantes[i].d_val - d_val) < 1e-9)
            return constantes[i].label;
        if(tipo == T_INT && constantes[i].tipo == T_INT && constantes[i].l_val == l_val)
            return constantes[i].label;
    }
    if(const_cnt >= MAX_CONST) fatal("excesso de constantes");
    Constante* c = &constantes[const_cnt];
    c->tipo = tipo;
    strcpy(c->lex, lex);
    c->d_val = d_val;
    c->l_val = l_val;
    c->label = const_cnt;
    const_cnt++;
    return c->label;
}

void gen_prelude(FILE *s) {
    fprintf(s,
        ".section .data\n"
        ".section .text\n"
        ".global _start\n"
         ".align 2\n"
        "_start:\n"
        "  bl inicio\n\n"
    );
}

void fim(FILE *s) {
    fprintf(s,
        "  mov x8,93\n"
        "  mov x0,0\n"
        "  svc 0\n"
    );
}

void escrever_str(FILE *s, const char *tex) {
    static int lab = 0;
    char lbl[32];
    sprintf(lbl, ".Lstr%d", lab++);
    
    fprintf(s,
        "  .section .rodata\n"
        "%s: .asciz \"%s\"\n"
        "  .section .text\n"
        "  mov x0,1\n"
        "  ldr x1,=%s\n"
        "  mov x2,%zu\n"
        "  mov x8,64\n"
        "  svc 0\n",
        lbl, tex, lbl, strlen(tex)
    );
}

void carregar_valor(FILE *s, Variavel* var) {
    if(var->eh_parametro) {
        fprintf(s, "  ldr w0, [x29, #%d]\n", var->offset);
    } else {
        switch(tamanho_tipo(var->tipo)) {
            case 1: fprintf(s, "  ldrb w0, [x29, #%d]\n", var->offset); break;
            case 4: fprintf(s, "  ldr w0, [x29, #%d]\n", var->offset); break;
            case 8: fprintf(s, "  ldr x0, [x29, #%d]\n", var->offset); break;
        }
    }
}

void armazenar_valor(FILE *s, Variavel* var) {
    if(var->eh_parametro) {
        fprintf(s, "  str w0, [x29, #%d]\n", var->offset);
    } else {
        switch(tamanho_tipo(var->tipo)) {
            case 1: fprintf(s, "  strb w0, [x29, #%d]\n", var->offset); break;
            case 4: fprintf(s, "  str w0, [x29, #%d]\n", var->offset); break;
            case 8: fprintf(s, "  str x0, [x29, #%d]\n", var->offset); break;
        }
    }
}

void carregar_constante(FILE *s, int label) {
    Constante* c = &constantes[label];
    if (c->tipo == T_FLU || c->tipo == T_DOBRO) {
        fprintf(s, "  ldr x0, =const_%d\n", label);
        if (c->tipo == T_FLU) {
            fprintf(s, "  ldr s0, [x0]\n");
        } else {
            fprintf(s, "  ldr d0, [x0]\n");
        }
    } else if (c->tipo == T_INT) {
        fprintf(s, "  ldr x0, =const_%d\n", label);
        fprintf(s, "  ldr w0, [x0]\n");
    }
}

TipoToken expressao(FILE *s, int escopo) {
    TipoToken tipo = T_pINT;
    char id[32];
    int primeiro_termo = 1;
    TipoToken op_pendente = 0;
    
    while(1) {
        if(L.tk.tipo == T_ID) {
            strcpy(id, L.tk.lex);
            Variavel* var = buscar_var(id, escopo);
            if(!var) {
                Funcao* fn = buscar_fn(id);
                if(fn) {
                    proximoToken();
                    excessao(T_PAREN_ESQ);
                    fprintf(s, "  // chamada: %s\n", id);
                    int arg_cnt = 0;
                    while(L.tk.tipo != T_PAREN_DIR) {
                        expressao(s, escopo);
                        fprintf(s, "  str w0, [sp, #-16]!\n");
                        arg_cnt++;
                        
                        if(L.tk.tipo == T_VIRGULA) proximoToken();
                    }
                    excessao(T_PAREN_DIR);
                    fprintf(s, "  bl %s\n", id);
                    fprintf(s, "  add sp, sp, #%d\n", arg_cnt * 16);
                    tipo = fn->retorno;
                } else {
                    fatal("variável ou função não declarada");
                }
            } else {
                proximoToken();
                carregar_valor(s, var);
                if(primeiro_termo) {
                    tipo = var->tipo;
                } else {
                    // Após carregar, gera operação pendente
                    switch(op_pendente) {
                        case T_MAIS: 
                            if (tipo == T_pFLU) fprintf(s, "  fadd s0, s1, s0\n");
                            else if (tipo == T_pDOBRO) fprintf(s, "  fadd d0, d1, d0\n");
                            else fprintf(s, "  add w0, w1, w0\n");
                            break;
                        case T_MENOS: 
                            if (tipo == T_pFLU) fprintf(s, "  fsub s0, s1, s0\n");
                            else if (tipo == T_pDOBRO) fprintf(s, "  fsub d0, d1, d0\n");
                            else fprintf(s, "  sub w0, w1, w0\n");
                            break;
                        case T_VEZES: 
                            if (tipo == T_pFLU) fprintf(s, "  fmul s0, s1, s0\n");
                            else if (tipo == T_pDOBRO) fprintf(s, "  fmul d0, d1, d0\n");
                            else fprintf(s, "  mul w0, w1, w0\n");
                            break;
                        case T_DIV: 
                            if (tipo == T_pFLU) fprintf(s, "  fdiv s0, s1, s0\n");
                            else if (tipo == T_pDOBRO) fprintf(s, "  fdiv d0, d1, d0\n");
                            else fprintf(s, "  sdiv w0, w1, w0\n");
                            break;
                        default: fatal("erro fatal, operador não é"); break;
                        
                    }
                }
            }
        }
        else if(L.tk.tipo == T_INT) {
            char num[32];
            strcpy(num, L.tk.lex);
            long l_val = L.tk.valor_l;
            proximoToken();
            
            if (l_val < 65536) {
                fprintf(s, "  mov w0, #%ld\n", l_val);
            } else {
                int label = adicionar_constante(T_INT, num, 0.0, l_val);
                carregar_constante(s, label);
            }
            if(!primeiro_termo) {
                // Após carregar, gera operação pendente
                switch(op_pendente) {
                    case T_MAIS: 
                        if (tipo == T_pFLU) fprintf(s, "  fadd s0, s1, s0\n");
                        else if (tipo == T_pDOBRO) fprintf(s, "  fadd d0, d1, d0\n");
                        else fprintf(s, "  add w0, w1, w0\n");
                        break;
                    case T_MENOS: 
                        if (tipo == T_pFLU) fprintf(s, "  fsub s0, s1, s0\n");
                        else if (tipo == T_pDOBRO) fprintf(s, "  fsub d0, d1, d0\n");
                        else fprintf(s, "  sub w0, w1, w0\n");
                        break;
                    case T_VEZES: 
                        if (tipo == T_pFLU) fprintf(s, "  fmul s0, s1, s0\n");
                        else if (tipo == T_pDOBRO) fprintf(s, "  fmul d0, d1, d0\n");
                        else fprintf(s, "  mul w0, w1, w0\n");
                        break;
                    case T_DIV: 
                        if (tipo == T_pFLU) fprintf(s, "  fdiv s0, s1, s0\n");
                        else if (tipo == T_pDOBRO) fprintf(s, "  fdiv d0, d1, d0\n");
                        else fprintf(s, "  sdiv w0, w1, w0\n");
                        break;
                    default: fatal("erro fatal, operador não é"); break;
                }
            }
        }
        else if(L.tk.tipo == T_FLU || L.tk.tipo == T_DOBRO) {
            char num[32];
            strcpy(num, L.tk.lex);
            double d_val = L.tk.valor_d;
            TipoToken const_tipo = L.tk.tipo;
            proximoToken();
            
            int label = adicionar_constante(const_tipo, num, d_val, 0);
            carregar_constante(s, label);
            tipo = const_tipo == T_FLU ? T_pFLU : T_pDOBRO;
            
            if(!primeiro_termo) {
                // Após carregar, gera operação pendente
                switch(op_pendente) {
                    case T_MAIS: 
                        if (tipo == T_pFLU) fprintf(s, "  fadd s0, s1, s0\n");
                        else fprintf(s, "  fadd d0, d1, d0\n");
                        break;
                    case T_MENOS: 
                        if (tipo == T_pFLU) fprintf(s, "  fsub s0, s1, s0\n");
                        else fprintf(s, "  fsub d0, d1, d0\n");
                        break;
                    case T_VEZES: 
                        if (tipo == T_pFLU) fprintf(s, "  fmul s0, s1, s0\n");
                        else fprintf(s, "  fmul d0, d1, d0\n");
                        break;
                    case T_DIV: 
                        if (tipo == T_pFLU) fprintf(s, "  fdiv s0, s1, s0\n");
                        else fprintf(s, "  fdiv d0, d1, d0\n");
                        break;
                    default: fatal("erro fatal, operador não é"); break;
                }
            }
        }
        else if(L.tk.tipo == T_CAR) {
            char val = L.tk.lex[0];
            proximoToken();
            fprintf(s, "  mov w0, #%d\n", val);
            if(!primeiro_termo) {
                // Após carregar, gera operação pendente
                switch(op_pendente) {
                    case T_MAIS: fprintf(s, "  add w0, w1, w0\n"); break;
                    case T_MENOS: fprintf(s, "  sub w0, w1, w0\n"); break;
                    case T_VEZES: fprintf(s, "  mul w0, w1, w0\n"); break;
                    case T_DIV: fprintf(s, "  sdiv w0, w1, w0\n"); break;
                    default: fatal("erro fatal, operador não é"); break;
                }
            }
        }
        else {
            fatal("termo inválido na expressão");
        }
        
        if(primeiro_termo) {
            primeiro_termo = 0;
        }
        
        if(L.tk.tipo == T_MAIS || L.tk.tipo == T_MENOS || 
           L.tk.tipo == T_VEZES || L.tk.tipo == T_DIV) {
            op_pendente = L.tk.tipo;
            proximoToken();
            
            // Salva valor atual para operação pendente
            if (tipo == T_pFLU) {
                fprintf(s, "  fmov s1, s0\n");
            } else if (tipo == T_pDOBRO) {
                fprintf(s, "  fmov d1, d0\n");
            } else {
                fprintf(s, "  mov w1, w0\n");
            }
        } else {
            break;
        }
    }
    
    return tipo;
}

void verificar_atribuicao(FILE *s, const char* id, int escopo) {
    Variavel* var = buscar_var(id, escopo);
    if(!var) fatal("variável não declarada");
    
    excessao(T_IGUAL);
    
    TipoToken tipo_exp = expressao(s, escopo);
    
    armazenar_valor(s, var);
}

void declaracao_var(FILE *s, int *offset, int escopo, int eh_parametro) {
    TipoToken tipo = L.tk.tipo;
    int tam = tamanho_tipo(tipo);
    int alinhamento = alinhamento_tipo(tipo);
    
    if(tam == 0) fatal("tipo inválido");
    
    if (!eh_parametro) {
        *offset = (*offset - tam - alinhamento + 1) & ~(alinhamento - 1);
    }
    proximoToken();
    
    if(L.tk.tipo != T_ID) fatal("nome de variável esperado");
    
    Funcao* f = &funcs[fn_cnt-1];
    if(f->var_conta >= MAX_VAR) fatal("excesso de variáveis");
    
    Variavel* var = &f->vars[f->var_conta];
    strcpy(var->nome, L.tk.lex);
    var->tipo = tipo;
    var->offset = *offset;
    var->escopo = escopo;
    var->eh_parametro = eh_parametro;
    f->var_conta++;
    
    if (!eh_parametro) {
        // Offset já atualizado
    } else {
        *offset += 8;
    }
    proximoToken();
    
    if(!eh_parametro && L.tk.tipo == T_IGUAL) {
        verificar_atribuicao(s, var->nome, escopo);
    }
}

void verificar_retorno(FILE *s, int escopo) {
    excessao(T_RETORNAR);
    
    expressao(s, escopo);
    
    Funcao* f = &funcs[fn_cnt-1];
    fprintf(s, "  mov sp, x29\n");
    fprintf(s, "  ldp x29, x30, [sp], #%d\n", f->tamanho_frame);
    fprintf(s, "  ret\n");
    
    excessao(T_PONTO_VIRGULA);
}

void escrever_valor(FILE *s, TipoToken tipo) {
    if (tipo == T_pFLU) {
        fprintf(s, "  bl _imprime_float\n");
    } else if (tipo == T_pDOBRO) {
        fprintf(s, "  bl _imprime_double\n");
    } else if (tipo == T_pCAR) {
        fprintf(s, "  bl _imprime_char\n");
    } else if (tipo == T_pBOOL) {
        fprintf(s, "  bl _imprime_bool\n");
    } else {
        fprintf(s, "  bl _imprime_int\n");
    }
}

void verificar_stmt(FILE *s, int *offset, int escopo) {
    if(escopo == 0) escopo = escopo_global;
    
    if(L.tk.tipo == T_DEF) {
        proximoToken();
        if(L.tk.tipo != T_REG) fatal("registrador esperado");
        char reg[16]; strcpy(reg, L.tk.lex);
        proximoToken(); excessao(T_IGUAL);
        if(L.tk.tipo != T_INT && L.tk.tipo != T_CAR) 
            fatal("valor inteiro ou caractere esperado");
        char val[16]; strcpy(val, L.tk.lex);
        proximoToken(); excessao(T_PONTO_VIRGULA);
        fprintf(s, "  mov %s, #%s\n", reg, val);
        return;
    }
    
    if (L.tk.tipo == T_RETORNAR) {
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
        declaracao_var(s, offset, escopo, 0);
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
                if(L.tk.tipo == T_TEX) {
                    char tmp[256]; strcpy(tmp, L.tk.lex);
                    escrever_str(s, tmp);
                    proximoToken();
                } 
                else if(L.tk.tipo == T_INT || L.tk.tipo == T_FLU || L.tk.tipo == T_DOBRO) {
                    char num[32]; strcpy(num, L.tk.lex);
                    if (L.tk.tipo == T_INT) {
                        long l_val = L.tk.valor_l;
                        if (l_val < 65536) {
                            fprintf(s, "  mov w0, #%ld\n", l_val);
                        } else {
                            int label = adicionar_constante(T_INT, num, 0.0, l_val);
                            carregar_constante(s, label);
                        }
                        fprintf(s, "  bl _imprime_int\n");
                    } else {
                        double d_val = L.tk.valor_d;
                        int label = adicionar_constante(L.tk.tipo, num, d_val, 0);
                        carregar_constante(s, label);
                        if (L.tk.tipo == T_FLU) {
                            fprintf(s, "  bl _imprime_float\n");
                        } else {
                            fprintf(s, "  bl _imprime_double\n");
                        }
                    }
                    proximoToken();
                } 
                else if(L.tk.tipo == T_ID) {
                    char id[32]; strcpy(id, L.tk.lex);
                    Variavel* var = buscar_var(id, escopo);
                    if(!var) fatal("variável não declarada");
                    
                    carregar_valor(s, var);
                    escrever_valor(s, var->tipo);
                    proximoToken();
                }
                else if(L.tk.tipo == T_CAR) {
                    char val = L.tk.lex[0];
                    fprintf(s, "  mov w0, #%d\n", val);
                    fprintf(s, "  bl _imprime_char\n");
                    proximoToken();
                }
                else fatal("argumento inválido");
                
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
            fprintf(s, "  mov x8, %s\n  svc 0\n", num);
            return;
        }
        
        Funcao* fn = buscar_fn(idn);
        if(!fn) fatal("função não declarada");
        
        int arg_cnt = 0;
        while(L.tk.tipo != T_PAREN_DIR) {
            expressao(s, escopo);
            fprintf(s, "  str w0, [sp, #-16]!\n");
            arg_cnt++;
            
            if(L.tk.tipo == T_VIRGULA) proximoToken();
        }
        excessao(T_PAREN_DIR);
        excessao(T_PONTO_VIRGULA);
        
        fprintf(s, "  bl %s\n", idn);
        fprintf(s, "  add sp, sp, #%d\n", arg_cnt * 16);
        return;
    }
    
    if(L.tk.tipo == T_COL_ESQ) {
        proximoToken();
        int novo_escopo = ++escopo_global;
        while(L.tk.tipo != T_COL_DIR) {
            verificar_stmt(s, offset, novo_escopo);
        }
        proximoToken();
        return;
    }
    
    fatal("declaração inválida");
}

void coletar_parametros(FILE *s, Funcao* f) {
    f->param_offset = 16;
    while (L.tk.tipo != T_PAREN_DIR) {
        declaracao_var(s, &f->param_offset, 0, 1);
        
        if (L.tk.tipo == T_VIRGULA) {
            proximoToken();
        } else {
            break;
        }
    }
}

void verificar_fn(FILE *s) {
    TipoToken rt = L.tk.tipo;
    proximoToken();
    
    if(L.tk.tipo != T_ID) fatal("nome de função esperado");
    
    char fnome[32];
    strcpy(fnome, L.tk.lex);
    funcs[fn_cnt].var_conta = 0;
    funcs[fn_cnt].retorno = rt;
    funcs[fn_cnt].escopo_atual = 0;
    funcs[fn_cnt].tamanho_frame = 0;
    funcs[fn_cnt].param_offset = 16;
    strcpy(funcs[fn_cnt++].nome, fnome);
    proximoToken();
    
    excessao(T_PAREN_ESQ);
    coletar_parametros(s, &funcs[fn_cnt-1]);
    excessao(T_PAREN_DIR);
    excessao(T_COL_ESQ);
    
    // Calcular tamanho do frame antes de gerar prólogo
    int offset = 0;
    Lexer save = L;
    while (L.tk.tipo != T_COL_DIR) {
        if (L.tk.tipo == T_pCAR || L.tk.tipo == T_pINT || L.tk.tipo == T_pFLU || 
            L.tk.tipo == T_pBOOL || L.tk.tipo == T_pDOBRO || L.tk.tipo == T_pLONGO) {
            TipoToken tipo = L.tk.tipo;
            int tam = tamanho_tipo(tipo);
            int alinhamento = alinhamento_tipo(tipo);
            proximoToken();
            if (L.tk.tipo != T_ID) {
                L = save;
                break;
            }
            offset = (offset - tam - alinhamento + 1) & ~(alinhamento - 1);
            proximoToken();
            if (L.tk.tipo == T_IGUAL) {
                while (L.tk.tipo != T_PONTO_VIRGULA) 
                    proximoToken();
            }
            if (L.tk.tipo == T_PONTO_VIRGULA) 
                proximoToken();
        } else {
            proximoToken();
        }
    }
    L = save;
    
    int frame_size = (-offset + 15) & ~15;
    if (frame_size < 16) frame_size = 16;
    funcs[fn_cnt-1].tamanho_frame = frame_size;
    
    fprintf(s, ".align 2\n");
    fprintf(s, "%s:\n", fnome);
    fprintf(s, "  stp x29, x30, [sp, #-%d]!\n", frame_size);
    fprintf(s, "  mov x29, sp\n");
    
    offset = 0;
    while(L.tk.tipo != T_COL_DIR) {
        verificar_stmt(s, &offset, 0);
    }
    
    fprintf(s, "  mov sp, x29\n");
    fprintf(s, "  ldp x29, x30, [sp], #%d\n", frame_size);
    fprintf(s, "  ret\n");
    proximoToken();
}

void gerar_constantes(FILE *s) {
    if (const_cnt == 0) return;
    
    fprintf(s, "  .section .rodata\n");
    for (int i=0; i<const_cnt; i++) {
        fprintf(s, "  .align 8\n");
        fprintf(s, "const_%d:\n", i);
        if (constantes[i].tipo == T_INT) {
            fprintf(s, "  .word %ld\n", constantes[i].l_val);
        } else if (constantes[i].tipo == T_FLU) {
            float f = (float)constantes[i].d_val;
            fprintf(s, "  .float %f\n", f);
        } else if (constantes[i].tipo == T_DOBRO) {
            fprintf(s, "  .double %f\n", constantes[i].d_val);
        }
    }
    fprintf(s, "  .section .text\n\n");
}

int main(int argc, char **argv) {
    if(argc < 2) return 1;
    
    int manter_asm = (argc >= 3 && strcmp(argv[2], "-asm") == 0);
    
    char* buf = malloc(MAX_SRC);
    char nomeArquivo[256];
    snprintf(nomeArquivo, sizeof(nomeArquivo), "%s.fpb", argv[1]);
    FILE* in = fopen(nomeArquivo, "r");
    if(!in) return 2;
    
    size_t n = fread(buf, 1, MAX_SRC, in);
    buf[n] = 0; 
    fclose(in);
    
    L.src = buf;
    L.pos = 0;
    proximoToken();
    
    char asm_s[128], asm_o[128], cmd[256];
    snprintf(asm_s, 128, "%s.s", argv[1]);
    FILE *s = fopen(asm_s, "w");
    
    gen_prelude(s);
    while(L.tk.tipo != T_FIM) {
        verificar_fn(s);
    }
    gerar_constantes(s);
    FILE *lib = fopen("biblis/impressao.asm", "r");
    char linha[512];
    while (fgets(linha, sizeof(linha), lib)) {
        // Garante quebra de linha final
        size_t len = strlen(linha);
        if (len > 0 && linha[len-1] != '\n') {
            linha[len]   = '\n';
            linha[len+1] = '\0';
        }
        fputs(linha, s);
    }
    fclose(lib);
    fclose(s);
    
    snprintf(asm_o, 128, "%s.o", argv[1]);
    
    snprintf(cmd, 256, "as %s -o %s", asm_s, asm_o);
    if(system(cmd)) return 3;
    
    snprintf(cmd, 256, "ld %s -o %s", asm_o, argv[1]);
    if(system(cmd)) return 4;
    
    if(!manter_asm) remove(asm_s);
    remove(asm_o);
    
    free(buf);
    return 0;
}