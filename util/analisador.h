#pragma once
/*
* [FUNÇÃO]: Analisador de semantica e sinetica.
* [IMPLEMENTAÇÃO]: @Shiniga-OP.
* [BASE]: Assembly.
* [SISTEMA OPERACIONAL]: ANDROID.
* [ARQUITETURA]: ARM64-LINUX-ANDROID(ARM64).
* [LINGUAGEM]: Português Brasil(PT-BR).
* [DATA]: 07/02/2026.
* [ATUAL]: 19/02/2026.
* [PONTEIRO]: dereferencia automatica, acesso a endereços apenas com "@ponteiro".
*/
// buscar
Variavel* buscar_var(const char* nome, int escopo);
Funcao* buscar_fn(const char* nome);
Macro* buscar_macro(const char* nome);
// util
TipoToken expressao(FILE* s, int escopo);
TipoToken termo(FILE* s, int escopo);
TipoToken fator(FILE* s, int escopo);
TipoToken converter_tipos(FILE* s, TipoToken tipo_anterior, TipoToken tipo_atual);
const char* token_str(TipoToken t);
void excessao(TipoToken t);
void fatal(const char* m);
void proximoToken();
// processar:
void processar_args(FILE* s, Funcao* f);
char* processar_caminho();
int processar_var_tam(int escopo);
// verificar
void verificar_fn(FILE* s);
void verificar_stmt(FILE* s, int* pos, int escopo);
void verificar_retorno(FILE* s, int escopo);
void verificar_atribuicao(FILE* s, const char* id, int escopo);
void verificar_por(FILE* s, int escopo);
void verificar_enq(FILE* s, int escopo);
void verificar_espaco(FILE* s);
void verificar_global(FILE* s);
void verificar_def();
void verificar_matriz(FILE* s, Variavel* var, int escopo, int indices[], int nivel);
// tratar
TipoToken tratar_id(FILE* s, int escopo);
TipoToken tratar_chamada_funcao(FILE* s, int escopo, const char* nome, Funcao* fn);
TipoToken tratar_inteiro(FILE* s);
TipoToken tratar_flutuante(FILE* s);
TipoToken tratar_caractere(FILE* s);
TipoToken tratar_texto(FILE* s);

#include "gerador.h"

// [DEBUG]:
const char* token_str(TipoToken t) {
    switch(t) {
        case T_ID: return "identificador";
        case T_ESPACO_ID: return "tipo de espaço";
        case T_GLOBAL: return "#global";
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
        case T_MAIS_IGUAL: return "+=";
        case T_MENOS_IGUAL: return "-=";
        case T_VEZES_IGUAL: return "*=";
        case T_DIV_IGUAL: return "/=";
        case T_PORCEN_IGUAL: return "%=";
        case T_CONVERTA: return "(converter)";
        case T_IGUAL_IGUAL: return "==";
        case T_DIFERENTE: return "!=";
        case T_DIFERENTE_ABS: return "!";
        case T_NAO: return "!";
        case T_MAIOR: return ">";
        case T_MENOR: return "<";
        case T_MAIOR_IGUAL: return ">=";
        case T_MENOR_IGUAL: return "<=";
        case T_TAMBEM_TAMBEM: return "&&";
        case T_OU_OU: return "||";
        case T_INTERROGACAO: return "?";
        case T_DOIS_PONTOS: return ":";
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
        case T_ESPACO: return "#espaço";
        case T_DEF: return "#def";
        case T_ALINHAR: return "#alinhar";
        case T_FIM: return "fim";
        case T_MAIOR_MAIOR: return ">>";
        case T_MENOR_MENOR: return "<<";
        case T_OU: return "|";
        case T_TAMBEM: return "&";
        case T_PARE: return "pare";
        default: return "desconhecido";
    }
}

void fatal(const char* m) {
    printf("%s [ERRO] linha: %d coluna: %d, %s próximo de \"%s\"\n", arquivoAtual, L.tk.pos.linha + 1, L.tk.pos.coluna + 1, m, L.tk.lex);
    liberar_buf();
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

void proximoToken() {
    char c;
    int i = 0;

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
        if(strcmp(L.tk.lex, "espaco") == 0 || strcmp(L.tk.lex, "espaço") == 0) {
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
        if(strcmp(L.tk.lex, "alinhar") == 0) {
            L.tk.tipo = T_ALINHAR;
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
        else if(strcmp(L.tk.lex, "verdade") == 0 || strcmp(L.tk.lex, "falso") == 0) L.tk.tipo = T_BOOL;
        else if(strcmp(L.tk.lex, "dobro") == 0) L.tk.tipo = T_pDOBRO;
        else if(strcmp(L.tk.lex, "longo") == 0) L.tk.tipo = T_pLONGO;
        else if(strcmp(L.tk.lex, "vazio") == 0) L.tk.tipo = T_pVAZIO;
        else if(strcmp(L.tk.lex, "se") == 0) L.tk.tipo = T_SE;
        else if(strcmp(L.tk.lex, "senao") == 0 || strcmp(L.tk.lex, "senão") == 0) L.tk.tipo = T_SENAO;
        else if(strcmp(L.tk.lex, "por") == 0) L.tk.tipo = T_POR;
        else if(strcmp(L.tk.lex, "enq") == 0 || strcmp(L.tk.lex, "enquanto") == 0) L.tk.tipo = T_ENQ;
        else if(strcmp(L.tk.lex, "retorne") == 0 || strcmp(L.tk.lex, "retornar") == 0) L.tk.tipo = T_RETORNAR;
        else if(strcmp(L.tk.lex, "pare") == 0) L.tk.tipo = T_PARE;
        else if(strcmp(L.tk.lex, "final") == 0) L.tk.tipo = T_FINAL;
        else if(strcmp(L.tk.lex, "->") == 0) L.tk.tipo = T_LAMBDA;
        else {
            L.tk.tipo = T_ID;
            // verifica se ha espaços definidos e se esse nome corresponde
            for(int j = 0; j < espaco_cnt; j++) {
                if(strcmp(espacos[j].nome, L.tk.lex) == 0) {
                    L.tk.tipo = T_ESPACO_ID;
                    break;
                }
            }
        }
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
        // converte pra valor sem mascarar
        if(eh_hex) {
            L.tk.valor_l = strtoul(L.tk.lex + 2, NULL, 16);
        } else {
            L.tk.valor_l = strtoul(L.tk.lex + 2, NULL, 2);
        }
        L.tk.tipo = T_BYTE;
        return;
    }
    if((c == '-' && isdigit((unsigned char)L.fonte[L.pos + 1])) || isdigit((unsigned char)c) || (c == '.' && isdigit((unsigned char)L.fonte[L.pos + 1]))) {
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
                if(ponto) fatal("numero inválido");
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
                
                char escape_car;
                switch(n) {
                    case 'n': escape_car = '\n'; break;
                    case 't': escape_car = '\t'; break;
                    case 'r': escape_car = '\r'; break;
                    case '0': escape_car = '\0'; break;
                    case '\\': escape_car = '\\'; break;
                    case '\'': escape_car = '\''; break;
                    case '\"': escape_car = '\"'; break;
                    case 'a': escape_car = '\a'; break;
                    case 'b': escape_car = '\b'; break;
                    case 'v': escape_car = '\v'; break;
                    case 'f': escape_car = '\f'; break;
                    default: escape_car = n;break;
                }
                if(i < MAX_TOK - 1) L.tk.lex[i++] = escape_car;
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
        
        char valor_car;
        
        // verifica se é uma sequencia de escape
        if(L.fonte[L.pos] == '\\') {
            L.pos++; L.coluna_atual++;
            
            if(!L.fonte[L.pos]) fatal("caractere mal formado");
            
            // lrocessa sequências de escape comuns
            switch(L.fonte[L.pos]) {
                case 'n': valor_car = '\n'; break; // quebra de linha
                case 't': valor_car = '\t'; break; // tab horizontal
                case 'r': valor_car = '\r'; break;
                case '0': valor_car = '\0'; break; // fim de texto
                case '\\': valor_car = '\\'; break;
                case '\'': valor_car = '\''; break; // aspas simples escapada
                case '\"': valor_car = '\"'; break; // aspas normal escapada
                case 'a': valor_car = '\a'; break;  // alerta
                case 'b': valor_car = '\b'; break;  // apaga
                case 'v': valor_car = '\v'; break;  // tab vertical
                case 'f': valor_car = '\f'; break;
                default:
                // se não for uma sequencia de escape conhecida, mantem o caractere literal
                valor_car = L.fonte[L.pos];
                break;
            }
            L.pos++; L.coluna_atual++;
        } else {
            // caractere normal
            valor_car = L.fonte[L.pos];
            L.pos++; L.coluna_atual++;
        }
        // verifica se fecha com aspas simples
        if(L.fonte[L.pos] != '\'') fatal("caractere mal formado");
        L.pos++; L.coluna_atual++;
        // armazena o valor
        L.tk.tipo = T_CAR;
        L.tk.lex[0] = valor_car;
        L.tk.lex[1] = '\0';
        L.tk.valor_l = (long)valor_car;
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
                    L.tk.tipo = T_CONVERTA;
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
            } else if(L.fonte[L.pos + 1] == '=') {
                L.pos++;
                L.coluna_atual++;
                L.tk.tipo = T_MAIS_IGUAL;
            } else L.tk.tipo = T_MAIS;
        break;
        case '-': 
            if(L.fonte[L.pos + 1] == '-') {
                L.tk.tipo = T_MENOS_MENOS;
                L.pos++;
                L.coluna_atual++;
            } else if(L.fonte[L.pos + 1] == '=') {
                L.pos++;
                L.coluna_atual++;
                L.tk.tipo = T_MENOS_IGUAL;
            } else L.tk.tipo = T_MENOS;
        break;
        case '*':
            if(L.fonte[L.pos + 1] == '=') {
                L.pos++;
                L.coluna_atual++;
                L.tk.tipo = T_VEZES_IGUAL;
            } else L.tk.tipo = T_VEZES;
        break;
        case '/':
            if(L.fonte[L.pos + 1] == '=') {
                L.pos++;
                L.coluna_atual++;
                L.tk.tipo = T_DIV_IGUAL;
            } else L.tk.tipo = T_DIV;
        break;
        case '%':
            if(L.fonte[L.pos + 1] == '=') {
                L.pos++;
                L.coluna_atual++;
                L.tk.tipo = T_PORCEN_IGUAL;
            } else L.tk.tipo = T_PORCEN;
        break;
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
                if(L.fonte[L.pos + 2] == '=') {
                    L.tk.tipo = T_DIFERENTE_ABS;
                }
                L.pos++;
                L.coluna_atual++;
            } else L.tk.tipo = T_NAO;
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
        case '?':
            L.tk.tipo = T_INTERROGACAO;
        break;
        case ':':
            L.tk.tipo = T_DOIS_PONTOS;
        break;
        default: fatal("Símbolo inválido"); break;
    }
    L.tk.lex[0] = c;
    L.tk.lex[1] = 0;
    L.pos++;
    L.coluna_atual++;
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

Espaco* buscar_espaco(const char* nome) {
    for(int i = 0; i < espaco_cnt; i++) {
        if(strcmp(espacos[i].nome, nome) == 0) return &espacos[i];
    }
    return NULL;
}

// [VERIFICAÇÃO]:
void verificar_alinhar() {
    excessao(T_ALINHAR);
    if(L.tk.tipo != T_INT) fatal("[verificar_alinhar] valor inteiro esperado");
    proximo_alinhamento = L.tk.valor_l;
    proximoToken();
    if(L.tk.tipo == T_PONTO_VIRGULA) excessao(T_PONTO_VIRGULA);
}

void verificar_global(FILE* s) {
    excessao(T_GLOBAL);
    // verifica se é um tipo(variavel global) ou identificador(função global)
    if(eh_tipo(L.tk.tipo) || L.tk.tipo == T_FINAL || L.tk.tipo == T_pBYTE) {
        // se é uma declaração de variavel global
        // escopo especial(-1) pra variaveis globais
        int pos = 0;
        // Salva o escopo atual
        int escopo_atual = escopo_global;
        escopo_global = -1; // marca como processamento global
        
        verificar_stmt(s, &pos, -1);
        // restaura escopo
        escopo_global = escopo_atual;
        
        globais[global_cnt - 1].alinhamento = proximo_alinhamento;
        proximo_alinhamento = 0;
    } else {
        // se é uma função global
        if(L.tk.tipo != T_ID) fatal("[verificar_global] nome de função esperado");
        
        char fnome[32];
        strcpy(fnome, L.tk.lex);
        proximoToken();
        
        excessao(T_PAREN_ESQ);
        excessao(T_PAREN_DIR);
        
        if(L.tk.tipo == T_PONTO_VIRGULA) excessao(T_PONTO_VIRGULA);
        
        // marca a função como global
        fprintf(s, ".global %s\n", fnome);
        for(int i = 0; i < fn_cnt; i++) {
            if(strcmp(funcs[i].nome, fnome) == 0) {
                funcs[i].eh_global = 1;
                break;
            }
        }
        proximo_alinhamento = 0;
    }
}

void verificar_def() {
    excessao(T_DEF); // consome #def
    
    if(L.tk.tipo != T_ID) fatal("nome do macro esperado");
    
    char nome_macro[32];
    strcpy(nome_macro, L.tk.lex);
    proximoToken(); // consome o nome do macro
    
    // suporta inteiros ou longos
    if(L.tk.tipo != T_INT && L.tk.tipo != T_LONGO) fatal("valor inteiro ou longo esperado para o macro");
    
    long valor = L.tk.valor_l;
    proximoToken(); // consome o valor
    
    if(L.tk.tipo == T_PONTO_VIRGULA) excessao(T_PONTO_VIRGULA); // consome o ;
    
    if(macro_cnt >= MAX_MACROS) {
        MAX_MACROS += 2;
        Macro *temp = realloc(macros, MAX_MACROS * sizeof(Macro));
        if(temp == NULL) {
            printf("[verificar_def]: Erro ao realocar memória, macros: %d\n", macro_cnt);
            free(macros);
            exit(EXIT_FAILURE);
        }
        macros = temp;
    }
    Macro* m = &macros[macro_cnt++];
    strcpy(m->nome, nome_macro);
    m->valor = valor;
}

void verificar_espaco(FILE* s) {
    excessao(T_ESPACO);
    
    if(L.tk.tipo != T_ID) fatal("nome do espaço esperado");
    
    char nome_espaco[64];
    strcpy(nome_espaco, L.tk.lex);
    proximoToken();
    
    // verifica se ja existe um espaço com esse nome
    for(int i = 0; i < espaco_cnt; i++) {
        if(strcmp(espacos[i].nome, nome_espaco) == 0) {
            fatal("[verificar_espaco] espaço já definido");
        }
    }
    if(espaco_cnt >= MAX_ESPACO) {
        MAX_ESPACO += 2;
        Espaco *temp = realloc(espacos, MAX_ESPACO * sizeof(Espaco));
        if(!temp) {
            printf("[verificar_espaco]: Erro ao alocar memória, espacos %d\n", espaco_cnt);
            exit(1);
        }
        espacos = temp;
    }
    Espaco* esp = &espacos[espaco_cnt++];
    strcpy(esp->nome, nome_espaco);
    esp->campos = malloc(MAX_VAR * sizeof(Variavel));
    esp->campo_cnt = 0;
    esp->tam_total = 0;
    
    excessao(T_CHAVE_ESQ);
    // processa os campos da estrutura
    while(L.tk.tipo != T_CHAVE_DIR && L.tk.tipo != T_FIM) {
        if(esp->campo_cnt >= MAX_VAR) fatal("[verificar_espaco] excesso de campos no espaço");
        
        // verifica se é um tipo valido ou espaço
        int tipo_valido = eh_tipo(L.tk.tipo);
        if(!tipo_valido && L.tk.tipo == T_ID) {
            // pode ser um espaço definido anteriormente
            for(int i = 0; i < espaco_cnt; i++) {
                if(strcmp(espacos[i].nome, L.tk.lex) == 0) {
                    L.tk.tipo = T_ESPACO_ID;
                    tipo_valido = 1;
                    break;
                }
            }
        }
        if(!tipo_valido) fatal("[verificar_espaco] tipo inválido para campo");
        
        TipoToken tipo_campo = L.tk.tipo;
        proximoToken();
        
        int eh_ponteiro = 0;
        int num_dims = 0;
        int* dims = malloc(MAX_DIMS * sizeof(int));
        memset(dims, 0, MAX_DIMS * sizeof(int));
        
        // verifica se é ponteiro
        if(L.tk.tipo == T_VEZES) {
            eh_ponteiro = 1;
            proximoToken();
        } 
        // verifica se é array
        while(L.tk.tipo == T_COL_ESQ) {
            if(num_dims >= MAX_DIMS) {
                MAX_DIMS += 2;
                int *temp = realloc(dims, MAX_DIMS * sizeof(int));
                if(!temp) {
                    printf("[verificar_espaco]: Erro ao alocar memória, dimensões %d\n", num_dims);
                    free(dims);
                    exit(1);
                }
                dims = temp;
            }
            proximoToken();
            
            if(L.tk.tipo == T_INT) {
                dims[num_dims] = L.tk.valor_l;
                proximoToken();
            } else if(L.tk.tipo == T_ID) {
                // pode ser macro ou variavel final
                Variavel* var = buscar_var(L.tk.lex, 0);
                Macro* macro = buscar_macro(L.tk.lex);
                
                if(var && var->eh_final) {
                    dims[num_dims] = var->valor;
                } else if(macro) {
                    dims[num_dims] = macro->valor;
                } else {
                    fatal("[verificar_espaco] tamanho de array inválido");
                }
                proximoToken();
            } else {
                // array sem tamanho especificado(tamanho 0)
                dims[num_dims] = 0;
            }
            excessao(T_COL_DIR);
            num_dims++;
        }
        if(L.tk.tipo != T_ID) fatal("[verificar_espaco] nome do campo esperado");
        
        Variavel* campo = &esp->campos[esp->campo_cnt];
        strcpy(campo->nome, L.tk.lex);
        campo->tipo_base = tipo_campo;
        campo->eh_ponteiro = eh_ponteiro;
        campo->eh_array = (num_dims > 0);
        campo->num_dims = num_dims;
        campo->dims = dims;
        campo->eh_final = 0;
        campo->escopo = 0;
        campo->eh_parametro = 0;
        
        // calcula tamanho do campo
        int tam_campo = 0;
        if(tipo_campo == T_ESPACO_ID) {
            // encontra o espaço referenciado
            for(int i = 0; i < espaco_cnt; i++) {
                if(strcmp(espacos[i].nome, L.tk.lex) == 0) {
                    tam_campo = espacos[i].tam_total;
                    break;
                }
            }
        } else {
            tam_campo = tam_tipo(tipo_campo);
        }
        
        if(eh_ponteiro) {
            tam_campo = 8;
        } else if(num_dims > 0) {
            for(int i = 0; i < num_dims; i++) {
                if(dims[i] > 0) {
                    tam_campo *= dims[i];
                } else {
                    // array sem tamanho especificado, tamanho minimo 1
                    tam_campo *= 1;
                }
            }
        }
        // alinha para 8 bytes
        esp->tam_total = (esp->tam_total + 7) & ~7;
        campo->pos = esp->tam_total;
        esp->tam_total += tam_campo;
        
        esp->campo_cnt++;
        proximoToken();
        
        excessao(T_PONTO_VIRGULA);
    }
    excessao(T_CHAVE_DIR);
    
    // finaliza calculando tamanho total alinhado
    esp->tam_total = (esp->tam_total + 15) & ~15;
    fprintf(s, "// Espaço \"%s\" definido: %d bytes\n", nome_espaco, esp->tam_total);
}

void verificar_retorno(FILE* s, int escopo) {
    excessao(T_RETORNAR);
    if(L.tk.tipo == T_PONTO_VIRGULA) {
        fprintf(s, "  b 1f\n");
        excessao(T_PONTO_VIRGULA);
        return;
    }
    TipoToken tipo_exp = expressao(s, escopo);
    // em funções que retornam ponteiro/array espera T_PONTEIRO ou T_TEX
    if(funcs[fn_cnt - 1].retorno == T_PONTEIRO || funcs[fn_cnt - 1].retorno == T_TEX) {
        if(tipo_exp != T_PONTEIRO && tipo_exp != T_TEX) fatal("[verificar_retorno] retorno deve ser ponteiro ou endereço");
    } else if(!tipos_compativeis(funcs[fn_cnt - 1].retorno, tipo_exp)) {
        fatal("[verificar_retorno] tipo de retorno incompatível");
    }
    fprintf(s, "  b 1f\n");
    excessao(T_PONTO_VIRGULA);
}

void verificar_atribuicao(FILE* s, const char* id, int escopo) {
    if(debug_o) printf("[verificar_atribuicao]: atribuição em: %s\n", id);
    if(id[0] == '@') {
        char var_nome[32];
        strcpy(var_nome, id + 1); // remove o @
        
        Variavel* var = buscar_var(var_nome, escopo);
        if(!var || !var->eh_ponteiro) {
            fatal("[verificar_atribuicao] @ só pode ser usado com ponteiros");
        }
        excessao(T_IGUAL);
        TipoToken tipo_exp = expressao(s, escopo);
        
        if(tipo_exp != T_pINT && tipo_exp != T_pLONGO && tipo_exp != T_PONTEIRO) {
            fatal("[verificar_atribuicao] endereço deve ser inteiro ou longo ou ponteiro");
        }
        // atribui o endereço diretamente no ponteiro
        fprintf(s, "  str x0, [x29, %d]\n", var->pos);
        return;
    }
    if(strchr(id, '.')) {
        char var_nome[32];
        char campo_nome[32];
        // separa variavel e campo
        char* ponto = strchr(id, '.');
        strncpy(var_nome, id, ponto - id);
        var_nome[ponto - id] = '\0';
        strcpy(campo_nome, ponto + 1);
        
        if(debug_o) printf("[verificar_atribuicao]: acesso a campo: %s\n", campo_nome);
        
        Variavel* var = buscar_var(var_nome, escopo);
        if(!var || var->tipo_base != T_ESPACO_ID) {
            fatal("[verificar_atribuicao] variável não é um espaço");
        }
        int via_ponteiro = var->eh_ponteiro;
        // busca espaço e campo
        Espaco* esp = buscar_espaco(var->espaco);
        if(debug_o) printf("[verificar_atribuicao]: acesso a campo, espaço: %s\ncampo_cnt: %d\n", esp->nome, esp->campo_cnt);
        
        if(!esp) {
            fatal("[verificar_atribuicao] espaço não encontrado");
        }
        Variavel* campo = NULL;
        for(int i = 0; i < esp->campo_cnt; i++) {
            if(strcmp(esp->campos[i].nome, campo_nome) == 0) {
                campo = &esp->campos[i];
                break;
            }
        }
        if(!campo) fatal("[verificar_atribuicao] campo não encontrado");
        
        // acesso a array(campo[indice])
        if(L.tk.tipo == T_COL_ESQ && campo->eh_array) {
            // processa indice
            excessao(T_COL_ESQ);
            expressao(s, escopo); // indice em w0
            fprintf(s, "  str w0, [sp, -16]!\n"); // salva indice
            excessao(T_COL_DIR);
            
            excessao(T_IGUAL);
            
            // processa valor
            TipoToken tipo_valor = expressao(s, escopo);
            
            if(!tipos_compativeis(campo->tipo_base, tipo_valor)) {
                char msg[100];
                sprintf(msg, "[verificar_atribuicao] tipo incompatível para elemento do array %s", campo_nome);
                fatal(msg);
            }
            // recupera indice
            fprintf(s, "  ldr w1, [sp], 16\n"); // w1 = indice
            
            // calcula endereço base do campo
            if(via_ponteiro) {
                if(var->escopo == -1) { fprintf(s, "  ldr x2, = global_%s\n", var->nome); fprintf(s, "  ldr x2, [x2]\n"); }
                else fprintf(s, "  ldr x2, [x29, %d]\n", var->pos);
                if(campo->pos != 0) fprintf(s, "  add x2, x2, %d\n", campo->pos);
            } else if(var->escopo == -1) {
                fprintf(s, "  ldr x2, = global_%s\n", var->nome);
                fprintf(s, "  add x2, x2, %d\n", campo->pos);
            } else {
                fprintf(s, "  add x2, x29, %d\n", var->pos + campo->pos);
            }
            // calcula pos: indice * tamanho_do_elemento
            int tam_elem = tam_tipo(campo->tipo_base);
            if(tam_elem == 1) {
                fprintf(s, "  add x2, x2, x1\n");
            } else if(tam_elem == 4) {
                fprintf(s, "  add x2, x2, x1, lsl 2\n");
            } else if(tam_elem == 8) {
                fprintf(s, "  add x2, x2, x1, lsl 3\n");
            }
            // armazena o valor
            if(campo->tipo_base == T_pCAR || campo->tipo_base == T_pBOOL || campo->tipo_base == T_pBYTE)
            fprintf(s, "  strb w0, [x2]\n");
            else if(campo->tipo_base == T_pINT)
            fprintf(s, "  str w0, [x2]\n");
            else if(campo->tipo_base == T_pFLU)
            fprintf(s, "  str s0, [x2]\n");
            else if(campo->tipo_base == T_pDOBRO)
            fprintf(s, "  str d0, [x2]\n");
            else if(campo->tipo_base == T_pLONGO)
            fprintf(s, "  str x0, [x2]\n");
            return;
        }
        // se não for array(ou se for array sem indice, tratado como erro ou ponteiro depois),
        // ai sim espera o igual pra atribuição normal
        excessao(T_IGUAL); 
        
        // atribuição direta ao campo
        TipoToken tipo_exp = expressao(s, escopo);
        
        // pra ponteiros pra caractere, aceita texto literal
        if(campo->eh_ponteiro && campo->tipo_base == T_pCAR && tipo_exp == T_TEX) {
            fprintf(s, "  ldr x0, = %s\n", texs[tex_cnt - 1].nome);
        }
        // verifica compatibilidade de tipos
        if(!tipos_compativeis(campo->tipo_base, tipo_exp) && 
           !(campo->eh_ponteiro && campo->tipo_base == T_pCAR && tipo_exp == T_TEX)) {
            char msg[100];
            sprintf(msg, "[verificar_atribuicao] tipo incompatível para campo %s: esperado %s, encontrado %s",
                    campo_nome, token_str(campo->tipo_base), token_str(tipo_exp));
            fatal(msg);
        }
        // calcula endereço do campo
        if(via_ponteiro) {
            if(var->escopo == -1) { fprintf(s, "  ldr x1, = global_%s\n", var->nome); fprintf(s, "  ldr x1, [x1]\n"); }
            else fprintf(s, "  ldr x1, [x29, %d]\n", var->pos);
            if(campo->pos != 0) fprintf(s, "  add x1, x1, %d\n", campo->pos);
        } else if(var->escopo == -1) {
            fprintf(s, "  ldr x1, = global_%s\n", var->nome);
            fprintf(s, "  add x1, x1, %d\n", campo->pos);
        } else {
            fprintf(s, "  add x1, x29, %d\n", var->pos + campo->pos);
        }
        // armazena o valor
        if(campo->eh_ponteiro) {
            fprintf(s, "  ldr x1, [x29, %d]\n", var->pos);
            if(campo->tipo_base == T_pLONGO || campo->tipo_base == T_pDOBRO) fprintf(s, "  str x0, [x1]\n");
            else if(campo->tipo_base == T_pINT) fprintf(s, "  str w0, [x1]\n");
            else if(campo->tipo_base == T_pCAR) fprintf(s, "  strb w0, [x1]\n");
        } else {
            if(campo->tipo_base == T_pCAR || campo->tipo_base == T_pBOOL || campo->tipo_base == T_pBYTE)
            fprintf(s, "  strb w0, [x1]\n");
            else if(campo->tipo_base == T_pINT)
            fprintf(s, "  str w0, [x1]\n");
            else if(campo->tipo_base == T_pFLU)
            fprintf(s, "  str s0, [x1]\n");
            else if(campo->tipo_base == T_pDOBRO)
            fprintf(s, "  str d0, [x1]\n");
            else if(campo->tipo_base == T_pLONGO)
            fprintf(s, "  str x0, [x1]\n");
        }
        return;
    }
    Variavel* var = buscar_var(id, escopo);
    if(!var) fatal("[verificar_atribuicao] variável não declarada");
    if(var->eh_final) fatal("[verificar_atribuicao] não é possível alterar uma variável final");
    if(var->eh_array) fatal("[verificar_atribuicao] não é possível armazenar valor direto em array");
    
    // verifica qual operador de atribuição foi usado
    TipoToken op_atrib = L.tk.tipo;
    TipoToken op_aritmetico = T_FIM;
    
    if(op_atrib == T_MAIS_IGUAL) op_aritmetico = T_MAIS;
    else if(op_atrib == T_MENOS_IGUAL) op_aritmetico = T_MENOS;
    else if(op_atrib == T_VEZES_IGUAL) op_aritmetico = T_VEZES;
    else if(op_atrib == T_DIV_IGUAL) op_aritmetico = T_DIV;
    else if(op_atrib == T_PORCEN_IGUAL) op_aritmetico = T_PORCEN;
    else if(op_atrib != T_IGUAL) fatal("[verificar_atribuicao] operador de atribuição esperado");
    
    proximoToken(); // consome o operador
    
    if(op_aritmetico != T_FIM) {
        // atribuição composta: var += expr, var -= expr, etc
        // primeiro carrega o valor atual da variavel
        if(var->escopo == -1) {
            if(var->tipo_base == T_pCAR || var->tipo_base == T_pBOOL || var->tipo_base == T_pBYTE) {
                fprintf(s, "  ldr x0, = global_%s\n", var->nome);
                fprintf(s, "  ldrb w0, [x0]\n");
            } else if(var->tipo_base == T_pINT) {
                fprintf(s, "  ldr x0, = global_%s\n", var->nome);
                fprintf(s, "  ldr w0, [x0]\n");
            } else if(var->tipo_base == T_pFLU) {
                fprintf(s, "  ldr x0, = global_%s\n", var->nome);
                fprintf(s, "  ldr s0, [x0]\n");
            } else if(var->tipo_base == T_pDOBRO) {
                fprintf(s, "  ldr x0, = global_%s\n", var->nome);
                fprintf(s, "  ldr d0, [x0]\n");
            } else if(var->tipo_base == T_pLONGO || var->eh_ponteiro) {
                fprintf(s, "  ldr x0, = global_%s\n", var->nome);
                fprintf(s, "  ldr x0, [x0]\n");
            }
        } else {
            if(var->tipo_base == T_pCAR || var->tipo_base == T_pBOOL || var->tipo_base == T_pBYTE) {
                fprintf(s, "  ldrb w0, [x29, %d]\n", var->pos);
            } else if(var->tipo_base == T_pINT) {
                fprintf(s, "  ldr w0, [x29, %d]\n", var->pos);
            } else if(var->tipo_base == T_pFLU) {
                fprintf(s, "  ldr s0, [x29, %d]\n", var->pos);
            } else if(var->tipo_base == T_pDOBRO) {
                fprintf(s, "  ldr d0, [x29, %d]\n", var->pos);
            } else if(var->tipo_base == T_pLONGO || var->eh_ponteiro) {
                fprintf(s, "  ldr x0, [x29, %d]\n", var->pos);
            }
        }
        // salva o valor atual na pilha
        if(var->tipo_base == T_pFLU) fprintf(s, "  str s0, [sp, -16]!\n");
        else if(var->tipo_base == T_pDOBRO) fprintf(s, "  str d0, [sp, -16]!\n");
        else if(var->tipo_base == T_pLONGO || var->eh_ponteiro) fprintf(s, "  str x0, [sp, -16]!\n");
        else fprintf(s, "  str w0, [sp, -16]!\n");
        
        // processa a expressão do lado direito
        TipoToken tipo_exp = expressao(s, escopo);
        
        // recupera o valor original em registrador alternativo
        if(var->tipo_base == T_pFLU) fprintf(s, "  ldr s1, [sp], 16\n");
        else if(var->tipo_base == T_pDOBRO) fprintf(s, "  ldr d1, [sp], 16\n");
        else if(var->tipo_base == T_pLONGO || var->eh_ponteiro) fprintf(s, "  ldr x1, [sp], 16\n");
        else fprintf(s, "  ldr w1, [sp], 16\n");
        
        // converte tipos se necessario
        TipoToken tipo_final = converter_tipos(s, var->tipo_base, tipo_exp);
        
        // aplica a operação aritmetica
        gerar_operacao(s, op_aritmetico, tipo_final);
        
        // armazena o resultado de volta na variavel
        armazenar_valor(s, var);
    } else {
        // atribuição simples: var = expr
        TipoToken tipo_exp = expressao(s, escopo);
        
        // determina o tipo do registrador baseado no tipo da variavel
        char reg_tipo;
        int reg_num;
        
        if(var->tipo_base == T_pFLU) reg_tipo = 's';
        else if(var->tipo_base == T_pDOBRO) reg_tipo = 'd';
        else if(var->tipo_base == T_pLONGO || var->tipo_base == T_PONTEIRO) reg_tipo = 'x';
        else reg_tipo = 'w';
        
        // tenta alocar um registrador temporario
        reg_num = alocar_reg(reg_tipo);
        
        if(reg_num >= 0) {
            // tem registrador disponivel
            if(debug_o) printf("[verificar_atribuicao]: usando registrador %c%d pra valor temporário\n", reg_tipo, reg_num);
            
            // move o resultado da expressão pra o registrador temporario
            if(reg_tipo == 's') {
                fprintf(s, "  fmov s%d, s0\n", reg_num);
            } else if(reg_tipo == 'd') {
                fprintf(s, "  fmov d%d, d0\n", reg_num);
            } else if(reg_tipo == 'x') {
                fprintf(s, "  mov x%d, x0\n", reg_num);
            } else {
                fprintf(s, "  mov w%d, w0\n", reg_num);
            }
            // armazena o valor
            if(var->eh_ponteiro) {
                fprintf(s, "  ldr x1, [x29, %d]\n", var->pos); // carrega o endereço
                if(var->tipo_base == T_pCAR || var->tipo_base == T_pBOOL || var->tipo_base == T_pBYTE)
                fprintf(s, "  strb w0, [x1]\n");
                else if(var->tipo_base == T_pINT)
                fprintf(s, "  str w0, [x1]\n");
                else if(var->tipo_base == T_pFLU)
                fprintf(s, "  str s0, [x1]\n");
                else if(var->tipo_base == T_pDOBRO)
                fprintf(s, "  str d0, [x1]\n");
                else
                fprintf(s, "  str x0, [x1]\n");
            } else {
                // restaura do registrador temporario antes de armazenar
                if(reg_tipo == 's') {
                    fprintf(s, "  fmov s0, s%d\n", reg_num);
                } else if(reg_tipo == 'd') {
                    fprintf(s, "  fmov d0, d%d\n", reg_num);
                } else if(reg_tipo == 'x') {
                    fprintf(s, "  mov x0, x%d\n", reg_num);
                } else {
                    fprintf(s, "  mov w0, w%d\n", reg_num);
                }
                armazenar_valor(s, var);
            }
            // libera o registrador
            liberar_reg(reg_tipo, reg_num);
        } else {
            // sem registradores disponiveis, usa pilha
            if(var->eh_ponteiro) {
                fprintf(s, "  ldr x1, [x29, %d]\n", var->pos); // carrega o endereço
                if(var->tipo_base == T_pCAR || var->tipo_base == T_pBOOL || var->tipo_base == T_pBYTE)
                fprintf(s, "  strb w0, [x1]\n");
                else if(var->tipo_base == T_pINT)
                fprintf(s, "  str w0, [x1]\n");
                else if(var->tipo_base == T_pFLU)
                fprintf(s, "  str s0, [x1]\n");
                else if(var->tipo_base == T_pDOBRO)
                fprintf(s, "  str d0, [x1]\n");
                else fprintf(s, "  str x0, [x1]\n");
            } else {
                armazenar_valor(s, var);
            }
        }
    }
}

void verificar_matriz(FILE* s, Variavel* var, int escopo, int indices[], int nivel) {
    int item_idc = 0;
    
    while(L.tk.tipo != T_CHAVE_DIR && L.tk.tipo != T_FIM) {
        if(nivel < var->num_dims - 1) {
            // sub-matriz(recursão)
            excessao(T_CHAVE_ESQ);
            indices[nivel] = item_idc;
            verificar_matriz(s, var, escopo, indices, nivel + 1);
            excessao(T_CHAVE_DIR);
        } else {
            // elemento final
            indices[nivel] = item_idc;
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
        item_idc++;
        if(L.tk.tipo == T_VIRGULA) proximoToken();
        else break;
    }
}

void verificar_se(FILE* s, int escopo) {
    excessao(T_SE);
    excessao(T_PAREN_ESQ);
    
    TipoToken tipo_cond = expressao(s, escopo);
    
    // salva o resultado da condição em um registrador temporario se possivel
    char reg_tipo;
    int reg_temp = -1;
    
    if(tipo_cond == T_pFLU) {
        reg_tipo = 's';
        reg_temp = alocar_reg(reg_tipo);
        if(reg_temp >= 0) {
            if(debug_o) printf("[verificar_se]: usando registrador s%d\n", reg_temp);
            fprintf(s, "  fmov s%d, s0\n", reg_temp);
        } else {
            fprintf(s, "  str s0, [sp, -16]!\n");
        }
    } else if(tipo_cond == T_pDOBRO) {
        reg_tipo = 'd';
        reg_temp = alocar_reg(reg_tipo);
        if(reg_temp >= 0) {
            if(debug_o) printf("[verificar_se]: usando registrador d%d\n", reg_temp);
            fprintf(s, "  fmov d%d, d0\n", reg_temp);
        } else {
            fprintf(s, "  str d0, [sp, -16]!\n");
        }
    } else if(tipo_cond == T_pLONGO || tipo_cond == T_PONTEIRO) {
        reg_tipo = 'x';
        reg_temp = alocar_reg(reg_tipo);
        if(reg_temp >= 0) {
            if(debug_o) printf("[verificar_se]: usando registrador x%d\n", reg_temp);
            fprintf(s, "  mov x%d, x0\n", reg_temp);
        } else {
            fprintf(s, "  str x0, [sp, -16]!\n");
        }
    } else {
        reg_tipo = 'w';
        reg_temp = alocar_reg(reg_tipo);
        if(reg_temp >= 0) {
            if(debug_o) printf("[verificar_se]: usando registrador w%d\n", reg_temp);
            fprintf(s, "  mov w%d, w0\n", reg_temp);
        } else {
            fprintf(s, "  str w0, [sp, -16]!\n");
        }
    }
    // verifica se a expressão precisa ser convertida pra booleano
    if(tipo_cond != T_pINT && tipo_cond != T_pBOOL) {
        // carrega o valor salvo
        if(reg_temp >= 0) {
            if(tipo_cond == T_pFLU) {
                fprintf(s, "  fmov s0, s%d\n", reg_temp);
                fprintf(s, "  fcmp s0, 0.0\n");
            } else if(tipo_cond == T_pDOBRO) {
                fprintf(s, "  fmov d0, d%d\n", reg_temp);
                fprintf(s, "  fcmp d0, 0.0\n");
            } else if(tipo_cond == T_pLONGO) {
                fprintf(s, "  mov x0, x%d\n", reg_temp);
                fprintf(s, "  cmp x0, 0\n");
            } else {
                fprintf(s, "  mov w0, w%d\n", reg_temp);
                fprintf(s, "  cmp w0, 0\n");
            }
        } else {
            if(tipo_cond == T_pFLU) {
                fprintf(s, "  ldr s0, [sp], 16\n");
                fprintf(s, "  fcmp s0, 0.0\n");
            } else if(tipo_cond == T_pDOBRO) {
                fprintf(s, "  ldr d0, [sp], 16\n");
                fprintf(s, "  fcmp d0, 0.0\n");
            } else if(tipo_cond == T_pLONGO) {
                fprintf(s, "  ldr x0, [sp], 16\n");
                fprintf(s, "  cmp x0, 0\n");
            } else {
                fprintf(s, "  ldr w0, [sp], 16\n");
                fprintf(s, "  cmp w0, 0\n");
            }
        }
        fprintf(s, "  cset w0, ne\n");
        tipo_cond = T_pBOOL;
    } else {
        // ja é inteiro ou booleano, apenas carrega
        if(reg_temp >= 0) {
            fprintf(s, "  mov w0, w%d\n", reg_temp);
        } else {
            fprintf(s, "  ldr w0, [sp], 16\n");
        }
    }
    // libera registrador temporário se foi alocado
    if(reg_temp >= 0) {
        liberar_reg(reg_tipo, reg_temp);
    }
    excessao(T_PAREN_DIR);
    
    int rotulo_falso = escopo_global++;
    // agora w0 contem 0(falso) ou 1(verdade)
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
    em_loop = true;
    nivel_loop++;
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
    
    // empilha o rotulo de "pare"(fim do loop)
    empilhar_pare(rotulo_fim);
    
    fprintf(s, ".B%d:\n", rotulo_inicio);
    
    // processa condição
    if(L.tk.tipo != T_PONTO_VIRGULA) {
        TipoToken tipo_cond = expressao(s, novo_escopo);
        
        // Salva o resultado da condição em um registrador temporário
        char reg_tipo;
        int reg_temp = -1;
        
        if(tipo_cond == T_pFLU) {
            reg_tipo = 's';
            reg_temp = alocar_reg(reg_tipo);
            if(reg_temp >= 0) {
                if(debug_o) printf("[verificar_por]: usando registrador s%d\n", reg_temp);
                fprintf(s, "  fmov s%d, s0\n", reg_temp);
            } else {
                fprintf(s, "  str s0, [sp, -16]!\n");
            }
        } else if(tipo_cond == T_pDOBRO) {
            reg_tipo = 'd';
            reg_temp = alocar_reg(reg_tipo);
            if(reg_temp >= 0) {
                if(debug_o) printf("[verificar_por]: usando registrador d%d\n", reg_temp);
                fprintf(s, "  fmov d%d, d0\n", reg_temp);
            } else {
                fprintf(s, "  str d0, [sp, -16]!\n");
            }
        } else if(tipo_cond == T_pLONGO || tipo_cond == T_PONTEIRO) {
            reg_tipo = 'x';
            reg_temp = alocar_reg(reg_tipo);
            if(reg_temp >= 0) {
                if(debug_o) printf("[verificar_por]: usando registrador x%d\n", reg_temp);
                fprintf(s, "  mov x%d, x0\n", reg_temp);
            } else {
                fprintf(s, "  str x0, [sp, -16]!\n");
            }
        } else {
            reg_tipo = 'w';
            reg_temp = alocar_reg(reg_tipo);
            if(reg_temp >= 0) {
                if(debug_o) printf("[verificar_por]: usando registrador w%d\n", reg_temp);
                fprintf(s, "  mov w%d, w0\n", reg_temp);
            } else {
                fprintf(s, "  str w0, [sp, -16]!\n");
            }
        }
        // verifica se precisa converter pra booleano
        if(tipo_cond != T_pINT && tipo_cond != T_pBOOL) {
            // carrega o valor salvo
            if(reg_temp >= 0) {
                if(tipo_cond == T_pFLU) {
                    fprintf(s, "  fmov s0, s%d\n", reg_temp);
                    fprintf(s, "  fcmp s0, 0.0\n");
                } else if(tipo_cond == T_pDOBRO) {
                    fprintf(s, "  fmov d0, d%d\n", reg_temp);
                    fprintf(s, "  fcmp d0, 0.0\n");
                } else if(tipo_cond == T_pLONGO) {
                    fprintf(s, "  mov x0, x%d\n", reg_temp);
                    fprintf(s, "  cmp x0, 0\n");
                } else {
                    fprintf(s, "  mov w0, w%d\n", reg_temp);
                    fprintf(s, "  cmp w0, 0\n");
                }
            } else {
                if(tipo_cond == T_pFLU) {
                    fprintf(s, "  ldr s0, [sp], 16\n");
                    fprintf(s, "  fcmp s0, 0.0\n");
                } else if(tipo_cond == T_pDOBRO) {
                    fprintf(s, "  ldr d0, [sp], 16\n");
                    fprintf(s, "  fcmp d0, 0.0\n");
                } else if(tipo_cond == T_pLONGO) {
                    fprintf(s, "  ldr x0, [sp], 16\n");
                    fprintf(s, "  cmp x0, 0\n");
                } else {
                    fprintf(s, "  ldr w0, [sp], 16\n");
                    fprintf(s, "  cmp w0, 0\n");
                }
            }
            fprintf(s, "  cset w0, ne\n");
        } else {
            // ja é inteiro ou booleano, apenas carrega
            if(reg_temp >= 0) {
                fprintf(s, "  mov w0, w%d\n", reg_temp);
            } else {
                fprintf(s, "  ldr w0, [sp], 16\n");
            }
        }
        // libera registrador temporario se foi alocado
        if(reg_temp >= 0) liberar_reg(reg_tipo, reg_temp);
        
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
    
    // restaura pra processar o incremento
    L.pos = pos_incremento;
    L.linha_atual = linha_incremento;
    L.coluna_atual = coluna_incremento;
    L.tk = tk_incremento;
    
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
    // desempilha o rotulo de "pare" apos o loop
    desempilhar_pare();
    em_loop = false;
    nivel_loop--;
}

void verificar_enq(FILE* s, int escopo) {
    em_loop = true;
    nivel_loop++;
    excessao(T_ENQ);
    excessao(T_PAREN_ESQ);

    int novo_escopo = ++escopo_global;
    int rotulo_inicio = escopo_global++;
    int rotulo_fim    = escopo_global++;

    // empilha o rotulo de "pare"
    empilhar_pare(rotulo_fim);

    fprintf(s, ".B%d:\n", rotulo_inicio);

    // avalia a condição usando a mesma abordagem
    TipoToken tipo_cond = expressao(s, novo_escopo);
    
    // salva o resultado da condição em um registrador temporario
    char reg_tipo;
    int reg_temp = -1;
    
    if(tipo_cond == T_pFLU) {
        reg_tipo = 's';
        reg_temp = alocar_reg(reg_tipo);
        if(reg_temp >= 0) {
            if(debug_o) printf("[verificar_enq]: usando registrador s%d\n", reg_temp);
            fprintf(s, "  fmov s%d, s0\n", reg_temp);
        } else {
            fprintf(s, "  str s0, [sp, -16]!\n");
        }
    } else if(tipo_cond == T_pDOBRO) {
        reg_tipo = 'd';
        reg_temp = alocar_reg(reg_tipo);
        if(reg_temp >= 0) {
            if(debug_o) printf("[verificar_enq]: usando registrador d%d\n", reg_temp);
            fprintf(s, "  fmov d%d, d0\n", reg_temp);
        } else {
            fprintf(s, "  str d0, [sp, -16]!\n");
        }
    } else if(tipo_cond == T_pLONGO || tipo_cond == T_PONTEIRO) {
        reg_tipo = 'x';
        reg_temp = alocar_reg(reg_tipo);
        if(reg_temp >= 0) {
            if(debug_o) printf("[verificar_enq]: usando registrador x%d\n", reg_temp);
            fprintf(s, "  mov x%d, x0\n", reg_temp);
        } else {
            fprintf(s, "  str x0, [sp, -16]!\n");
        }
    } else {
        reg_tipo = 'w';
        reg_temp = alocar_reg(reg_tipo);
        if(reg_temp >= 0) {
            if(debug_o) printf("[verificar_enq]: usando registrador w%d\n", reg_temp);
            fprintf(s, "  mov w%d, w0\n", reg_temp);
        } else {
            fprintf(s, "  str w0, [sp, -16]!\n");
        }
    }
    // verifica se precisa converter pra booleano
    if(tipo_cond != T_pINT && tipo_cond != T_pBOOL) {
        // carrega o valor salvo
        if(reg_temp >= 0) {
            if(tipo_cond == T_pFLU) {
                fprintf(s, "  fmov s0, s%d\n", reg_temp);
                fprintf(s, "  fcmp s0, 0.0\n");
            } else if(tipo_cond == T_pDOBRO) {
                fprintf(s, "  fmov d0, d%d\n", reg_temp);
                fprintf(s, "  fcmp d0, 0.0\n");
            } else if(tipo_cond == T_pLONGO) {
                fprintf(s, "  mov x0, x%d\n", reg_temp);
                fprintf(s, "  cmp x0, 0\n");
            } else {
                fprintf(s, "  mov w0, w%d\n", reg_temp);
                fprintf(s, "  cmp w0, 0\n");
            }
        } else {
            if(tipo_cond == T_pFLU) {
                fprintf(s, "  ldr s0, [sp], 16\n");
                fprintf(s, "  fcmp s0, 0.0\n");
            } else if(tipo_cond == T_pDOBRO) {
                fprintf(s, "  ldr d0, [sp], 16\n");
                fprintf(s, "  fcmp d0, 0.0\n");
            } else if(tipo_cond == T_pLONGO) {
                fprintf(s, "  ldr x0, [sp], 16\n");
                fprintf(s, "  cmp x0, 0\n");
            } else {
                fprintf(s, "  ldr w0, [sp], 16\n");
                fprintf(s, "  cmp w0, 0\n");
            }
        }
        fprintf(s, "  cset w0, ne\n");
    } else {
        // ja é inteiro ou booleano, apenas carrega
        if(reg_temp >= 0) {
            fprintf(s, "  mov w0, w%d\n", reg_temp);
        } else {
            fprintf(s, "  ldr w0, [sp], 16\n");
        }
    }
    // libera registrador temporario se foi alocado
    if(reg_temp >= 0) liberar_reg(reg_tipo, reg_temp);
    
    excessao(T_PAREN_DIR);

    fprintf(s, "  cmp w0, 0\n");
    fprintf(s, "  beq .B%d\n", rotulo_fim);
    
    if(L.tk.tipo == T_CHAVE_ESQ) {
        proximoToken();
        while(L.tk.tipo != T_CHAVE_DIR) {
            verificar_stmt(s, &funcs[fn_cnt-1].frame_tam, novo_escopo);
        }
        excessao(T_CHAVE_DIR);
    } else {
        verificar_stmt(s, &funcs[fn_cnt-1].frame_tam, novo_escopo);
    }
    // volta pro começo
    fprintf(s, "  b .B%d\n", rotulo_inicio);
    fprintf(s, ".B%d:\n", rotulo_fim);
    // desempilha o rotulo de "pare"
    desempilhar_pare();
    em_loop = false;
    nivel_loop--;
}

void verificar_stmt(FILE* s, int* pos, int escopo) {
    if(escopo == 0) escopo = escopo_global;
    if(debug_o) printf("[verificar_stmt]: token tipo: %s", token_str(L.tk.tipo));
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
            fprintf(s, "  and w0, w0, 0xFF\n"); // garante que é byte
            fprintf(s, "  uxtw x0, w0\n"); // estende sem sinal pra 64 bits
        } else if(tipo_exp == T_pINT) {
            // int -> pnteiro: estende sem sinal pra 64 bits  
            fprintf(s, "  uxtw x0, w0\n"); // estende sem sinal para 64 bits
        } else if(tipo_exp == T_pLONGO || tipo_exp == T_PONTEIRO) {
            // longo -> ponteiro: ja é 64 bits, não precisa conversão
        } else {
            fatal("[verificar_stmt] endereço deve ser byte, inteiro ou longo ou ponteiro");
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
        // verifica se é um arquivo .fpb ou .asm
        char* extensao = strrchr(caminho, '.');
        if(extensao == NULL) fatal("[verificar_stmt] arquivo sem extensão");
        
        FILE* arquivo_incluir = NULL;
        
        // tenta primeiro com o caminho relativo ao FPB_DIR
        char* base_dir = processar_caminho();
        if(strlen(base_dir) > 0) {
            char caminho_completo[512];
            snprintf(caminho_completo, sizeof(caminho_completo), 
            "%s/%s", base_dir, caminho);
            arquivo_incluir = fopen(caminho_completo, "r");
        }
        // se não encontrou, tenta no diretório atual
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
        if(strcmp(extensao, ".fpb") == 0 || strcmp(extensao, ".FBP") == 0) {
            // processa arquivo .fpb (compila inline)
            fprintf(s, "\n// inicio de %s\n", caminho);
            // salva estado atual do lexer
            Lexer lexer_salvo = L;
            char* fonte_salvo = (char*)L.fonte;
            size_t pos_salvo = L.pos;
            int linha_salvo = L.linha_atual;
            int coluna_salvo = L.coluna_atual;
            Token tk_salvo = L.tk;
            char* arquivoAtual_salvo = arquivoAtual;
            // le o conteudo do arquivo .fpb
            fseek(arquivo_incluir, 0, SEEK_END);
            long tam = ftell(arquivo_incluir);
            fseek(arquivo_incluir, 0, SEEK_SET);
            char* conteudo_fpb = malloc(tam + 1);
            fread(conteudo_fpb, 1, tam, arquivo_incluir);
            conteudo_fpb[tam] = '\0';
            fclose(arquivo_incluir);
            // atualiza o lexer com o novo conteúdo
            L.fonte = conteudo_fpb;
            L.pos = 0;
            L.linha_atual = 1;
            L.coluna_atual = 1;
            arquivoAtual = caminho;
            // processa todo o arquivo .fpb
            proximoToken();
            while(L.tk.tipo != T_FIM) {
                if(L.tk.tipo == T_INCLUIR) {
                    // recursivamente processa outros incluir
                    int pos = 0;
                    verificar_stmt(s, &pos, 0);
                } else if(L.tk.tipo == T_DEF) verificar_def();
                else if(L.tk.tipo == T_ESPACO) verificar_espaco(s);
                else if(L.tk.tipo == T_GLOBAL) verificar_global(s);
                else verificar_fn(s);
            }
            fprintf(s, "\n// fim de %s\n\n", caminho);
            // restaura estado original do lexer
            free(conteudo_fpb);
            L = lexer_salvo;
            L.fonte = fonte_salvo;
            L.pos = pos_salvo;
            L.linha_atual = linha_salvo;
            L.coluna_atual = coluna_salvo;
            L.tk = tk_salvo;
            arquivoAtual = arquivoAtual_salvo;
        } else if(strcmp(extensao, ".asm") == 0 || strcmp(extensao, ".s") == 0) {
            // copia arquivo .asm diretamente
            fprintf(s, "\n// inicio de %s\n", caminho);
            char linha[512];
            while(fgets(linha, sizeof(linha), arquivo_incluir)) {
                if(strstr(linha, ".section .data") != NULL) {
                    fputs(linha, s);
                    fputs("  .align 2\n", s);
                } else if(strstr(linha, ": .asciz") != NULL) {
                    fputs("  .align 2\n", s);
                    fputs(linha, s);
                } else {
                    fputs(linha, s);
                }
            }
            fprintf(s, "\n// fim de %s\n\n", caminho);
            fclose(arquivo_incluir);
        } else {
            fclose(arquivo_incluir);
            fatal("[verificar_stmt] extensão de arquivo não suportada (use .fpb ou .asm)");
        }
        return;
    }
    if(L.tk.tipo == T_GLOBAL) {
        proximoToken(); // consome "global"
        // deve ser seguido por declaração de função
        if(!eh_tipo(L.tk.tipo)) {
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
    if(eh_tipo(L.tk.tipo) || eh_final) {
        declaracao_var(s, pos, escopo, 0, eh_final, (escopo == -1) ? 1 : 0);
        if(L.tk.tipo == T_PONTO_VIRGULA) excessao(T_PONTO_VIRGULA);
        return;
    }
    if(L.tk.tipo == T_ID) {
        char idn[64];
        strcpy(idn, L.tk.lex);
        if(debug_o) printf("[verificar_stmt]: ID ACESSADO: %s\n", idn);
        proximoToken();
        if(debug_o) printf("[verificar_stmt]: PROXIMO ACESSADO: %s\n", token_str(L.tk.tipo));
        if(L.tk.tipo == T_PONTO) {
            // acesso a campo de estrutura
            char var_nome[32];
            char campo_nome[32];
            
            strcpy(var_nome, idn); // salva o nome da variável(n)
            proximoToken(); // consome o ponto
            
            if(L.tk.tipo != T_ID) fatal("[verificar_stmt] nome do campo esperado após ponto");
            
            strcpy(campo_nome, L.tk.lex); // salva o nome do campo(i)
            proximoToken(); // consome o nome do campo
            
            // verifica se é acesso a array (n.i[indice])
            if(L.tk.tipo == T_COL_ESQ) {
                // concatena pra formar "n.i" e chama verificar_atribuicao que ja trata arrays
                char nome_completo[128];
                snprintf(nome_completo, sizeof(nome_completo), "%s.%s", var_nome, campo_nome);
                
                // agora verifica se é atribuição(n.i[0] = valor)
                if(L.tk.tipo == T_COL_ESQ) {
                    // processa o acesso ao array normalmente
                    // a logica será tratada em verificar_atribuicao
                    verificar_atribuicao(s, nome_completo, escopo);
                }
                if(L.tk.tipo == T_PONTO_VIRGULA) excessao(T_PONTO_VIRGULA);
                return;
            }
            // agora verifica se é uma atribuição(n.i = 0)
            if(eh_atribuicao(L.tk.tipo)) {
                // concatena pra passar pra verificar_atribuicao
                char nome_completo[128];
                snprintf(nome_completo, sizeof(nome_completo), "%s.%s", var_nome, campo_nome);
                verificar_atribuicao(s, nome_completo, escopo);
                if(L.tk.tipo == T_PONTO_VIRGULA) excessao(T_PONTO_VIRGULA);
                return;
            }
        }
        if(eh_atribuicao(L.tk.tipo)) {
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
            // === ACESSO A ELEMENTO DE ARRAY ===
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
            if(strcmp(idn,"_asm_") == 0) {
                if(debug_o) printf("[verificar_stmt]: chamada a _asm_\n");
                fprintf(s, "// inicio assembly manual\n");
                int primeiro = 1;
                
                while(1) {
                    if(L.tk.tipo == T_TEX) {
                        // texto direto do assembly
                        fprintf(s, "%s", L.tk.lex);
                        proximoToken();
                    } else if(L.tk.tipo == T_ID) {
                        // id, pode ser variavel ou macro
                        char id_temp[64];
                        strcpy(id_temp, L.tk.lex);
                        Variavel* var = buscar_var(id_temp, escopo);
                        Macro* macro = buscar_macro(id_temp);
                        
                        if(var) {
                            if(debug_o) printf("[verificar_stmt]: parametro variável: %s, tipo: %s\n", var->nome, token_str(var->tipo_base));
                            // global, senão local
                            if(var->escopo == -1) fprintf(s, "global_%s", var->nome);
                            else fprintf(s, "[x29, %d]", var->pos);
                            
                            proximoToken();
                        } else if(macro) {
                            // é uma macro
                            if(debug_o) printf("[verificar_stmt]: parametro macro: %s = %ld\n", macro->nome, macro->valor);
                            fprintf(s, "%ld", macro->valor);
                            proximoToken();
                        } else {
                            // não encontrado, trata como texto literal
                            fprintf(s, "%s", id_temp);
                            proximoToken();
                        }
                    } else if(L.tk.tipo == T_INT) {
                        // literal inteiro
                        fprintf(s, "%ld", L.tk.valor_l);
                        proximoToken();
                    } else if(L.tk.tipo == T_CAR) {
                        // literal caractere, trata caracteres de escape
                        char c = (char)L.tk.valor_l;
                        // verifica se é um caractere de controle
                        switch(c) {
                            case '\n': fprintf(s, "\n"); break;
                            case '\t': fprintf(s, "\t"); break;
                            case '\r': fprintf(s, "\r"); break;
                            case '\0': fprintf(s, "\\0"); break;  // caractere nulo
                            case '\\': fprintf(s, "\\"); break;
                            case '\'': fprintf(s, "'"); break;
                            case '\"': fprintf(s, "\""); break;
                            case '\a': fprintf(s, "\a"); break;
                            case '\b': fprintf(s, "\b"); break;
                            case '\v': fprintf(s, "\v"); break;
                            case '\f': fprintf(s, "\f"); break;
                            default:
                            // caractere imprimivel normal
                            if(c >= 32 && c <= 126) {
                                fprintf(s, "%c", c);
                            } else {
                                // caractere não imprimível, usa valor decimal
                                fprintf(s, "%d", (int)c);
                            }
                            break;
                        }
                        proximoToken();
                    } else if(L.tk.tipo == T_FLU || L.tk.tipo == T_DOBRO) {
                        // literal flutuante
                        fprintf(s, "%f", L.tk.valor_d);
                        proximoToken();
                    } else if(L.tk.tipo == T_BYTE) {
                        // literal byte
                        fprintf(s, "%ld", L.tk.valor_l);
                        proximoToken();
                    } else if(L.tk.tipo == T_BOOL) {
                        // literal booleano
                        fprintf(s, "%s", strcmp(L.tk.lex, "verdade") == 0 ? "1" : "0");
                        proximoToken();
                    } else {
                        // outros tokens podem ser parte da sintaxe
                        if(L.tk.tipo == T_VIRGULA) {
                            proximoToken();
                            continue;
                        }
                        if(L.tk.tipo == T_PAREN_DIR) break;
                        // se não for reconhecido, imprime como texto
                        fprintf(s, "%s", token_str(L.tk.tipo));
                        proximoToken();
                    }
                    if(L.tk.tipo == T_VIRGULA) {
                        proximoToken();
                        continue;
                    }
                    if(L.tk.tipo == T_PAREN_DIR) break;
                }
                excessao(T_PAREN_DIR);
                fprintf(s, "\n// fim assembly manual\n");
                if(L.tk.tipo == T_PONTO_VIRGULA) excessao(T_PONTO_VIRGULA);
                return;
            }
            if(strcmp(idn,"escrever") == 0) {
                while(1) {
                    // verifica se é acesso a campo de estrutura (ex: p.nome)
                    if(L.tk.tipo == T_ID) {
                        // salva o estado pra verificar se é acesso a campo
                        char primeiro_id[64];
                        strcpy(primeiro_id, L.tk.lex);
                        // faz uma pré-visualização
                        size_t pos_salvo = L.pos;
                        int linha_salvo = L.linha_atual;
                        int coluna_salvo = L.coluna_atual;
                        Token tk_salvo = L.tk;
                        
                        proximoToken(); // avança pra ver o proximo token
                        
                        if(L.tk.tipo == T_PONTO) {
                            // é acesso a campo, restaura e processa como expressão completa
                            L.pos = pos_salvo;
                            L.linha_atual = linha_salvo;
                            L.coluna_atual = coluna_salvo;
                            L.tk = tk_salvo;
                            
                            // processa como expressão normal (que vai lidar com o ponto)
                            TipoToken tipo_arg = expressao(s, escopo);
                            escrever_valor(s, tipo_arg);
                        } else {
                            // não é acesso a campo, restaura e processa normalmente
                            L.pos = pos_salvo;
                            L.linha_atual = linha_salvo;
                            L.coluna_atual = coluna_salvo;
                            L.tk = tk_salvo;
                            
                            // processa normalmente
                            Variavel* var = buscar_var(primeiro_id, escopo);
                            if(var && var->eh_ponteiro) {
                                if(var->tipo_base == T_pCAR) {
                                    if(var->escopo == -1) { // global    
                                    fprintf(s, "  ldr x0, = global_%s\n", var->nome);    
                                    fprintf(s, "  ldr x0, [x0]\n"); // carrega o ponteiro    
                                } else { // local    
                                    fprintf(s, "  ldr x0, [x29, %d]\n", var->pos);
                                }    
                                escrever_valor(s, T_TEX); // escrever como texto
                                proximoToken();
                                } else {
                                    fprintf(s, "  ldr x1, [x29, %d]\n", var->pos);
                                    // carrega o valor apontado baseado no tipo base
                                    if(var->tipo_base == T_pBOOL)
                                    fprintf(s, "  ldrb w0, [x1]\n");
                                    else if(var->tipo_base == T_pINT)
                                    fprintf(s, "  ldr w0, [x1]\n");
                                    else if(var->tipo_base == T_pFLU)
                                    fprintf(s, "  ldr s0, [x1]\n");
                                    else if(var->tipo_base == T_pDOBRO)
                                    fprintf(s, "  ldr d0, [x1]\n");
                                    else if(var->tipo_base == T_pLONGO)
                                    fprintf(s, "  ldr x0, [x1]\n");
                                    escrever_valor(s, var->tipo_base);
                                    proximoToken();
                                }
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
                                // não é variável conhecida, processa como expressão
                                TipoToken tipo_arg = expressao(s, escopo);
                                escrever_valor(s, tipo_arg);
                            }
                        }
                    } else {
                        // não é ID, processa como expressão normal
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
                // === CHAMADA DE FUNÇÃO NORMAL ===
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
    if(L.tk.tipo == T_PARE) {
        if(rotulo_pare_topo < 0) {
            fatal("[verificar_stmt] \"pare\" usado fora de loop");
        }
        // gera salto pro rotulo de "pare"(fim do loop)
        fprintf(s, "  b .B%d\n", topo_pare());
        proximoToken();
        if(L.tk.tipo == T_PONTO_VIRGULA) excessao(T_PONTO_VIRGULA);
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
    
    funcs[fn_cnt].vars = malloc(MAX_VAR * sizeof(Variavel));
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
        int tam_vars_locais = processar_var_tam(escopo_global);
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
// [TRATAMENTO]:
TipoToken tratar_texto(FILE* s) {
    int id = add_tex(L.tk.lex);
    
    if(debug_o) printf("[tratar_texto]: Adicionando texto \"%s\" com id %d\n", L.tk.lex, id);
    
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
        } else if(strcmp(id, "bytes") == 0) {
            proximoToken(); // avança alem de "bytes"
            excessao(T_PAREN_ESQ);
            int tam = 0;
            if(L.tk.tipo == T_ID) {
                Variavel* vb = buscar_var(L.tk.lex, escopo);
                if(vb) {
                    if(vb->tipo_base == T_ESPACO_ID) { Espaco* esp = buscar_espaco(vb->espaco); tam = esp ? esp->tam_total : 0; }
                    else if(vb->eh_array) tam = vb->bytes;
                    else tam = tam_tipo(vb->tipo_base);
                } else {
                    Espaco* esp = buscar_espaco(L.tk.lex);
                    if(esp) tam = esp->tam_total;
                    else fatal("[bytes] variável ou espaço não encontrado");
                }
                proximoToken();
            } else if(eh_tipo(L.tk.tipo)) {
                tam = tam_tipo(L.tk.tipo);
                proximoToken();
            } else fatal("[bytes] identificador ou tipo esperado");
            excessao(T_PAREN_DIR);
            fprintf(s, "  mov x0, %d\n", tam);
            return T_pLONGO;
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
    
    if(L.tk.tipo == T_PONTO) {
        if(var->tipo_base != T_ESPACO_ID) {
            fatal("[tratar_id] tentativa de acessar campo em tipo não-estrutura");
        }
        int via_ponteiro = var->eh_ponteiro;
        proximoToken(); // consome o ponto
        
        if(L.tk.tipo != T_ID) {
            fatal("[tratar_id] nome do campo esperado após ponto");
        }
        // guarda o nome do campo
        char nome_campo[32];
        strcpy(nome_campo, L.tk.lex);
        
        // busca o espaço usando o nome armazenado na variavel
        Espaco* esp = NULL;
        for(int i = 0; i < espaco_cnt; i++) {
            if(strcmp(espacos[i].nome, var->espaco) == 0) {
                esp = &espacos[i];
                break;
            }
        }
        if(!esp) {
            char msg[100];
            sprintf(msg, "[tratar_id] espaço '%s' não encontrado", var->espaco);
            fatal(msg);
        }
        // busca o campo no espaço
        Variavel* campo = NULL;
        for(int i = 0; i < esp->campo_cnt; i++) {
            if(strcmp(esp->campos[i].nome, nome_campo) == 0) {
                campo = &esp->campos[i];
                break;
            }
        }
        if(!campo) {
            char msg[100];
            sprintf(msg, "[tratar_id] campo '%s' não encontrado no espaço '%s'", 
                nome_campo, var->espaco);
                fatal(msg);
        }
        proximoToken(); // consome o nome do campo
        
        // verifica se é acesso a array(campo[indice])
        if(L.tk.tipo == T_COL_ESQ && campo->eh_array) {
            // calcula endereço base do array dentro do espaço
            if(via_ponteiro) {
                if(var->escopo == -1) { fprintf(s, "  ldr x1, = global_%s\n", var->nome); fprintf(s, "  ldr x1, [x1]\n"); }
                else fprintf(s, "  ldr x1, [x29, %d]\n", var->pos);
                if(campo->pos != 0) fprintf(s, "  add x1, x1, %d\n", campo->pos);
            } else if(var->escopo == -1) {
                fprintf(s, "  ldr x1, = global_%s\n", var->nome);
                fprintf(s, "  add x1, x1, %d\n", campo->pos);
            } else {
                fprintf(s, "  add x1, x29, %d\n", var->pos + campo->pos);
            }
            // processa todos os indices do array
            int dim_atual = 0;
            while(L.tk.tipo == T_COL_ESQ && dim_atual < campo->num_dims) {
                excessao(T_COL_ESQ);
                expressao(s, escopo); // indice em w0
                fprintf(s, "  str w0, [sp, -16]!\n"); // salva indice
                excessao(T_COL_DIR);
                dim_atual++;
            }
            // calcula posição total
            if(dim_atual > 0) {
                // inicia pos
                fprintf(s, "  mov w0, 0\n"); // w0 = posição acumulada
                // pra cada dimensão, calcula pos parcial
                for(int i = 0; i < dim_atual; i++) {
                    fprintf(s, "  ldr w1, [sp], 16\n"); // w1 = indice atual
                    
                    // calcula stride pra essa dimensão
                    int stride = tam_tipo(campo->tipo_base);
                    for(int j = i + 1; j < campo->num_dims; j++) {
                        if(campo->dims[j] > 0) stride *= campo->dims[j];
                    }
                    // pos += indice * stride
                    fprintf(s, "  mov w2, %d\n", stride);
                    fprintf(s, "  mul w1, w1, w2\n");
                    fprintf(s, "  add w0, w0, w1\n");
                }
                // x1 = endereço base + pos
                fprintf(s, "  add x1, x1, x0\n");
            }
            // carrega o valor
            if(campo->eh_ponteiro) {
                fprintf(s, "  ldr x0, [x1]\n");
                return T_PONTEIRO;
            } else {
                if(campo->tipo_base == T_pCAR || campo->tipo_base == T_pBOOL || campo->tipo_base == T_pBYTE)
                fprintf(s, "  ldrb w0, [x1]\n");
                else if(campo->tipo_base == T_pINT)
                fprintf(s, "  ldr w0, [x1]\n");
                else if(campo->tipo_base == T_pFLU)
                fprintf(s, "  ldr s0, [x1]\n");
                else if(campo->tipo_base == T_pDOBRO)
                fprintf(s, "  ldr d0, [x1]\n");
                else if(campo->tipo_base == T_pLONGO)
                fprintf(s, "  ldr x0, [x1]\n");
                return campo->tipo_base;
            }
        }
        // calcula endereço do campo
        if(via_ponteiro) {
            if(var->escopo == -1) { fprintf(s, "  ldr x0, = global_%s\n", var->nome); fprintf(s, "  ldr x0, [x0]\n"); }
            else fprintf(s, "  ldr x0, [x29, %d]\n", var->pos);
            if(campo->pos != 0) fprintf(s, "  add x0, x0, %d\n", campo->pos);
        } else if(var->escopo == -1) {
            fprintf(s, "  ldr x0, = global_%s\n", var->nome);
            fprintf(s, "  add x0, x0, %d\n", campo->pos);
        } else {
            fprintf(s, "  add x0, x29, %d\n", var->pos + campo->pos);
        }
        // se for um campo que também é espaço, precisa de tratamento especial
        if(campo->tipo_base == T_ESPACO_ID) {
            // retorna endereço do sub espaço
            return T_PONTEIRO;
        }
        // carrega o valor do campo
        if(campo->eh_ponteiro) {
            fprintf(s, "  ldr x0, [x0]\n");
            if(campo->tipo_base == T_pCAR) return T_TEX;
            else return T_PONTEIRO;
        } else if(campo->eh_array) {
            // retorna endereço do array
            return T_PONTEIRO;
        } else {
            // carrega baseado no tipo
            if(campo->tipo_base == T_pCAR || campo->tipo_base == T_pBOOL || campo->tipo_base == T_pBYTE)
            fprintf(s, "  ldrb w0, [x0]\n");
            else if(campo->tipo_base == T_pINT)
            fprintf(s, "  ldr w0, [x0]\n");
            else if(campo->tipo_base == T_pFLU)
            fprintf(s, "  ldr s0, [x0]\n");
            else if(campo->tipo_base == T_pDOBRO)
            fprintf(s, "  ldr d0, [x0]\n");
            else if(campo->tipo_base == T_pLONGO)
            fprintf(s, "  ldr x0, [x0]\n");
            
            return campo->tipo_base;
        }
    }
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
        int* indices = malloc(MAX_DIMS * sizeof(int));
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
        if(var->tipo_base == T_pCAR || var->tipo_base == T_pBOOL || var->tipo_base == T_pBYTE)
        fprintf(s, "  ldrb w0, [x1]\n");
        else if(var->tipo_base == T_pINT)
        fprintf(s, "  ldr w0, [x1]\n"); 
        else if(var->tipo_base == T_pFLU)
        fprintf(s, "  ldr s0, [x1]\n");
        else if(var->tipo_base == T_pDOBRO)
        fprintf(s, "  ldr d0, [x1]\n");
        else if(var->tipo_base == T_pLONGO || var->tipo_base == T_PONTEIRO)
        fprintf(s, "  ldr x0, [x1]\n");
        
        return var->tipo_base;
    } else {
        carregar_valor(s, var);
        return var->tipo_base;
    }
    return var->tipo_base;
}

TipoToken tratar_chamada_funcao(FILE* s, int escopo, const char* nome, Funcao* fn) {
    if(fn == NULL) fatal("INTERNO CRITICO, FUNÇÃO INEXISTENTE!");
    
    // guarda valores dos parametros
    TipoToken param_tipos[8];
    int param_conta = 0;
    
    // passo 1: avalia e salva todos os args em temporarios
    int params_pilha = 0;
    
    if(L.tk.tipo != T_PAREN_DIR) {
        do {
            param_tipos[param_conta] = expressao(s, escopo);
            
            if(param_conta < fn->var_conta) {
                Variavel* param_esperado = &fn->vars[param_conta];
                TipoToken tipo_esperado = param_esperado->tipo_base;
                TipoToken tipo_fornecido = param_tipos[param_conta];
                
                // so converte se:
                // 1. não for array
                // 2. tipos esperado e fornecido não forem ponteiros
                // 3. tipos forem diferentes
                int eh_array = param_esperado->eh_array;
                int eh_ponteiro = (tipo_esperado == T_PONTEIRO || tipo_fornecido == T_PONTEIRO ||
                tipo_esperado == T_TEX || tipo_fornecido == T_TEX);
                
                if(!eh_array && !eh_ponteiro && tipo_esperado != tipo_fornecido) {
                    if(debug_o) {
                        printf("[tratar_chamada_funcao]: convertendo param %d de %s para %s\n",
                        param_conta, token_str(tipo_fornecido), token_str(tipo_esperado));
                    }
                    gerar_convert(s, tipo_fornecido, tipo_esperado);
                    param_tipos[param_conta] = tipo_esperado;
                }
            }
            // salva o valor atual(ta em x0/w0/s0/d0)
            if(param_tipos[param_conta] == T_pFLU) {
                fprintf(s, "  str s0, [sp, -16]!  // salva param %d (float)\n", param_conta);
            } else if(param_tipos[param_conta] == T_pDOBRO) {
                fprintf(s, "  str d0, [sp, -16]!  // salva param %d (double)\n", param_conta);
            } else if(param_tipos[param_conta] == T_PONTEIRO || param_tipos[param_conta] == T_pLONGO) {
                fprintf(s, "  str x0, [sp, -16]!  // salva param %d (ponteiro/longo)\n", param_conta);
            } else {
                fprintf(s, "  str w0, [sp, -16]!  // salva param %d (int/bool/char/byte)\n", param_conta);
            }
            param_conta++;
        } while(L.tk.tipo == T_VIRGULA && (proximoToken(), 1));
    }
    excessao(T_PAREN_DIR);
    // calcula quantos vão na pilha da função chamada
    params_pilha = (param_conta > 8) ? (param_conta - 8) : 0;
    
    // passo 2: prepara pilha para parametros 9+
    if(params_pilha > 0) {
        fprintf(s, "  sub sp, sp, %d  // espaço para parâmetros 9+ na pilha\n", params_pilha * 16);
    }
    // passo 3: Carrega valores na ordem inversao
    for(int i = param_conta - 1; i >= 0; i--) {
        // calcula de onde pegar(estão salvos na pilha em ordem)
        int pos_salvo = (param_conta - i - 1) * 16;
        
        if(i < 8) {
            // vai em registrador
            if(param_tipos[i] == T_pFLU) {
                fprintf(s, "  ldr s%d, [sp, %d]  // carrega param %d (flu) em s%d\n", 
                        i, pos_salvo, i, i);
            } else if(param_tipos[i] == T_pDOBRO) {
                fprintf(s, "  ldr d%d, [sp, %d]  // carrega param %d (dobro) em d%d\n", 
                        i, pos_salvo, i, i);
            } else if(param_tipos[i] == T_PONTEIRO || param_tipos[i] == T_pLONGO) {
                fprintf(s, "  ldr x%d, [sp, %d]  // carrega param %d (ptr/longo) em x%d\n", 
                        i, pos_salvo, i, i);
            } else {
                // int, bool, car, byte = carrega em w mas passa em x
                fprintf(s, "  ldr w%d, [sp, %d]  // carrega param %d (int/bool) em w%d\n", 
                        i, pos_salvo, i, i);
                fprintf(s, "  mov x%d, x%d  // estende pra 64 bits\n", i, i);
            }
        } else {
            // vai na pilha da função chamada
            int pilha_pos = (i - 8) * 16;
            
            if(param_tipos[i] == T_pFLU) {
                fprintf(s, "  ldr s0, [sp, %d]  // param %d (flu) para pilha\n", pos_salvo, i);
                fprintf(s, "  str s0, [sp, %d]  // armazena na pilha da função chamada\n", pilha_pos);
            } else if(param_tipos[i] == T_pDOBRO) {
                fprintf(s, "  ldr d0, [sp, %d]  // param %d (dobro) para pilha\n", pos_salvo, i);
                fprintf(s, "  str d0, [sp, %d]  // armazena na pilha da função chamada\n", pilha_pos);
            } else if(param_tipos[i] == T_PONTEIRO || param_tipos[i] == T_pLONGO) {
                fprintf(s, "  ldr x0, [sp, %d]  // param %d (ptr/longo) para pilha\n", pos_salvo, i);
                fprintf(s, "  str x0, [sp, %d]  // armazena na pilha da função chamada\n", pilha_pos);
            } else {
                fprintf(s, "  ldr w0, [sp, %d]  // param %d (int/bool) para pilha\n", pos_salvo, i);
                fprintf(s, "  str w0, [sp, %d]  // armazena na pilha da função chamada\n", pilha_pos);
            }
        }
    }
    // passo 4 limpa nossos temporarios
    fprintf(s, "  add sp, sp, %d  // limpa temporarios\n", param_conta * 16);
    
    // passo 5: chama a função
    fprintf(s, "  bl %s\n", nome);
    
    // passo 6: limpa pilha da função chamada(parâmetros 9+)
    if(params_pilha > 0) {
        fprintf(s, "  add sp, sp, %d  // limpa parâmetros da pilha\n", params_pilha * 16);
    }
    // ajusta retorno se necessario
    if(fn->retorno == T_pFLU) fprintf(s, "  fmov s0, s0\n");
    else if(fn->retorno == T_pDOBRO) fprintf(s, "  fmov d0, d0\n");
    
    return fn->retorno;
}

TipoToken tratar_bool(FILE* s) {
    int valor = (strcmp(L.tk.lex, "verdade") == 0) ? 1 : 0;
    proximoToken();
    fprintf(s, "  mov w0, %d\n", valor);
    return T_pBOOL;
}

TipoToken tratar_inteiro(FILE* s) {
    char num[32];
    strcpy(num, L.tk.lex);
    unsigned long l_val = (unsigned long)L.tk.valor_l;

    proximoToken();

    // longo:
    if(L.tk.tipo == T_ID &&
       (strcmp(L.tk.lex, "L") == 0 || strcmp(L.tk.lex, "l") == 0)) {

        proximoToken();

        if(l_val <= 0xFFFF) {
            fprintf(s, "  mov x0, %lu\n", l_val);
        } else {
            int shift = 0;
            int primeiro = 1;
            unsigned long v = l_val;

            while(v) {
                unsigned long p = v & 0xFFFF;
                if(p) {
                    if(primeiro) {
                        fprintf(s, "  movz x0, %lu, lsl %d\n", p, shift);
                        primeiro = 0;
                    } else {
                        fprintf(s, "  movk x0, %lu, lsl %d\n", p, shift);
                    }
                }
                v >>= 16;
                shift += 16;
            }
        }
        return T_pLONGO;
    }
    // inteiro
    if(l_val <= 0xFFFF) {
        fprintf(s, "  mov w0, %lu\n", l_val);
    } else {
        fprintf(s, "  movz w0, %lu\n", l_val & 0xFFFF);
        if((l_val >> 16) & 0xFFFF) {
            fprintf(s, "  movk w0, %lu, lsl 16\n",
                    (l_val >> 16) & 0xFFFF);
        }
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
    char val = (char)L.tk.valor_l;
    proximoToken();
    fprintf(s, "  mov w0, %d\n", val);
    return T_pCAR;
}

TipoToken tratar_byte(FILE* s) {
    char num[32];
    strcpy(num, L.tk.lex);
    unsigned long byte_val = (unsigned long)L.tk.valor_l;  // MUDOU: unsigned long para aceitar valores 32-bit
    proximoToken();
    // bytes sempre usam valor imediato
    if(byte_val <= 0xFFFF) {
        fprintf(s, "  mov w0, %lu // byte: %s\n", byte_val, num);
    } else {
        // valores maiores que 16 bits precisam de movz/movk
        fprintf(s, "  movz w0, %lu // byte: %s\n", byte_val & 0xFFFF, num);
        if((byte_val >> 16) & 0xFFFF) {
            fprintf(s, "  movk w0, %lu, lsl 16\n", (byte_val >> 16) & 0xFFFF);
        }
    }
    return T_pBYTE;
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
        TipoToken tipo_param = L.tk.tipo == T_TEX ? T_PONTEIRO : L.tk.tipo;
        if(debug_o) printf("[processar_args]: tipo do parâmetro é %s\n", token_str(tipo_param));
        
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
        tipo_param == T_TEX || tipo_param == T_pCAR || tipo_param == T_pBOOL || eh_array_param)) {
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

int processar_var_tam(int escopo) {
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

TipoToken fator(FILE* s, int escopo) {
    if(L.tk.tipo == T_CONVERTA) {
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
    if(L.tk.tipo == T_NAO) {
        proximoToken(); // consome o !
        TipoToken tipo = fator(s, escopo); // processa o operando
        
        if(tipo != T_pINT && tipo != T_pBOOL) {
            // tenta converter para booleano se necessario
            if(tipo == T_pFLU) {
                fprintf(s, "  fcmp s0, 0.0\n");
                fprintf(s, "  cset w0, ne\n"); // 1 se diferente de 0
                tipo = T_pBOOL;
            } else if(tipo == T_pDOBRO) {
                fprintf(s, "  fcmp d0, 0.0\n");
                fprintf(s, "  cset w0, ne\n");
                tipo = T_pBOOL;
            } else {
                fatal("[fator] operando do operador ! deve ser inteiro, booleano ou conversível para booleano");
            }
        }
        // aplica o NÃO logico
        fprintf(s, "  cmp w0, 0\n");
        fprintf(s, "  cset w0, eq\n"); // 1 se igual a 0(não)
        
        return T_pBOOL;
    }
    if(L.tk.tipo == T_ARROBA) {
        proximoToken();
        if(L.tk.tipo != T_ID) fatal("[fator] @ espera identificador");
        Variavel* var = buscar_var(L.tk.lex, escopo);
        if(!var) fatal("[fator] variável não encontrada");
        
        if(var->eh_ponteiro)
        fprintf(s, "  ldr x0, [x29, %d]\n", var->pos); // carrega o VALOR do ponteiro
        else if(var->eh_parametro && var->eh_array)
        fprintf(s, "  ldr x0, [x29, %d]\n", var->pos);
        else
        fprintf(s, "  add x0, x29, %d\n", var->pos);   // endereço de variavel normal
        
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
    else if(L.tk.tipo == T_BOOL) return tratar_bool(s);
    else {
        fatal("[fator] fator inválido");
        return T_pINT;
    }
}

TipoToken termo(FILE* s, int escopo) {
    TipoToken tipo_esq = fator(s, escopo);
    
    while(L.tk.tipo == T_VEZES || L.tk.tipo == T_DIV ||
          L.tk.tipo == T_PORCEN || L.tk.tipo == T_MENOR_MENOR ||
          L.tk.tipo == T_MAIOR_MAIOR || L.tk.tipo == T_TAMBEM) {
        
        TipoToken op = L.tk.tipo;
        
        char reg_tipo;
        
        if(tipo_esq == T_pFLU) reg_tipo = 's';
        else if(tipo_esq == T_pDOBRO) reg_tipo = 'd';
        else if(tipo_esq == T_pLONGO || tipo_esq == T_PONTEIRO) reg_tipo = 'x';
        else reg_tipo = 'w';
        
        int reg_temp_esq = alocar_reg(reg_tipo);
        
        if(reg_temp_esq >= 0) {
            // registrador
            if(debug_o) printf("[termo]: usando registrador %c%d para operando\n", reg_tipo, reg_temp_esq);
            
            // salva no registrador
            if(reg_tipo == 's') {
                fprintf(s, "  fmov s%d, s0  // salva em reg\n", reg_temp_esq);
            } else if(reg_tipo == 'd') {
                fprintf(s, "  fmov d%d, d0  // salva em reg\n", reg_temp_esq);
            } else if(reg_tipo == 'x') {
                fprintf(s, "  mov x%d, x0  // salva em reg\n", reg_temp_esq);
            } else {
                fprintf(s, "  mov w%d, w0  // salva em reg\n", reg_temp_esq);
            }
        } else {
            // pilha
            if(debug_o) printf("[termo]: pilha usada para operando\n");
            
            if(tipo_esq == T_pFLU) fprintf(s, "  str s0, [sp, -16]!\n");
            else if(tipo_esq == T_pDOBRO) fprintf(s, "  str d0, [sp, -16]!\n");
            else if(tipo_esq == T_pLONGO || tipo_esq == T_PONTEIRO) fprintf(s, "  str x0, [sp, -16]!\n");
            else fprintf(s, "  str w0, [sp, -16]!\n");
        }
        proximoToken();
        TipoToken tipo_dir = fator(s, escopo);
        TipoToken tipo_resultado = converter_tipos(s, tipo_esq, tipo_dir);
        
        // recupera primeiro operando
        if(reg_temp_esq >= 0) {
            if(reg_tipo == 's') {
                fprintf(s, "  fmov s1, s%d  // restaura do reg\n", reg_temp_esq);
            } else if(reg_tipo == 'd') {
                fprintf(s, "  fmov d1, d%d  // restaura do reg\n", reg_temp_esq);
            } else if(reg_tipo == 'x') {
                fprintf(s, "  mov x1, x%d  // restaura do reg\n", reg_temp_esq);
            } else {
                fprintf(s, "  mov w1, w%d  // restaura do reg\n", reg_temp_esq);
            }
            liberar_reg(reg_tipo, reg_temp_esq);
        } else {
            if(tipo_esq == T_pFLU) fprintf(s, "  ldr s1, [sp], 16\n");
            else if(tipo_esq == T_pDOBRO) fprintf(s, "  ldr d1, [sp], 16\n");
            else if(tipo_esq == T_pLONGO || tipo_esq == T_PONTEIRO) fprintf(s, "  ldr x1, [sp], 16\n");
            else fprintf(s, "  ldr w1, [sp], 16\n");
        }
        gerar_operacao(s, op, tipo_resultado);
        tipo_esq = tipo_resultado;
    }
    return tipo_esq;
}

TipoToken expressao(FILE* s, int escopo) {
    if(L.tk.tipo == T_ID) {
        char id[32];
        strcpy(id, L.tk.lex);
        if(debug_o) printf("[expressao]: recebeu ID: %s\n", id);
        // salva a posição atual
        size_t pos_salvo = L.pos;
        int linha_salvo = L.linha_atual;
        int coluna_salvo = L.coluna_atual;
        Token tk_salvo = L.tk;
        // avança pra ver se tem "="
        proximoToken();
        if(L.tk.tipo == T_IGUAL) {
            if(debug_o) printf("[expressao]: achou atribuição no escopo: %d\n", escopo);
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
    
    // processa primeiro nível: comparações e operadores aritmeticos
    TipoToken tipo = termo(s, escopo);
    
    // processa operadores de comparação
    if(L.tk.tipo == T_IGUAL_IGUAL || L.tk.tipo == T_DIFERENTE || 
        L.tk.tipo == T_MAIOR || L.tk.tipo == T_MENOR ||
        L.tk.tipo == T_MAIOR_IGUAL || L.tk.tipo == T_MENOR_IGUAL) {
        
        TipoToken op = L.tk.tipo;
        proximoToken();
        
        // salva o primeiro operando
        char reg_tipo_esq;
        int reg_temp_esq = -1;
        
        if(tipo == T_pFLU) {
            reg_tipo_esq = 's';
            reg_temp_esq = alocar_reg('s');
            
            if(reg_temp_esq >= 0) {
                if(debug_o) printf("[expressao]: usando registrador %c%d\n", reg_tipo_esq, reg_temp_esq);
                fprintf(s, "  fmov s%d, s0\n", reg_temp_esq);
            } else {
                fprintf(s, "  str s0, [sp, -16]!\n");
            }
        } else if(tipo == T_pDOBRO) {
            reg_tipo_esq = 'd';
            reg_temp_esq = alocar_reg('d');
            
            if(reg_temp_esq >= 0) {
                if(debug_o) printf("[expressao]: usando registrador %c%d\n", reg_tipo_esq, reg_temp_esq);
                fprintf(s, "  fmov d%d, d0\n", reg_temp_esq);
            } else {
                fprintf(s, "  str d0, [sp, -16]!\n");
            }
        } else if(tipo == T_PONTEIRO || tipo == T_pLONGO) {
            reg_tipo_esq = 'x';
            reg_temp_esq = alocar_reg('x');
            
            if(reg_temp_esq >= 0) {
                if(debug_o) printf("[expressao]: usando registrador %c%d\n", reg_tipo_esq, reg_temp_esq);
                fprintf(s, "  mov x%d, x0\n", reg_temp_esq);
            } else {
                fprintf(s, "  str x0, [sp, -16]!\n");
            }
        } else {
            reg_tipo_esq = 'w';
            reg_temp_esq = alocar_reg('x'); // w é tratado como x
            if(reg_temp_esq >= 0) {
                if(debug_o) printf("[expressao]: usando registrador %c%d\n", reg_tipo_esq, reg_temp_esq);
                fprintf(s, "  mov w%d, w0\n", reg_temp_esq);
            } else {
                fprintf(s, "  str w0, [sp, -16]!\n");
            }
        }
        TipoToken tipo_dir = termo(s, escopo);
        
        // recupera o primeiro operando
        if(reg_temp_esq >= 0) {
            if(tipo == T_pFLU) {
                fprintf(s, "  fmov s1, s%d\n", reg_temp_esq);
                if(reg_temp_esq >= 8) liberar_reg('s', reg_temp_esq);
            } else if(tipo == T_pDOBRO) {
                fprintf(s, "  fmov d1, d%d\n", reg_temp_esq);
                if(reg_temp_esq >= 8) liberar_reg('d', reg_temp_esq);
            } else if(tipo == T_PONTEIRO || tipo == T_pLONGO) {
                fprintf(s, "  mov x1, x%d\n", reg_temp_esq);
                if(reg_temp_esq >= 8) liberar_reg('x', reg_temp_esq);
            } else {
                fprintf(s, "  mov w1, w%d\n", reg_temp_esq);
                if(reg_temp_esq >= 8) liberar_reg('x', reg_temp_esq);
            }
        } else {
            // usa pilha
            if(tipo == T_pFLU) {
                fprintf(s, "  ldr s1, [sp], 16\n");
            } else if(tipo == T_pDOBRO) {
                fprintf(s, "  ldr d1, [sp], 16\n");
            } else if(tipo == T_PONTEIRO || tipo == T_pLONGO) {
                fprintf(s, "  ldr x1, [sp], 16\n");
            } else {
                fprintf(s, "  ldr w1, [sp], 16\n");
            }
        }
        tipo = converter_tipos(s, tipo, tipo_dir);
        gerar_comparacao(s, op, tipo);
        tipo = T_pBOOL;
    }
    // processa operadores logicos
    while(L.tk.tipo == T_TAMBEM_TAMBEM || L.tk.tipo == T_OU_OU) {
        TipoToken op = L.tk.tipo;
        proximoToken();
        
        if(debug_o) printf("[expressao]: processando operador lógico %s\n", token_str(op));
        
        // salva primeiro operando(ja deve ser booleano da comparação)
        fprintf(s, "  str w0, [sp, -16]!\n");
        // processa segundo operando(que pode ser outra expressao completa)
        TipoToken tipo_dir = expressao(s, escopo); // chama recursivamente
        
        // pra operadores logicos, precisa que ambos sejam booleanos
        if(tipo_dir != T_pINT && tipo_dir != T_pBOOL) {
            if(tipo_dir == T_pFLU) {
                fprintf(s, "  fcmp s0, 0.0\n");
                fprintf(s, "  cset w0, ne\n");
                tipo_dir = T_pBOOL;
            } else if(tipo_dir == T_pDOBRO) {
                fprintf(s, "  fcmp d0, 0.0\n");
                fprintf(s, "  cset w0, ne\n");
                tipo_dir = T_pBOOL;
            } else {
                fatal("[expressao] operando do operador lógico deve ser inteiro ou booleano");
            }
        }
        // recupera primeiro operando
        fprintf(s, "  ldr w1, [sp], 16\n");
        // primeiro operando tambem deve ser booleano
        if(tipo != T_pINT && tipo != T_pBOOL) {
            if(tipo == T_pFLU) {
                // ja deveria ter sido convertido
                fatal("[expressao] primeiro operando do operador lógico não é booleano");
            }
        }
        // aplica operador logico
        if(op == T_TAMBEM_TAMBEM) {
            fprintf(s, "  cmp w1, 0\n");
            fprintf(s, "  cset w1, ne\n");
            fprintf(s, "  cmp w0, 0\n");
            fprintf(s, "  cset w0, ne\n");
            fprintf(s, "  and w0, w1, w0\n");
        } else { // T_OU_OU
            fprintf(s, "  cmp w1, 0\n");
            fprintf(s, "  cset w1, ne\n");
            fprintf(s, "  cmp w0, 0\n");
            fprintf(s, "  cset w0, ne\n");
            fprintf(s, "  orr w0, w1, w0\n");
        }
        tipo = T_pBOOL;
    }
    // processa operadores aritmeticos restantes
    while(L.tk.tipo == T_MAIS || L.tk.tipo == T_MENOS ||
          L.tk.tipo == T_MENOR_MENOR || L.tk.tipo == T_OU) {
        if(debug_o) printf("[expressao]: achou operação: %s\n", token_str(L.tk.tipo));
        TipoToken op = L.tk.tipo;
        proximoToken();
        // salva o primeiro resultado:
        if(tipo == T_pFLU) fprintf(s, "  str s0, [sp, -16]!\n");
        else if(tipo == T_pDOBRO) fprintf(s, "  str d0, [sp, -16]!\n");
        else if(tipo == T_PONTEIRO || tipo == T_pLONGO) fprintf(s, "  str x0, [sp, -16]!\n");
        else fprintf(s, "  str w0, [sp, -16]!\n");
        
        TipoToken tipo_dir = termo(s, escopo);
        
        // recupera o primeiro resultado:
        if(tipo == T_pFLU) fprintf(s, "  ldr s1, [sp], 16\n");
        else if(tipo == T_pDOBRO) fprintf(s, "  ldr d1, [sp], 16\n");
        else if(tipo == T_PONTEIRO || tipo == T_pLONGO) fprintf(s, "  ldr x1, [sp], 16\n");
        else fprintf(s, "  ldr w1, [sp], 16\n");
        
        tipo = converter_tipos(s, tipo, tipo_dir);
        gerar_operacao(s, op, tipo);
    }
    // operador ternario: condição ? valor_verdadeiro : valor_falso
    if(L.tk.tipo == T_INTERROGACAO) {
        if(debug_o) printf("[expressao]: processando operador ternário\n");
        proximoToken();
        
        // a condição ja ta em w0/x0
        // gera labels únicos
        static int ternario_id = 0;
        int id_atual = ternario_id++;
        
        // testa a condição
        if(tipo == T_pFLU) fprintf(s, "  fcmp s0, 0.0\n");
        else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d0, 0.0\n");
        else if(tipo == T_pLONGO || tipo == T_PONTEIRO) fprintf(s, "  cmp x0, 0\n");
        else fprintf(s, "  cmp w0, 0\n");
        fprintf(s, "  beq .ternario_falso_%d\n", id_atual);
        
        // caso verdadeiro
        TipoToken tipo_verdadeiro = expressao(s, escopo);
        fprintf(s, "  b .ternario_fim_%d\n", id_atual);
        
        // verifica o :
        excessao(T_DOIS_PONTOS);
        
        // caso falso
        fprintf(s, ".ternario_falso_%d:\n", id_atual);
        TipoToken tipo_falso = expressao(s, escopo);
        
        fprintf(s, ".ternario_fim_%d:\n", id_atual);
        
        // converte tipos se necessario
        tipo = converter_tipos(s, tipo_verdadeiro, tipo_falso);
    }
    return tipo;
}

void iniciar(FILE* s) {
    while(L.tk.tipo != T_FIM) {
        if(L.tk.tipo == T_INCLUIR) {
            int pos = 0;
            verificar_stmt(s, &pos, 0);
        } else if(L.tk.tipo == T_DEF) verificar_def();
        else if(L.tk.tipo == T_ESPACO) verificar_espaco(s);
        else if(L.tk.tipo == T_GLOBAL) verificar_global(s);
        else if(L.tk.tipo == T_ALINHAR) verificar_alinhar();
        else verificar_fn(s);
    }
    gerar_consts(s);
    gerar_texs(s);
    gerar_globais(s);
}