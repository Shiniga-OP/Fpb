/*
* [NÍVEL OTIMIZADOR]: 1.
* [FUNÇÃO]: eliminação de codigo e dados mortos + reorganização.
*/
#ifndef OTIMI1_H
#define OTIMI1_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

static bool debug1 = false;

void economiaReg(const char* arquivo_asm) {
    // abre o arquivo gerado anteriormente(ja sem funções mortas)
    FILE *arq = fopen(arquivo_asm, "r");
    if(!arq) return;
    // cria um temporario para a saida otimizada
    char temp_nome[300];
    snprintf(temp_nome, sizeof(temp_nome), "%s.fpbo", arquivo_asm);
    FILE *saida = fopen(temp_nome, "w");
    if(!saida) {
        fclose(arq);
        return;
    }
    char prev_lin[256] = ""; // buffer pra a linha anterior
    char atual_lin[256]; // nuffer pra a linha atual
    
    int instrucoes_rm = 0;
    // Lê o arquivo linha por linha
    while(fgets(atual_lin, sizeof(atual_lin), arq)) {
        // variaveis pra armazenar os registradores detectados
        char reg_codigo[16] = ""; 
        char reg_dst[16] = "";
        int otimizado = 0;
        // verifica se a linha anterior era um armazenamento na pilha
        // padrão: str REG, [sp, -16]!
        // O espaço inicial no sscanf ignora indentação
        if(prev_lin[0] != '\0' && 
            (strstr(prev_lin, "str") != NULL) && 
            (strstr(prev_lin, "[sp, -16]!") != NULL)) {
            // tenta extrair o registrador de origem da linha anterior
            // "str w0, [sp, -16]!" -> extrai "w0"
            sscanf(prev_lin, " str %[^,], [sp, -16]!", reg_codigo);
            // verifica se a linha atual carrega da pilha
            // padrão: ldr REG, [sp], 16
            if(reg_codigo[0] != '\0' && 
                (strstr(atual_lin, "ldr") != NULL) && 
                (strstr(atual_lin, "[sp], 16") != NULL)) {
                // tenta extrair o registrador de destino
                sscanf(atual_lin, " ldr %[^,], [sp], 16", reg_dst);

                if(reg_dst[0] != '\0') {
                    // >>>padrão detectado: push seguido de pop<<<
                    otimizado = 1;
                    instrucoes_rm += 2;
                    // caso 1: o registrador é o mesmo(str w0... ldr w0...)
                    // ação: apenas ignora ambas as linhas(não escreve nada)
                    if(strcmp(reg_codigo, reg_dst) == 0) {
                        if(debug1) printf("  [Otimi1] Removido store/load redundante de %s\n", reg_codigo);
                    } 
                    // caso 2: registradores diferentes(str w0... ldr w1...)
                    // ação: substitui o acesso a memoria por um mov direto
                    else {
                        if(debug1) printf("  [Otimi1] substituido pilha por mov: %s -> %s\n", reg_codigo, reg_dst);
                        // detecta se é ponto flutuante(começa com "s" ou "d") ou inteiro("w" ou "x")
                        if(reg_codigo[0] == 's' || reg_codigo[0] == 'd') {
                            fprintf(saida, "  fmov %s, %s // otimizado(era pilha)\n", reg_dst, reg_codigo);
                        } else {
                            fprintf(saida, "  mov %s, %s // otimizado(era pilha)\n", reg_dst, reg_codigo);
                        }
                    }
                    // limpa o buffer anterior pra não processá-lo na proxima iteração
                    prev_lin[0] = '\0'; 
                    continue; // pula pra a proxima leitura sem escrever "atual_lin" nem "prev_lin"
                }
            }
        }
        // se não houve otimização escreve a linha anterior pendente
        if(prev_lin[0] != '\0') {
            fputs(prev_lin, saida);
        }
        // a linha atual vira a linha anterior para a proxima iteração
        strcpy(prev_lin, atual_lin);
    }
    // escreve a ultima linha que sobrou no buffer se houver
    if(prev_lin[0] != '\0') fputs(prev_lin, saida);
    
    fclose(arq);
    fclose(saida);
    // substitui o arquivo original pelo otimizado
    remove(arquivo_asm);
    rename(temp_nome, arquivo_asm);
    if(debug1 && instrucoes_rm > 0) {
        printf("  [Otimi1] Total de instruções de pilha removidas: %d\n", instrucoes_rm);
    }
}

void otimizarO1(const char* arquivo_asm) {
    FILE *arquivo = fopen(arquivo_asm, "r");
    if(!arquivo) {
        printf("Erro: Não foi possível abrir o arquivo %s\n", arquivo_asm);
        return;
    }
    // primeira passagem: coleta nomes de funções e verifica referencias
    char linha[256];
    // estrutura pra armazenar informações sobre as funções
    typedef struct {
        char nome[100];
        bool referenciada;
        long inicio; // posição no arquivo onde a função começa
        long fim; // posição no arquivo onde a função termina
        bool dentro_funcao;
    } Funcao;
    Funcao funcoes[100];
    int num_funcoes = 0;
    
    long posicao_atual = 0;
    // primeira passagem: identifica todas as funções
    while(fgets(linha, sizeof(linha), arquivo)) {
        posicao_atual = ftell(arquivo);
        // verifica se é inicio de função
        if(strstr(linha, "// fn:") != NULL) {
            // extrai nome sem os colchetes
            char *inicio_nome = strstr(linha, "[");
            char *fim_nome = strstr(linha, "]");
            
            if(inicio_nome && fim_nome && fim_nome > inicio_nome) {
                int tam = fim_nome - inicio_nome - 1;
                strncpy(funcoes[num_funcoes].nome, inicio_nome + 1, tam);
                funcoes[num_funcoes].nome[tam] = '\0';
            } else {
                // usa sscanf se não encontrar colchetes
                sscanf(linha, "// fn: %s", funcoes[num_funcoes].nome);
                // remove colchetes se existirem
                char *abre_colchete = strchr(funcoes[num_funcoes].nome, '[');
                if(abre_colchete) *abre_colchete = '\0';
                char *fecha_colchete = strchr(funcoes[num_funcoes].nome, ']');
                if(fecha_colchete) *fecha_colchete = '\0';
            }
            funcoes[num_funcoes].referenciada = false;
            funcoes[num_funcoes].inicio = posicao_atual - strlen(linha);
            funcoes[num_funcoes].dentro_funcao = true;
            // verifica se a função é global
            // procura pela declaração .global no arquivo
            long pos_salvo = ftell(arquivo);
            char busca_global[256];
            snprintf(busca_global, sizeof(busca_global), ".global %s", funcoes[num_funcoes].nome);
            rewind(arquivo);
            char linha_global[256];
            while(fgets(linha_global, sizeof(linha_global), arquivo)) {
                if(strstr(linha_global, busca_global) != NULL) {
                    funcoes[num_funcoes].referenciada = true; // Marca como referenciada
                    if(debug1) printf("Função global encontrada: '%s'\n", funcoes[num_funcoes].nome);
                    break;
                }
            }
            fseek(arquivo, pos_salvo, SEEK_SET); // volta pra posição original
            if(debug1) printf("Encontrada função: '%s' (%s)\n", funcoes[num_funcoes].nome, 
            funcoes[num_funcoes].referenciada ? "GLOBAL" : "local");
            num_funcoes++;
        }
        // verifica se é fim de função
        else if(strstr(linha, "// fim:") != NULL && num_funcoes > 0) {
            for(int i = num_funcoes - 1; i >= 0; i--) {
                if(funcoes[i].dentro_funcao) {
                    funcoes[i].fim = posicao_atual;
                    funcoes[i].dentro_funcao = false;
                    break;
                }
            }
        }
    }
    // volta ao inicio do arquivo pra procurar referencias
    rewind(arquivo);
    
    if(debug1) printf("\n=== PROCURANDO REFERÊNCIAS ===\n");
    // procura por referencias "bl nome_funcao" pra cada função
    int linha_num = 0;
    while(fgets(linha, sizeof(linha), arquivo)) {
        linha_num++;
        // remove comentarios da linha pra analise
        char linha_sem_comentario[256];
        strcpy(linha_sem_comentario, linha);
        char *comentario = strstr(linha_sem_comentario, "//");
        if(comentario) *comentario = '\0';
        
        // verifica se tem referencia a alguma função
        char *bl_pos = strstr(linha_sem_comentario, "bl ");
        if(bl_pos != NULL) {
            if(debug1) printf("Linha %d: Encontrado 'bl' em: %s", linha_num, linha);
            
            // pula "bl " pra pegar o nome da função
            char *nome_inicio = bl_pos + 3; // +3 pra pular "bl "
            
            // encontra o final do nome da função(espaço, tab, fim de linha, etc)
            char *nome_fim = nome_inicio;
            while(*nome_fim && !isspace(*nome_fim) && *nome_fim != ',' && *nome_fim != ']' && *nome_fim != '\n' && *nome_fim != '\r') {
                nome_fim++;
            }
            if(nome_fim > nome_inicio) {
                // extrai o nome da função
                char nome_chamado[100];
                int tam = nome_fim - nome_inicio;
                strncpy(nome_chamado, nome_inicio, tam);
                nome_chamado[tam] = '\0';
                
                if(debug1) printf("  Nome extraído: '%s'\n", nome_chamado);
                
                // verifica se é uma das funções que ta rastreando
                for(int i = 0; i < num_funcoes; i++) {
                    if(strcmp(funcoes[i].nome, nome_chamado) == 0) {
                        funcoes[i].referenciada = true;
                        if(debug1) printf("  *** FUNÇÃO REFERENCIADA: %s ***\n", funcoes[i].nome);
                        break;
                    }
                }
            }
        }
        // tambem procura por referencias com tabulação
        char *bl_tab_pos = strstr(linha_sem_comentario, "\tbl ");
        if(bl_tab_pos != NULL) {
            if(debug1) printf("Linha %d: Encontrado '\\tbl' em: %s", linha_num, linha);
            
            // pula "\tbl " pra pegar o nome da função
            char *nome_inicio = bl_tab_pos + 4; // +4 pra pular "\tbl "
            
            // encontra o final do nome da função
            char *nome_fim = nome_inicio;
            while(*nome_fim && !isspace(*nome_fim) && *nome_fim != ',' && *nome_fim != ']' && *nome_fim != '\n' && *nome_fim != '\r') {
                nome_fim++;
            }
            if(nome_fim > nome_inicio) {
                // Extrair o nome da função
                char nome_chamado[100];
                int tam = nome_fim - nome_inicio;
                strncpy(nome_chamado, nome_inicio, tam);
                nome_chamado[tam] = '\0';
                
                if(debug1) printf("  Nome extraído: '%s'\n", nome_chamado);
                
                // verifica se é uma das funções que ta rastreando
                for(int i = 0; i < num_funcoes; i++) {
                    if(strcmp(funcoes[i].nome, nome_chamado) == 0) {
                        funcoes[i].referenciada = true;
                        if(debug1) printf("  *** FUNÇÃO REFERENCIADA: %s ***\n", funcoes[i].nome);
                        break;
                    }
                }
            }
        }
    }
    fclose(arquivo);
    if(debug1) {
        printf("\n=== RESULTADO DA ANÁLISE ===\n");
        for(int i = 0; i < num_funcoes; i++) {
            printf("Função %s: %s\n", funcoes[i].nome, funcoes[i].referenciada ? "REFERENCIADA" : "NÃO REFERENCIADA");
        }
    }
    // segunda passagem: cria novo arquivo sem as funções não referenciadas
    arquivo = fopen(arquivo_asm, "r");
    if(!arquivo) {
        printf("Erro: Não foi possível reabrir o arquivo %s\n", arquivo_asm);
        return;
    }
    char arquivo_temp[300];
    snprintf(arquivo_temp, sizeof(arquivo_temp), "%s.fpbo", arquivo_asm);
    FILE *temp = fopen(arquivo_temp, "w");
    if(!temp) {
        printf("Erro: Não foi possível criar arquivo temporário\n");
        fclose(arquivo);
        return;
    }
    posicao_atual = 0;
    bool pular_secao = false;
    int funcao_atual_idc = -1;
    
    while (fgets(linha, sizeof(linha), arquivo)) {
        long proxima_posicao = ftell(arquivo);
        // verifica se é inicio de função
        if(strstr(linha, "// fn:") != NULL) {
            // extrai nome da função da linha atual(pra comparação)
            char nome_temp[100];
            char *inicio_nome = strstr(linha, "[");
            char *fim_nome = strstr(linha, "]");
            
            if(inicio_nome && fim_nome && fim_nome > inicio_nome) {
                int tam = fim_nome - inicio_nome - 1;
                strncpy(nome_temp, inicio_nome + 1, tam);
                nome_temp[tam] = '\0';
            } else {
                sscanf(linha, "// fn: %s", nome_temp);
                // remove colchetes se existirem
                char *abre_colchete = strchr(nome_temp, '[');
                if(abre_colchete) *abre_colchete = '\0';
                char *fecha_colchete = strchr(nome_temp, ']');
                if(fecha_colchete) *fecha_colchete = '\0';
            }
            // encontra indice da função atual
            funcao_atual_idc = -1;
            for(int i = 0; i < num_funcoes; i++) {
                if(strcmp(funcoes[i].nome, nome_temp) == 0) {
                    funcao_atual_idc = i;
                    break;
                }
            }
            // decide se pula a função
            if(funcao_atual_idc != -1 && !funcoes[funcao_atual_idc].referenciada) {
                pular_secao = true;
                if(debug1) printf("Removendo função não referenciada: %s\n", nome_temp);
            } else {
                pular_secao = false;
                fputs(linha, temp); // escreve linha de inicio da função
            }
        }
        // verificar se é fim de função
        else if(strstr(linha, "// fim:") != NULL) {
            if(pular_secao) {
                pular_secao = false; // para de pular
            } else {
                fputs(linha, temp); // escreve linha de fim da função
            }
            funcao_atual_idc = -1;
        }
        // linha normal
        else {
            if(!pular_secao) {
                fputs(linha, temp);
            }
        }
        posicao_atual = proxima_posicao;
    }
    fclose(arquivo);
    fclose(temp);
    economiaReg(arquivo_temp);
    // substitui arquivo original pelo temporario
    remove(arquivo_asm);
    rename(arquivo_temp, arquivo_asm);
    if(debug1) {
        printf("\n=== OTIMIZAÇÃO CONCLUÍDA ===\n");
        printf("Otimização O1 concluída para %s:\n", arquivo_asm);
        for(int i = 0; i < num_funcoes; i++) {
            if(!funcoes[i].referenciada) {
                printf("  - %s (removida)\n", funcoes[i].nome);
            } else {
                printf("  + %s (mantida)\n", funcoes[i].nome);
            }
        }
    }
}
#endif