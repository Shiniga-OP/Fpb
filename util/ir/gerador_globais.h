#pragma once
#include "geradorir.h"

inline std::string simboloGlobal(const char* nome) {
    return std::string("_global_") + nome;
}

inline CampoEstatico campoDoPonteiroGlobal() {
    CampoEstatico c;
    c.tipo = TipoVariavel::NULO_PONTEIRO;
    c.nome = nullptr;
    c.valorInicial = 0;
    c.ehFinal = false;
    return c;
}

inline void GeradorIR::registrarGlobaisDeclaradas(const Programa& programa) {
    for(int i = 0; i < programa.variaveisGlobais.tam; i++) {
        const DeclaracaoVariavel& decl = programa.variaveisGlobais[i];
        std::string nomeStr = decl.nome;
        if(decl.tipo != TipoVariavel::ESPACO && !decl.ehFinal) {
            throw std::runtime_error("Variável global '" + nomeStr + "': por enquanto só 'estatico Nome x' (espaço) ou 'estatico final'");
        }
        for(int k = 0; k < globaisDeclarados.tam; k++) {
            if(nomeStr == globaisDeclarados[k]->nome) throw std::runtime_error("Variável global '" + nomeStr + "' definida mais de uma vez");
        }
        globaisDeclarados.empurrar(&decl);
    }
}

// no começo de cada função: liga cada global a um slot local com o mesmo nome.
inline void GeradorIR::vincularGlobais(ContextoFuncao& ctx, Vetor<ComandoIR>& saida) {
    for(int i = 0; i < globaisDeclarados.tam; i++) {
        const DeclaracaoVariavel& g = *globaisDeclarados[i];
        ArgumentoIR jaExiste;
        if(ctx.variaveisLocais.buscar(g.nome, &jaExiste)) continue; // parâmetro sombreia a global
        int slot = ctx.variaveisLocais.proximoSlot++;
        if(g.ehFinal) {
            ArgumentoIR valor = resolverExpressao(g.valorInicial, saida, ctx);
            valor = converterParaTipo(valor, tipoEhFlutuante(g.tipo), larguraDoTipo(g.tipo), saida, ctx);
            if(valor.tipo != ArgumentoIR::Tipo::IMEDIATO) {
                std::string nomeStr = g.nome;
                throw std::runtime_error("'final' exige um valor constante em tempo de compilação (global '" + nomeStr + "')");
            }
            ctx.variaveisLocais.definirComSlot(g.nome, constanteDoTipo(valor.imediato, g.tipo), slot);
            continue;
        }
        int id = obterIdEspaco(g.nomeEspaco);
        CampoEstatico campo = campoDoPonteiroGlobal();
        ArgumentoIR endereco = enderecoDeGlobal(simboloGlobal(g.nome), campo, saida, ctx);
        ComandoIR leitura;
        leitura.tipo = ComandoIR::Tipo::LEITURA_INDEXADA;
        leitura.instrucaoLeituraIndexada.slotDestino = slot;
        leitura.instrucaoLeituraIndexada.ponteiro = endereco;
        leitura.instrucaoLeituraIndexada.indice = imediatoInt(0);
        saida.empurrar(leitura);
        ctx.variaveisLocais.definirComSlot(g.nome, argumentoEspaco(slot, id), slot);
    }
}

// função sintética: aloca e constrói cada objeto global e guarda o ponteiro na variável global.
inline FuncaoIR GeradorIR::gerarIniciarGlobais(Vetor<DadoEstaticoIR>& dados) {
    DeclaracaoFuncao decl;
    decl.iniciar();
    decl.nome = strdup("_iniciar_globais");
    FuncaoIR f;
    f.iniciar();
    bytesArraysAtual = 0;
    f.nome = strdup(decl.nome);
    ContextoFuncao ctx(decl, dados);
    ctx.variaveisLocais.iniciar(0);
    for(int i = 0; i < globaisDeclarados.tam; i++) {
        const DeclaracaoVariavel& g = *globaisDeclarados[i];
        if(g.ehFinal) continue;
        int marcaTemp = ctx.tempSlot.tam;
        int id = obterIdEspaco(g.nomeEspaco);
        int slot = ctx.variaveisLocais.proximoSlot++;
        ArgumentoIR ref = argumentoEspaco(slot, id);
        emitirAlocar(slot, id, f.instrucoes);
        if(g.temLista) throw std::runtime_error(std::string("Variável global '") + g.nome + "': lista de inicialização ainda não é suportada, use 'estatico Nome x = expr;'");
        if(g.listaInicial.tam == 1) {
            ArgumentoIR valor = resolverExpressao(g.listaInicial[0], f.instrucoes, ctx);
            if(valor.tipo == ArgumentoIR::Tipo::REGISTRADOR && valor.idEspaco == id) {
                if(valor.ehTemporario) moverEspaco(valor, ref, id, f.instrucoes, ctx);
                else copiarObjeto(valor, ref, id, f.instrucoes, ctx);
            } else {
                emitirCriar(ref, valor, id, f.instrucoes);
            }
        } else {
            zerarEspaco(ref, id, f.instrucoes);
        }
        CampoEstatico campo = campoDoPonteiroGlobal();
        ArgumentoIR endereco = enderecoDeGlobal(simboloGlobal(g.nome), campo, f.instrucoes, ctx);
        emitirEscritaCampo(endereco, 0, TipoVariavel::NULO_PONTEIRO, copiarArgumento(ref), f.instrucoes);
        emitirLiberacaoTemporarios(marcaTemp, true, f.instrucoes, ctx);
    }
    f.totalSlots = ctx.variaveisLocais.proximoSlot;
    f.bytesArrays = bytesArraysAtual;
    ctx.variaveisLocais.liberar();
    ctx.liberar();
    decl.liberar();
    alocarRegistradores(f);
    return f;
}