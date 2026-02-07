/*
* [FUNÇÃO]: Compilador.
* [IMPLEMENTAÇÃO]: @Shiniga-OP.
* [BASE]: Assembly.
* [SISTEMA OPERACIONAL]: ANDROID.
* [ARQUITETURA]: ARM64-LINUX-ANDROID(ARM64).
* [LINGUAGEM]: Português Brasil(PT-BR).
* [DATA]: 06/07/2025.
* [ATUAL]: 07/02/2026.
* [PONTEIRO]: dereferencia automatica, acesso a endereços apenas com "@ponteiro".
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
// otimizadores:
#include "util/otimi1.h"
#include "util/otimi2.h"

typedef enum {
    // mutação:
    T_FINAL,
    // tipos:
    T_ID, T_INT, T_TEX, T_CAR, T_FLU, T_BOOL, T_DOBRO,
    T_LONGO, T_BYTE, T_ESPACO_ID,
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
    T_SE, T_SENAO, T_IGUAL_IGUAL, T_DIFERENTE, T_DIFERENTE_ABS,
    T_MAIOR, T_MENOR, T_MAIOR_IGUAL, T_MENOR_IGUAL,
    T_TAMBEM_TAMBEM, T_OU_OU, T_NAO,
    // loops:
    T_POR, T_ENQ, T_PARE,
    // retornos:
    T_pCAR, T_pINT, T_pFLU, T_pBOOL, T_pDOBRO, 
    T_pLONGO, T_pBYTE, T_PONTEIRO,
    T_pVAZIO,
    // definições:
    T_DEF, T_FIM, T_RETORNAR, T_INCLUIR, 
    T_ESPACO, T_GLOBAL,
    // bits:
    T_MAIOR_MAIOR, T_MENOR_MENOR, T_TAMBEM, T_OU
} TipoToken;

typedef struct {
    int linha;
    int coluna;
    const char* arquivo;
} Posicao;

typedef struct {
    TipoToken tipo;
    char lex[64];
    double valor_d; // pra constantes de ponto flutuante
    long valor_l; // pra constantes inteiras grandes
    Posicao pos;
} Token;

typedef struct {
    char nome[64];
    char espaco[64];
    TipoToken tipo_base;
    int eh_ponteiro;
    int eh_array;
    int eh_parametro;
    int espaco_id; // 0 se não for espaço
    int eh_final;
    int num_dims; // numero de dimensões
    int* dims; // tamanho de cada dimensão
    int bytes;
    int pos;
    int escopo;
    long valor; // inteiros
    double valor_f; // flutuantes
    char reg[8];
} Variavel;

typedef struct {
    char nome[64];
    TipoToken retorno;
    Variavel* vars;
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
    char lex[64];
    double d_val;
    long l_val;
    int titulo;
} Constante;

typedef struct {
    char nome[64];
    long valor;
} Macro;

typedef struct {
    char nome[64];
    Variavel* campos;
    int campo_cnt;
    int tam_total;
} Espaco;

typedef struct {
    char nome[64];
    char* valor;
} Tex;

typedef struct {
    bool x[19];
    bool s[19];
    bool d[19];
} Regs;

// maximos:
static int MAX_VAR = 128;
static int MAX_MACROS = 128;
static int MAX_TOK = 556;
static int MAX_TEX = 128;
static int MAX_FN = 128;
static int MAX_PARE = 128;
static int MAX_ESPACO = 128;
static int MAX_CONST = 128;
static int MAX_DIMS = 6;

static Lexer L;
static Funcao* funcs;
static int fn_cnt = 0;
static int escopo_global = 0;
static Constante* constantes;
static int const_cnt = 0;
static Tex* texs;
static int tex_cnt = 0;
static Macro* macros;
static int macro_cnt = 0;
static Variavel* globais;
static int global_cnt = 0;
static Espaco* espacos;
static int espaco_cnt = 0;
static int* rotulos_pare; // pilha de rotulos pra "pare"
static int rotulo_pare_topo = -1; // topo da pilha
static bool em_loop = false;
static int nivel_loop = 0;
static char* arquivoAtual;
static Regs regs = {0};
static bool debug_o = false;

// declaracao
void declaracao_var(FILE* s, int* pos, int escopo, int eh_parametro, int eh_final, int eh_global);
// alocação:
int alocar_reg(char tipo);
void liberar_reg(char tipo, int reg);
void alocar_buf();
void realocar_buf(void* p, char* tipo);
void liberar_buf();
// util
int eh_tipo(TipoToken tipo);
int tipos_compativeis(TipoToken tipo1, TipoToken tipo2);
int tam_tipo(TipoToken t);
void armazenar_valor(FILE* s, Variavel* var);
int calcular_pos_matriz(Variavel* var, int indices[]);
void empilhar_pare(int rotulo);
int desempilhar_pare();
int topo_pare();

// essenciais:
#include "util/analisador.h"
#include "util/gerador.h"

// [ALOCAÇÃO]:
void alocar_buf() {
    funcs = malloc(MAX_FN * sizeof(Funcao));
    if(!funcs) fatal("[alocar_buf]: erro ao alocar funcs");
    espacos = malloc(MAX_ESPACO * sizeof(Espaco));
    if(!espacos) fatal("[alocar_buf]: erro ao alocar espacos");
    rotulos_pare = malloc(MAX_PARE * sizeof(int));
    if(!rotulos_pare) fatal("[alocar_buf]: erro ao alocar rotulos_pare");
    texs = malloc(MAX_TEX * sizeof(Tex));
    if(!texs) fatal("[alocar_buf]: erro ao alocar texs");
    constantes = malloc(MAX_CONST * sizeof(Constante));
    if(!constantes) fatal("[alocar_buf]: erro ao alocar constantes");
    macros = malloc(MAX_MACROS * sizeof(Macro));
    if(!macros) fatal("[alocar_buf]: erro ao alocar macros");
    globais = malloc(MAX_VAR * sizeof(Variavel));
    if(!globais) fatal("[alocar_buf]: erro ao alocar globais");
}

void liberar_buf() {
    for(int i = 0; i < espaco_cnt; i++) {
        for(int j = 0; j < espacos[i].campo_cnt; i++) free(espacos[i].campos[i].dims);
        free(espacos[i].campos);
    }
    for(int i = 0; i < fn_cnt; i++) {
        for(int j = 0; j < funcs[i].var_conta; i++) free(funcs[i].vars[i].dims);
        free(funcs[i].vars);
    }
    free(funcs);
    free(espacos);
    free(rotulos_pare);
    free(texs);
    free(constantes);
    free(macros);
    free(globais);
}

int alocar_reg(char tipo) {
    int regLivre = -1; // -1 = pilha
    // pra registradores de inteiros(x/w)
    if(tipo == 'x' || tipo == 'w') {
        // usa x8-x15 pra temporarios(não preservados pelo chamador)
        for(int i = 8; i <= 19; i++) {
            if(!regs.x[i]) {
                regs.x[i] = true;
                regLivre = i;
            }
        }
    }
    // pra registradores de ponto flutuante(s)
    else if(tipo == 's') {
        // s0-s7 são usados pra parametros, então usamos s8-s15
        for(int i = 8; i <= 19; i++) {
            if(!regs.s[i]) {
                regs.s[i] = true;
                regLivre = i;
            }
        }
    }
    // pra registradores de ponto flutuante dobro(d)
    else if(tipo == 'd') {
        // d0-d7 são usados pra parametros, então usamos d8-d15
        for(int i = 8; i <= 19; i++) {
            if(!regs.d[i]) {
                regs.d[i] = true;
                regLivre = i;
            }
        }
    }
    return regLivre;
}

void liberar_reg(char tipo, int reg) {
    if(tipo == 'x' || tipo == 'w') {
        if(reg >= 8 && reg <= 19) regs.x[reg] = false;
        else if(reg >= 19 && reg <= 22) regs.x[reg] = false;
    } else if(tipo == 's') {
        if(reg >= 8 && reg <= 19) regs.s[reg] = false;
    } else if(tipo == 'd') {
        if(reg >= 8 && reg <= 19) regs.d[reg] = false;
    }
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
    if((tipo1 == T_pBYTE && (tipo2 == T_pCAR || tipo2 == T_pINT)) || 
       (tipo2 == T_pBYTE && (tipo1 == T_pCAR || tipo1 == T_pINT))) return 1;
    if((tipo1 == T_pCAR && tipo2 == T_pINT) || (tipo1 == T_pINT && tipo2 == T_pCAR)) return 1;
    if((tipo1 == T_pINT && tipo2 == T_pLONGO) || (tipo1 == T_pLONGO && tipo2 == T_pINT)) return 1;
    if((tipo1 == T_pINT && tipo2 == T_pFLU) || (tipo1 == T_pFLU && tipo2 == T_pINT)) return 1;
    if((tipo1 == T_pLONGO && tipo2 == T_pFLU) || (tipo1 == T_pFLU && tipo2 == T_pLONGO)) return 1;
    if((tipo1 == T_pINT && tipo2 == T_pDOBRO) || (tipo1 == T_pDOBRO && tipo2 == T_pINT)) return 1;
    if((tipo1 == T_pLONGO && tipo2 == T_pDOBRO) || (tipo1 == T_pDOBRO && tipo2 == T_pLONGO)) return 1;
    if((tipo1 == T_pFLU && tipo2 == T_pDOBRO) || (tipo1 == T_pDOBRO && tipo2 == T_pFLU)) return 1;
    return 0;
}

int eh_tipo(TipoToken tipo) {
    TipoToken tipos[] = {
        T_pVAZIO, T_pINT, T_pFLU, T_pCAR, T_pBOOL,
        T_pDOBRO, T_pLONGO, T_PONTEIRO, T_pBYTE, T_ESPACO_ID};
    for(int i = 0; i < sizeof(tipos)/sizeof(tipos[0]); i++) {
        if(tipo == tipos[i]) return 1;
    }
    return 0;
}

void empilhar_pare(int rotulo) {
    if(rotulo_pare_topo >= MAX_PARE - 1) {
        MAX_PARE += 2;
        int *temp = realloc(rotulos_pare, MAX_PARE * sizeof(int));
        if(!temp) {
            printf("[empilhar_pare]: Erro ao alocar memória, rotulos %d\n", rotulo_pare_topo);
            exit(1);
        }
    }
    rotulos_pare[++rotulo_pare_topo] = rotulo;
}

int desempilhar_pare() {
    if(rotulo_pare_topo < 0) fatal("[desempilhar_pare] pilha vazia");
    return rotulos_pare[rotulo_pare_topo--];
}

int topo_pare() {
    if(rotulo_pare_topo < 0) fatal("[topo_pare] nenhum loop ativo");
    return rotulos_pare[rotulo_pare_topo];
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

void declaracao_var(FILE* s, int* pos, int escopo, int eh_parametro, int eh_final, int eh_global) {
    TipoToken tipo_base = L.tk.tipo;
    int eh_ponteiro = 0;
    int num_dims = 0;
    int* dims = malloc(MAX_DIMS * sizeof(int));
    
    char nome_espaco[64] = {0};
    int eh_espaco = 0;
    int tam_total = 0;
    if(debug_o) printf("[declaracao_var]: o tipo é: %s\n", L.tk.lex);
    // verifica se é um espaço definido
    if(tipo_base == T_ESPACO_ID) {
        Espaco* esp = buscar_espaco(L.tk.lex);
        if(esp) {
            eh_espaco = 1;
            strcpy(nome_espaco, L.tk.lex);
        }
    }
    proximoToken();

    if(L.tk.tipo == T_VEZES) {
        eh_ponteiro = 1;
        proximoToken();
    } else {
        while(L.tk.tipo == T_COL_ESQ) {
            if(num_dims >= MAX_DIMS) {
                MAX_DIMS += 2;
                int *temp = realloc(dims, MAX_DIMS * sizeof(int));
                if(!temp) {
                    printf("[declaracao_var]: Erro ao alocar memória, dimensões %d\n", num_dims);
                    free(dims);
                    exit(1);
                }
                dims = temp;
            }
            proximoToken();
            long tam_array = 0;
            
            if(L.tk.tipo == T_INT) {
                tam_array = L.tk.valor_l;
                proximoToken();
            } else if(L.tk.tipo == T_ID) { 
                char id_tam[64];
                strcpy(id_tam, L.tk.lex);
                
                Variavel* var_tam = buscar_var(id_tam, escopo);
                if(var_tam) {
                    if(var_tam->tipo_base != T_pINT && var_tam->tipo_base != T_pLONGO) fatal("[declaracao_var] a variavel de tamanho deve ser do tipo 'int' ou 'longo'");
                    if(!var_tam->eh_final) fatal("[declaracao_var] o tamanho do array deve ser um literal inteiro ou uma variavel declarada como 'final'");
                    
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
    
    char nome_var[64];
    strcpy(nome_var, L.tk.lex); // salva o nome da variavel
    proximoToken(); // consome o nome da variavel

    // === VARIAVEL COMUM ===
    Variavel* var;
    
    if(eh_global) {
        if(debug_o) printf("[declaracao_var]: variavel global: %s\n", nome_var);
        if(global_cnt >= MAX_VAR) {
            MAX_VAR += 2;
            Variavel *temp = realloc(globais, MAX_VAR * sizeof(Variavel));
            if(temp == NULL) {
                printf("[declaracao_var]: Erro ao realocar memória, globais: %d\n", global_cnt);
                free(globais);
                exit(EXIT_FAILURE);
            }
            globais = temp;
        }
        var = &globais[global_cnt++];
        var->escopo = -1; // global
        var->eh_parametro = 0;
    } else {
        if(debug_o) printf("[declaracao_var]: variavel comum: %s\n", nome_var);
        Funcao* f = &funcs[fn_cnt - 1];
        if(f->var_conta >= MAX_VAR) {
            MAX_VAR += 2;
            Variavel *temp = realloc(f->vars, MAX_VAR * sizeof(Variavel));
            if(temp == NULL) {
                printf("[declaracao_var]: Erro ao realocar memória, função: %s vars: %d\n", f->nome, f->var_conta);
                free(f->vars);
                exit(1);
            }
            f->vars = temp;
        }
        var = &f->vars[f->var_conta];
        var->escopo = escopo;
        var->eh_parametro = eh_parametro;
        f->var_conta++;
    }
    // preenche campos comuns
    strcpy(var->nome, nome_var);
    var->tipo_base = tipo_base;
    var->eh_ponteiro = eh_ponteiro;
    var->eh_array = (num_dims > 0);
    var->num_dims = num_dims;
    var->eh_final = eh_final;
    var->valor = 0;
    var->dims = dims;
    if(eh_espaco) strcpy(var->espaco, nome_espaco);
    
    // calcula tamanho
    if(eh_espaco) {
        if(debug_o) {
            printf("[declaracao_var]: nome do espaço: %s\n", nome_espaco);
            printf("[declaracao_var]: espaco_cnt = %d\n", espaco_cnt);
            if(espaco_cnt > 0) printf("[declaracao_var]: espacos[0].nome = %s\n", espacos[0].nome);
        }
        Espaco* esp = buscar_espaco(nome_espaco);
        if(!esp) {
            printf("[declaracao_var]: espaço '%s' não encontrado!\n", nome_espaco);
            fatal("[declaracao_var] espaço não encontrado");
        }
        tam_total = esp->tam_total;
    } else {
        tam_total = tam_tipo(tipo_base);
    }
    for(int i = 0; i < num_dims; i++) {
        if(dims[i] > 0) tam_total *= dims[i];
    }
    if(eh_ponteiro) tam_total = 8;
    var->bytes = tam_total;
    
    // === POSICIONAMENTO E DEFINIÇÃO ===
    if(eh_global) {
        // globais: armazena em seção de dados, não precisa de posição no frame
        var->pos = 0; // não usado pra globais
        if(debug_o) {
            printf("[declaracao_var]: definindo: %s\n", var->nome);
            printf("[declaracao_var]: token atual: %s\n", token_str(L.tk.tipo));
        }
        if(L.tk.tipo == T_IGUAL) {
            if(debug_o) printf("[declaracao_var]: atribuindo a %s\n", var->nome);
            if(!L.fonte || L.pos >= strlen(L.fonte)) {
                char msg[256];
                snprintf(msg, sizeof(msg), "[declaracao_var] tentativa de chamar proximoToken() com fonte inválida. pos=%zu, len=%zu", L.pos, strlen(L.fonte));
                fatal(msg);
            }
            proximoToken();
            if(num_dims > 0 && tipo_base == T_pCAR && L.tk.tipo == T_TEX) {
                if(debug_o) printf("[declaracao_var]: atribuindo %s como array de texto", var->nome);
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
            } else if(eh_ponteiro && L.tk.tipo == T_TEX) {
                if(debug_o) printf("[declaracao_var]: atribuindo %s como ponteiro de texto", var->nome);
                // ponteiro pra texto literal
                int id_tex = add_tex(L.tk.lex);
                var->valor = id_tex; // guarda o ID do texto
                proximoToken();
            } else {
                // inicia simples com constante
                if(L.tk.tipo == T_INT) {
                    if(debug_o) printf("[declaracao_var]: atribuindo %s como int, valor: %ld", var->nome, L.tk.valor_l);
                    var->valor = (int)L.tk.valor_l;
                    var->valor_f = 0;
                    proximoToken();
                } else if(L.tk.tipo == T_CAR && L.tk.lex[0] != 0) {
                    var->valor = (char)L.tk.valor_l;
                    var->valor_f = 0;
                    proximoToken();
                } else if(L.tk.tipo == T_BYTE) {
                    var->valor = (int)L.tk.valor_l;
                    var->valor_f = 0;
                    proximoToken();
                } else if(L.tk.tipo == T_FLU || L.tk.tipo == T_DOBRO) {
                    var->valor_f = L.tk.valor_d;
                    var->valor = 0; // não usa pra flutuantes
                    proximoToken();
                } else if(L.tk.tipo == T_LONGO) {
                    if(debug_o) printf("[declaracao_var]: atribuindo %s com longo, valor: %ld", var->nome, L.tk.valor_l);
                    var->valor = L.tk.valor_l;
                    var->valor_f = 0;
                    proximoToken();
                } else {
                    fatal("[declaracao_var] inicialização global deve ser constante");
                }
                if(debug_o) printf("[declaracao_var]: nome: %s\nescopo: %d\nvalor normal: %ld\nvalor flutuante: %f\n", var->nome, var->escopo, var->valor, var->valor_f);
            }
        }
    } else {
        // locais: calcula posição no frame
        if(!eh_parametro) {
            *pos = *pos - tam_total;
            *pos = *pos & ~15; // alinhamento
            var->pos = *pos;
        } else {
            var->pos = *pos;
            *pos += 8; // parametros são sempre 8 bytes(ponteiro ou valor)
        }
        if(L.tk.tipo == T_IGUAL) {
            proximoToken();
            
            if(eh_espaco && L.tk.tipo == T_CHAVE_ESQ) {
                // iniciado como espaço com { }
                excessao(T_CHAVE_ESQ);
                
                Espaco* esp = buscar_espaco(nome_espaco);
                if(!esp) fatal("[declaracao_var] espaço não encontrado");
                
                for(int i = 0; i < esp->campo_cnt; i++) {
                    Variavel* campo = &esp->campos[i];
                    // processa valor do campo
                    TipoToken tipo_valor = expressao(s, escopo);
                    
                    if(!tipos_compativeis(campo->tipo_base, tipo_valor)) {
                        char msg[100];
                        sprintf(msg, "[declaracao_var] tipo incompatível na inicialização do espaço (campo %s)", campo->nome);
                        fatal(msg);
                    }
                    // calcula posição do campo
                    int pos_campo = var->pos + campo->pos;
                    // armazena o valor
                    if(campo->eh_ponteiro) {
                        fprintf(s, "  str x0, [x29, %d]\n", pos_campo);
                    } else {
                        if(campo->tipo_base == T_pCAR || campo->tipo_base == T_pBOOL || campo->tipo_base == T_pBYTE)
                        fprintf(s, "  strb w0, [x29, %d]\n", pos_campo);
                        else if(campo->tipo_base == T_pINT)
                        fprintf(s, "  str w0, [x29, %d]\n", pos_campo);
                        else if(campo->tipo_base == T_pFLU)
                        fprintf(s, "  str s0, [x29, %d]\n", pos_campo);
                        else if(campo->tipo_base == T_pDOBRO)
                        fprintf(s, "  str d0, [x29, %d]\n", pos_campo);
                        else if(campo->tipo_base == T_pLONGO)
                        fprintf(s, "  str x0, [x29, %d]\n", pos_campo);
                        else if(campo->tipo_base == T_ESPACO_ID)
                        fatal("[declaracao_var] inicialização de espaço aninhado não suportada");
                    }
                    if(i < esp->campo_cnt - 1) {
                        if(L.tk.tipo == T_VIRGULA) proximoToken();
                        else fatal("[declaracao_var] vírgula esperada entre campos do espaço");
                    }
                }
                excessao(T_CHAVE_DIR);
            } else if(num_dims > 0 && tipo_base == T_pCAR && L.tk.tipo == T_TEX) {
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
                int* indices = malloc(MAX_DIMS * sizeof(int));
                verificar_matriz(s, var, escopo, indices, 0);
                excessao(T_CHAVE_DIR);
            } else if(eh_ponteiro && L.tk.tipo == T_TEX) {
                int id_tex = add_tex(L.tk.lex);
                fprintf(s, "  ldr x0, = %s\n", texs[id_tex].nome);
                fprintf(s, "  str x0, [x29, %d]\n", var->pos);
                proximoToken();
            } else {
                TipoToken tipo_exp = expressao(s, escopo);

                // tenta usar um registrador temporario para a expressão
                char reg_tipo;
                int reg_temp = -1;

                if(tipo_exp == T_pFLU) {
                    reg_tipo = 's';
                    reg_temp = alocar_reg(reg_tipo);
                } else if(tipo_exp == T_pDOBRO) {
                    reg_tipo = 'd';
                    reg_temp = alocar_reg(reg_tipo);
                } else if(tipo_exp == T_PONTEIRO || tipo_exp == T_pLONGO) {
                    reg_tipo = 'x';
                    reg_temp = alocar_reg(reg_tipo);
                } else {
                    reg_tipo = 'w';
                    reg_temp = alocar_reg('x'); // w é tratado como x
                }
                if(reg_temp >= 0) {
                    if(debug_o) printf("[declaracao_var]: usando registrador %c%d\n", reg_tipo, reg_temp);
                    // tem registrador temporario, salvar nele
                    if(reg_tipo == 's') fprintf(s, "  fmov s%d, s0\n", reg_temp);
                    else if(reg_tipo == 'd') fprintf(s, "  fmov d%d, d0\n", reg_temp);
                    else if(reg_tipo == 'x') fprintf(s, "  mov x%d, x0\n", reg_temp);
                    else fprintf(s, "  mov w%d, w0\n", reg_temp);
                }
                if(eh_ponteiro) {
                    if(tipo_exp == T_pINT) fprintf(s, "  sxtw x0, w0\n");
                    
                    fprintf(s, "  str x0, [x29, %d]\n", var->pos);
                } else armazenar_valor(s, var);
                
                // se usou registrador temporario, liberar
                if(reg_temp >= 0) liberar_reg(reg_tipo, reg_temp);
            }
        }
    }
}

int main(int argc, char** argv) {
    alocar_buf();
    for(int i = 0; i < 16; i++) {
        regs.x[i] = false;
        regs.s[i] = false;
        regs.d[i] = false;
    }
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
        } else if(strcmp(argv[i], "-debug") == 0) {
            debug_o = true;
            debug1 = true;
            debug2 = true;
            printf("fpb: DEBUG ATIVADO\n");
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
        printf("fpb exemplo.fpb -debug : ativa o debug de análise dos tokens\n");
        printf("\npara definir o diretorio das bibliotecas.\nmodifique a variavel de ambiente \"$FPB_DIR\"\n");
        return 0;
    }
    if(modoVersao) {
        printf("[FOCA-DO ESTÚDIOS]\nFPB (Fácil Programação Baixo nivel) - v0.0.3 (beta)\n");
        return 0;
    }
    if(modoConfig) {
        printf("[configuração]:\n");
        printf("arquitetura padrão: ARM64 Linux Android\n\n");
        printf("buffers iniciais:\n");
        printf("variaveis: %i\n", MAX_VAR);
        printf("constantes: %i\n", MAX_CONST);
        printf("texs: %i\n", MAX_TEX);
        printf("funcoes: %i\n", MAX_FN);
        printf("dimensões: %i\n", MAX_DIMS);
        printf("macros: %i\n", MAX_MACROS);
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
    // obtem o tamanho do arquivo
    fseek(en, 0, SEEK_END);
    long tam = ftell(en);
    fseek(en, 0, SEEK_SET);
    // alocar exatamente o necessario + 1 pro \0
    char* buf = malloc(tam + 1);
    if(!buf) {
        printf("Erro de alocação de memória\n");
        fclose(en);
        return 1;
    }
    // le o arquivo
    size_t n = fread(buf, 1, tam, en);
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
    liberar_buf();
    return 0;
}