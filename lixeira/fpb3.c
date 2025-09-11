/*
* [INFO]:
* [IMPLEMENTAÇÃO]: @Shiniga-OP.
* [BASE]: Assembly.
* [SISTEMA OPERACIONAL]: ANDROID.
* [ARQUITETURA]: AARCH64-LINUX-ANDROID(ARM64).
* [LINGUAGEM]: Português Brasil(PT-BR).
* [DATA]: 06/07/2025.
* [ATUAL]: 10/09/2025.
* [NOTA]: Implementar otimização de registradores.
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
    T_MAIS_MAIS, T_MENOS_MENOS,
    // condicionais:
    T_SE, T_SENAO, T_IGUAL_IGUAL, T_DIFERENTE,
    T_MAIOR, T_MENOR, T_MAIOR_IGUAL, T_MENOR_IGUAL,
    // loops:
    T_POR, T_ENQ,
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
    double valor_d; // pra constpos de ponto flutuante
    long valor_l; // pra constpos inteiras grandes
    Posicao pos;
} Token;

typedef struct {
    char nome[32];
    TipoToken tipo_base;
    int eh_ponteiro;
    int eh_array;
    int tam_array;
    int eh_parametro;
    int pos;
    int escopo;
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

typedef struct {
    char nome[4];
    int livre;
    int eh_flu;
    int eh_dobro;
    int preservado;
    int sujo;
    int constante;
    union {
        long valor_int;
        double valor_flu;
    };
    int ultimo_uso;
} Registrador;

typedef struct {
    float pesos[8];
    float bias;
    float taxa_aprendizado;
} Perceptron;

/*
* dados do registrador pra IA:
* [0] ultimo_uso(normalizado)
* [1] eh_preservado(0 ou 1)
* [2] eh_sujo(0 ou 1) 
* [3] eh_flutuante(0 ou 1)
* [4] eh_constante(0 ou 1)
* [5] proximo_uso_estimado(0-1)
* [6] custo_derramamento(0-1)
* [7] prioridade_tipo(0-1)
*/

static Perceptron p;

static Lexer L;
static Funcao funcs[MAX_FN];
static int fn_cnt = 0;
static int escopo_global = 0;
static Constante constpos[MAX_CONST];
static int const_cnt = 0;
static Tex texs[MAX_TEX];
static int tex_cnt = 0;
static char* arquivoAtual;
static int reg_cnt = 0;
static Registrador regs[] = {
    // parametros:
    {"x0", 1, 0, 0, 0, 0, 0, 0},{"x1", 1, 0, 0, 0, 0, 0, 0},
    {"x2", 1, 0, 0, 0, 0, 0, 0},{"x3", 1, 0, 0, 0, 0, 0, 0},
    {"x4", 1, 0, 0, 0, 0, 0, 0},{"x5", 1, 0, 0, 0, 0, 0, 0},
    {"x6", 1, 0, 0, 0, 0, 0, 0},{"x7", 1, 0, 0, 0, 0, 0, 0},
    // flutuante:
    {"s0", 1, 1, 0, 0, 0, 0, 0},{"s1", 1, 1, 0, 0, 0, 0, 0},
    {"s2", 1, 1, 0, 0, 0, 0, 0},{"s3", 1, 1, 0, 0, 0, 0, 0},
    {"s4", 1, 1, 0, 0, 0, 0, 0},{"s5", 1, 1, 0, 0, 0, 0, 0},
    {"s6", 1, 1, 0, 0, 0, 0, 0},{"s7", 1, 1, 0, 0, 0, 0, 0},
    // dobro:
    {"d0", 1, 0, 1, 0, 0, 0, 0},{"d1", 1, 0, 1, 0, 0, 0, 0},
    {"d2", 1, 0, 1, 0, 0, 0, 0},{"d3", 1, 0, 1, 0, 0, 0, 0},
    {"d4", 1, 0, 1, 0, 0, 0, 0},{"d5", 1, 0, 1, 0, 0, 0, 0},
    {"d6", 1, 0, 1, 0, 0, 0, 0},{"d7", 1, 0, 1, 0, 0, 0, 0},
    // preservados:
    {"x19", 1, 0, 0, 1, 0, 0, 0},{"x20", 1, 0, 0, 1, 0, 0, 0},
    {"x21", 1, 0, 0, 1, 0, 0, 0},{"x22", 1, 0, 0, 1, 0, 0, 0},
    {"x23", 1, 0, 0, 1, 0, 0, 0},{"x24", 1, 0, 0, 1, 0, 0, 0},
    {"x25", 1, 0, 0, 1, 0, 0, 0},{"x26", 1, 0, 0, 1, 0, 0, 0},
    {"x27", 1, 0, 0, 1, 0, 0, 0},{"x28", 1, 0, 0, 1, 0, 0, 0}
};

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
void verificar_stmt(FILE* s, int* pos, int escopo);
void verificar_retorno(FILE* s, int escopo);
void verificar_atribuicao(FILE* s, const char* id, int escopo);
void verificar_por(FILE* s, int escopo);
void verificar_enq(FILE* s, int escopo);
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
void armazenar_valor(FILE* s, Variavel* var);
void escrever_valor(FILE* s, TipoToken tipo);
// registradores:
const char* alocar_reg(FILE* s, TipoToken tipo);
void liberar_reg(const char* reg);
void derrame(FILE* s, const char* reg_nome);
const char* achar_pra_derrame();
// IA:
int degrau(float x);
int prever(float entrada[]);
void treinar(float entrada[], int saidaEsperada);
void iniciarPerceptron();

// [IA]:
int degrau(float x) {
    return x > 0 ? 1 : 0;
}

int prever(float entrada[]) {
    float soma = p.bias;
    for(int i = 0; i < 8; i++) soma += p.pesos[i] * entrada[i];
    return degrau(soma); // 1 = derramar, 0 = manter
}

void treinar(float entrada[], int saidaEsperada) {
    int saida = prever(entrada);
    int erro = saidaEsperada - saida;
    // ajusta pesos
    for(int i = 0; i < 8; i++) p.pesos[i] += p.taxa_aprendizado * erro * entrada[i];
    // ajusta o bias
    p.bias += p.taxa_aprendizado * erro;
}

// cada array tem 8 características
float dados[][8] = {
    // registradores ruins para derramar(deve derramar = 1)
    {0.9, 0, 1, 0, 0, 0.1, 0.2, 0.3}, // ultimo uso alto, não preservado, sujo
    {0.8, 0, 0, 0, 0, 0.2, 0.1, 0.4}, // ultimo uso alto, não preservado
    {0.7, 0, 1, 1, 0, 0.3, 0.4, 0.2}, // sujo, flutuante, baixa prioridade
    {0.6, 0, 0, 0, 1, 0.4, 0.3, 0.1}, // constante, próximo uso baixo
    
    // registradores bons para manter(deve manter = 0)
    {0.1, 1, 0, 1, 1, 0.9, 0.8, 0.7}, // preservado, constante, alto próximo uso
    {0.2, 1, 0, 0, 0, 0.8, 0.7, 0.6}, // preservado, limpo
    {0.3, 0, 0, 1, 0, 0.7, 0.6, 0.8}, // flutuante, alto próximo uso
    {0.4, 1, 0, 0, 1, 0.6, 0.5, 0.9} // preservado, constante, alta prioridade
};
int saidas_esperadas[] = {1, 1, 1, 1, 0, 0, 0, 0};

void iniciarPerceptron() {
    // inicializa a IA:
    for(int i = 0; i < 8; i++) p.pesos[i] = 0;
    p.bias = 0;
    p.taxa_aprendizado = 0.1f;
    // treinamento:
    for(int epoca = 0; epoca < 500; epoca++) {
        for(int i = 0; i < 8; i++) treinar(dados[i], saidas_esperadas[i]);
    }
}

// [OTIMIZAÇÃO]:
const char* alocar_reg(FILE* s, TipoToken tipo) {
    static int contador_uso = 0;
    contador_uso++;
    
    printf("DEBUG: alocar_reg chamado para tipo %d, contador=%d\n", tipo, contador_uso);
    
    // primeiro tenta achar registrador livre do tipo correto
    for(int i = 0; i < 32; i++) {
        if(!regs[i].livre) continue;
        
        // verifica compatibilidade de tipo
        if(tipo == T_pFLU && !regs[i].eh_flu) continue;
        if(tipo == T_pDOBRO && !regs[i].eh_dobro) continue;
        if((tipo == T_pINT || tipo == T_pCAR) && (regs[i].eh_flu || regs[i].eh_dobro)) continue;
        
        printf("DEBUG: Registrador %s alocado\n", regs[i].nome);
        regs[i].livre = 0;
        regs[i].sujo = 1;
        regs[i].ultimo_uso = contador_uso;
        return regs[i].nome;
    }
    
    printf("DEBUG: Nenhum registrador livre, procurando para derramar\n");
    
    // se não achou livre derrama o menos usado recentemente
    int indice_derramar = -1;
    int menor_uso = INT_MAX;
    
    // primeiro tenta derramar não preservados
    for(int i = 0; i < 32; i++) {
        if(regs[i].preservado) continue;
        
        if(regs[i].ultimo_uso < menor_uso) {
            menor_uso = regs[i].ultimo_uso;
            indice_derramar = i;
        }
    }
    
    // se não achou não preservado, derramar qualquer um
    if(indice_derramar == -1) {
        for(int i = 0; i < 32; i++) {
            if(regs[i].ultimo_uso < menor_uso) {
                menor_uso = regs[i].ultimo_uso;
                indice_derramar = i;
            }
        }
    }
    
    if(indice_derramar == -1) {
        fprintf(stderr, "ERRO: Nenhum registrador disponível para derramamento\n");
        exit(1);
    }
    
    printf("DEBUG: Derramando registrador %s\n", regs[indice_derramar].nome);
    
    // derramar o registrador selecionado
    derrame(s, regs[indice_derramar].nome);
    
    // aloca o registrador derramado
    regs[indice_derramar].livre = 0;
    regs[indice_derramar].sujo = 1;
    regs[indice_derramar].ultimo_uso = contador_uso;
    
    printf("DEBUG: Registrador %s alocado após derramamento\n", regs[indice_derramar].nome);
    
    return regs[indice_derramar].nome;
}

void liberar_reg(const char* nome_reg) {
    printf("DEBUG: Liberando registrador %s\n", nome_reg);
    for(int i = 0; i < 32; i++) {
        if(strcmp(regs[i].nome, nome_reg) == 0) {
            regs[i].livre = 1;
            regs[i].sujo = 0;
            regs[i].constante = 0;
            printf("DEBUG: Registrador %s liberado\n", nome_reg);
            break;
        }
    }
}

void derrame(FILE* s, const char* reg_nome) {
    for(int i = 0; i < 32; i++) {
        if(strcmp(regs[i].nome, reg_nome) == 0) {
            if(regs[i].sujo) {
                fprintf(s, "  str %s, [sp, -16]!  // Derrrame %s\n", reg_nome, reg_nome);
                regs[i].sujo = 0;
            }
            regs[i].livre = 1;
            break;
        }
    }
}

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
        case T_POR: return "por";
        case T_ENQ: return "enq";
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

    while(1) {
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
        else if(strcmp(L.tk.lex, "por") == 0) L.tk.tipo = T_POR;
        else if(strcmp(L.tk.lex, "enq") == 0) L.tk.tipo = T_ENQ;
        else if(strcmp(L.tk.lex, "retorne") == 0) L.tk.tipo = T_RETORNAR;
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
    const char* reg = alocar_reg(s, T_pLONGO);
    fprintf(s, "  ldr %s, =%s\n", reg, texs[id].nome);
    fprintf(s, "  mov x0, %s\n", reg);
    liberar_reg(reg);
    proximoToken();
    return T_TEX;
}

TipoToken tratar_id(FILE* s, int escopo) {
    char id[32];
    strcpy(id, L.tk.lex);
    Variavel* var = buscar_var(id, escopo);
    
    if(!var) {
        Funcao* fn = buscar_fn(id);
        if(fn) {
            proximoToken(); // consume o ID
            excessao(T_PAREN_ESQ);
            return tratar_chamada_funcao(s, escopo, id, fn);
        } else fatal("variável ou função não declarada");
    }
    proximoToken();
    
    // verifica se é acesso a array: id seguido de '['
    if(L.tk.tipo == T_COL_ESQ) {
        excessao(T_COL_ESQ);
        // calcula o indice(resultado em w0)
        expressao(s, escopo);
        const char* reg_indice = alocar_reg(s, T_pINT);
        fprintf(s, "  mov %s, w0\n", reg_indice); // salva o índice
        
        excessao(T_COL_DIR);
        
        const char* reg_base = alocar_reg(s, T_pLONGO);
        const char* reg_tam = alocar_reg(s, T_pINT);
        const char* reg_pos = alocar_reg(s, T_pINT);
        
        // se for parametro, carrega o endereço base da pilha (é um ponteiro)
        if(var->eh_parametro) 
            fprintf(s, "  ldr %s, [x29, %d]\n", reg_base, var->pos); // carrega o endereço base do array
        else 
            fprintf(s, "  add %s, x29, %d\n", reg_base, var->pos); // endereço base do array(local)
        
        fprintf(s, "  mov %s, %d\n", reg_tam, tam_tipo(var->tipo_base)); // tamanho do item
        // calcula a posição: índice * tamanho
        fprintf(s, "  mul %s, %s, %s\n", reg_pos, reg_indice, reg_tam);
        
        // ajusta a posição baseado no tamanho do tipo
        if(tam_tipo(var->tipo_base) == 1) 
            fprintf(s, "  add %s, %s, %s\n", reg_base, reg_base, reg_pos); // bytes: soma direta
        else if(tam_tipo(var->tipo_base) == 4) 
            fprintf(s, "  add %s, %s, %s, lsl 2\n", reg_base, reg_base, reg_pos); // inteiros: multiplica por 4
        else if(tam_tipo(var->tipo_base) == 8) 
            fprintf(s, "  add %s, %s, %s, lsl 3\n", reg_base, reg_base, reg_pos); // longos: multiplica por 8
        
        // carrega o valor do item
        if(var->tipo_base == T_pCAR || var->tipo_base == T_pINT || var->tipo_base == T_pBOOL) 
            fprintf(s, "  ldrb w0, [%s]\n", reg_base);
        else if(var->tipo_base == T_pFLU) 
            fprintf(s, "  ldr s0, [%s]\n", reg_base);
        else if(var->tipo_base == T_pDOBRO) 
            fprintf(s, "  ldr d0, [%s]\n", reg_base);
        
        liberar_reg(reg_indice);
        liberar_reg(reg_base);
        liberar_reg(reg_tam);
        liberar_reg(reg_pos);
        
        return var->tipo_base;
    } else if(var->eh_array) {
        const char* reg = alocar_reg(s, T_pLONGO);
        if(var->eh_parametro) 
            fprintf(s, "  ldr %s, [x29, %d]\n", reg, var->pos); // carrega o endereço do array(parâmetro)
        else 
            fprintf(s, "  add %s, x29, %d\n", reg, var->pos); // endereço do array local
        fprintf(s, "  mov x0, %s\n", reg);
        liberar_reg(reg);
        return T_pLONGO;
    } else if(var->eh_ponteiro) {
        const char* reg = alocar_reg(s, T_pLONGO);
        fprintf(s, "  ldr %s, [x29, %d]\n", reg, var->pos);
        fprintf(s, "  mov x0, %s\n", reg);
        liberar_reg(reg);
        return T_pLONGO;
    } else {
        carregar_valor(s, var);
        return var->tipo_base;
    }
}

TipoToken tratar_chamada_funcao(FILE* s, int escopo, const char* nome, Funcao* fn) {
    if(fn == NULL) fatal("INTERNO CRITICO, FUNÇÃO INEXISTENTE!");
    int param_conta = 0;
    TipoToken param_tipos[MAX_PARAMS];
    
    if(L.tk.tipo != T_PAREN_DIR) {
        do {
            param_tipos[param_conta] = expressao(s, escopo);
            
            // Usar registradores para salvar temporariamente
            const char* reg_temp;
            if(param_tipos[param_conta] == T_pFLU) {
                reg_temp = alocar_reg(s, T_pFLU);
                fprintf(s, "  fmov %s, s0\n", reg_temp);
                fprintf(s, "  str %s, [sp, -16]!\n", reg_temp);
            } else if(param_tipos[param_conta] == T_pDOBRO) {
                reg_temp = alocar_reg(s, T_pDOBRO);
                fprintf(s, "  fmov %s, d0\n", reg_temp);
                fprintf(s, "  str %s, [sp, -16]!\n", reg_temp);
            } else if(tam_tipo(param_tipos[param_conta]) <= 4) {
                reg_temp = alocar_reg(s, T_pINT);
                fprintf(s, "  mov %s, w0\n", reg_temp);
                fprintf(s, "  str %s, [sp, -16]!\n", reg_temp);
            } else {
                reg_temp = alocar_reg(s, T_pLONGO);
                fprintf(s, "  mov %s, x0\n", reg_temp);
                fprintf(s, "  str %s, [sp, -16]!\n", reg_temp);
            }
            liberar_reg(reg_temp);
            
            param_conta++;
            if(param_conta >= MAX_PARAMS) fatal("excesso de parâmetros na chamada de função, max: 9");
        } while(L.tk.tipo == T_VIRGULA && (proximoToken(), 1));
    }
    excessao(T_PAREN_DIR); // consome ')'
    
    int int_reg = 0;
    int fp_s_reg = 0;
    int fp_d_reg = 0;
    
    for(int i = 0; i < param_conta; i++) {
        int pos = (param_conta - i - 1) * 16;
        
        if(int_reg < 8 && (param_tipos[i] == T_pINT || param_tipos[i] == T_pLONGO || param_tipos[i] == T_pCAR || param_tipos[i] == T_pBOOL)) {
            const char* reg = alocar_reg(s, T_pLONGO);
            fprintf(s, "  ldr %s, [sp, %d]\n", reg, pos);
            fprintf(s, "  mov x%d, %s\n", int_reg, reg);
            liberar_reg(reg);
            int_reg++;
        } else if(fp_s_reg < 8 && param_tipos[i] == T_pFLU) {
            const char* reg = alocar_reg(s, T_pFLU);
            fprintf(s, "  ldr %s, [sp, %d]\n", reg, pos);
            fprintf(s, "  fmov s%d, %s\n", fp_s_reg, reg);
            liberar_reg(reg);
            fp_s_reg++;
        } else if(fp_d_reg < 8 && param_tipos[i] == T_pDOBRO) {
            const char* reg = alocar_reg(s, T_pDOBRO);
            fprintf(s, "  ldr %s, [sp, %d]\n", reg, pos);
            fprintf(s, "  fmov d%d, %s\n", fp_d_reg, reg);
            liberar_reg(reg);
            fp_d_reg++;
        } else {
            // args 9+ = pilha
            const char* reg = alocar_reg(s, param_tipos[i]);
            if(param_tipos[i] == T_pFLU) {
                fprintf(s, "  ldr %s, [sp, %d]\n", reg, pos);
                fprintf(s, "  str %s, [sp, -16]!\n", reg);
            } else if (param_tipos[i] == T_pDOBRO) {
                fprintf(s, "  ldr %s, [sp, %d]\n", reg, pos);
                fprintf(s, "  str %s, [sp, -16]!\n", reg);
            } else if (tam_tipo(param_tipos[i]) <= 4) {
                fprintf(s, "  ldr %s, [sp, %d]\n", reg, pos);
                fprintf(s, "  str %s, [sp, -16]!\n", reg);
            } else {
                fprintf(s, "  ldr %s, [sp, %d]\n", reg, pos);
                fprintf(s, "  str %s, [sp, -16]!\n", reg);
            }
            liberar_reg(reg);
        }
    }
    
    if(param_conta > 0) fprintf(s, "  add sp, sp, %d\n", param_conta * 16);
    
    fprintf(s, "  bl %s\n", nome);
    
    // Garantir retorno no registrador correto
    if(fn->retorno == T_pFLU) {
        const char* reg = alocar_reg(s, T_pFLU);
        fprintf(s, "  fmov %s, s0\n", reg);
        fprintf(s, "  fmov s0, %s\n", reg);
        liberar_reg(reg);
    } else if(fn->retorno == T_pDOBRO) {
        const char* reg = alocar_reg(s, T_pDOBRO);
        fprintf(s, "  fmov %s, d0\n", reg);
        fprintf(s, "  fmov d0, %s\n", reg);
        liberar_reg(reg);
    }
    
    return fn->retorno;
}

TipoToken tratar_inteiro(FILE* s) {
    char num[32];
    strcpy(num, L.tk.lex);
    long l_val = L.tk.valor_l;
    proximoToken();
    
    const char* reg = alocar_reg(s, T_pINT);
    if(l_val < 65536) {
        fprintf(s, "  mov %s, %ld\n", reg, l_val);
    } else {
        int titulo = add_const(T_INT, num, 0.0, l_val);
        fprintf(s, "  ldr %s, =const_%d\n", reg, titulo);
        fprintf(s, "  ldr %s, [%s]\n", reg, reg);
    }
    fprintf(s, "  mov w0, %s\n", reg);
    liberar_reg(reg);
    return T_pINT;
}

TipoToken tratar_flutuante(FILE* s) {
    char num[32];
    strcpy(num, L.tk.lex);
    double d_val = L.tk.valor_d;
    TipoToken const_tipo = L.tk.tipo;
    proximoToken();
    
    const char* reg = alocar_reg(s, const_tipo == T_FLU ? T_pFLU : T_pDOBRO);
    int titulo = add_const(const_tipo, num, d_val, 0);
    
    if(const_tipo == T_FLU) {
        fprintf(s, "  ldr %s, =const_%d\n", reg, titulo);
        fprintf(s, "  ldr %s, [%s]\n", reg, reg);
        fprintf(s, "  fmov s0, %s\n", reg);
    } else {
        fprintf(s, "  ldr %s, =const_%d\n", reg, titulo);
        fprintf(s, "  ldr %s, [%s]\n", reg, reg);
        fprintf(s, "  fmov d0, %s\n", reg);
    }
    
    liberar_reg(reg);
    return const_tipo == T_FLU ? T_pFLU : T_pDOBRO;
}

TipoToken tratar_caractere(FILE* s) {
    char val = L.tk.lex[0];
    proximoToken();
    
    const char* reg = alocar_reg(s, T_pCAR);
    fprintf(s, "  mov %s, %d\n", reg, val);
    fprintf(s, "  mov w0, %s\n", reg);
    liberar_reg(reg);
    
    return T_pCAR;
}

// [BUSCA]:
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

void coletar_args(FILE* s, Funcao* f) {
    int int_reg_idc = 0;
    int fp_s_reg_idc = 0; // single precision
    int fp_d_reg_idc = 0; // double precision
    int pilha_pos = 0; // pra parametros que vão para a pilha
    
    f->param_pos = 0; // reseta

    while(L.tk.tipo != T_PAREN_DIR) {
        Variavel* var = &f->vars[f->var_conta];
        
        // Alocar registrador apropriado baseado no tipo
        const char* reg_alocado = NULL;
        if(int_reg_idc < 8 && (L.tk.tipo == T_pINT || L.tk.tipo == T_pLONGO || 
            L.tk.tipo == T_pCAR || L.tk.tipo == T_pBOOL)) {
            reg_alocado = alocar_reg(s, L.tk.tipo);
            sprintf(var->reg, "x%d", int_reg_idc++);
            var->pos = -1;  // indica registrador
        } else if(fp_s_reg_idc < 8 && L.tk.tipo == T_pFLU) {
            reg_alocado = alocar_reg(s, T_pFLU);
            sprintf(var->reg, "s%d", fp_s_reg_idc++);
            var->pos = -1;
        } else if(fp_d_reg_idc < 8 && L.tk.tipo == T_pDOBRO) {
            reg_alocado = alocar_reg(s, T_pDOBRO);
            sprintf(var->reg, "d%d", fp_d_reg_idc++);
            var->pos = -1;
        } else {
            // vai pra a pilha - alocar registrador temporário
            reg_alocado = alocar_reg(s, L.tk.tipo);
            var->reg[0] = '\0';
            var->pos = pilha_pos;
            pilha_pos += 8;  // 8 bytes por parametro
        }
        
        if(reg_alocado) liberar_reg(reg_alocado);
        declaracao_var(s, &pilha_pos, 0, 1);
        
        if(L.tk.tipo == T_VIRGULA) proximoToken();
        else break;
    }
}

// [VERIFICAÇÃO]:
void verificar_retorno(FILE* s, int escopo) {
    excessao(T_RETORNAR);
    if(L.tk.tipo == T_PONTO_VIRGULA) {
        fprintf(s, "  b .epilogo_%d\n", fn_cnt - 1);
        excessao(T_PONTO_VIRGULA);
        return;
    }
    TipoToken tipo_exp = expressao(s, escopo);
    // em funções que retornam ponteiro/array espera T_pLONGO
    if(funcs[fn_cnt - 1].retorno == T_pLONGO) {
        if(tipo_exp != T_pLONGO) fatal("[verificar_retorno] retorno deve ser ponteiro ou endereço");
    } else if(!tipos_compativeis(funcs[fn_cnt - 1].retorno, tipo_exp)) {
        fatal("[verificar_retorno] tipo de retorno incompatível");
    }
    fprintf(s, "  b .epilogo_%d\n", fn_cnt - 1);
    excessao(T_PONTO_VIRGULA);
}

void verificar_atribuicao(FILE* s, const char* id, int escopo) {
    Variavel* var = buscar_var(id, escopo);
    if(!var) fatal("[verificar_atribuicao] variável não declarada");
    
    if(var->eh_array) fatal("[verificar_atribuicao] não é possível armazenar valor direto em array");
    
    excessao(T_IGUAL);
    TipoToken tipo_exp = expressao(s, escopo);
    armazenar_valor(s, var);
}

void verificar_se(FILE* s, int escopo) {
    excessao(T_SE);
    excessao(T_PAREN_ESQ);
    
    TipoToken tipo_cond = expressao(s, escopo);
    
    if(tipo_cond != T_pINT && tipo_cond != T_pBOOL) fatal("[verificar_se] condição deve ser inteiro ou booleano");
    
    excessao(T_PAREN_DIR);
    
    int rotulo_falso = escopo_global++;
    
    // Usar registrador para a comparação
    const char* reg_cond = alocar_reg(s, T_pINT);
    fprintf(s, "  mov %s, w0\n", reg_cond);
    fprintf(s, "  cmp %s, 0\n", reg_cond);
    fprintf(s, "  beq .B%d\n", rotulo_falso);
    liberar_reg(reg_cond);
    
    if(L.tk.tipo == T_CHAVE_ESQ) {
        proximoToken();
        while(L.tk.tipo != T_CHAVE_DIR) verificar_stmt(s, &funcs[fn_cnt - 1].frame_tam, escopo + 1);
        excessao(T_CHAVE_DIR);
    } else verificar_stmt(s, &funcs[fn_cnt - 1].frame_tam, escopo + 1);
    
    int rotulo_fim = escopo_global++;
    fprintf(s, "  b .B%d\n", rotulo_fim);
    fprintf(s, ".B%d:\n", rotulo_falso);
    
    if(L.tk.tipo == T_SENAO) {
        proximoToken();
        if(L.tk.tipo == T_CHAVE_ESQ) {
            proximoToken();
            while(L.tk.tipo != T_CHAVE_DIR) verificar_stmt(s, &funcs[fn_cnt - 1].frame_tam, escopo + 1);
            excessao(T_CHAVE_DIR);
        } else verificar_stmt(s, &funcs[fn_cnt - 1].frame_tam, escopo + 1);
    }
    fprintf(s, ".B%d:\n", rotulo_fim);
}

void verificar_por(FILE* s, int escopo) {
    excessao(T_POR);
    excessao(T_PAREN_ESQ);
    
    int novo_escopo = ++escopo_global;
    // declaração da variavel de controle
    if(L.tk.tipo == T_pINT) {
        declaracao_var(s, &funcs[fn_cnt - 1].frame_tam, novo_escopo, 0);
        excessao(T_PONTO_VIRGULA);
    } else if(L.tk.tipo == T_ID) {
        // atribuição a variavel que existe
        char id[32];
        strcpy(id, L.tk.lex);
        Variavel* var = buscar_var(id, escopo);
        if(!var) fatal("[verificar_por] variável não declarada");
        
        proximoToken();
        if(L.tk.tipo == T_IGUAL) verificar_atribuicao(s, id, escopo);
        excessao(T_PONTO_VIRGULA);
    } else {
        // pode ser vazio sem inicialização
        excessao(T_PONTO_VIRGULA);
    }
    
    // condição
    int rotulo_inicio = escopo_global++;
    int rotulo_fim = escopo_global++;
    
    fprintf(s, ".B%d:\n", rotulo_inicio);
    TipoToken tipo_cond = expressao(s, novo_escopo);
    if(tipo_cond != T_pINT && tipo_cond != T_pBOOL) {
        fatal("[verificar_por] condição do loop deve ser inteiro ou booleano");
    }
    
    // Usar registrador para a condição
    const char* reg_cond = alocar_reg(s, T_pINT);
    fprintf(s, "  mov %s, w0\n", reg_cond);
    fprintf(s, "  cmp %s, 0\n", reg_cond);
    fprintf(s, "  beq .B%d\n", rotulo_fim);
    liberar_reg(reg_cond);
    
    excessao(T_PONTO_VIRGULA);
    
    size_t inicio_incremento = L.pos;
    
    while(L.tk.tipo != T_PAREN_DIR && L.tk.tipo != T_FIM) proximoToken();
    
    size_t fim_incremento = L.pos;
    excessao(T_PAREN_DIR);
    
    if(L.tk.tipo == T_CHAVE_ESQ) {
        proximoToken();
        while(L.tk.tipo != T_CHAVE_DIR) verificar_stmt(s, &funcs[fn_cnt-1].frame_tam, novo_escopo);
        excessao(T_CHAVE_DIR);
    } else verificar_stmt(s, &funcs[fn_cnt-1].frame_tam, novo_escopo);
    fprintf(s, "  // incremento\n");
    
    Lexer estado_original = L;
    
    L.pos = inicio_incremento;
    L.linha_atual = estado_original.linha_atual;
    L.coluna_atual = estado_original.coluna_atual;
    proximoToken();
    
    while(L.pos < fim_incremento) {
        if(L.tk.tipo == T_ID) {
            char id[32];
            strcpy(id, L.tk.lex);
            proximoToken();
            
            if(L.tk.tipo == T_IGUAL) verificar_atribuicao(s, id, novo_escopo);
            else expressao(s, novo_escopo);
        } else expressao(s, novo_escopo);
        if(L.tk.tipo == T_VIRGULA) proximoToken();
    }
    L = estado_original;
    
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
    
    // Usar registrador para a condição
    const char* reg_cond = alocar_reg(s, T_pINT);
    fprintf(s, "  mov %s, w0\n", reg_cond);
    fprintf(s, "  cmp %s, 0\n", reg_cond);
    fprintf(s, "  beq .B%d\n", rotulo_fim);
    liberar_reg(reg_cond);
    
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
        
        if(L.tk.tipo != T_PONTO_VIRGULA) fatal("[verificar_stmt] ponto e vírgula esperado após o caminho do arquivo");
        
        proximoToken();
        
        FILE* arquivo_include = fopen(caminho, "r");
        if(!arquivo_include) {
            char mensagem_erro[300];
            snprintf(mensagem_erro, sizeof(mensagem_erro), "[verificar_stmt] não foi possível abrir: %s", caminho);
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
        if(L.tk.tipo != T_REG) fatal("[verificar_stmt] registrador esperado");
        char reg[16]; strcpy(reg, L.tk.lex);
        proximoToken(); excessao(T_IGUAL);
        if(L.tk.tipo != T_INT && L.tk.tipo != T_CAR) 
            fatal("[verificar_stmt] valor inteiro ou caractere esperado");
        char val[16]; strcpy(val, L.tk.lex);
        proximoToken(); excessao(T_PONTO_VIRGULA);
        
        // Alocar registrador para a operação
        const char* reg_temp = alocar_reg(s, T_pINT);
        fprintf(s, "  mov %s, %s\n", reg_temp, val);
        fprintf(s, "  mov %s, %s\n", reg, reg_temp);
        liberar_reg(reg_temp);
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
        declaracao_var(s, pos, escopo, 0);
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
        } else if(L.tk.tipo == T_COL_ESQ) {
            // >>>>>>ACESSO A ELEMENTO DE ARRAY<<<<<<
            Variavel* var = buscar_var(idn, escopo);
            if(!var || !var->eh_array) fatal("[verificar_stmt] não é um array");
            
            excessao(T_COL_ESQ);
            expressao(s, escopo); // indice(resultado em w0)
            
            const char* reg_indice = alocar_reg(s, T_pINT);
            fprintf(s, "  mov %s, w0\n", reg_indice); // salva o indice
            
            excessao(T_COL_DIR);
            
            if(L.tk.tipo == T_IGUAL) {
                // atribuição a item de array
                proximoToken(); // consome '='
                TipoToken tipo_valor = expressao(s, escopo); // valor(resultado em w0)
                
                if(!tipos_compativeis(var->tipo_base, tipo_valor)) {
                    char msg[100];
                    sprintf(msg, "[verificar_stmt] tipo incompatível: esperado %s, encontrado %s", 
                            token_str(var->tipo_base), token_str(tipo_valor));
                    fatal(msg);
                }
                
                const char* reg_base = alocar_reg(s, T_pLONGO);
                const char* reg_tam = alocar_reg(s, T_pINT);
                const char* reg_pos = alocar_reg(s, T_pINT);
                
                // se for parâmetro, carrega o endereço base da pilha(é um ponteiro)
                if(var->eh_parametro) 
                    fprintf(s, "  ldr %s, [x29, %d]\n", reg_base, var->pos); // carrega o endereço base do array
                else 
                    fprintf(s, "  add %s, x29, %d\n", reg_base, var->pos); // endereço base do array(local)
                
                fprintf(s, "  mov %s, %d\n", reg_tam, tam_tipo(var->tipo_base)); // tamanho item
                fprintf(s, "  mul %s, %s, %s\n", reg_pos, reg_indice, reg_tam); // indice * tamanho
                
                if(tam_tipo(var->tipo_base) == 1) 
                    fprintf(s, "  add %s, %s, %s\n", reg_base, reg_base, reg_pos); // bytes: soma direta
                else if(tam_tipo(var->tipo_base) == 4) 
                    fprintf(s, "  add %s, %s, %s, lsl 2\n", reg_base, reg_base, reg_pos); // inteiros: multiplica por 4
                else if(tam_tipo(var->tipo_base) == 8) 
                    fprintf(s, "  add %s, %s, %s, lsl 3\n", reg_base, reg_base, reg_pos); // longos: multiplica por 8
                
                // armazena o valor no item do array
                const char* reg_valor = alocar_reg(s, tipo_valor);
                if(tipo_valor == T_pFLU) {
                    fprintf(s, "  fmov %s, s0\n", reg_valor);
                    fprintf(s, "  str %s, [%s]\n", reg_valor, reg_base);
                } else if(tipo_valor == T_pDOBRO) {
                    fprintf(s, "  fmov %s, d0\n", reg_valor);
                    fprintf(s, "  str %s, [%s]\n", reg_valor, reg_base);
                } else if(tam_tipo(tipo_valor) <= 4) {
                    fprintf(s, "  mov %s, w0\n", reg_valor);
                    fprintf(s, "  str %s, [%s]\n", reg_valor, reg_base);
                } else {
                    fprintf(s, "  mov %s, x0\n", reg_valor);
                    fprintf(s, "  str %s, [%s]\n", reg_valor, reg_base);
                }
                
                liberar_reg(reg_indice);
                liberar_reg(reg_base);
                liberar_reg(reg_tam);
                liberar_reg(reg_pos);
                liberar_reg(reg_valor);
            } else {
                fatal("[verificar_stmt] acesso a elemento de array deve ser usado em expressão ou atribuição");
            }
            excessao(T_PONTO_VIRGULA);
            return;
        } else if(L.tk.tipo == T_PAREN_ESQ) {
            excessao(T_PAREN_ESQ);
            
            if(strcmp(idn,"escrever") == 0) {
                while(1) {
                    if(L.tk.tipo == T_ID) {
                        Variavel* var = buscar_var(L.tk.lex, escopo);
                        if(var && var->eh_ponteiro) {
                            const char* reg = alocar_reg(s, T_pLONGO);
                            if(var->eh_parametro) 
                                fprintf(s, "  ldr %s, [x29, %d]\n", reg, var->pos);
                            else 
                                fprintf(s, "  ldr %s, [x29, %d]\n", reg, var->pos);
                            fprintf(s, "  mov x0, %s\n", reg);
                            liberar_reg(reg);
                            escrever_valor(s, T_TEX);
                            proximoToken();
                        } else if(var && var->eh_array && var->tipo_base == T_pCAR) {
                            const char* reg = alocar_reg(s, T_pLONGO);
                            fprintf(s, "  add %s, x29, %d\n", reg, var->pos);
                            fprintf(s, "  mov x0, %s\n", reg);
                            liberar_reg(reg);
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
                return;
            } else {
                // >>>>>CHAMADA DE FUNÇÃO NORMAL<<<<<
                Funcao* fn = buscar_fn(idn);
                if(!fn) fatal("[verificar_stmt] função não declarada");
                
                tratar_chamada_funcao(s, escopo, idn, fn);
                excessao(T_PONTO_VIRGULA);
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
    TipoToken tipo_real = (eh_ponteiro || eh_array) ? T_pLONGO : rt;
    
    funcs[fn_cnt].var_conta = 0;
    funcs[fn_cnt].retorno = tipo_real;
    funcs[fn_cnt].escopo_atual = 0;
    funcs[fn_cnt].frame_tam = 0;
    funcs[fn_cnt].param_pos = 16;
    strcpy(funcs[fn_cnt++].nome, fnome);
    proximoToken();

    excessao(T_PAREN_ESQ);
    coletar_args(s, &funcs[fn_cnt - 1]);
    excessao(T_PAREN_DIR);

    if(L.tk.tipo == T_PONTO_VIRGULA) {
        eh_prototipo = 1;
        proximoToken();
        return; // pré definição não gera codigo
    } else excessao(T_CHAVE_ESQ);

    if(!eh_prototipo) {
        int pos = -16; // começa apos registros salvos
        Lexer salvo = L;
        // calcular espaço pra variáveis locais
        while(L.tk.tipo != T_CHAVE_DIR) {
            if(L.tk.tipo == T_pCAR || L.tk.tipo == T_pINT || L.tk.tipo == T_pFLU || L.tk.tipo == T_pBOOL || L.tk.tipo == T_pDOBRO || L.tk.tipo == T_pLONGO) {
                TipoToken tipo = L.tk.tipo;
                int tam = tam_tipo(tipo);
                proximoToken();
                
                if(L.tk.tipo == T_ID) {
                    pos -= tam;
                    // alinhamento para o tipo
                    pos = pos & ~(tam - 1);
                    proximoToken();
                    
                    if(L.tk.tipo == T_IGUAL) {
                        while(L.tk.tipo != T_PONTO_VIRGULA && L.tk.tipo != T_CHAVE_DIR) {
                            proximoToken();
                        }
                    }
                    if(L.tk.tipo == T_PONTO_VIRGULA) proximoToken();
                }
            } else proximoToken();
        }
        L = salvo;
        // calcula o tamanho do frame
        int frame_tam = ((-pos + 15) & ~15); // alinhado pra 16 bytes
        
        // espaço para registros preservados
        if(tipo_real != T_pVAZIO) frame_tam += 32; // espaço pra registros salvos(x19-x22)
        
        // espaço para parametros que podem ser salvos
        for(int i = 0; i < funcs[fn_cnt-1].var_conta; i++) {
            Variavel* var = &funcs[fn_cnt-1].vars[i];
            if(var->eh_parametro && var->reg[0] != '\0') frame_tam += 8; // 8 bytes por parâmetro salvo
        }
        
        // garante mínimo de 16 bytes e alinhamento de 16
        frame_tam = (frame_tam < 16) ? 16 : frame_tam;
        frame_tam = (frame_tam + 15) & ~15;
        
        funcs[fn_cnt - 1].frame_tam = frame_tam;

        // >>>>>>PROLOGO DA FUNÇÃO<<<<<<
        fprintf(s, ".align 2\n");
        fprintf(s, "%s:\n", fnome);
        fprintf(s, "  // [prologo]\n");
        
        // prologo usando registradores alocados
        const char* reg_fp = alocar_reg(s, T_pLONGO);
        const char* reg_lr = alocar_reg(s, T_pLONGO);
        
        fprintf(s, "  stp %s, %s, [sp, -%d]!\n", reg_fp, reg_lr, frame_tam);
        fprintf(s, "  mov %s, sp\n", reg_fp);
        
        liberar_reg(reg_fp);
        liberar_reg(reg_lr);

        if(tipo_real != T_pVAZIO) {
            const char* reg1 = alocar_reg(s, T_pLONGO);
            const char* reg2 = alocar_reg(s, T_pLONGO);
            const char* reg3 = alocar_reg(s, T_pLONGO);
            const char* reg4 = alocar_reg(s, T_pLONGO);
            
            fprintf(s, "  stp %s, %s, [%s, 16]\n", reg1, reg2, reg_fp);
            fprintf(s, "  stp %s, %s, [%s, 32]\n", reg3, reg4, reg_fp);
            
            liberar_reg(reg1);
            liberar_reg(reg2);
            liberar_reg(reg3);
            liberar_reg(reg4);
        }

        // salva parametros que tão nos registradores
        int param_pos = 16;
        for(int i = 0; i < funcs[fn_cnt-1].var_conta; i++) {
            Variavel* var = &funcs[fn_cnt-1].vars[i];
            if(var->eh_parametro && var->reg[0] != '\0') {
                const char* reg_temp = alocar_reg(s, T_pLONGO);
                fprintf(s, "  mov %s, %s\n", reg_temp, var->reg);
                fprintf(s, "  str %s, [x29, %d]  // salvar param %s\n", 
                        reg_temp, param_pos, var->nome);
                liberar_reg(reg_temp);
                var->pos = param_pos; // atualiza posição real
                param_pos += 8;
            }
        }
        // gera o corpo da função
        while(L.tk.tipo != T_CHAVE_DIR) verificar_stmt(s, &pos, 0);
        fprintf(s, "  b .epilogo_%d\n", fn_cnt - 1);
        // >>>>>>EPÍLOGO DA FUNÇÃO<<<<<<
        fprintf(s, ".epilogo_%d:\n", fn_cnt - 1);
        
        if(tipo_real != T_pVAZIO) {
            const char* reg1 = alocar_reg(s, T_pLONGO);
            const char* reg2 = alocar_reg(s, T_pLONGO);
            const char* reg3 = alocar_reg(s, T_pLONGO);
            const char* reg4 = alocar_reg(s, T_pLONGO);
            
            fprintf(s, "  ldp %s, %s, [x29, 16]\n", reg1, reg2);
            fprintf(s, "  ldp %s, %s, [x29, 32]\n", reg3, reg4);
            
            liberar_reg(reg1);
            liberar_reg(reg2);
            liberar_reg(reg3);
            liberar_reg(reg4);
        }
        
        const char* reg_fp_epi = alocar_reg(s, T_pLONGO);
        const char* reg_lr_epi = alocar_reg(s, T_pLONGO);
        
        fprintf(s, "  mov sp, %s\n", reg_fp_epi);
        fprintf(s, "  ldp %s, %s, [sp], %d\n", reg_fp_epi, reg_lr_epi, frame_tam);
        fprintf(s, "  ret\n");
        
        liberar_reg(reg_fp_epi);
        liberar_reg(reg_lr_epi);
        
        proximoToken(); // consome }
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
        "  bl inicio\n"
        "  mov x0, 0\n"
        "  mov x8, 93\n"
        "  svc 0\n");
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
        if(constpos[i].tipo == T_INT) fprintf(s, "  .word %ld\n", constpos[i].l_val);
        else if(constpos[i].tipo == T_FLU) {
            float f = (float)constpos[i].d_val;
            fprintf(s, "  .float %f\n", f);
        } else if(constpos[i].tipo == T_DOBRO) fprintf(s, "  .double %f\n", constpos[i].d_val);
    }
    fprintf(s, "  .section .text\n\n");
}

void gerar_operacao(FILE* s, TipoToken op, TipoToken tipo) {
    const char* reg1 = alocar_reg(s, tipo);
    const char* reg2 = alocar_reg(s, tipo);
    
    switch(op) {
        case T_MAIS: 
            if(tipo == T_pFLU) {
                fprintf(s, "  fmov %s, s1\n", reg1);
                fprintf(s, "  fmov %s, s0\n", reg2);
                fprintf(s, "  fadd %s, %s, %s\n", reg2, reg1, reg2);
                fprintf(s, "  fmov s0, %s\n", reg2);
            } else if(tipo == T_pDOBRO) {
                fprintf(s, "  fmov %s, d1\n", reg1);
                fprintf(s, "  fmov %s, d0\n", reg2);
                fprintf(s, "  fadd %s, %s, %s\n", reg2, reg1, reg2);
                fprintf(s, "  fmov d0, %s\n", reg2);
            } else {
                fprintf(s, "  mov %s, w1\n", reg1);
                fprintf(s, "  mov %s, w0\n", reg2);
                fprintf(s, "  add %s, %s, %s\n", reg2, reg1, reg2);
                fprintf(s, "  mov w0, %s\n", reg2);
            }
            break;
        case T_MENOS: 
            if(tipo == T_pFLU) {
                fprintf(s, "  fmov %s, s1\n", reg1);
                fprintf(s, "  fmov %s, s0\n", reg2);
                fprintf(s, "  fsub %s, %s, %s\n", reg2, reg1, reg2);
                fprintf(s, "  fmov s0, %s\n", reg2);
            } else if(tipo == T_pDOBRO) {
                fprintf(s, "  fmov %s, d1\n", reg1);
                fprintf(s, "  fmov %s, d0\n", reg2);
                fprintf(s, "  fsub %s, %s, %s\n", reg2, reg1, reg2);
                fprintf(s, "  fmov d0, %s\n", reg2);
            } else {
                fprintf(s, "  mov %s, w1\n", reg1);
                fprintf(s, "  mov %s, w0\n", reg2);
                fprintf(s, "  sub %s, %s, %s\n", reg2, reg1, reg2);
                fprintf(s, "  mov w0, %s\n", reg2);
            }
            break;
        case T_VEZES: 
            if(tipo == T_pFLU) {
                fprintf(s, "  fmov %s, s1\n", reg1);
                fprintf(s, "  fmov %s, s0\n", reg2);
                fprintf(s, "  fmul %s, %s, %s\n", reg2, reg1, reg2);
                fprintf(s, "  fmov s0, %s\n", reg2);
            } else if(tipo == T_pDOBRO) {
                fprintf(s, "  fmov %s, d1\n", reg1);
                fprintf(s, "  fmov %s, d0\n", reg2);
                fprintf(s, "  fmul %s, %s, %s\n", reg2, reg1, reg2);
                fprintf(s, "  fmov d0, %s\n", reg2);
            } else {
                fprintf(s, "  mov %s, w1\n", reg1);
                fprintf(s, "  mov %s, w0\n", reg2);
                fprintf(s, "  mul %s, %s, %s\n", reg2, reg1, reg2);
                fprintf(s, "  mov w0, %s\n", reg2);
            }
            break;
        case T_DIV: 
            if(tipo == T_pFLU) {
                fprintf(s, "  fmov %s, s1\n", reg1);
                fprintf(s, "  fmov %s, s0\n", reg2);
                fprintf(s, "  fdiv %s, %s, %s\n", reg2, reg1, reg2);
                fprintf(s, "  fmov s0, %s\n", reg2);
            } else if(tipo == T_pDOBRO) {
                fprintf(s, "  fmov %s, d1\n", reg1);
                fprintf(s, "  fmov %s, d0\n", reg2);
                fprintf(s, "  fdiv %s, %s, %s\n", reg2, reg1, reg2);
                fprintf(s, "  fmov d0, %s\n", reg2);
            } else {
                fprintf(s, "  mov %s, w1\n", reg1);
                fprintf(s, "  mov %s, w0\n", reg2);
                fprintf(s, "  sdiv %s, %s, %s\n", reg2, reg1, reg2);
                fprintf(s, "  mov w0, %s\n", reg2);
            }
            break;
        default: fatal("[gerar_operacao] operador inválido");
    }
    liberar_reg(reg1);
    liberar_reg(reg2);
}

void gerar_comparacao(FILE* s, TipoToken op, TipoToken tipo) {
    const char* reg1 = alocar_reg(s, tipo);
    const char* reg2 = alocar_reg(s, tipo);
    const char* reg_res = alocar_reg(s, T_pINT);
    
    switch(op) {
        case T_IGUAL_IGUAL:
            if(tipo == T_pFLU) {
                fprintf(s, "  fmov %s, s1\n", reg1);
                fprintf(s, "  fmov %s, s0\n", reg2);
                fprintf(s, "  fcmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, eq\n", reg_res);
            } else if(tipo == T_pDOBRO) {
                fprintf(s, "  fmov %s, d1\n", reg1);
                fprintf(s, "  fmov %s, d0\n", reg2);
                fprintf(s, "  fcmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, eq\n", reg_res);
            } else {
                fprintf(s, "  mov %s, w1\n", reg1);
                fprintf(s, "  mov %s, w0\n", reg2);
                fprintf(s, "  cmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, eq\n", reg_res);
            }
            break;
        case T_DIFERENTE:
            if(tipo == T_pFLU) {
                fprintf(s, "  fmov %s, s1\n", reg1);
                fprintf(s, "  fmov %s, s0\n", reg2);
                fprintf(s, "  fcmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, ne\n", reg_res);
            } else if(tipo == T_pDOBRO) {
                fprintf(s, "  fmov %s, d1\n", reg1);
                fprintf(s, "  fmov %s, d0\n", reg2);
                fprintf(s, "  fcmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, ne\n", reg_res);
            } else {
                fprintf(s, "  mov %s, w1\n", reg1);
                fprintf(s, "  mov %s, w0\n", reg2);
                fprintf(s, "  cmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, ne\n", reg_res);
            }
            break;
        case T_MAIOR:
            if(tipo == T_pFLU) {
                fprintf(s, "  fmov %s, s1\n", reg1);
                fprintf(s, "  fmov %s, s0\n", reg2);
                fprintf(s, "  fcmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, gt\n", reg_res);
            } else if(tipo == T_pDOBRO) {
                fprintf(s, "  fmov %s, d1\n", reg1);
                fprintf(s, "  fmov %s, d0\n", reg2);
                fprintf(s, "  fcmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, gt\n", reg_res);
            } else {
                fprintf(s, "  mov %s, w1\n", reg1);
                fprintf(s, "  mov %s, w0\n", reg2);
                fprintf(s, "  cmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, gt\n", reg_res);
            }
            break;
        case T_MENOR:
            if(tipo == T_pFLU) {
                fprintf(s, "  fmov %s, s1\n", reg1);
                fprintf(s, "  fmov %s, s0\n", reg2);
                fprintf(s, "  fcmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, lt\n", reg_res);
            } else if(tipo == T_pDOBRO) {
                fprintf(s, "  fmov %s, d1\n", reg1);
                fprintf(s, "  fmov %s, d0\n", reg2);
                fprintf(s, "  fcmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, lt\n", reg_res);
            } else {
                fprintf(s, "  mov %s, w1\n", reg1);
                fprintf(s, "  mov %s, w0\n", reg2);
                fprintf(s, "  cmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, lt\n", reg_res);
            }
            break;
        case T_MAIOR_IGUAL:
            if(tipo == T_pFLU) {
                fprintf(s, "  fmov %s, s1\n", reg1);
                fprintf(s, "  fmov %s, s0\n", reg2);
                fprintf(s, "  fcmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, ge\n", reg_res);
            } else if(tipo == T_pDOBRO) {
                fprintf(s, "  fmov %s, d1\n", reg1);
                fprintf(s, "  fmov %s, d0\n", reg2);
                fprintf(s, "  fcmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, ge\n", reg_res);
            } else {
                fprintf(s, "  mov %s, w1\n", reg1);
                fprintf(s, "  mov %s, w0\n", reg2);
                fprintf(s, "  cmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, ge\n", reg_res);
            }
            break;
        case T_MENOR_IGUAL:
            if(tipo == T_pFLU) {
                fprintf(s, "  fmov %s, s1\n", reg1);
                fprintf(s, "  fmov %s, s0\n", reg2);
                fprintf(s, "  fcmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, le\n", reg_res);
            } else if(tipo == T_pDOBRO) {
                fprintf(s, "  fmov %s, d1\n", reg1);
                fprintf(s, "  fmov %s, d0\n", reg2);
                fprintf(s, "  fcmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, le\n", reg_res);
            } else {
                fprintf(s, "  mov %s, w1\n", reg1);
                fprintf(s, "  mov %s, w0\n", reg2);
                fprintf(s, "  cmp %s, %s\n", reg1, reg2);
                fprintf(s, "  cset %s, le\n", reg_res);
            }
            break;
        default: fatal("[gerar_comparacao] operador de comparação inválido");
    }
    fprintf(s, "  mov w0, %s\n", reg_res);
    
    liberar_reg(reg1);
    liberar_reg(reg2);
    liberar_reg(reg_res);
}

TipoToken converter_tipos(FILE* s, TipoToken tipo_anterior, TipoToken tipo_atual) {
    const char* reg1 = alocar_reg(s, tipo_anterior);
    const char* reg2 = alocar_reg(s, tipo_atual);
    
    if(tipo_anterior == T_pFLU && tipo_atual == T_pDOBRO) {
        fprintf(s, "  fmov %s, s1\n", reg1);
        fprintf(s, "  fmov %s, s0\n", reg2);
        fprintf(s, "  fcvt d1, s1\n");
        fprintf(s, "  fcvt d0, s0\n");
        fprintf(s, "  fmov d1, %s\n", reg1);
        fprintf(s, "  fmov d0, %s\n", reg2);
        liberar_reg(reg1);
        liberar_reg(reg2);
        return T_pDOBRO;
    } else if(tipo_anterior == T_pDOBRO && tipo_atual == T_pFLU) {
        fprintf(s, "  fmov %s, d1\n", reg1);
        fprintf(s, "  fmov %s, d0\n", reg2);
        fprintf(s, "  fcvt s1, d1\n");
        fprintf(s, "  fcvt s0, d0\n");
        fprintf(s, "  fmov s1, %s\n", reg1);
        fprintf(s, "  fmov s0, %s\n", reg2);
        liberar_reg(reg1);
        liberar_reg(reg2);
        return T_pFLU;
    } else if(tipo_anterior == T_pINT && tipo_atual == T_pFLU) {
        fprintf(s, "  mov %s, w1\n", reg1);
        fprintf(s, "  mov %s, w0\n", reg2);
        fprintf(s, "  scvtf s0, w0\n");
        fprintf(s, "  scvtf s1, w1\n");
        fprintf(s, "  fcvt d1, s1\n");
        fprintf(s, "  fcvt d0, s0\n");
        fprintf(s, "  fmov d1, %s\n", reg1);
        fprintf(s, "  fmov d0, %s\n", reg2);
        liberar_reg(reg1);
        liberar_reg(reg2);
        return T_pDOBRO;
    } else if(tipo_anterior == T_pINT && tipo_atual == T_pDOBRO) {
        fprintf(s, "  mov %s, w1\n", reg1);
        fprintf(s, "  mov %s, w0\n", reg2);
        fprintf(s, "  scvtf d0, w0\n");
        fprintf(s, "  scvtf d1, w1\n");
        fprintf(s, "  fmov d1, %s\n", reg1);
        fprintf(s, "  fmov d0, %s\n", reg2);
        liberar_reg(reg1);
        liberar_reg(reg2);
        return T_pDOBRO;
    }
    
    liberar_reg(reg1);
    liberar_reg(reg2);
    // se não precisa de conversão retorna o tipo dominante
    return (tam_tipo(tipo_atual) > tam_tipo(tipo_anterior)) ? tipo_atual : tipo_anterior;
}

void escrever_valor(FILE* s, TipoToken tipo) {
    if(tipo == T_pFLU) fprintf(s, "  bl _escrever_flu\n");
    else if(tipo == T_pDOBRO) fprintf(s, "  bl _escrever_double\n");
    else if(tipo == T_pCAR) fprintf(s, "  bl _escrever_car\n");
    else if(tipo == T_pBOOL) fprintf(s, "  bl _escrever_bool\n");
    else if(tipo == T_TEX || tipo == T_LONGO) fprintf(s, "  bl _escrever_tex\n");
    else fprintf(s, "  bl _escrever_int\n");
}

void carregar_valor(FILE* s, Variavel* var) {
    if(var->eh_ponteiro) fatal("[carregar_valor] erro interno: carregar_valor chamado para ponteiro");
    else if(var->eh_array) {
        const char* reg = alocar_reg(s, T_pLONGO);
        fprintf(s, "  add %s, x29, %d\n", reg, var->pos);
        fprintf(s, "  mov x0, %s\n", reg);
        liberar_reg(reg);
    }
    else {
        const char* reg = alocar_reg(s, var->tipo_base);
        switch(tam_tipo(var->tipo_base)) {
            case 1: 
                fprintf(s, "  ldrb %s, [x29, %d]\n", reg, var->pos);
                fprintf(s, "  mov w0, %s\n", reg);
                break;
            case 4: 
                if(var->tipo_base == T_pFLU) {
                    fprintf(s, "  ldr %s, [x29, %d]\n", reg, var->pos);
                    fprintf(s, "  fmov s0, %s\n", reg);
                }
                else {
                    fprintf(s, "  ldr %s, [x29, %d]\n", reg, var->pos);
                    fprintf(s, "  mov w0, %s\n", reg);
                }
                break;
            case 8:
                if(var->tipo_base == T_pDOBRO) {
                    fprintf(s, "  ldr %s, [x29, %d]\n", reg, var->pos);
                    fprintf(s, "  fmov d0, %s\n", reg);
                }
                else {
                    fprintf(s, "  ldr %s, [x29, %d]\n", reg, var->pos);
                    fprintf(s, "  mov x0, %s\n", reg);
                }
                break;
        }
        liberar_reg(reg);
    }
}

void armazenar_valor(FILE* s, Variavel* var) {
    if(var->eh_ponteiro) {
        const char* reg = alocar_reg(s, T_pLONGO);
        fprintf(s, "  ldr %s, [x29, %d]\n", reg, var->pos);
        fprintf(s, "  str x0, [%s]\n", reg);
        liberar_reg(reg);
    }
    else if(var->eh_array) fatal("[armazenar_valor] não é possível armazenar valor direto em array");
    else {
        const char* reg = alocar_reg(s, var->tipo_base);
        switch(tam_tipo(var->tipo_base)) {
            case 1: 
                fprintf(s, "  mov %s, w0\n", reg);
                fprintf(s, "  strb %s, [x29, %d]\n", reg, var->pos);
                break;
            case 4:
                if(var->tipo_base == T_pFLU) {
                    fprintf(s, "  fmov %s, s0\n", reg);
                    fprintf(s, "  str %s, [x29, %d]\n", reg, var->pos);
                }
                else {
                    fprintf(s, "  mov %s, w0\n", reg);
                    fprintf(s, "  str %s, [x29, %d]\n", reg, var->pos);
                }
                break;
            case 8:
                if(var->tipo_base == T_pDOBRO) {
                    fprintf(s, "  fmov %s, d0\n", reg);
                    fprintf(s, "  str %s, [x29, %d]\n", reg, var->pos);
                }
                else {
                    fprintf(s, "  mov %s, x0\n", reg);
                    fprintf(s, "  str %s, [x29, %d]\n", reg, var->pos);
                }
                break;
        }
        liberar_reg(reg);
    }
}

void carregar_const(FILE* s, int titulo) {
    Constante* c = &constpos[titulo];
    
    if(c->tipo == T_FLU) {
        const char* reg_addr = alocar_reg(s, T_pLONGO);
        const char* reg_val = alocar_reg(s, T_pFLU);
        fprintf(s, "  ldr %s, =const_%d\n", reg_addr, titulo);
        fprintf(s, "  ldr %s, [%s]\n", reg_val, reg_addr);
        fprintf(s, "  fmov s0, %s\n", reg_val);
        liberar_reg(reg_addr);
        liberar_reg(reg_val);
    } else if(c->tipo == T_DOBRO) {
        const char* reg_addr = alocar_reg(s, T_pLONGO);
        const char* reg_val = alocar_reg(s, T_pDOBRO);
        fprintf(s, "  ldr %s, =const_%d\n", reg_addr, titulo);
        fprintf(s, "  ldr %s, [%s]\n", reg_val, reg_addr);
        fprintf(s, "  fmov d0, %s\n", reg_val);
        liberar_reg(reg_addr);
        liberar_reg(reg_val);
    } else if(c->tipo == T_INT) {
        const char* reg_addr = alocar_reg(s, T_pLONGO);
        const char* reg_val = alocar_reg(s, T_pINT);
        fprintf(s, "  ldr %s, =const_%d\n", reg_addr, titulo);
        fprintf(s, "  ldr %s, [%s]\n", reg_val, reg_addr);
        fprintf(s, "  mov w0, %s\n", reg_val);
        liberar_reg(reg_addr);
        liberar_reg(reg_val);
    }
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
    if(const_cnt >= MAX_CONST) fatal("[add_const] excesso de constpos");
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
    if(tex_cnt >= MAX_TEX) fatal("[add_tex] excesso de textos");
    Tex* tex = &texs[tex_cnt];
    strcpy(tex->valor, valor);
    sprintf(tex->nome, ".tex_%d", tex_cnt);
    tex_cnt++;
    return tex_cnt - 1;
}

void empurrar_arg(FILE* s, TipoToken tipo) {
    const char* reg = alocar_reg(s, tipo);
    
    if(tipo == T_pFLU) {
        fprintf(s, "  fmov %s, s0\n", reg);
        fprintf(s, "  str %s, [sp, -16]!\n", reg);
    } else if(tipo == T_pDOBRO) {
        fprintf(s, "  fmov %s, d0\n", reg);
        fprintf(s, "  str %s, [sp, -16]!\n", reg);
    } else {
        int tam = tam_tipo(tipo);
        if(tam <= 4) {
            fprintf(s, "  mov %s, w0\n", reg);
            fprintf(s, "  str %s, [sp, -16]!\n", reg);
        } else {
            fprintf(s, "  mov %s, x0\n", reg);
            fprintf(s, "  str %s, [sp, -16]!\n", reg);
        }
    }
    liberar_reg(reg);
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
        fatal("[fator] fator inválido");
        return T_pINT;
    }
}

TipoToken termo(FILE* s, int escopo) {
    TipoToken tipo = fator(s, escopo);
    
    while(L.tk.tipo == T_VEZES || L.tk.tipo == T_DIV) {
        TipoToken op = L.tk.tipo;
        proximoToken();
        
        // salva o valor atual em registradores temporários
        const char* reg_temp1 = alocar_reg(s, tipo);
        if(tipo == T_pFLU) {
            fprintf(s, "  fmov %s, s0\n", reg_temp1);
        } else if (tipo == T_pDOBRO) {
            fprintf(s, "  fmov %s, d0\n", reg_temp1);
        } else {
            fprintf(s, "  mov %s, w0\n", reg_temp1);
        }
        
        TipoToken tipo_dir = fator(s, escopo);
        
        // salva o segundo valor em outro registrador temporário
        const char* reg_temp2 = alocar_reg(s, tipo_dir);
        if(tipo_dir == T_pFLU) {
            fprintf(s, "  fmov %s, s0\n", reg_temp2);
        } else if (tipo_dir == T_pDOBRO) {
            fprintf(s, "  fmov %s, d0\n", reg_temp2);
        } else {
            fprintf(s, "  mov %s, w0\n", reg_temp2);
        }
        
        // move os valores de volta para os registradores de trabalho
        if(tipo == T_pFLU) {
            fprintf(s, "  fmov s1, %s\n", reg_temp1);
            fprintf(s, "  fmov s0, %s\n", reg_temp2);
        } else if (tipo == T_pDOBRO) {
            fprintf(s, "  fmov d1, %s\n", reg_temp1);
            fprintf(s, "  fmov d0, %s\n", reg_temp2);
        } else {
            fprintf(s, "  mov w1, %s\n", reg_temp1);
            fprintf(s, "  mov w0, %s\n", reg_temp2);
        }
        liberar_reg(reg_temp1);
        liberar_reg(reg_temp2);
        
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
        
        // salva o valor atual em registradores temporários
        const char* reg_temp1 = alocar_reg(s, tipo);
        if(tipo == T_pFLU) {
            fprintf(s, "  fmov %s, s0\n", reg_temp1);
        } else if (tipo == T_pDOBRO) {
            fprintf(s, "  fmov %s, d0\n", reg_temp1);
        } else {
            fprintf(s, "  mov %s, w0\n", reg_temp1);
        }
        
        TipoToken tipo_dir = termo(s, escopo);
        
        // salva o segundo valor em outro registrador temporário
        const char* reg_temp2 = alocar_reg(s, tipo_dir);
        if(tipo_dir == T_pFLU) {
            fprintf(s, "  fmov %s, s0\n", reg_temp2);
        } else if (tipo_dir == T_pDOBRO) {
            fprintf(s, "  fmov %s, d0\n", reg_temp2);
        } else {
            fprintf(s, "  mov %s, w0\n", reg_temp2);
        }
        
        // move os valores de volta para os registradores de trabalho
        if(tipo == T_pFLU) {
            fprintf(s, "  fmov s1, %s\n", reg_temp1);
            fprintf(s, "  fmov s0, %s\n", reg_temp2);
        } else if (tipo == T_pDOBRO) {
            fprintf(s, "  fmov d1, %s\n", reg_temp1);
            fprintf(s, "  fmov d0, %s\n", reg_temp2);
        } else {
            fprintf(s, "  mov w1, %s\n", reg_temp1);
            fprintf(s, "  mov w0, %s\n", reg_temp2);
        }
        liberar_reg(reg_temp1);
        liberar_reg(reg_temp2);
        
        tipo = converter_tipos(s, tipo, tipo_dir);
        gerar_operacao(s, op, tipo);
    }
    
    if(L.tk.tipo == T_IGUAL_IGUAL || L.tk.tipo == T_DIFERENTE || 
        L.tk.tipo == T_MAIOR || L.tk.tipo == T_MENOR ||
        L.tk.tipo == T_MAIOR_IGUAL || L.tk.tipo == T_MENOR_IGUAL) {
        
        TipoToken op = L.tk.tipo;
        proximoToken();
        
        // salva primeiro operando em registrador temporário
        const char* reg_temp1 = alocar_reg(s, tipo);
        if(tipo == T_pFLU) {
            fprintf(s, "  fmov %s, s0\n", reg_temp1);
        } else if (tipo == T_pDOBRO) {
            fprintf(s, "  fmov %s, d0\n", reg_temp1);
        } else {
            fprintf(s, "  mov %s, w0\n", reg_temp1);
        }
        
        // segundo operando
        TipoToken tipo_dir = termo(s, escopo);
        
        // salva segundo operando em registrador temporário
        const char* reg_temp2 = alocar_reg(s, tipo_dir);
        if(tipo_dir == T_pFLU) {
            fprintf(s, "  fmov %s, s0\n", reg_temp2);
        } else if (tipo_dir == T_pDOBRO) {
            fprintf(s, "  fmov %s, d0\n", reg_temp2);
        } else {
            fprintf(s, "  mov %s, w0\n", reg_temp2);
        }
        
        // move os valores de volta para os registradores de trabalho
        if(tipo == T_pFLU) {
            fprintf(s, "  fmov s1, %s\n", reg_temp1);
            fprintf(s, "  fmov s0, %s\n", reg_temp2);
        } else if (tipo == T_pDOBRO) {
            fprintf(s, "  fmov d1, %s\n", reg_temp1);
            fprintf(s, "  fmov d0, %s\n", reg_temp2);
        } else {
            fprintf(s, "  mov w1, %s\n", reg_temp1);
            fprintf(s, "  mov w0, %s\n", reg_temp2);
        }
        liberar_reg(reg_temp1);
        liberar_reg(reg_temp2);
        
        tipo = converter_tipos(s, tipo, tipo_dir);
        gerar_comparacao(s, op, tipo);
        tipo = T_pBOOL;
    }
    return tipo;
}

void declaracao_var(FILE* s, int* pos, int escopo, int eh_parametro) {
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

    if(L.tk.tipo != T_ID) fatal("[declaracao_var] nome de variável esperado");

    Funcao* f = &funcs[fn_cnt - 1];
    if(f->var_conta >= MAX_VAR) fatal("[declaracao_var] excesso de variáveis");

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
            const char* reg_addr = alocar_reg(s, T_pLONGO);
            const char* reg_val = alocar_reg(s, T_pINT);
            
            fprintf(s, "  add %s, x29, %d\n", reg_addr, var->pos);
            for(int i = 0; i <= strlen(texto_valor); i++) {
                fprintf(s, "  mov %s, %d\n", reg_val, texto_valor[i]);
                fprintf(s, "  strb %s, [%s, %d]\n", reg_val, reg_addr, i);
            }
            
            liberar_reg(reg_addr);
            liberar_reg(reg_val);
            proximoToken();
        } else if(eh_array) {
            excessao(T_CHAVE_ESQ);
            int i = 0;
            while(L.tk.tipo != T_CHAVE_DIR) {
                if(tam_array > 0 && i >= tam_array) fatal("[declaracao_var] excesso de elementos");
                TipoToken tipo_valor = expressao(s, escopo);
                if(tipo_valor != tipo_base) fatal("[declaracao_var] tipo incompatível");
                
                const char* reg_addr = alocar_reg(s, T_pLONGO);
                const char* reg_val = alocar_reg(s, tipo_base);
                
                int pos = var->pos + i * tam_tipo(tipo_base);
                if(pos >= 0) {
                    fprintf(s, "  add %s, x29, %d\n", reg_addr, pos);
                    if(tipo_base == T_pCAR || tipo_base == T_pINT || tipo_base == T_pBOOL) {
                        fprintf(s, "  mov %s, w0\n", reg_val);
                        fprintf(s, "  strb %s, [%s]\n", reg_val, reg_addr);
                    } else if(tipo_base == T_pFLU) {
                        fprintf(s, "  fmov %s, s0\n", reg_val);
                        fprintf(s, "  str %s, [%s]\n", reg_val, reg_addr);
                    } else if(tipo_base == T_pDOBRO) {
                        fprintf(s, "  fmov %s, d0\n", reg_val);
                        fprintf(s, "  str %s, [%s]\n", reg_val, reg_addr);
                    }
                } else {
                    const char* reg_pos = alocar_reg(s, T_pINT);
                    fprintf(s, "  mov %s, %d\n", reg_pos, pos);
                    if(tipo_base == T_pCAR || tipo_base == T_pINT || tipo_base == T_pBOOL) {
                        fprintf(s, "  mov %s, w0\n", reg_val);
                        fprintf(s, "  strb %s, [x29, %s]\n", reg_val, reg_pos);
                    } else if(tipo_base == T_pFLU) {
                        fprintf(s, "  fmov %s, s0\n", reg_val);
                        fprintf(s, "  str %s, [x29, %s]\n", reg_val, reg_pos);
                    } else if(tipo_base == T_pDOBRO) {
                        fprintf(s, "  fmov %s, d0\n", reg_val);
                        fprintf(s, "  str %s, [x29, %s]\n", reg_val, reg_pos);
                    }
                    liberar_reg(reg_pos);
                }
                
                liberar_reg(reg_addr);
                liberar_reg(reg_val);
                i++;
                if(L.tk.tipo == T_VIRGULA) proximoToken();
            }
            excessao(T_CHAVE_DIR);
            if(var->tam_array == 0) var->tam_array = i;
        } else if(eh_ponteiro && L.tk.tipo == T_TEX) {
            int id_tex = add_tex(L.tk.lex);
            const char* reg_addr = alocar_reg(s, T_pLONGO);
            const char* reg_val = alocar_reg(s, T_pLONGO);
            
            fprintf(s, "  ldr %s, =%s\n", reg_val, texs[id_tex].nome);
            if(var->pos >= 0) {
                fprintf(s, "  str %s, [x29, %d]\n", reg_val, var->pos);
            } else {
                const char* reg_pos = alocar_reg(s, T_pINT);
                fprintf(s, "  mov %s, %d\n", reg_pos, var->pos);
                fprintf(s, "  str %s, [x29, %s]\n", reg_val, reg_pos);
                liberar_reg(reg_pos);
            }
            liberar_reg(reg_addr);
            liberar_reg(reg_val);
            proximoToken();
        } else if(eh_ponteiro) fatal("[declaracao_var] esperado texto para ponteiro");
        else {
            TipoToken tipo_exp = expressao(s, escopo);
            armazenar_valor(s, var);
        }
    }
    if(eh_parametro) *pos += 16;
}

int main(int argc, char** argv) {
    if(argc < 2) {
        printf("FPB: sem arquivos de entrada\n");
        return 1;
    }
    if(strcmp(argv[1], "-ajuda") == 0) {
        printf("[informação]:\n");
        printf("fpb -v : versão e o distribuidor\n");
        printf("fpb -c : configurações do compilador\n");
        printf("[compilação]:\n");
        printf("fpb exemplo : compila um arquivo.fpb e gera o binário na pasta atual\n");
        printf("fpb exemplo -s pasta/exemplo : compila um arquivo.fpb e cria um arquivo em um caminho personalizavel\n");
        printf("fpb exemplo -asm : compila mantendo o ASM intermediario na pasta atual\n");
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
        return 0;
    }
    iniciarPerceptron();
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
    
    if(!s) {
        printf("FPB: [ERRO] não foi possível criar o ASM intermediario\n");
        return 1;
    }

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
