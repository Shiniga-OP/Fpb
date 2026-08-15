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
    // combina chamadas sequenciais(N de uma vez)
    int combinacoes_feitas = 0;
    
    for(int i = 0; i < num_linhas - 1; i++) {
        char *t1 = linhas[i];
        while(isspace(*t1)) t1++;
        char *t2 = linhas[i+1];
        while(isspace(*t2)) t2++;
        
        // verifica se começa um bloco ldr+bl
        if(!strstr(t1, "ldr x0, = .tex_") || !strstr(t2, "bl _escrever_tex")) continue;
        
        // encontra quantas chamadas consecutivas existem a partir de i
        int num_seq = 0;
        int indices[500]; // indices das linhas ldr de cada chamada
        int j = i;
        while(j < num_linhas - 1) {
            char *ta = linhas[j];
            while(isspace(*ta)) ta++;
            char *tb = linhas[j+1];
            while(isspace(*tb)) tb++;
            if(strstr(ta, "ldr x0, = .tex_") && strstr(tb, "bl _escrever_tex")) {
                indices[num_seq++] = j;
                j += 2;
            } else {
                break;
            }
        }
        // so combina se houver 2 ou mais chamadas seguidas
        if(num_seq < 2) continue;
        
        // coleta conteúdos de todos os textos da sequência
        char novo_tex[8192] = "";
        int ok = 1;
        char nomes[500][50];
        for(int k = 0; k < num_seq; k++) {
            char tex[50];
            char *ta = linhas[indices[k]];
            while(isspace(*ta)) ta++;
            sscanf(ta, "ldr x0, = %s", tex);
            char *fim = strchr(tex, '\n'); if(fim) *fim = '\0';
            strcpy(nomes[k], tex);
            
            char conteudo[1024] = "";
            for(int s = 0; s < num_texs; s++) {
                if(strcmp(texs[s].nome, tex) == 0) {
                    strcpy(conteudo, texs[s].conteudo);
                    break;
                }
            }
            if(!conteudo[0]) { ok = 0; break; }
            if(strlen(novo_tex) + strlen(conteudo) >= sizeof(novo_tex) - 1) { ok = 0; break; }
            strcat(novo_tex, conteudo);
            if(debug2) printf("Combinando tex: %s\n", tex);
        }
        
        if(!ok) continue;
        
        // substitui todas as linhas da sequência
        // primeira chamada vira ldr+bl para o texto combinado
        free(linhas[indices[0]]);
        linhas[indices[0]] = malloc(100);
        snprintf(linhas[indices[0]], 100, "  ldr x0, = .tex_comb_%d\n", combinacoes_feitas);
        free(linhas[indices[0]+1]);
        linhas[indices[0]+1] = malloc(100);
        strcpy(linhas[indices[0]+1], "  bl _escrever_tex\n");
        
        // demais chamadas viram linhas vazias
        for(int k = 1; k < num_seq; k++) {
            free(linhas[indices[k]]);
            linhas[indices[k]] = malloc(1);
            linhas[indices[k]][0] = '\0';
            free(linhas[indices[k]+1]);
            linhas[indices[k]+1] = malloc(1);
            linhas[indices[k]+1][0] = '\0';
        }
        // marca textos originais como não usados(se não referenciados em outro lugar)
        for(int k = 0; k < num_seq; k++) {
            int ainda_usada = 0;
            for(int jj = 0; jj < num_linhas; jj++) {
                if(linhas[jj][0] == '\0') continue;
                char *partes_j = linhas[jj];
                while(isspace(*partes_j)) partes_j++;
                char ref[100];
                snprintf(ref, sizeof(ref), "ldr x0, = %s", nomes[k]);
                if(strstr(partes_j, ref)) { ainda_usada = 1; break; }
            }
            if(!ainda_usada) {
                for(int s = 0; s < num_texs; s++) {
                    if(strcmp(texs[s].nome, nomes[k]) == 0) {
                        texs[s].usada = 0;
                        if(debug2) printf("Marcando %s como não usada\n", nomes[k]);
                    }
                }
            }
        }
        
        // adiciona novo texto combinado
        strcpy(texs[num_texs].nome, ".tex_comb_");
        sprintf(texs[num_texs].nome + 10, "%d", combinacoes_feitas);
        strcpy(texs[num_texs].conteudo, novo_tex);
        texs[num_texs].usada = 1;
        num_texs++;
        combinacoes_feitas++;
        i = j - 1; // pula a sequência inteira
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