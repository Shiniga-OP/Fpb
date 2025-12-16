/*
* [NÍVEL OTIMIZADOR]: 2.
* [FUNÇÃO]: junção de tarefas de texto.
*/
#ifndef OTIMI2_H
#define OTIMI2_H

#include "otimi1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINHA 1024

static bool debug2 = false;

void otimizarO2(const char* arquivo_asm) {
    FILE *entrada = fopen(arquivo_asm, "r");
    if(!entrada) {
        perror("Erro ao abrir arquivo");
        return;
    }
    // le todo o conteudo
    char **linhas = malloc(10000 * sizeof(char*));
    int num_linhas = 0;
    char buffer[MAX_LINHA];
    
    while(fgets(buffer, MAX_LINHA, entrada)) {
        linhas[num_linhas] = malloc(strlen(buffer) + 1);
        strcpy(linhas[num_linhas], buffer);
        num_linhas++;
    }
    fclose(entrada);
    
    typedef struct {
        char nome[50];
        char conteudo[1024];
        int usada;
    } TexInfo;
    
    TexInfo texs[1000];
    int num_texs = 0;
    
    for(int i = 0; i < num_linhas; i++) {
        char *partes = linhas[i];
        while(isspace(*partes)) partes++;
        
        if(strstr(partes, ".tex_") && strstr(partes, ".asciz") && strchr(partes, ':')) {
            char *nome_inicio = strstr(partes, ".tex_");
            char *nome_fim = strchr(nome_inicio, ':');
            if(nome_fim) {
                strncpy(texs[num_texs].nome, nome_inicio, nome_fim - nome_inicio);
                texs[num_texs].nome[nome_fim - nome_inicio] = '\0';
                texs[num_texs].usada = 1;
                
                // encontra a primeira aspa não escapada
                char *aspas1 = partes;
                while(*aspas1 && *aspas1 != '"') aspas1++;
                
                if(*aspas1 == '"') {
                    // encontra a segunda aspa não escapada
                    char *aspas2 = aspas1 + 1;
                    int escape = 0;
                    
                    while(*aspas2 && !(*aspas2 == '"' && !escape)) {
                        if(*aspas2 == '\\' && !escape) {
                            escape = 1;
                        } else {
                            escape = 0;
                        }
                        aspas2++;
                    }
                    
                    if(*aspas2 == '"') {
                        // copia o conteudo entre aspas(incluindo escapes)
                        int len = aspas2 - aspas1 - 1;
                        if(len < 1024) {
                            strncpy(texs[num_texs].conteudo, aspas1 + 1, len);
                            texs[num_texs].conteudo[len] = '\0';
                            num_texs++;
                        }
                    }
                }
            }
        }
    }
    // marca textos referenciados no código
    for(int i = 0; i < num_linhas; i++) {
        if(linhas[i][0] != '\0') {
            char *partes = linhas[i];
            while(isspace(*partes)) partes++;
            
            for(int s = 0; s < num_texs; s++) {
                char padrao_ref[100];
                snprintf(padrao_ref, sizeof(padrao_ref), "ldr x0, = %s", texs[s].nome);
                
                if(strstr(partes, padrao_ref)) {
                    texs[s].usada = 1;
                }
            }
        }
    }
    // combina chamadas sequenciais
    int combinacoes_feitas = 0;
    
    for(int i = 0; i < num_linhas - 3; i++) {
        char *l1 = linhas[i];
        char *t1 = l1;
        while(isspace(*t1)) t1++;
        char *l2 = linhas[i+1];
        char *t2 = l2;
        while(isspace(*t2)) t2++;
        char *l3 = linhas[i+2];
        char *t3 = l3;
        while(isspace(*t3)) t3++;
        char *l4 = linhas[i+3];
        char *t4 = l4;
        while(isspace(*t4)) t4++;
        
        if(strstr(t1, "ldr x0, = .tex_") && 
            strstr(t2, "bl _escrever_tex") &&
            strstr(t3, "ldr x0, = .tex_") && 
            strstr(t4, "bl _escrever_tex")) {
            
            int pode_combinar = 1;
            if(strstr(t2, "bl _escrever_tex") && strlen(t2) > 20) pode_combinar = 0;
            if(strstr(t3, "ldr x0, = .tex_") && strlen(t3) > 20) pode_combinar = 0;
            
            if(pode_combinar) {
                char tex1[50], tex2[50];
                sscanf(t1, "ldr x0, = %s", tex1);
                sscanf(t3, "ldr x0, = %s", tex2);
                
                char *fim1 = strchr(tex1, '\n'); if (fim1) *fim1 = '\0';
                char *fim2 = strchr(tex2, '\n'); if (fim2) *fim2 = '\0';
                
                char conteudo1[1024] = "", conteudo2[1024] = "";
                for(int s = 0; s < num_texs; s++) {
                    if(strcmp(texs[s].nome, tex1) == 0) strcpy(conteudo1, texs[s].conteudo);
                    if(strcmp(texs[s].nome, tex2) == 0) strcpy(conteudo2, texs[s].conteudo);
                }
                if(conteudo1[0] && conteudo2[0]) {
                    if(debug2) printf("Combinando texs consecutivas: %s + %s\n", tex1, tex2);
                    
                    char novo_tex[2048];
                    snprintf(novo_tex, sizeof(novo_tex), "%s%s", conteudo1, conteudo2);
                    // substitui as 4 linhas
                    free(linhas[i]);
                    free(linhas[i+1]);
                    free(linhas[i+2]); 
                    free(linhas[i+3]);
                    
                    linhas[i] = malloc(100);
                    snprintf(linhas[i], 100, "  ldr x0, = .tex_comb_%d\n", combinacoes_feitas);
                    
                    linhas[i+1] = malloc(100);
                    strcpy(linhas[i+1], "  bl _escrever_tex\n");
                    
                    linhas[i+2] = malloc(1); linhas[i+2][0] = '\0';
                    linhas[i+3] = malloc(1); linhas[i+3][0] = '\0';
                    
                    // verifica se os textos ainda são referenciados em outras partes
                    int tex1_usada = 0;
                    int tex2_usada = 0;
                    
                    for(int j = 0; j < num_linhas; j++) {
                        if(j != i && j != (i+2) && linhas[j][0] != '\0') {
                            char *partes_j = linhas[j];
                            while(isspace(*partes_j)) partes_j++;
                            
                            char ref1[100], ref2[100];
                            snprintf(ref1, sizeof(ref1), "ldr x0, = %s", tex1);
                            snprintf(ref2, sizeof(ref2), "ldr x0, = %s", tex2);
                            
                            if(strstr(partes_j, ref1)) tex1_usada = 1;
                            if(strstr(partes_j, ref2)) tex2_usada = 1;
                        }
                    }
                    // so marca como não usado se não for referenciado em outro lugar
                    for(int s = 0; s < num_texs; s++) {
                        if(strcmp(texs[s].nome, tex1) == 0 && !tex1_usada) {
                            texs[s].usada = 0;
                            if(debug2) printf("Marcando %s como não usada\n", tex1);
                        }
                        if(strcmp(texs[s].nome, tex2) == 0 && !tex2_usada) {
                            texs[s].usada = 0;
                            if(debug2) printf("Marcando %s como não usada\n", tex2);
                        }
                    }
                    // adiciona nova texto
                    strcpy(texs[num_texs].nome, ".tex_comb_");
                    sprintf(texs[num_texs].nome + 10, "%d", combinacoes_feitas);
                    strcpy(texs[num_texs].conteudo, novo_tex);
                    texs[num_texs].usada = 1;
                    num_texs++;
                    combinacoes_feitas++;
                    i += 3;
                }
            }
        }
    }
    // escreve arquivo preservando todo o .rodata exceto textos .tex_X não usadas
    FILE *saida = fopen(arquivo_asm, "w");
    if(!saida) {
        perror("Erro ao abrir arquivo para escrita");
        return;
    }
    int em_rodata = 0;
    
    for(int i = 0; i < num_linhas; i++) {
        char *linha = linhas[i];
        char *partes = linha;
        while (isspace(*partes)) partes++;
        // entrando na seção .rodata
        if(strstr(partes, ".section .rodata")) {
            em_rodata = 1;
            fputs(linha, saida);
        }
        // saindo da seção .rodata
        else if(em_rodata && strstr(partes, ".section .text")) {
            em_rodata = 0;
            fputs(linha, saida);
        }
        // dentro do .rodata: filtra apenas .tex_X usados
        else if(em_rodata && strstr(partes, ".tex_") && strstr(partes, ".asciz")) {
            // extrai nome do texto
            char nome_tex[50];
            char *nome_inicio = strstr(partes, ".tex_");
            char *nome_fim = strchr(nome_inicio, ':');
            if(nome_fim) {
                strncpy(nome_tex, nome_inicio, nome_fim - nome_inicio);
                nome_tex[nome_fim - nome_inicio] = '\0';
                
                // verifica se esse .tex_X ta usado
                int esta_usada = 0;
                for (int s = 0; s < num_texs; s++) {
                    if (strcmp(texs[s].nome, nome_tex) == 0 && texs[s].usada) {
                        esta_usada = 1;
                        break;
                    }
                }
                if(esta_usada) fputs(linha, saida);
                // se não ta usado, não escreve(remove o texto)
            } else {
                // não é um texto .tex_X formatado corretamente, escreve normalmente
                fputs(linha, saida);
            }
        }
        // fora do .rodata ou outras linhas do .rodata: escreve normalmente
        else if(linha[0] != '\0') {
            fputs(linha, saida);
        }
        
        free(linhas[i]);
    }
    free(linhas);
    // adiciona textos combinadas ao final do arquivo(nova seção .rodata)
    fprintf(saida, "\n.section .rodata\n.align 2\n");
    for(int i = 0; i < num_texs; i++) {
        if(strstr(texs[i].nome, ".tex_comb_") && texs[i].usada) {
            fprintf(saida, "%s: .asciz \"%s\"\n", texs[i].nome, texs[i].conteudo);
            if(debug2) printf("Incluindo texto combinado: %s\n", texs[i].nome);
        }
    }
    fclose(saida);
    if(debug2) printf("Otimização 2 concluída: %d combinações de texs feitas\n", combinacoes_feitas);
    otimizarO1(arquivo_asm);
}
#endif