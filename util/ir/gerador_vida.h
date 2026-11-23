// util/ir/gerador_vida.h
#pragma once
#include "geradorir.h"

inline ArgumentoIR GeradorIR::argumentoEspaco(int slot, int idEspaco) {
    ArgumentoIR arg;
    arg.tipo = ArgumentoIR::Tipo::REGISTRADOR;
    arg.simbolo = nullptr;
    arg.imediato = 0;
    arg.registrador = slot;
    arg.larguraBytes = 8;
    arg.ehPonteiro = true;
    arg.idEspaco = idEspaco;
    return arg;
}

// "Nome_criar(destino, valor)": usada por "Nome x = valor;", "x = valor;" e pela conversão de texto literal em espaço.
inline void GeradorIR::emitirCriar(const ArgumentoIR& destino, ArgumentoIR valor, int idEspaco, Vetor<ComandoIR>& saida) {
    std::string nomeEspacoStr = espacos.declaracoes[idEspaco]->nome;
    std::string nomeCriar = nomeEspacoStr + "_criar";
    int totalParametros;
    if(!funcoesUsuario.buscar(nomeCriar.c_str(), &totalParametros)) {
        throw std::runtime_error("Espaço '" + nomeEspacoStr + "' não tem o construtor '" + nomeCriar + "'");
    }
    if(totalParametros != 2) {
        throw std::runtime_error("O construtor '" + nomeCriar + "' precisa ter exatamente 1 parâmetro além de 'este'");
    }
    ComandoIR comando;
    comando.tipo = ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO;
    comando.chamadaFuncaoUsuario.iniciar();
    comando.chamadaFuncaoUsuario.nome = strdup(nomeCriar.c_str());
    comando.chamadaFuncaoUsuario.argumentos.empurrar(copiarArgumento(destino));
    comando.chamadaFuncaoUsuario.argumentos.empurrar(valor);
    saida.empurrar(comando);
}

// "Nome_liberar(alvo)", se o espaço tiver esse método.
inline void GeradorIR::emitirLiberar(const ArgumentoIR& alvo, int idEspaco, Vetor<ComandoIR>& saida) {
    std::string nome;
    if(!buscarMetodo(idEspaco, "liberar", nome)) return;
    ComandoIR comando;
    comando.tipo = ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO;
    comando.chamadaFuncaoUsuario.iniciar();
    comando.chamadaFuncaoUsuario.nome = strdup(nome.c_str());
    comando.chamadaFuncaoUsuario.argumentos.empurrar(copiarArgumento(alvo));
    saida.empurrar(comando);
}

// "mliberar(alvo)": devolve a memória da casca de um temporário de heap.
inline void GeradorIR::emitirMliberar(const ArgumentoIR& alvo, Vetor<ComandoIR>& saida) {
    int totalParametros;
    if(!funcoesUsuario.buscar("mliberar", &totalParametros)) {
        throw std::runtime_error("Operador de espaço exige 'mliberar' (faltou #incluir a biblioteca de memória?)");
    }
    ComandoIR comando;
    comando.tipo = ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO;
    comando.chamadaFuncaoUsuario.iniciar();
    comando.chamadaFuncaoUsuario.nome = strdup("mliberar");
    comando.chamadaFuncaoUsuario.argumentos.empurrar(copiarArgumento(alvo));
    saida.empurrar(comando);
}

// cópia de verdade: "Nome_copiar(destino, origem)". Sem "copiar" e sem "liberar" é cópia rasa; com "liberar" e sem "copiar" é erro (dois donos do mesmo buffer).
inline void GeradorIR::copiarObjeto(const ArgumentoIR& origem, const ArgumentoIR& destino, int idEspaco, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    std::string nomeEspacoStr = espacos.declaracoes[idEspaco]->nome;
    std::string nomeCopiar = nomeEspacoStr + "_copiar";
    int totalParametros;
    if(funcoesUsuario.buscar(nomeCopiar.c_str(), &totalParametros)) {
        if(totalParametros != 2) {
            throw std::runtime_error("O método '" + nomeCopiar + "' precisa ter exatamente 1 parâmetro além de 'este'");
        }
        ComandoIR comando;
        comando.tipo = ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO;
        comando.chamadaFuncaoUsuario.iniciar();
        comando.chamadaFuncaoUsuario.nome = strdup(nomeCopiar.c_str());
        comando.chamadaFuncaoUsuario.argumentos.empurrar(copiarArgumento(destino));
        comando.chamadaFuncaoUsuario.argumentos.empurrar(copiarArgumento(origem));
        saida.empurrar(comando);
        return;
    }
    if(funcoesUsuario.buscar((nomeEspacoStr + "_liberar").c_str(), &totalParametros)) {
        throw std::runtime_error("Espaço '" + nomeEspacoStr + "' tem 'liberar' mas não tem 'copiar': a cópia rasa liberaria o mesmo buffer duas vezes");
    }
    copiarEspaco(origem, destino, idEspaco, saida, ctx);
}

// move o conteúdo de um temporário de heap pro destino: copia os campos, devolve só a casca e tira o temporário da lista (o destino virou o dono).
inline void GeradorIR::moverEspaco(const ArgumentoIR& temp, const ArgumentoIR& destino, int idEspaco, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    copiarEspaco(temp, destino, idEspaco, saida, ctx);
    emitirMliberar(temp, saida);
    removerTemporario(temp.registrador, ctx);
}

// "Nome x;" sem valor: zera os campos, pra "liberar" não receber lixo da pilha.
inline void GeradorIR::zerarEspaco(const ArgumentoIR& destino, int idEspaco, Vetor<ComandoIR>& saida) {
    const DeclaracaoEspaco& espaco = *espacos.declaracoes[idEspaco];
    for(int i = 0; i < espaco.campos.tam; i++) {
        int posCampo;
        TipoVariavel tipoCampo;
        posicaoDoCampo(idEspaco, espaco.campos[i].nome, &posCampo, &tipoCampo);
        emitirEscritaCampo(destino, posCampo, tipoCampo, imediatoInt(0), saida);
    }
}

// texto literal (ou car*) usado onde se espera um espaço: constrói um espaço temporário no frame via "Nome_criar" e agenda o "liberar" dele pro fim do comando.
inline ArgumentoIR GeradorIR::criarTemporarioDeValor(int idEspaco, ArgumentoIR valor, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    int bytes = ((tamanhoDoEspaco(idEspaco) + 15) / 16) * 16;
    int posArray = bytesArraysAtual;
    bytesArraysAtual += bytes;
    int slot = ctx.variaveisLocais.proximoSlot++;
    ComandoIR comandoEndereco;
    comandoEndereco.tipo = ComandoIR::Tipo::ENDERECO_FRAME;
    comandoEndereco.instrucaoEnderecoFrame.slotDestino = slot;
    comandoEndereco.instrucaoEnderecoFrame.posArray = posArray;
    saida.empurrar(comandoEndereco);
    ArgumentoIR temp = argumentoEspaco(slot, idEspaco);
    temp.espacoPorValor = true;
    emitirCriar(temp, valor, idEspaco, saida);
    registrarTemporario(slot, idEspaco, false, ctx);
    return temp;
}

// variável por valor cujo espaço tem "liberar": entra na lista de escopo.
inline void GeradorIR::registrarLiberacaoAutomatica(int slot, int idEspaco, ContextoFuncao& ctx) {
    std::string nome;
    if(!buscarMetodo(idEspaco, "liberar", nome)) return;
    ctx.liberarSlot.empurrar(slot);
    ctx.liberarId.empurrar(idEspaco);
}

// libera, em ordem inversa, as variáveis registradas a partir de "marca". "truncar" = false no "retorne" (o escopo continua valendo no outro caminho).
inline void GeradorIR::emitirLiberacoesDeEscopo(int marca, bool truncar, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    for(int i = ctx.liberarSlot.tam - 1; i >= marca; i--) {
        emitirLiberar(argumentoEspaco(ctx.liberarSlot[i], ctx.liberarId[i]), ctx.liberarId[i], saida);
    }
    if(truncar && ctx.liberarSlot.tam > marca) {
        ctx.liberarSlot.tam = marca;
        ctx.liberarId.tam = marca;
    }
}

inline void GeradorIR::registrarTemporario(int slot, int idEspaco, bool heap, ContextoFuncao& ctx) {
    ctx.tempSlot.empurrar(slot);
    ctx.tempId.empurrar(idEspaco);
    ctx.tempHeap.empurrar(heap);
}

inline void GeradorIR::removerTemporario(int slot, ContextoFuncao& ctx) {
    for(int i = 0; i < ctx.tempSlot.tam; i++) {
        if(ctx.tempSlot[i] != slot) continue;
        for(int j = i; j + 1 < ctx.tempSlot.tam; j++) {
            ctx.tempSlot[j] = ctx.tempSlot[j + 1];
            ctx.tempId[j] = ctx.tempId[j + 1];
            ctx.tempHeap[j] = ctx.tempHeap[j + 1];
        }
        ctx.tempSlot.tam--;
        ctx.tempId.tam--;
        ctx.tempHeap.tam--;
        return;
    }
}

// libera os temporários criados a partir de "marca" (os do comando atual): "liberar" do conteúdo e, se for de heap, "mliberar" da casca.
inline void GeradorIR::emitirLiberacaoTemporarios(int marca, bool truncar, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    for(int i = ctx.tempSlot.tam - 1; i >= marca; i--) {
        ArgumentoIR alvo = argumentoEspaco(ctx.tempSlot[i], ctx.tempId[i]);
        emitirLiberar(alvo, ctx.tempId[i], saida);
        if(ctx.tempHeap[i]) emitirMliberar(alvo, saida);
    }
    if(truncar && ctx.tempSlot.tam > marca) {
        ctx.tempSlot.tam = marca;
        ctx.tempId.tam = marca;
        ctx.tempHeap.tam = marca;
    }
}

inline bool GeradorIR::temConstrutor(int idEspaco) {
    std::string nome = std::string(espacos.declaracoes[idEspaco]->nome) + "_criar";
    int totalParametros;
    return funcoesUsuario.buscar(nome.c_str(), &totalParametros);
}

inline void GeradorIR::emitirAlocar(int slot, int idEspaco, Vetor<ComandoIR>& saida) {
    int totalParametros;
    if(!funcoesUsuario.buscar("malocar", &totalParametros)) {
        throw std::runtime_error("Objeto no heap exige 'malocar' (faltou #incluir a biblioteca de memória?)");
    }
    ComandoIR comando;
    comando.tipo = ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO;
    comando.chamadaFuncaoUsuario.iniciar();
    comando.chamadaFuncaoUsuario.nome = strdup("malocar");
    ArgumentoIR tamanho = imediatoInt(tamanhoDoEspaco(idEspaco));
    tamanho.larguraBytes = 8;
    comando.chamadaFuncaoUsuario.argumentos.empurrar(tamanho);
    comando.chamadaFuncaoUsuario.capturaResultado = true;
    comando.chamadaFuncaoUsuario.slotDestino = slot;
    comando.chamadaFuncaoUsuario.resultadoLargura = 8;
    saida.empurrar(comando);
}

// "Nome_criar(variavel, args...)": a variável já precisa estar definida na tabela
inline void GeradorIR::emitirConstrutor(const DeclaracaoVariavel& decl, int idEspaco, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    std::string nomeCriar = std::string(espacos.declaracoes[idEspaco]->nome) + "_criar";
    Vetor<Expressao> args;
    args.iniciar();
    Expressao este;
    este.tipo = Expressao::Tipo::IDENTIFICADOR;
    este.valor = strdup(decl.nome);
    args.empurrar(este);
    for(int i = 0; i < decl.listaInicial.tam; i++) args.empurrar(decl.listaInicial[i]);
    ComandoIR comando;
    comando.tipo = ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO;
    comando.chamadaFuncaoUsuario.iniciar();
    comando.chamadaFuncaoUsuario.nome = strdup(nomeCriar.c_str());
    resolverArgumentosChamada(nomeCriar.c_str(), args, comando.chamadaFuncaoUsuario.argumentos, saida, ctx);
    saida.empurrar(comando);
    free(este.valor);
    args.liberar();
}