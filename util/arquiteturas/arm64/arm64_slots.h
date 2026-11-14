// util/arquiteturas/arm64/arm64_slots.h
#pragma once
#include "arm64.h"

// nomes dos registradores fisicos disponiveis pro alocador de
// "liberacao por ultimo uso"(indice i == x{19+i}). Restrito a x19..x27
// de proposito: sao os unicos registradores verdadeiramente
// callee-saved da AAPCS64(ver comentario de TOTAL_REGISTRADORES_FISICOS
// em ir.h). Setado por registradorDoSlot em ir.h.
inline const char* ARM64::nomeRegistradorFisico(int indice) {
    static const char* nomes[TOTAL_REGISTRADORES_FISICOS] = {
        "x19", "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27"
    };
    return nomes[indice];
}

// calcula quais indices de registrador fisico(0..TOTAL_REGISTRADORES_FISICOS-1,
// ver nomeRegistradorFisico) a funcao de fato usa, varrendo
// registradorDoSlot. Usado tanto pra dimensionar o frame(espaco extra
// pra salvar cada um) quanto pra emitir os proprios puxar/pegar no
// prologo/epilogo. Devolve os indices em
// ordem crescente(preenche "saida", que o chamador aloca com tamanho
// TOTAL_REGISTRADORES_FISICOS e le so os primeiros "*totalUsados").
inline void ARM64::calcularRegistradoresUsados(const FuncaoIR& funcao, int* saida, int* totalUsados) {
    bool usado[TOTAL_REGISTRADORES_FISICOS];
    for(int i = 0; i < TOTAL_REGISTRADORES_FISICOS; i++) usado[i] = false;
    for(int s = 0; s < funcao.registradorDoSlot.tam; s++) {
        int r = funcao.registradorDoSlot[s];
        if(r != DONO_SLOT_NENHUM) usado[r] = true;
    }
    *totalUsados = 0;
    for(int i = 0; i < TOTAL_REGISTRADORES_FISICOS; i++) {
        if(usado[i]) saida[(*totalUsados)++] = i;
    }
}

// consulta se um slot(logico) ganhou registrador fisico via "liberacao
// por ultimo uso". Retorna o nome do registrador fisico(ex: "x19") ou
// nullptr se o slot nao ganhou registrador e continua na pilha.
inline const char* ARM64::registradorFisicoDoSlot(int slot) {
    if(!funcaoAtual) return nullptr;
    if(slot < 0 || slot >= funcaoAtual->registradorDoSlot.tam) return nullptr;
    int r = funcaoAtual->registradorDoSlot[slot];
    if(r == DONO_SLOT_NENHUM) return nullptr;
    return nomeRegistradorFisico(r);
}

// emite os stp/ldp que salvam(salvar==true, no prologo) ou restauram
// (salvar==false, no epilogo) os registradores callee-saved que esta
// funcao usa, num espaco dedicado do frame logo APOS a area de slots
// (posição "basePos" = totalSlots*16, ANTES da area de x29/x30) e
// ANTES da area de x29/x30(posição tamFrame-16). Nao pode ocupar
// [sp, #0..totalSlots*16), pois essa faixa e' usada por
// emitirCarregarSlot/emitirGravarSlot pra slots sem registrador fisico
// (ver "slot * 16" nessas funcoes). Emite em pares(stp/ldp) quando
// sobra 2, e um unico str/ldr se sobrar 1 impar. Essencial pra corrigir
// o bug de slots(variaveis) perdendo seu valor apos uma chamada de
// função de usuário(bl): sem isso, a FUNÇÃO CHAMADA e' livre pra reusar
// o mesmo registrador fisico(ex: x19) pros PRÓPRIOS slots dela, sem
// nenhuma obrigação de preservar o valor que o CHAMADOR tinha guardado la.
inline void ARM64::emitirPuxarPegarSalvo(std::ostringstream& saida, const int* registradoresUsados, int totalRegistradoresUsados, int basePos, bool salvar) {
    int i = 0;
    while(i < totalRegistradoresUsados) {
        int pos = basePos + i * 8;
        if(i + 1 < totalRegistradoresUsados) {
            const char* r1 = nomeRegistradorFisico(registradoresUsados[i]);
            const char* r2 = nomeRegistradorFisico(registradoresUsados[i + 1]);
            if(salvar) saida << "stp " << r1 << ", " << r2 << ", [sp, #" << pos << "]\n";
            else saida << "ldp " << r1 << ", " << r2 << ", [sp, #" << pos << "]\n";
            i += 2;
        } else {
            const char* r1 = nomeRegistradorFisico(registradoresUsados[i]);
            if(salvar) saida << "str " << r1 << ", [sp, #" << pos << "]\n";
            else saida << "ldr " << r1 << ", [sp, #" << pos << "]\n";
            i += 1;
        }
    }
}

// carrega o valor de um parametro(guardado na pilha pelo prologo da funcao atual, ou no proprio registrador fisico se o
// slot ganhou um via "liberacao por ultimo uso") no registrador de destino. Nunca assume que o valor ainda esta no
// registrador xN original de chamada(x0-x5): qualquer chamada dentro do corpo(svc, bl) pode ter sujado x0-x5 nesse meio tempo.
inline void ARM64::emitirCarregarParametro(std::ostringstream& saida, const char* registradorDestino, int indiceParametro) {
    const char* fisico = registradorFisicoDoSlot(indiceParametro);
    if(fisico) {
        if(std::string(fisico) != registradorDestino) saida << "mov " << registradorDestino << ", " << fisico << "\n";
        return;
    }
    saida << "ldr " << registradorDestino << ", [sp, #" << (baseSlots + indiceParametro * 16) << "]\n";
}

// converte o nome de um registrador de 64 bits("x9") pro seu equivalente
// de 32 bits("w9"), usado quando o valor envolvido e do tipo "car"(1
// byte): a ALU do ARM64 so opera em w(32 bits) ou x(64 bits) inteiros,
// então pra um valor de 1 byte usamos w e as instruções de memória
// estreitas(ldrb/strb) em vez de x/ldr/str de 8 bytes.
inline std::string ARM64::paraRegistrador32(const char* registradorX) {
    std::string r = registradorX;
    if(!r.empty() && r[0] == 'x') r[0] = 'w';
    return r;
}

// carrega um slot(parametro ou variavel) no registrador de destino. Se o slot ganhou registrador físico via "liberacao por
// ultimo uso", e um simples mov entre registradores(sem tocar memoria; usa w quando largura <= 4, x quando largura == 8).
// Senão, respeita a largura do tipo na pilha: largura==1(car) usa ldrb num registrador w(zero-estendido, trunca pra 1
// byte), largura==4(int) usa ldr num registrador w(32 bits, sem truncar), largura==8(longo/ponteiro) usa ldr num
// registrador x(64 bits).
inline void ARM64::emitirCarregarSlot(std::ostringstream& saida, const char* registradorDestino, int slot, int larguraBytes) {
    const char* fisico = registradorFisicoDoSlot(slot);
    bool usaW = (larguraBytes <= 4);
    if(fisico) {
        std::string destinoStr = usaW ? paraRegistrador32(registradorDestino) : std::string(registradorDestino);
        std::string origemStr = usaW ? paraRegistrador32(fisico) : std::string(fisico);
        if(destinoStr != origemStr) saida << "mov " << destinoStr << ", " << origemStr << "\n";
        return;
    }
    if(larguraBytes == 1) {
        saida << "ldrb " << paraRegistrador32(registradorDestino) << ", [sp, #" << (baseSlots + slot * 16) << "]\n";
    } else if(larguraBytes == 4) {
        saida << "ldr " << paraRegistrador32(registradorDestino) << ", [sp, #" << (baseSlots + slot * 16) << "]\n";
    } else {
        saida << "ldr " << registradorDestino << ", [sp, #" << (baseSlots + slot * 16) << "]\n";
    }
}

// grava o registrador de origem num slot. Se o slot ganhou registrador fisico via "liberacao por ultimo uso", e um mov
// entre registradores(sem tocar memoria; usa w quando largura <= 4, x quando largura == 8). Senao, respeita a largura
// do tipo na pilha: largura==1(car) usa strb(so o byte baixo e gravado), largura==4(int) usa str num registrador
// w(32 bits), largura==8(longo/ponteiro) usa str num registrador x(64 bits).
inline void ARM64::emitirGravarSlot(std::ostringstream& saida, const char* registradorOrigem, int slot, int larguraBytes) {
    const char* fisico = registradorFisicoDoSlot(slot);
    bool usaW = (larguraBytes <= 4);
    if(fisico) {
        std::string destinoStr = usaW ? paraRegistrador32(fisico) : std::string(fisico);
        std::string origemStr = usaW ? paraRegistrador32(registradorOrigem) : std::string(registradorOrigem);
        if(destinoStr != origemStr) saida << "mov " << destinoStr << ", " << origemStr << "\n";
        return;
    }
    if(larguraBytes == 1) {
        saida << "strb " << paraRegistrador32(registradorOrigem) << ", [sp, #" << (baseSlots + slot * 16) << "]\n";
    } else if(larguraBytes == 4) {
        saida << "str " << paraRegistrador32(registradorOrigem) << ", [sp, #" << (baseSlots + slot * 16) << "]\n";
    } else {
        saida << "str " << registradorOrigem << ", [sp, #" << (baseSlots + slot * 16) << "]\n";
    }
}

// monta qualquer imediato de 32("usaW"=true) ou 64 bits no registrador
// de destino via movz(primeiro bloco de 16 bits, zera o resto) seguido
// de movk(demais blocos de 16 bits, preserva o resto ja montado). "mov"
// sozinho so aceita um subconjunto restrito de imediatos(cabe numa
// constante de 16 bits, possivelmente deslocada/invertida pelo proprio
// assembler) - qualquer valor fora desse subconjunto(ex: 333334, que
// não cabe em 16 bits). movz/movk cobre QUALQUER imediato de
// 32/64 bits, sem excecao, as custas de ate 2(w) ou 4(x) instruções.
// valores negativos são tratados via o bit paterno de complemento de
// dois do proprio "longo"(imediato ja chega truncado pra largura certa
// pelo GeradorIR, ver resolverConversao), reinterpretado como sem sinal
// pra fatiar em blocos de 16 bits sem estouro de sinal em shift.
inline void ARM64::emitirMovImediato(std::ostringstream& saida, const char* registradorDestino, long valor, bool usaW) {
    std::string destino = usaW ? paraRegistrador32(registradorDestino) : std::string(registradorDestino);
    unsigned long bruto = usaW ? (unsigned long)(unsigned int)valor : (unsigned long)valor;
    int totalBlocos = usaW ? 2 : 4;

    if(bruto == 0) {
        saida << "movz " << destino << ", #0\n";
        return;
    }
    bool primeiro = true;
    for(int i = 0; i < totalBlocos; i++) {
        unsigned int bloco = (unsigned int)((bruto >> (i * 16)) & 0xFFFF);
        if(bloco == 0) continue; // pula blocos zerados: o caso "valor inteiro == 0" ja foi tratado acima
        if(primeiro) {
            saida << "movz " << destino << ", #" << bloco;
            if(i > 0) saida << ", lsl #" << (i * 16);
            saida << "\n";
            primeiro = false;
        } else {
            saida << "movk " << destino << ", #" << bloco << ", lsl #" << (i * 16) << "\n";
        }
    }
}

// carrega um ArgumentoIR(imediato ou parametro/variavel-registrador)
// no registrador de destino. Não aceita SIMBOLO_TEXTO: comparar
// ponteiros de texto com "==" não faz sentido nesta linguagem ainda.
inline void ARM64::emitirCarregarValorEm(std::ostringstream& saida, const char* registradorDestino, const ArgumentoIR& arg) {
    emitirCarregarValorEmComLargura(saida, registradorDestino, arg, arg.larguraBytes);
}

// mesma coisa que emitirCarregarValorEm, so que "larguraOperacao" pode
// ser MAIOR que arg.larguraBytes(ex: um "int"(4 bytes) somado a um
// ponteiro(8 bytes) numa operacao de 64 bits, caso classico de
// "ponteiro + deslocamento_int"). Quando isso acontece, carrega o
// valor normalmente em w(32 bits) e da "sxtw" pra estender o SINAL
// pros 32 bits altos de x: sem isso, um "mov w10, -4" seguido de um
// "add x9, x9, x10" de 64 bits usaria x10 = 0x00000000FFFFFFFC(quase
// 4 bilhoes) em vez de -4, porque escrever em w SEMPRE zera os bits
// altos de x(comportamento do ARM64), nunca estende o sinal sozinho.
inline void ARM64::emitirCarregarValorEmComLargura(std::ostringstream& saida, const char* registradorDestino, const ArgumentoIR& arg, int larguraOperacao) {
    bool precisaEstenderSinal = (larguraOperacao == 8 && arg.larguraBytes <= 4);
    std::string destino32 = paraRegistrador32(registradorDestino);
    if(arg.tipo == ArgumentoIR::Tipo::IMEDIATO) {
        // um imediato "car"/"int" já chega truncado pelo GeradorIR; usamos o registrador x/w normalmente(movz/movk aceita ambos), so a leitura/gravacao de memoria precisa da largura estreita.
        bool usaW = (arg.larguraBytes <= 4);
        emitirMovImediato(saida, registradorDestino, arg.imediato, usaW);
    } else if(arg.tipo == ArgumentoIR::Tipo::REGISTRADOR) {
        emitirCarregarSlot(saida, registradorDestino, arg.registrador, arg.larguraBytes);
    } else {
        throw std::runtime_error("Arquitetura ARM64: esta operação não suporta texto(SIMBOLO_TEXTO) como operando");
    }
    if(precisaEstenderSinal) {
        saida << "sxtw " << registradorDestino << ", " << destino32 << "\n";
    }
}

// emite "slot = valor": materializa "valor" em x9(registrador de rascunho, mesma reserva documentada em emitirTexTam) e grava no slot
// de 16 bytes da variável(seja declaração inicial ou reatribuição).
inline void ARM64::emitirReatribuicao(std::ostringstream& saida, const InstrucaoReatribuicao& instrucao) {
    const ArgumentoIR& valor = instrucao.valor;
    if(valor.tipo == ArgumentoIR::Tipo::SIMBOLO_TEXTO) {
        saida << "ldr x9, =" << valor.simbolo << "\n";
    } else if(valor.tipo == ArgumentoIR::Tipo::REGISTRADOR) {
        emitirCarregarSlot(saida, "x9", valor.registrador, valor.larguraBytes);
    } else {
        bool usaW = (valor.larguraBytes <= 4);
        emitirMovImediato(saida, "x9", valor.imediato, usaW);
    }
    emitirGravarSlot(saida, "x9", instrucao.slotDestino, valor.larguraBytes);
}