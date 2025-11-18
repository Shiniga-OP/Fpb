/*
* [FUNÇÃO]: eliminação de codigo e dados mortos
* [DESCRIÇÃO]: analisa o assembly gerado e remove funções e blocos de dados não utilizados,
* recorrendo apenas a analise de dependencia recursiva(grafo de chamadas).
*/
#ifndef OTIMI1_H
#define OTIMI1_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINHAS_ASM 10000
#define MAX_TAM_LINHA 256
#define MAX_LABELS 512

// tipos de labels pra otimização
#define TIPO_CODIGO 1 // label de função(inicio, _escrever_flu)
#define TIPO_DADOS 2 // label de dados(const_0, .tex_0, buffers)
#define TIPO_OUTROS 0 // labels internos ou _start(.B1, 1:, _start)

typedef struct {
    char nome[64];
    int linha_inicio;
    int linha_fim;
    int usado; // 0 = morto, 1 = vivo
    int tipo; // TIPO_CODIGO, TIPO_DADOS ou TIPO_OUTROS
} LabelInfo;

static char linhas[MAX_LINHAS_ASM][MAX_TAM_LINHA];
static int total_linhas = 0;
static LabelInfo labels[MAX_LABELS];
static int total_labels = 0;

// busca indice do label pelo nome
int buscar_label_idc(const char* nome) {
    for(int i=0; i<total_labels; i++) {
        if(strcmp(labels[i].nome, nome) == 0) return i;
    }
    return -1;
}
// verifica se uma linha é uma chamada de função(bl)
// retorna o nome da função chamada em buffer_nome
int obter_chamada(const char* linha, char* buffer_nome) {
    char* p = strstr(linha, "bl ");
    if(p) {
        p += 3;
        while(*p == ' ') p++;
        int i = 0;
        while(*p && *p != '\n' && *p != ' ' && *p != '/') {
            buffer_nome[i++] = *p++;
        }
        buffer_nome[i] = '\0';
        return 1;
    }
    return 0;
}
// verifica se uma linha referencia um label de dados(ldr, adr)
// retorna o nome do label referenciado em buffer_nome
int obter_referencia_de_dados(const char* linha, char* buffer_nome) {
    const char* instr[] = {"ldr ", "adr ", "bl ", "b "}; // instruções comuns que referenciam labels
    int num_instr = sizeof(instr) / sizeof(instr[0]);
    for(int k=0; k<num_instr; k++) {
        const char* p = strstr(linha, instr[k]);
        if(p) {
            char* ref_inicio = NULL;
            // caso ldr x0, = label(referencia a dados/constantes)
            char* equals_sign = strchr(p, '=');
            if(equals_sign) {
                ref_inicio = equals_sign + 1;
            } 
            // caso adr x1, 8f(referencia a labels no codigo)
            else {
                // procura o ultimo token(o label)
                char* ultimo_espaco = strrchr(p, ' '); 
                if(ultimo_espaco) {
                   ref_inicio = ultimo_espaco + 1;
                }
            }
            if(!ref_inicio) continue;
            // limpa espaços e caracteres de referência propagacao/retropropagacao
            while(*ref_inicio == ' ' || *ref_inicio == 'f' || *ref_inicio == 'b') ref_inicio++; 

            int i = 0;
            // copia o nome do label ate um espaço, virgula, ou nova linha
            while(*ref_inicio && *ref_inicio != '\n' && *ref_inicio != ' ' && *ref_inicio != ',' && i < 63) {
                buffer_nome[i++] = *ref_inicio++;
            }
            buffer_nome[i] = '\0';
            // remove "f" ou "b" se for o ultimo caractere (e.g., de 8f -> 8)
            if(i > 0 && (buffer_nome[i-1] == 'f' || buffer_nome[i-1] == 'b')) buffer_nome[i-1] = '\0';

            // garante que o nome não é só um registro(x0, x1)
            if(i > 1 && (buffer_nome[0] == 'x' || buffer_nome[0] == 'w' || buffer_nome[0] == 's')) return 0;
            if(i == 0) return 0; // nome vazio

            return 1;
        }
    }
    return 0;
}
// marca recursivamente funções(TIPO_CODIGO) como usadas
void marcar_codigo_usado(int idc_label) {
    if(idc_label < 0 || idc_label >= total_labels || labels[idc_label].tipo != TIPO_CODIGO) return;
    if(labels[idc_label].usado) return; // ja visitado

    labels[idc_label].usado = 1;
    // percorre o corpo da função procurando chamadas(bl)
    for(int i = labels[idc_label].linha_inicio; i <= labels[idc_label].linha_fim; i++) {
        char chamada[64];
        if(obter_chamada(linhas[i], chamada)) {
            // se encontrar uma chamada, marca a função alvo
            int alvo = buscar_label_idc(chamada);
            if(alvo != -1 && labels[alvo].tipo == TIPO_CODIGO) {
                marcar_codigo_usado(alvo);
            }
        }
    }
}
// rastreia o codigo vivo e marca os labels de dados(TIPO_DADOS) que são referenciados
void rastrear_ref_dados() {
    // percorre todas as linhas em busca de referencias nos blocos de codigo marcados como vivos
    for(int i = 0; i < total_linhas; i++) {
        // encontra a qual label pertence a linha(otimização: so verifica se pertence a um bloco CODIGO usado)
        int label_dono_idc = -1;
        for(int j = 0; j < total_labels; j++) {
            if(labels[j].tipo == TIPO_CODIGO && labels[j].usado) {
                if(i >= labels[j].linha_inicio && i <= labels[j].linha_fim) {
                    label_dono_idc = j;
                    break;
                }
            }
        }
        // se a linha ta em um bloco de codigo vivo
        if(label_dono_idc != -1) {
            char referencia[64];
            if(obter_referencia_de_dados(linhas[i], referencia)) {
                int alvo = buscar_label_idc(referencia);
                if(alvo != -1 && labels[alvo].tipo == TIPO_DADOS) {
                    labels[alvo].usado = 1; // marca o bloco de dados como vivo
                }
            }
        }
    }
}

void otimizarO1(const char* arquivo_asm) {
    printf("[OTIMIZADOR]: Iniciando analise O1 em %s...\n", arquivo_asm);

    FILE* f = fopen(arquivo_asm, "r");
    if(!f) {
        printf("[OTIMIZADOR]: Erro ao abrir arquivo ASM\n");
        return;
    }
    // carregar arquivo pra memoria
    total_linhas = 0;
    while(fgets(linhas[total_linhas], MAX_TAM_LINHA, f) && total_linhas < MAX_LINHAS_ASM) {
        total_linhas++;
    }
    fclose(f);
    // mapea labels(funções e dados)
    total_labels = 0;
    int label_atual_idc = -1;
    char secao_atual[16] = ""; // pra rastrear .text, .rodata, .data

    for(int i = 0; i < total_linhas; i++) {
        char limpa[MAX_TAM_LINHA];
        strcpy(limpa, linhas[i]);
        if(strchr(limpa, '\n')) *strchr(limpa, '\n') = 0;
        // rastrea seção
        if(strstr(limpa, ".section .text")) { 
            if(label_atual_idc != -1) labels[label_atual_idc].linha_fim = i - 1; label_atual_idc = -1;
            strcpy(secao_atual, ".text"); 
            continue; 
        }
        if(strstr(limpa, ".section .rodata")) { 
            if(label_atual_idc != -1) labels[label_atual_idc].linha_fim = i - 1; label_atual_idc = -1;
            strcpy(secao_atual, ".rodata"); 
            continue; 
        }
        if(strstr(limpa, ".section .data")) { 
            if(label_atual_idc != -1) labels[label_atual_idc].linha_fim = i - 1; label_atual_idc = -1;
            strcpy(secao_atual, ".data"); 
            continue; 
        }
        // detecta Label(termina com ":")
        int tam = strlen(limpa);
        if(tam > 0 && limpa[tam-1] == ':') {
            
            char nome[64];
            strncpy(nome, limpa, tam-1);
            nome[tam-1] = '\0';

            // limpeza: remove espaços/tabs no inicio
            char* p = nome;
            while (*p == ' ' || *p == '\t') p++;
            char nome_limpo[64];
            strcpy(nome_limpo, p);

            int num_label = isdigit(nome_limpo[0]);
            int tipo_label = TIPO_OUTROS;
            // determinação do tipo de label
            // prioridade A: funções criticas e de biblioteca(sempre CODIGO)
            if(strcmp(nome_limpo, "inicio") == 0) {
                tipo_label = TIPO_CODIGO;
            } 
            // prioridade B: labels de codigi de alto nivel(em .text, não internos, não numericos)
            else if(strcmp(secao_atual, ".text") == 0) {
                if(nome_limpo[0] != '.' && strcmp(nome_limpo, "_start") != 0 && !num_label) {
                    tipo_label = TIPO_CODIGO;
                }
            } 
            // prioridade C: labels de dados(em .rodata/.data, não internos, e não numericos p/ evitar colisão com labels internos)
            else if(strcmp(secao_atual, ".rodata") == 0 || strcmp(secao_atual, ".data") == 0) {
                 if(nome_limpo[0] != '.' && !num_label) { 
                    tipo_label = TIPO_DADOS;
                }
            }
            // logica de fechamento de bloco de antes
            // o bloco anterior so deve ser fechado se o novo label for: um label rastreavel(TIPO_CODIGO, TIPO_DADOS) ou um label de controle interno(.B1, .epilogo, etc)
            int deve_fechar_antes = (tipo_label != TIPO_OUTROS || nome_limpo[0] == '.');

            if(label_atual_idc != -1 && deve_fechar_antes) {
                labels[label_atual_idc].linha_fim = i - 1;
            }
            // logica de rastreamento do novo label:
            if(tipo_label != TIPO_OUTROS) {
                LabelInfo* l = &labels[total_labels];
                strcpy(l->nome, nome_limpo);
                l->linha_inicio = i;
                l->linha_fim = total_linhas - 1; 
                l->usado = 0;
                l->tipo = tipo_label;
                
                label_atual_idc = total_labels;
                total_labels++;
            } else if(deve_fechar_antes) {
                // se for um label interno de controle(.B1), fecha o anterior e desativa o rastreamento
                label_atual_idc = -1;
            }
            // se for um label numerico/local(TIPO_OUTROS e não deve_fechar_antes),
            // ele não é adicionado, e o rastreador(label_atual_idc) permanece no bloco anterior
        }
    }
    // analise de dependencia
    // eliminação de codigo morto
    int idc_inicio = buscar_label_idc("inicio");
    if(idc_inicio != -1) {
        // confia 100% na analise recursiva
        marcar_codigo_usado(idc_inicio);
    }
    // eliminação de dados mortos
    printf("[OTIMIZADOR]: Rastreando referencias de dados...\n");
    rastrear_ref_dados();

    // reescrever arquivo
    f = fopen(arquivo_asm, "w");
    if(!f) return;

    int linhas_removidas = 0;
    int dados_rm_conta = 0;
    int codigo_rm_conta = 0;

    for(int i = 0; i < total_linhas; i++) {
        int pular_linha = 0;
        // verifica se a linha atual pertence a um bloco morta(codigo ou Dados)
        for(int j = 0; j < total_labels; j++) {
            if(labels[j].tipo != TIPO_OUTROS && !labels[j].usado) {
                if(i >= labels[j].linha_inicio && i <= labels[j].linha_fim) {
                    pular_linha = 1;

                    if(labels[j].tipo == TIPO_CODIGO) codigo_rm_conta++;
                    if(labels[j].tipo == TIPO_DADOS) dados_rm_conta++;
                    break;
                }
            }
        }
        if(!pular_linha) fprintf(f, "%s",linhas[i]);
        else linhas_removidas++;
    }
    fclose(f);
    
    if(linhas_removidas > 0) {
        printf("[OTIMIZADOR]: %d linhas de código/dados mortos removidas.\n", linhas_removidas);
        printf("  - %d linhas de código morto (Funções).\n", codigo_rm_conta);
        printf("  - %d linhas de dados mortos (Buffers/Constantes).\n", dados_rm_conta);
        
        printf("[OTIMIZADOR]: Funções de Código removidas: ");
        for(int i=0; i<total_labels; i++) {
            if(labels[i].tipo == TIPO_CODIGO && !labels[i].usado) printf("%s ", labels[i].nome);
        }
        printf("\n");
        
        printf("[OTIMIZADOR]: Labels de Dados removidos: ");
         for(int i=0; i<total_labels; i++) {
            if(labels[i].tipo == TIPO_DADOS && !labels[i].usado) printf("%s ", labels[i].nome);
        }
        printf("\n");
    } else {
        printf("[OTIMIZADOR]: Nenhuma otimização necessária.\n");
    }
}
#endif