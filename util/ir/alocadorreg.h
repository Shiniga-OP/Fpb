// util/ir/alocadorregistradores.h
#pragma once
#include "../config.h"
#include "ir.h"

// ===== liberação por último uso =====
// ideia: cada slot(parâmetro ou variável) ganha um registrador físico 
// no ponto em que e definido/reatribuido pela primeira vez, e o mantem
// até a ULTIMA posição em que aparece como operando em qualquer lugar
// do corpo(incluindo dentro de blocos aninhados de "se"/"senao"). Ao
// passar dessa posição, o registrador volta pro reuso e pode ser dado
// a outro slot. Se o reuso esgotar num certo ponto, o slot que nao
// coube continua acessando a pilha(ldr/str), igual ao comportamento
// anterior a esta mudança.

// varre um bloco de comandos IR em ordem de emissao, numerando cada
// instrução(posicao sequencial) e, para cada ArgumentoIR::Tipo::REGISTRADOR
// encontrado(seja como destino ou como operando), atualiza
// "primeiraPos"(so a primeira vez) e "ultimaPos"(sempre, guarda o maior)
// nos vetores paralelos indexados por slot. "pos" e passado por
// referencia e incrementado a cada instrução, inclusive dentro de
// blocos aninhados de "se", pra manter uma numeracao unica e continua
// por toda a função(assim um "ultimo uso" dentro de um "se" e comparado
// corretamente com usos antes/depois do bloco).
inline void registrarUsoArgumento(const ArgumentoIR& arg, Vetor<int>& primeiraPos, Vetor<int>& ultimaPos, int pos) {
    if(arg.tipo != ArgumentoIR::Tipo::REGISTRADOR) return;
    int slot = arg.registrador;
    if(primeiraPos[slot] == -1) primeiraPos[slot] = pos;
    ultimaPos[slot] = pos; // sempre atualiza: queremos o MAIOR(ultimo uso)
}

inline void registrarDefinicaoSlot(int slot, Vetor<int>& primeiraPos, Vetor<int>& ultimaPos, int pos) {
    if(primeiraPos[slot] == -1) primeiraPos[slot] = pos;
    if(ultimaPos[slot] < pos) ultimaPos[slot] = pos;
}

inline void varrerBlocoParaUsos(const Vetor<ComandoIR>& bloco, Vetor<int>& primeiraPos, Vetor<int>& ultimaPos, int& pos) {
    for(int i = 0; i < bloco.tam; i++) {
        const ComandoIR& c = bloco[i];
        if(c.tipo == ComandoIR::Tipo::REATRIBUICAO) {
            registrarUsoArgumento(c.instrucaoReatribuicao.valor, primeiraPos, ultimaPos, pos);
            registrarDefinicaoSlot(c.instrucaoReatribuicao.slotDestino, primeiraPos, ultimaPos, pos);
        } else if(c.tipo == ComandoIR::Tipo::OPERACAO_ARITMETICA) {
            registrarUsoArgumento(c.instrucaoOperacaoAritmetica.esquerda, primeiraPos, ultimaPos, pos);
            registrarUsoArgumento(c.instrucaoOperacaoAritmetica.direita, primeiraPos, ultimaPos, pos);
            registrarDefinicaoSlot(c.instrucaoOperacaoAritmetica.slotDestino, primeiraPos, ultimaPos, pos);
        } else if(c.tipo == ComandoIR::Tipo::CHAMADA_SISTEMA_CRUA) {
            for(int j = 0; j < c.chamadaSistemaCrua.argumentos.tam; j++) {
                registrarUsoArgumento(c.chamadaSistemaCrua.argumentos[j], primeiraPos, ultimaPos, pos);
            }
            if(c.chamadaSistemaCrua.capturaResultado) {
                registrarDefinicaoSlot(c.chamadaSistemaCrua.slotDestino, primeiraPos, ultimaPos, pos);
            }
        } else if(c.tipo == ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO) {
            for(int j = 0; j < c.chamadaFuncaoUsuario.argumentos.tam; j++) {
                registrarUsoArgumento(c.chamadaFuncaoUsuario.argumentos[j], primeiraPos, ultimaPos, pos);
            }
            if(c.chamadaFuncaoUsuario.capturaResultado) {
                registrarDefinicaoSlot(c.chamadaFuncaoUsuario.slotDestino, primeiraPos, ultimaPos, pos);
            }
        } else if(c.tipo == ComandoIR::Tipo::RETORNE) {
            if(c.instrucaoRetorne.temValor) {
                registrarUsoArgumento(c.instrucaoRetorne.valor, primeiraPos, ultimaPos, pos);
            }
        } else if(c.tipo == ComandoIR::Tipo::LEITURA_INDEXADA) {
            registrarUsoArgumento(c.instrucaoLeituraIndexada.ponteiro, primeiraPos, ultimaPos, pos);
            registrarUsoArgumento(c.instrucaoLeituraIndexada.indice, primeiraPos, ultimaPos, pos);
            registrarDefinicaoSlot(c.instrucaoLeituraIndexada.slotDestino, primeiraPos, ultimaPos, pos);
        } else if(c.tipo == ComandoIR::Tipo::ESCRITA_INDEXADA) {
            registrarUsoArgumento(c.instrucaoEscritaIndexada.ponteiro, primeiraPos, ultimaPos, pos);
            registrarUsoArgumento(c.instrucaoEscritaIndexada.indice, primeiraPos, ultimaPos, pos);
            registrarUsoArgumento(c.instrucaoEscritaIndexada.valor, primeiraPos, ultimaPos, pos);
        } else if(c.tipo == ComandoIR::Tipo::SE) {
            registrarUsoArgumento(c.instrucaoSe.esquerda, primeiraPos, ultimaPos, pos);
            registrarUsoArgumento(c.instrucaoSe.direita, primeiraPos, ultimaPos, pos);
            varrerBlocoParaUsos(*c.instrucaoSe.entao, primeiraPos, ultimaPos, pos);
            if(c.instrucaoSe.temSenao) {
                varrerBlocoParaUsos(*c.instrucaoSe.senaoCorpo, primeiraPos, ultimaPos, pos);
            }
        } else if(c.tipo == ComandoIR::Tipo::ENQUANTO) {
            // marca todo slot ja definido antes do laço como "em uso" a
            // partir daqui: comandosCondicao E corpo podem reexecutar do
            // zero a cada iteração, então um slot lido/escrito em
            // qualquer um dos dois tem que sobreviver por TODO o laço,
            // nao so ate seu ultimo uso textual(senao o registrador
            // podia ser reciclado no meio de uma iteração seguinte e
            // corromper o valor).
            int posAntesDoLaco = pos;
            varrerBlocoParaUsos(*c.instrucaoEnquanto.comandosCondicao, primeiraPos, ultimaPos, pos);
            registrarUsoArgumento(c.instrucaoEnquanto.esquerda, primeiraPos, ultimaPos, pos);
            registrarUsoArgumento(c.instrucaoEnquanto.direita, primeiraPos, ultimaPos, pos);
            varrerBlocoParaUsos(*c.instrucaoEnquanto.corpo, primeiraPos, ultimaPos, pos);
            // apos varrer condicao+corpo, "pos" esta na posicao final do
            // laço: qualquer slot com uso registrado dentro do intervalo
            // [posAntesDoLaco, pos] tem sua ultimaPos estendida ate "pos",
            // garantindo que o registrador so seja liberado depois que o
            // laço inteiro(todas as iteracoes) ja tiver terminado.
            for(int s = 0; s < ultimaPos.tam; s++) {
                if(ultimaPos[s] >= posAntesDoLaco && ultimaPos[s] <= pos) {
                    ultimaPos[s] = pos;
                }
            }
        }
        pos++; // cada ComandoIR do bloco ocupa uma posição própria na numeração
    }
}

// aloca um registrador fisico(x19..x27) pra cada slot, seguindo a regra
// de "liberacao por ultimo uso": percorre os slots em ordem de primeira
// aparicao, empresta um registrador livre do reuso, e devolve pro reuso
// qualquer registrador cujo dono(outro slot) ja passou da sua ultima
// posicao de uso. Se o teus reuso acabar num certo ponto, o slot correspondente
// fica com DONO_SLOT_NENHUM e continua sendo acessado via pilha.
// "inicio"(funcaoIR.ehInicio) NUNCA recebe registrador fisico: e a
// unica funcao sem prologo/epilogo(ver gerarAssembly em arm64.h, pula o
// bloco de push/pop pra ehInicio), então nenhum registrador salvo
// que ela usasse seria salvo/restaurado — um valor guardado em, digamos,
// x19 dentro de "inicio" seria destruido pela primeira chamada de
// usuário(bl) que tambem usasse x19 pros proprios slots dela.
inline void alocarRegistradores(FuncaoIR& funcaoIR) {
    int totalSlots = funcaoIR.totalSlots;
    funcaoIR.registradorDoSlot.iniciar();
    for(int i = 0; i < totalSlots; i++) funcaoIR.registradorDoSlot.empurrar(DONO_SLOT_NENHUM);
    if(totalSlots == 0 || funcaoIR.ehInicio) return;

    Vetor<int> primeiraPos, ultimaPos;
    primeiraPos.iniciar();
    ultimaPos.iniciar();
    for(int i = 0; i < totalSlots; i++) {
        primeiraPos.empurrar(-1);
        ultimaPos.empurrar(-1);
    }
    // parâmetros(slots 0..totalParametros-1) ja nascem "definidos" na
    // posição 0(prologo da funcao), mesmo que nunca apareçam como
    // operando explícito depois.
    for(int i = 0; i < funcaoIR.totalParametros; i++) {
        primeiraPos[i] = 0;
        if(ultimaPos[i] < 0) ultimaPos[i] = 0;
    }
    int pos = 1; // posição 0 reservada pro prólogo/parâmetros acima
    varrerBlocoParaUsos(funcaoIR.instrucoes, primeiraPos, ultimaPos, pos);

    // reuso de registradores fisicos livres(indices 0..TOTAL_REGISTRADORES_FISICOS-1,
    // correspondendo a x19..x27 na arquitetura, todos salvos). "ocupadoPor[r]" guarda
    // qual slot esta usando o registrador r no momento(-1 se livre).
    int ocupadoPor[TOTAL_REGISTRADORES_FISICOS];
    for(int r = 0; r < TOTAL_REGISTRADORES_FISICOS; r++) ocupadoPor[r] = DONO_SLOT_NENHUM;

    // ordena os slots que tem alguma definição(primeiraPos >= 0) por
    // ordem de primeira aparição: ja que o
    // número de slots por função costuma ser pequeno.
    Vetor<int> ordem;
    ordem.iniciar();
    for(int s = 0; s < totalSlots; s++) {
        if(primeiraPos[s] >= 0) ordem.empurrar(s);
    }
    for(int i = 1; i < ordem.tam; i++) {
        int chave = ordem[i];
        int j = i - 1;
        while(j >= 0 && primeiraPos[ordem[j]] > primeiraPos[chave]) {
            ordem[j + 1] = ordem[j];
            j--;
        }
        ordem[j + 1] = chave;
    }

    for(int i = 0; i < ordem.tam; i++) {
        int slot = ordem[i];
        int inicioSlot = primeiraPos[slot];

        // libera qualquer registrador cujo dono ja passou do proprio
        // ultimo uso ANTES deste slot comecar. Tem que ser "<" estrito,
        // nunca "<=": se o ultimo uso de um slot e a definicao de outro
        // caem na MESMA posicao(ex: "meio = v10 + v11" - v10 e lido e
        // "meio" e definido na mesma instrucao IR), a leitura de v10
        // ainda precisa do registrador dele nessa instrucao; liberar
        // com "<=" da o registrador de v10 pro novo slot cedo demais e
        // corrompe o valor sendo lido no meio do calculo.
        for(int r = 0; r < TOTAL_REGISTRADORES_FISICOS; r++) {
            int dono = ocupadoPor[r];
            if(dono != DONO_SLOT_NENHUM && ultimaPos[dono] < inicioSlot && dono != slot) {
                ocupadoPor[r] = DONO_SLOT_NENHUM;
            }
        }
        // procura um registrador livre pro slot atual.
        int registradorEscolhido = DONO_SLOT_NENHUM;
        for(int r = 0; r < TOTAL_REGISTRADORES_FISICOS; r++) {
            if(ocupadoPor[r] == DONO_SLOT_NENHUM) { registradorEscolhido = r; break; }
        }
        if(registradorEscolhido != DONO_SLOT_NENHUM) {
            ocupadoPor[registradorEscolhido] = slot;
            funcaoIR.registradorDoSlot[slot] = registradorEscolhido;
        }
        // se não achou, o slot fica DONO_SLOT_NENHUM(estoura pra pilha).
    }
    primeiraPos.liberar();
    ultimaPos.liberar();
    ordem.liberar();
}