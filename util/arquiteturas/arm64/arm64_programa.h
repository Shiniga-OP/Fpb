// util/arquiteturas/arm64/arm64_programa.h
#pragma once
#include "arm64.h"

inline std::string ARM64::gerarAssembly(const ProgramaIR& programa) {
    std::ostringstream saida;

    if(programa.ehBibli) {
        for(int i = 0; i < programa.funcoes.tam; i++) {
            saida << ".global " << programa.funcoes[i].nome << "\n.type " << programa.funcoes[i].nome << ", %function\n";
        }
    } else {
        saida << ".global inicio\n";
    }
    saida << ".section .rodata\n";
    for(int i = 0; i < programa.dados.tam; i++) {
        const DadoEstaticoIR& dado = programa.dados[i];
        saida << dado.simbolo << ": .asciz \"" << escaparParaAsm(dado.valor, dado.tamanho) << "\"\n";
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
        int posFp = 0;
        int posSalvos = 16;
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
            static const char* registradoresParametro[] = { "x0", "x1", "x2", "x3", "x4", "x5" };
            for(int p = 0; p < funcao.totalParametros; p++) {
                emitirGravarSlot(saida, registradoresParametro[p], p, funcao.parametrosLargura[p]);
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
        } else {
            emitirRetorne(saida, comando.instrucaoRetorne);
        }
    }
}