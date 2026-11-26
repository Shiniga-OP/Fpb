// util/arquiteturas/arm64/arm64_programa.h
#pragma once
#include "arm64.h"

// sobe "maior" pros bytes de pilha da maior chamada do bloco(e dos aninhados) que passa dos x0-x7/v0-v7 (8 bytes cada)
inline void acumularBytesArgsSaida(const Vetor<ComandoIR>& bloco, int& maior) {
    for(int i = 0; i < bloco.tam; i++) {
        const ComandoIR& c = bloco[i];
        if(c.tipo == ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO) {
            int ints = 0, flus = 0, pilha = 0;
            for(int a = 0; a < c.chamadaFuncaoUsuario.argumentos.tam; a++) {
                if(c.chamadaFuncaoUsuario.argumentos[a].ehFlutuante) { if(flus++ >= 8) pilha++; }
                else { if(ints++ >= 8) pilha++; }
            }
            if(pilha * 8 > maior) maior = pilha * 8;
        }
        paraCadaFilho(c, acumularBytesArgsSaida, maior);
    }
}

// bytes de pilha que a MAIOR chamada do bloco precisa pra args alem de x0-x7/v0-v7 (8 bytes cada)
inline int bytesArgsSaida(const Vetor<ComandoIR>& bloco) {
    int maior = 0;
    acumularBytesArgsSaida(bloco, maior);
    return maior;
}

inline std::string ARM64::gerarAssembly(const ProgramaIR& programa) {
    std::ostringstream saida;

    for(int i = 0; i < programa.funcoes.tam; i++) {
        const FuncaoIR& f = programa.funcoes[i];
        if(programa.ehBibli || f.ehExportada || f.ehInicio) saida << ".global " << f.nome << "\n.type " << f.nome << ", %function\n";
    }
    saida << ".section .rodata\n";
    for(int i = 0; i < programa.dados.tam; i++) {
        const DadoEstaticoIR& dado = programa.dados[i];
        saida << dado.simbolo << ": .asciz \"" << escaparParaAsm(dado.valor, dado.tamanho) << "\"\n";
    }
    if(programa.globais.tam > 0) {
        saida << "\n.section .data\n";
        for(int i = 0; i < programa.globais.tam; i++) {
            const VariavelGlobalIR& global = programa.globais[i];
            const char* diretiva = global.largura == 1 ? ".byte" : global.largura == 2 ? ".hword" : global.largura == 4 ? ".word" : ".quad";
            saida << ".weak " << global.simbolo << "\n";
            saida << ".hidden " << global.simbolo << "\n";
            saida << ".balign " << global.largura << "\n";
            saida << global.simbolo << ": " << diretiva << " " << global.valorInicial << "\n";
        }
    }
    saida << "\n.section .text\n";
    for(int i = 0; i < programa.funcoes.tam; i++) {
        const FuncaoIR& funcao = programa.funcoes[i];
        funcaoAtual = &funcao;
        saida << funcao.nome << ":\n";

        int registradoresUsados[TOTAL_REGISTRADORES_FISICOS];
        int totalRegistradoresUsados;
        calcularRegistradoresUsados(funcao, registradoresUsados, &totalRegistradoresUsados);
        // espaco extra no frame pra salvar cada callee-saved usado(16
        // bytes por par, igual stp/ldp de x29/x30: arredonda pra cima
        // se for impar, mantendo o sp alinhado a 16 como a AAPCS64 exige).
        int espacoChamadaSalva = ((totalRegistradoresUsados + 1) / 2) * 16;

        // tamanho do frame desta funcao: 16 bytes fixos pra x29/x30(registrador link, necessario porque o corpo pode chamar outras funcoes via 'bl', o que sobrescreve x30), mais 16 bytes por
        // slot(8 usados + 8 de espaço, mantendo o sp alinhado a 16 como a AAPCS64 exige), mais espacoChamadaSalva(callee-saved usados por esta funcao, ver acima). Slot inclui parametros e variaveis
        // locais declaradas no corpo, nessa ordem(totalSlots >= totalParametros). Slots que ganharam registrador fisico
        // via "liberacao por ultimo uso" nao usam esse espaço pra leitura/escrita normal, mas o espaço continua reservado
        // no frame(simplifica o calculo de tamFrame e mantem os indices de slot estaveis).
        // layout do frame (do sp pra cima): x29/x30 em 0, callee-saved em 16,
        // slots depois deles, arrays por ultimo. Tudo que usa stp/ldp (alcance
        // maximo 504) fica nos primeiros bytes, longe do tamanho dos slots.
        int bytesSaida = (bytesArgsSaida(funcao.instrucoes) + 15) & ~15;
        int posFp = bytesSaida;
        int posSalvos = bytesSaida + 16;
        baseSlots = posSalvos + espacoChamadaSalva;
        baseArrays = baseSlots + funcao.totalSlots * 16;
        int tamFrame = baseArrays + funcao.bytesArrays;
        tamFrame = (tamFrame + 15) & ~15; // sp sempre alinhado a 16, mesmo com array de tamanho impar

        // 'inicio' tambem precisa de frame (slots e arrays moram em [sp, ...]),
        // mas nao salva x29/x30 nem callee-saved: ninguem espera nada de volta.
        saida << "sub sp, sp, #" << tamFrame << "\n";
        if(!funcao.ehInicio) {
            saida << "stp x29, x30, [sp, #" << posFp << "]\n";
            saida << "add x29, sp, #" << posFp << "\n";
            emitirPuxarPegarSalvo(saida, registradoresUsados, totalRegistradoresUsados, posSalvos, true);
            // materializa cada parametro recebido em x0..x5 no seu slot
            // logico: se o slot ganhou registrador fisico(liberacao por
            // ultimo uso), move pra la; senao grava na pilha como antes.
            // A partir daqui, nenhum uso do parametro confia mais no
            // registrador original x0-x5(pode ser sujado por qualquer chamada dentro do corpo).
            // inteiros e ponteiros chegam em x0..x7 e flutuantes em s0..s7/d0..d7, com contadores independentes(AAPCS64)
            int totalInteiros = 0;
            int totalFlutuantes = 0;
            int totalPilha = 0;
            for(int p = 0; p < funcao.totalParametros; p++) {
                int largura = funcao.parametrosLargura[p];
                bool flu = funcao.parametrosFlutuante[p];
                int& contador = flu ? totalFlutuantes : totalInteiros;
                if(contador >= 8) {
                    contador++;
                    // args alem do 8o ficam logo acima do frame, na area de saida do chamador
                    saida << "ldr x9, [sp, #" << (tamFrame + totalPilha++ * 8) << "]\n";
                    emitirGravarSlot(saida, "x9", p, largura);
                } else if(flu) {
                    saida << "fmov " << (largura == 4 ? "w9, s" : "x9, d") << totalFlutuantes++ << "\n";
                    emitirGravarSlot(saida, "x9", p, largura);
                } else {
                    std::string registrador = "x" + std::to_string(totalInteiros++);
                    emitirGravarSlot(saida, registrador.c_str(), p, largura);
                }
            }
            saida << "\n";
        }
        emitirBlocoComandos(saida, funcao.instrucoes, programa);
        // recebe prologo/epilogo nem "ret". Toda outra funcao pode ser
        // chamada via "bl" e precisa desfazer o frame e retornar.
        if(!funcao.ehInicio) {
            // rotulo de fim: alvo de qualquer "retorne" no meio do corpo
            // (inclusive dentro de "se"/"enquanto"), que salta direto pra
            // ca via "b _fim_<nome>" em vez de continuar o resto do corpo.
            // emitido sempre(mesmo que a funcao nao use "retorne" em
            // nenhum ponto), pra manter o calculo do frame/epilogo simples.
            // O restore dos salvo TEM que acontecer aqui(unico
            // ponto de saida), nunca antes: um "retorne" no meio do corpo
            // salta direto pra ca via "b", pulando qualquer codigo entre
            // o retorne e o fim do corpo textual da funcao.
            saida << "_fim_" << funcao.nome << ":\n";
            emitirPuxarPegarSalvo(saida, registradoresUsados, totalRegistradoresUsados, posSalvos, false);
            saida << "ldp x29, x30, [sp, #" << posFp << "]\n";
            saida << "add sp, sp, #" << tamFrame << "\n";
            saida << "ret\n\n";
        }
        funcaoAtual = nullptr;
    }
    std::string resultado = saida.str();
    return otimizarMovsRedundantes(resultado);
}

// mapa: id abstrato de chamada de sistema -> numero real da chamada de sistema no Linux/ARM64
inline long ARM64::numSistema(ChamadaSistemaId id) {
    switch(id) {
        case ChamadaSistemaId::ESCREVER: return 64;
        case ChamadaSistemaId::SAIR: return 93;
        case ChamadaSistemaId::ALOCAR: return 222; // mmap
        case ChamadaSistemaId::LIBERAR: return 215; // munmap
        case ChamadaSistemaId::ALEATORIO: return 278;
        case ChamadaSistemaId::ABRIR: return 56;
        case ChamadaSistemaId::FECHAR: return 57;
        case ChamadaSistemaId::LER: return 63;
        case ChamadaSistemaId::POSICIONAR: return 62;
        case ChamadaSistemaId::CLONE: return 220; // clone
        case ChamadaSistemaId::FUTEX: return 98;
        default: throw std::runtime_error("Arquitetura ARM64: chamada de sistema sem mapeamento");
    }
}

// emite cada ComandoIR de um bloco("entao" ou "senao" de um "se");
// usado tanto pelo corpo normal de uma funcao quanto recursivamente
// por blocos de "se" aninhados.
inline void ARM64::emitirBlocoComandos(std::ostringstream& saida, const Vetor<ComandoIR>& bloco, const ProgramaIR& programa) {
    for(int i = 0; i < bloco.tam; i++) {
        const ComandoIR& comando = bloco[i];
        if(comando.tipo == ComandoIR::Tipo::CHAMADA_SISTEMA_CRUA) {
            emitirChamadaSistemaCrua(saida, comando.chamadaSistemaCrua);
        } else if(comando.tipo == ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO) {
            emitirChamadaFuncaoUsuario(saida, comando.chamadaFuncaoUsuario);
        } else if(comando.tipo == ComandoIR::Tipo::SE) {
            emitirSe(saida, comando.instrucaoSe, programa);
        } else if(comando.tipo == ComandoIR::Tipo::ENQUANTO) {
            emitirEnquanto(saida, comando.instrucaoEnquanto, programa);
        } else if(comando.tipo == ComandoIR::Tipo::REATRIBUICAO) {
            emitirReatribuicao(saida, comando.instrucaoReatribuicao);
        } else if(comando.tipo == ComandoIR::Tipo::OPERACAO_ARITMETICA) {
            emitirOperacaoAritmetica(saida, comando.instrucaoOperacaoAritmetica);
        } else if(comando.tipo == ComandoIR::Tipo::LEITURA_INDEXADA) {
            emitirLeituraIndexada(saida, comando.instrucaoLeituraIndexada);
        } else if(comando.tipo == ComandoIR::Tipo::ESCRITA_INDEXADA) {
            emitirEscritaIndexada(saida, comando.instrucaoEscritaIndexada);
        } else if(comando.tipo == ComandoIR::Tipo::ENDERECO_FRAME) {
            emitirEnderecoFrame(saida, comando.instrucaoEnderecoFrame);
        } else if(comando.tipo == ComandoIR::Tipo::ENDERECO_SLOT) {
            emitirEnderecoSlot(saida, comando.instrucaoEnderecoSlot);
        } else if(comando.tipo == ComandoIR::Tipo::CONVERSAO_NUMERICA) {
            emitirConversaoNumerica(saida, comando.instrucaoConversaoNumerica);
        } else {
            emitirRetorne(saida, comando.instrucaoRetorne);
        }
    }
}