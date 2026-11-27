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

// monta a chamada de uma função de usuário("bl nome") com os argumentos dados(os ArgumentoIR passam a ser donos do que carregam).
inline ComandoIR GeradorIR::montarChamada(const char* nome, std::initializer_list<ArgumentoIR> args) {
    ComandoIR comando;
    comando.tipo = ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO;
    comando.chamadaFuncaoUsuario.iniciar();
    comando.chamadaFuncaoUsuario.nome = strdup(nome);
    for(const ArgumentoIR& arg : args) comando.chamadaFuncaoUsuario.argumentos.empurrar(arg);
    return comando;
}

inline void GeradorIR::emitirChamada(const char* nome, std::initializer_list<ArgumentoIR> args, Vetor<ComandoIR>& saida) {
    saida.empurrar(montarChamada(nome, args));
}

// grava o retorno de "comando"(x0 ou s0/d0) num slot novo, empurra o comando e devolve o REGISTRADOR desse slot.
inline ArgumentoIR GeradorIR::capturarChamada(ComandoIR& comando, TipoVariavel tipoRetorno, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    int slot = ctx.variaveisLocais.proximoSlot++;
    comando.chamadaFuncaoUsuario.capturaResultado = true;
    comando.chamadaFuncaoUsuario.slotDestino = slot;
    comando.chamadaFuncaoUsuario.resultadoLargura = larguraDoTipo(tipoRetorno);
    comando.chamadaFuncaoUsuario.resultadoFlutuante = tipoEhFlutuante(tipoRetorno);
    saida.empurrar(comando);
    return argumentoSlot(slot, tipoRetorno);
}

// reserva "bytes"(arredondado pra 16) na área de arrays do frame e deixa o endereço dela no slot.
inline void GeradorIR::reservarNoFrame(int bytes, int slot, Vetor<ComandoIR>& saida) {
    bytes = ((bytes + 15) / 16) * 16;
    ComandoIR comando;
    comando.tipo = ComandoIR::Tipo::ENDERECO_FRAME;
    comando.instrucaoEnderecoFrame.slotDestino = slot;
    comando.instrucaoEnderecoFrame.posArray = bytesArraysAtual;
    bytesArraysAtual += bytes;
    saida.empurrar(comando);
}

// "Nome_criar(destino, valor)": usada por "Nome x = valor;", "x = valor;" e pela conversão de texto literal em espaço.
inline void GeradorIR::emitirCriar(const ArgumentoIR& destino, ArgumentoIR valor, int idEspaco, Vetor<ComandoIR>& saida) {
    std::string nomeEspacoStr = espacos.declaracoes[idEspaco]->nome;
    std::string nomeCriar = nomeEspacoStr + "_.criar";
    int totalParametros;
    if(!funcoesUsuario.buscar(nomeCriar.c_str(), &totalParametros)) {
        throw std::runtime_error("Espaço '" + nomeEspacoStr + "' não tem o construtor '" + nomeCriar + "'");
    }
    if(totalParametros != 2) {
        throw std::runtime_error("O construtor '" + nomeCriar + "' precisa ter exatamente 1 parâmetro além de 'este'");
    }
    emitirChamada(nomeCriar.c_str(), {copiarArgumento(destino), valor}, saida);
}

// "Nome_liberar(alvo)", se o espaço tiver esse método.
inline void GeradorIR::emitirLiberar(const ArgumentoIR& alvo, int idEspaco, Vetor<ComandoIR>& saida) {
    // destrutores encadeados: o da classe primeiro, depois o de cada base
    for(int id = idEspaco; id >= 0; id = baseDoEspaco(id)) {
        std::string nome = std::string(espacos.declaracoes[id]->nome) + "_.liberar";
        int totalParametros;
        if(funcoesUsuario.buscar(nome.c_str(), &totalParametros)) emitirChamada(nome.c_str(), {copiarArgumento(alvo)}, saida);
    }
}

// "mliberar(alvo)": devolve a memória da casca de um temporário de heap.
inline void GeradorIR::emitirMliberar(const ArgumentoIR& alvo, Vetor<ComandoIR>& saida) {
    int totalParametros;
    if(!funcoesUsuario.buscar("mliberar", &totalParametros)) {
        throw std::runtime_error("Operador de espaço exige 'mliberar' (faltou #incluir a biblioteca de memória?)");
    }
    emitirChamada("mliberar", {copiarArgumento(alvo)}, saida);
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
        emitirChamada(nomeCopiar.c_str(), {copiarArgumento(destino), copiarArgumento(origem)}, saida);
        return;
    }
    bool temDestrutor = false;
    for(int id = idEspaco; id >= 0 && !temDestrutor; id = baseDoEspaco(id)) {
        std::string candidato = std::string(espacos.declaracoes[id]->nome) + "_.liberar";
        temDestrutor = funcoesUsuario.buscar(candidato.c_str(), &totalParametros);
    }
    if(temDestrutor) {
        throw std::runtime_error("Espaço '" + nomeEspacoStr + "' tem destrutor mas não tem 'copiar': a cópia rasa liberaria o mesmo buffer duas vezes");
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
    int slot = ctx.variaveisLocais.proximoSlot++;
    reservarNoFrame(tamDoEspaco(idEspaco), slot, saida);
    ArgumentoIR temp = argumentoEspaco(slot, idEspaco);
    temp.espacoPorValor = true;
    emitirCriar(temp, valor, idEspaco, saida);
    registrarTemporario(slot, idEspaco, false, ctx);
    return temp;
}

// variável por valor cujo espaço tem "liberar": entra na lista de escopo.
inline void GeradorIR::registrarLiberacaoAutomatica(int slot, int idEspaco, ContextoFuncao& ctx) {
    bool temDestrutor = false;
    for(int id = idEspaco; id >= 0 && !temDestrutor; id = baseDoEspaco(id)) {
        std::string candidato = std::string(espacos.declaracoes[id]->nome) + "_.liberar";
        int totalParametros;
        temDestrutor = funcoesUsuario.buscar(candidato.c_str(), &totalParametros);
    }
    if(!temDestrutor) return;
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
    std::string nome = std::string(espacos.declaracoes[idEspaco]->nome) + "_.criar";
    int totalParametros;
    return funcoesUsuario.buscar(nome.c_str(), &totalParametros);
}

inline void GeradorIR::emitirAlocar(int slot, int idEspaco, Vetor<ComandoIR>& saida) {
    int totalParametros;
    if(!funcoesUsuario.buscar("malocar", &totalParametros)) {
        throw std::runtime_error("Objeto no heap exige 'malocar' (faltou #incluir a biblioteca de memória?)");
    }
    ArgumentoIR tamanho = imediatoInt(tamDoEspaco(idEspaco));
    tamanho.larguraBytes = 8;
    ComandoIR comando = montarChamada("malocar", {tamanho});
    comando.chamadaFuncaoUsuario.capturaResultado = true;
    comando.chamadaFuncaoUsuario.slotDestino = slot;
    comando.chamadaFuncaoUsuario.resultadoLargura = 8;
    saida.empurrar(comando);
}

// "Nome_criar(variavel, args...)": a variável já precisa estar definida na tabela
inline void GeradorIR::emitirConstrutor(const DeclaracaoVariavel& decl, int idEspaco, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    std::string nomeCriar = std::string(espacos.declaracoes[idEspaco]->nome) + "_.criar";
    Vetor<Expressao> args;
    args.iniciar();
    Expressao este;
    este.tipo = Expressao::Tipo::IDENTIFICADOR;
    este.valor = strdup(decl.nome);
    args.empurrar(este);
    for(int i = 0; i < decl.listaInicial.tam; i++) args.empurrar(decl.listaInicial[i]);
    ComandoIR comando = montarChamada(nomeCriar.c_str(), {});
    resolverArgumentosChamada(nomeCriar.c_str(), args, comando.chamadaFuncaoUsuario.argumentos, saida, ctx);
    saida.empurrar(comando);
    free(este.valor);
    args.liberar();
}

// copia campo a campo o espaço cujo endereço base está em "origem" pro espaço cujo endereço base está em "destino". Cada campo vira
// um par LEITURA_INDEXADA/ESCRITA_INDEXADA(mesmo mecanismo de resolverAcessoCampo e emitirEscritaCampo), então não precisa de
// instrução nova no IR e o padding não é copiado. Serve pra "a = b", pro argumento por valor e, no futuro, pro retorno de espaço.
inline void GeradorIR::copiarEspaco(const ArgumentoIR& origem, const ArgumentoIR& destino, int idEspaco, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    const DeclaracaoEspaco& espaco = *espacos.declaracoes[idEspaco];
    for(int i = 0; i < espaco.campos.tam; i++) {
        int posCampo;
        TipoVariavel tipoCampo;
        posicaoDoCampo(idEspaco, espaco.campos[i].nome, &posCampo, &tipoCampo);
        int largura = larguraDoTipo(tipoCampo);

        int slot = ctx.variaveisLocais.proximoSlot++;
        ComandoIR leitura;
        leitura.tipo = ComandoIR::Tipo::LEITURA_INDEXADA;
        leitura.instrucaoLeituraIndexada.slotDestino = slot;
        leitura.instrucaoLeituraIndexada.ponteiro = copiarArgumento(origem);
        leitura.instrucaoLeituraIndexada.ponteiro.larguraElemento = largura;
        leitura.instrucaoLeituraIndexada.indice = imediatoInt(posCampo / largura);
        saida.empurrar(leitura);

        ArgumentoIR valor = argumentoSlot(slot, tipoCampo);
        emitirEscritaCampo(destino, posCampo, tipoCampo, valor, saida);
    }
}

// reserva um corpo novo do espaço "idEspaco" na área de arrays do frame de quem chama(mesmo mecanismo de gerarDeclaracaoVariavel),
// copia o conteúdo de "origem" pra lá e devolve o argumento com o endereço base da cópia. É o que o "Nome x" por valor recebe.
inline ArgumentoIR GeradorIR::criarCopiaEspaco(const ArgumentoIR& origem, int idEspaco, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    int slot = ctx.variaveisLocais.proximoSlot++;
    reservarNoFrame(tamDoEspaco(idEspaco), slot, saida);

    ArgumentoIR copia = copiarArgumento(origem);
    copia.registrador = slot;
    copia.espacoPorValor = true;
    copiarObjeto(origem, copia, idEspaco, saida, ctx);
    return copia;
}

// método marcado "@lista" da classe(ou de uma base). "nomeMetodo" recebe o nome dele(sem o prefixo da classe).
inline bool GeradorIR::buscarMetodoLista(int idEspaco, std::string& nomeMetodo) {
    for(int id = idEspaco; id >= 0; id = baseDoEspaco(id)) {
        const DeclaracaoEspaco& espaco = *espacos.declaracoes[id];
        for(int i = 0; i < espaco.metodos.tam; i++) {
            if(!espaco.metodos[i].ehLista) continue;
            nomeMetodo = espaco.metodos[i].nome;
            return true;
        }
    }
    return false;
}

// "Nome x = { a, b };" vale como lista de elementos quando a classe(ou uma base) tem um método "@lista". Senão continua sendo construtor ou um valor por campo.
inline bool GeradorIR::aceitaListaDeElementos(int idEspaco) {
    std::string nomeMetodo;
    return buscarMetodoLista(idEspaco, nomeMetodo);
}

// começa o espaço como em "Nome x;"(construtor sem argumentos, se existir, senão zera os campos) e emite "nome.metodoLista(item)" por item.
// Item "{...}" vira uma variável oculta do tipo do parâmetro do método, construída do mesmo jeito(recursivo).
inline void GeradorIR::emitirListaElementos(const char* nome, const Vetor<Expressao>& itens, int idEspaco, const ArgumentoIR& destino, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    int totalParametrosCriar = 0;
    std::string nomeCriar = std::string(espacos.declaracoes[idEspaco]->nome) + "_.criar";
    if(funcoesUsuario.buscar(nomeCriar.c_str(), &totalParametrosCriar) && totalParametrosCriar == 1) {
        DeclaracaoVariavel semArgumentos;
        semArgumentos.iniciar();
        semArgumentos.nome = strdup(nome);
        emitirConstrutor(semArgumentos, idEspaco, saida, ctx);
        free(semArgumentos.nome);
        semArgumentos.listaInicial.liberar();
    } else {
        zerarEspaco(destino, idEspaco, saida);
    }
    std::string nomeMetodo;
    buscarMetodoLista(idEspaco, nomeMetodo);
    std::string nomeFuncao;
    buscarMetodo(idEspaco, nomeMetodo.c_str(), nomeFuncao);
    const DeclaracaoFuncao* declaracao = nullptr;
    corposFuncoesUsuario.buscar(nomeFuncao.c_str(), &declaracao);
    std::string nomeChamada = std::string(nome) + "." + nomeMetodo;
    for(int i = 0; i < itens.tam; i++) {
        ChamadaFuncao chamada;
        chamada.iniciar();
        chamada.nome = strdup(nomeChamada.c_str());
        Expressao identificador;
        bool oculto = itens[i].tipo == Expressao::Tipo::LISTA;
        if(oculto) {
            if(!declaracao || declaracao->parametros[1].tipo != TipoVariavel::ESPACO || !declaracao->parametros[1].nomeEspaco) {
                throw std::runtime_error("Item '{ ... }' só vale quando o parâmetro de '" + nomeFuncao + "' é um espaço por valor");
            }
            std::string nomeOculto = "#lista" + std::to_string(contadorSimbolos++);
            DeclaracaoVariavel oculta;
            oculta.iniciar();
            oculta.tipo = TipoVariavel::ESPACO;
            oculta.nome = strdup(nomeOculto.c_str());
            oculta.nomeEspaco = strdup(declaracao->parametros[1].nomeEspaco);
            oculta.listaInicial = *itens[i].argumentosFuncao;
            oculta.temLista = true;
            gerarDeclaracaoVariavel(oculta, saida, ctx);
            free(oculta.nome);
            free(oculta.nomeEspaco);
            identificador.tipo = Expressao::Tipo::IDENTIFICADOR;
            identificador.valor = strdup(nomeOculto.c_str());
            chamada.argumentos.empurrar(identificador);
        } else {
            chamada.argumentos.empurrar(itens[i]);
        }
        gerarChamadaFuncao(chamada, saida, ctx);
        free(chamada.nome);
        if(oculto) free(identificador.valor);
        chamada.argumentos.liberar();
    }
}