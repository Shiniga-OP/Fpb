// util/ir/gerador_comandos.h
#pragma once
#include "geradorir.h"

// gera IR pra "retorne;" ou "retorne expr;", validando que a presença(ou
// ausência) de valor bate com o tipo de retorno declarado da função.
// "saida" e a lista de instruções da funcao(ou do bloco atual): recebe
// quaisquer comandos auxiliares que a resolucao da expressao precisar
// emitir ANTES do retorno(ex: uma chamada de funcao ou operacao
// aritmetica dentro de "retorne f() + 1;"). O ComandoIR::RETORNE em si
// e' empurrado por ultimo, igual todo outro "gerar*" deste arquivo.
inline void GeradorIR::gerarRetorne(const ComandoRetorne& comandoRetorne, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    std::string nomeStr = ctx.funcao.nome;
    if(ctx.funcao.tipoRetorno.ehNulo && comandoRetorne.temValor) {
        throw std::runtime_error("Função '" + nomeStr + "' é 'nulo' e não pode retornar um valor");
    }
    if(!ctx.funcao.tipoRetorno.ehNulo && !comandoRetorne.temValor) {
        throw std::runtime_error("Função '" + nomeStr + "' precisa retornar um valor do tipo declarado");
    }
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::RETORNE;
    comandoIR.instrucaoRetorne.temValor = comandoRetorne.temValor;
    if(comandoRetorne.temValor) {
        ArgumentoIR valorRetorno = resolverExpressao(comandoRetorne.valor, saida, ctx);
        if(valorRetorno.ehTemporario) removerTemporario(valorRetorno.registrador, ctx); // quem recebe vira dono
        valorRetorno = valorRepresentante(valorRetorno, ctx.funcao.tipoRetorno.tipo, saida, ctx);
        TipoVariavel tipoRetorno = ctx.funcao.tipoRetorno.tipo;
        comandoIR.instrucaoRetorne.valor = converterParaTipo(valorRetorno, tipoEhFlutuante(tipoRetorno), larguraDoTipo(tipoRetorno), saida, ctx);
    }
    emitirLiberacaoTemporarios(0, false, saida, ctx);
    emitirLiberacoesDeEscopo(0, false, saida, ctx);
    saida.empurrar(comandoIR);
}

// "int nome = expr;", "car* nome = expr;", array local ("int[10] nome;") ou, com valor de chamada de sistema,
// "nulo* p = #chamada_sistema id, ...;". Aloca um slot novo na tabela de variáveis locais e empurra em "saida" as
// instruções que materializam o valor nesse slot. A entrada na tabela é sempre REGISTRADOR (lê o slot em tempo de
// execução), nunca o literal resolvido: reatribuições futuras precisam ser enxergadas.
inline void GeradorIR::gerarDeclaracaoVariavel(const DeclaracaoVariavel& decl, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    int slot = ctx.variaveisLocais.proximoSlot++; // reserva já: temporários da expressão não podem pegar o mesmo slot
    ArgumentoIR referenciaSlot = argumentoSlot(slot, decl.tipo);
    
    if(decl.nomeEspaco) referenciaSlot.idEspaco = obterIdEspaco(decl.nomeEspaco);
    if(decl.ehArray) {
        if(decl.tamArray <= 0) throw std::runtime_error("Array precisa ter tamanho maior que zero");
        ctx.variaveisLocais.definirComSlot(decl.nome, referenciaSlot, slot);
        reservarNoFrame(decl.tamArray * larguraElementoApontado(decl.tipo), slot, saida);
        if(decl.temLista) {
            // "tipo[N] nome = { v0, v1, ... };": um ESCRITA_INDEXADA por elemento; os que a lista não cobre são zerados
            for(int i = 0; i < decl.tamArray; i++) {
                ArgumentoIR valor;
                if(i < decl.listaInicial.tam) valor = resolverExpressao(decl.listaInicial[i], saida, ctx);
                else valor = imediatoInt(0);
                valor = converterParaTipo(valor, referenciaSlot.elementoFlutuante, referenciaSlot.larguraElemento, saida, ctx);
                if(valor.tipo == ArgumentoIR::Tipo::IMEDIATO) valor.larguraBytes = referenciaSlot.larguraElemento;
                ComandoIR comandoEscrita;
                comandoEscrita.tipo = ComandoIR::Tipo::ESCRITA_INDEXADA;
                comandoEscrita.instrucaoEscritaIndexada.ponteiro = copiarArgumento(referenciaSlot);
                comandoEscrita.instrucaoEscritaIndexada.indice = imediatoInt(i);
                comandoEscrita.instrucaoEscritaIndexada.valor = valor;
                saida.empurrar(comandoEscrita);
            }
        }
        return;
    }
    if(decl.tipo == TipoVariavel::ESPACO) {
        // espaço por valor: o corpo mora na area de arrays do frame(mesmo mecanismo dos arrays locais) e o slot guarda o endereço base.
        // cada valor da lista vira uma escrita em base + posicao do campo.
        int id = referenciaSlot.idEspaco;
        const DeclaracaoEspaco& espaco = *espacos.declaracoes[id];
        bool listaDeElementos = decl.temLista && aceitaListaDeElementos(id);
        if(decl.temLista && !listaDeElementos && !temConstrutor(id) && decl.listaInicial.tam != espaco.campos.tam) {
            std::string nomeStr = espaco.nome;
            throw std::runtime_error("Espaço '" + nomeStr + "' tem " + std::to_string(espaco.campos.tam) + " campo(s), mas a lista de inicialização tem " + std::to_string(decl.listaInicial.tam) + " valor(es)");
        }
        referenciaSlot.espacoPorValor = true;
        ctx.variaveisLocais.definirComSlot(decl.nome, referenciaSlot, slot);
        reservarNoFrame(tamDoEspaco(id), slot, saida);
        registrarLiberacaoAutomatica(slot, id, ctx);
        if(listaDeElementos) {
            emitirListaElementos(decl.nome, decl.listaInicial, id, referenciaSlot, saida, ctx);
            return;
        }
        if(decl.temLista && temConstrutor(id)) {
            emitirConstrutor(decl, id, saida, ctx);
            return;
        }
        if(!decl.temLista && decl.listaInicial.tam == 1 && decl.listaInicial[0].tipo == Expressao::Tipo::NOVO && obterIdEspaco(decl.listaInicial[0].nomeFuncao) == id && temConstrutor(id)) {
            // "Nome x = novo Nome(args);": constrói direto na variável(sem alocar no heap nem copiar), igual a "Nome x = { args };"
            DeclaracaoVariavel construcao = decl;
            construcao.listaInicial = *decl.listaInicial[0].argumentosFuncao;
            emitirConstrutor(construcao, id, saida, ctx);
            return;
        }
        if(!decl.temLista && decl.listaInicial.tam == 1) {
            // "Nome x = expr;": se expr é um espaço do mesmo tipo, move(temporário) ou copia; senão chama o construtor "Nome_criar(x, expr)".
            ArgumentoIR valor = resolverExpressao(decl.listaInicial[0], saida, ctx);
            if(valor.tipo == ArgumentoIR::Tipo::REGISTRADOR && valor.idEspaco == id) {
                if(valor.ehTemporario) moverEspaco(valor, referenciaSlot, id, saida, ctx);
                else copiarObjeto(valor, referenciaSlot, id, saida, ctx);
                return;
            }
            emitirCriar(referenciaSlot, valor, id, saida);
            return;
        }
        if(!decl.temLista) {
            // "Nome x;": chama o construtor sem argumentos, se existir(só o "este"); senão zera os campos
            int totalParametrosCriar = 0;
            std::string nomeCriar = std::string(espacos.declaracoes[id]->nome) + "_.criar";
            if(funcoesUsuario.buscar(nomeCriar.c_str(), &totalParametrosCriar) && totalParametrosCriar == 1) {
                emitirConstrutor(decl, id, saida, ctx);
                return;
            }
            zerarEspaco(referenciaSlot, id, saida);
            return;
        }
        for(int i = 0; i < espaco.campos.tam; i++) {
            int posCampo;
            TipoVariavel tipoCampo;
            posicaoDoCampo(id, espaco.campos[i].nome, &posCampo, &tipoCampo);
            ArgumentoIR valor = resolverExpressao(decl.listaInicial[i], saida, ctx);
            valor = converterParaTipo(valor, tipoEhFlutuante(tipoCampo), larguraDoTipo(tipoCampo), saida, ctx);
            emitirEscritaCampo(referenciaSlot, posCampo, tipoCampo, valor, saida);
        }
        return;
    }
    if(decl.tipo == TipoVariavel::ESPACO_PONTEIRO && decl.temLista) {
        // "Nome* x = { ... };": objeto no heap. Quem libera é o "apagar", então não entra na lista de liberação de escopo.
        int id = referenciaSlot.idEspaco;
        const DeclaracaoEspaco& espaco = *espacos.declaracoes[id];
        if(!temConstrutor(id) && decl.listaInicial.tam != espaco.campos.tam) {
            std::string nomeStr = espaco.nome;
            throw std::runtime_error("Espaço '" + nomeStr + "' tem " + std::to_string(espaco.campos.tam) + " campo(s), mas a lista de inicialização tem " + std::to_string(decl.listaInicial.tam) + " valor(es)");
        }
        ctx.variaveisLocais.definirComSlot(decl.nome, referenciaSlot, slot);
        emitirAlocar(slot, id, saida);
        if(temConstrutor(id)) {
            emitirConstrutor(decl, id, saida, ctx);
            return;
        }
        for(int i = 0; i < espaco.campos.tam; i++) {
            int posCampo;
            TipoVariavel tipoCampo;
            posicaoDoCampo(id, espaco.campos[i].nome, &posCampo, &tipoCampo);
            ArgumentoIR valor = resolverExpressao(decl.listaInicial[i], saida, ctx);
            valor = converterParaTipo(valor, tipoEhFlutuante(tipoCampo), larguraDoTipo(tipoCampo), saida, ctx);
            emitirEscritaCampo(referenciaSlot, posCampo, tipoCampo, valor, saida);
        }
        return;
    }
    if(decl.ehChamadaSistema) {
        // a entrada na tabela precisa existir ANTES de gerar a chamada de
        // sistema(mesmo raciocínio do caso normal abaixo: leitura futura
        // tem que enxergar o slot, nao um valor congelado), mas o slot em
        // si so e definido de fato pelo ComandoIR::CHAMADA_SISTEMA_CRUA emitido a seguir(capturaResultado grava x0 nele apos o "svc").
        ctx.variaveisLocais.definirComSlot(decl.nome, referenciaSlot, slot);

        if(decl.chamadaSistemaInicial.identificadores.tam == 0) {
            throw std::runtime_error("#chamada_sistema precisa de ao menos o número da chamada de sistema");
        }
        ComandoIR comandoIR;
        comandoIR.tipo = ComandoIR::Tipo::CHAMADA_SISTEMA_CRUA;
        comandoIR.chamadaSistemaCrua.iniciar();
        for(int i = 0; i < decl.chamadaSistemaInicial.identificadores.tam; i++) {
            comandoIR.chamadaSistemaCrua.argumentos.empurrar(resolverExpressao(decl.chamadaSistemaInicial.identificadores[i], saida, ctx));
        }
        comandoIR.chamadaSistemaCrua.capturaResultado = true;
        comandoIR.chamadaSistemaCrua.slotDestino = slot;
        saida.empurrar(comandoIR);
        return;
    }
    ArgumentoIR valor = resolverExpressao(decl.valorInicial, saida, ctx);
    valor = valorRepresentante(valor, decl.tipo, saida, ctx);
    if(referenciaSlot.idEspaco >= 0 && valor.tipo == ArgumentoIR::Tipo::REGISTRADOR && valor.idEspaco >= 0 && !ehSubespaco(valor.idEspaco, referenciaSlot.idEspaco)) {
        std::string nomeStr = decl.nome;
        throw std::runtime_error("Variável '" + nomeStr + "' recebe um ponteiro de outro espaço");
    }
    valor = converterParaTipo(valor, referenciaSlot.ehFlutuante, larguraDoTipo(decl.tipo), saida, ctx);
    if(decl.ehFinal) {
        // constante: a entrada na tabela é o próprio IMEDIATO, então nenhum uso lê slot e nada é gravado
        if(valor.tipo != ArgumentoIR::Tipo::IMEDIATO) {
            std::string nomeStr = decl.nome;
            throw std::runtime_error("'final' exige um valor constante em tempo de compilação (variável '" + nomeStr + "')");
        }
        ctx.variaveisLocais.definirComSlot(decl.nome, constanteDoTipo(valor.imediato, decl.tipo), slot);
        return;
    }
    int larguraDestino = larguraDoTipo(decl.tipo); // a largura da gravacao no slot segue o tipo declarado da variavel, não o tipo da expressão
    if(valor.tipo == ArgumentoIR::Tipo::IMEDIATO) valor.larguraBytes = larguraDestino; // imediato não tem slot de origem: monta direto na largura do destino. Registrador mantém largura e sinal de origem pra ser carregado com a extensão certa

    // a entrada na tabela precisa ser sempre REGISTRADOR(lê o slot em tempo de execução), nunca o literal resolvido(SIMBOLO_TEXTO/IMEDIATO): o
    // slot pode ser sobrescrito por uma reatribuição mais adiante, e um
    // uso futuro da variável tem que enxergar o valor atual, não o valor
    // congelado da declaração. O "valor" resolvido serve so pra
    // materializar o conteúdo inicial no slot, no ComandoIR abaixo.
    ctx.variaveisLocais.definirComSlot(decl.nome, referenciaSlot, slot);

    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::REATRIBUICAO;
    comandoIR.instrucaoReatribuicao.slotDestino = slot;
    comandoIR.instrucaoReatribuicao.valor = valor;
    comandoIR.instrucaoReatribuicao.larguraDestino = larguraDestino;
    saida.empurrar(comandoIR);
}

// grava "valor"(já resolvido) na variável local "nome". Miolo de "nome = expr;" e de "nome = expr" usado como valor.
inline ArgumentoIR GeradorIR::gravarEmVariavel(const char* nome, ArgumentoIR valor, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ArgumentoIR existente;
    int slot;
    if(!ctx.variaveisLocais.buscar(nome, &existente, &slot)) {
        // nome solto dentro de um método: campo de instância ou estatico da classe de "este"
        ArgumentoIR este;
        if(ctx.variaveisLocais.buscar("este", &este) && este.idEspaco >= 0) {
            std::string simbolo;
            if(existeCampo(este.idEspaco, nome) || buscarEstatico(este.idEspaco, nome, simbolo)) {
                return gravarEmCampo("este", nome, valor, saida, ctx);
            }
        }
        std::string nomeStr = nome;
        throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
    }
    if(existente.tipo == ArgumentoIR::Tipo::IMEDIATO) {
        std::string nomeStr = nome;
        throw std::runtime_error("Variável '" + nomeStr + "' é final e não pode ser alterada");
    }
    if(existente.espacoPorValor) {
        std::string nomeStr = nome;
        int id = existente.idEspaco;
        if(valor.tipo == ArgumentoIR::Tipo::REGISTRADOR && valor.idEspaco >= 0 && valor.idEspaco != id) {
            throw std::runtime_error("Variável '" + nomeStr + "' só pode receber um espaço do mesmo tipo");
        }
        if(valor.tipo == ArgumentoIR::Tipo::REGISTRADOR && valor.idEspaco == id) {
            if(valor.registrador == slot) return valor; // x = x
            emitirLiberar(existente, id, saida);
            if(valor.ehTemporario) moverEspaco(valor, existente, id, saida, ctx);
            else copiarObjeto(valor, existente, id, saida, ctx);
            return valor;
        }
        emitirLiberar(existente, id, saida); // texto = "literal"
        emitirCriar(existente, valor, id, saida);
        return valor;
    }
    valor = valorRepresentante(valor, existente, saida, ctx);
    valor = converterParaTipo(valor, existente.ehFlutuante, existente.larguraBytes, saida, ctx);
    if(valor.tipo == ArgumentoIR::Tipo::IMEDIATO) valor.larguraBytes = existente.larguraBytes; // a gravação segue o tipo do slot já declarado

    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::REATRIBUICAO;
    comandoIR.instrucaoReatribuicao.slotDestino = slot;
    comandoIR.instrucaoReatribuicao.valor = valor;
    comandoIR.instrucaoReatribuicao.larguraDestino = existente.larguraBytes;
    saida.empurrar(comandoIR);
    return valor;
}

// "nome = expr;": resolve o lado direito e grava.
inline void GeradorIR::gerarReatribuicao(const Reatribuicao& reat, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ArgumentoIR valor = resolverExpressao(reat.valor, saida, ctx);
    gravarEmVariavel(reat.nome, valor, saida, ctx);
}

// grava "valor"(já resolvido) em ponteiro[indice]: converte pro tipo do elemento e emite a escrita.
inline ArgumentoIR GeradorIR::gravarEmIndexacao(const ArgumentoIR& ponteiro, const ArgumentoIR& indice, ArgumentoIR valor, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    if(ponteiro.idEspaco >= 0) {
        // "p[i] = valor" com "p" apontando pra espaço: copia o corpo pro elemento i. A memória do elemento ainda não tem conteúdo, então não há "liberar" do valor antigo.
        int id = ponteiro.idEspaco;
        if(valor.tipo != ArgumentoIR::Tipo::REGISTRADOR || valor.idEspaco < 0 || !ehSubespaco(valor.idEspaco, id)) {
            std::string nomeStr = espacos.declaracoes[id]->nome;
            throw std::runtime_error("O elemento de um vetor de '" + nomeStr + "' só pode receber um espaço do mesmo tipo");
        }
        ArgumentoIR destino = enderecoDoElementoEspaco(ponteiro, indice, saida, ctx);
        if(valor.ehTemporario) moverEspaco(valor, destino, id, saida, ctx);
        else copiarObjeto(valor, destino, id, saida, ctx);
        return valor;
    }
    valor = converterParaTipo(valor, ponteiro.elementoFlutuante, ponteiro.larguraElemento, saida, ctx);
    if(valor.tipo == ArgumentoIR::Tipo::IMEDIATO) valor.larguraBytes = ponteiro.larguraElemento;

    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::ESCRITA_INDEXADA;
    comandoIR.instrucaoEscritaIndexada.ponteiro = copiarArgumento(ponteiro);
    comandoIR.instrucaoEscritaIndexada.indice = indice;
    comandoIR.instrucaoEscritaIndexada.valor = valor;
    saida.empurrar(comandoIR);
    return valor;
}

// "nome[indice] = expr;"
inline void GeradorIR::gerarAtribuicaoIndexada(const AtribuicaoIndexada& atr, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    // "p[i] = e" com "p" campo de "este": vira "este.p[i] = e"
    const char* nomeBase = atr.nome;
    const char* campoBase = atr.campo;
    ArgumentoIR local;
    ArgumentoIR este;
    if(!atr.campo && !ctx.variaveisLocais.buscar(atr.nome, &local) && ctx.variaveisLocais.buscar("este", &este) && este.idEspaco >= 0 && existeCampo(este.idEspaco, atr.nome)) {
        nomeBase = "este";
        campoBase = atr.nome;
    }
    ArgumentoIR ponteiro = resolverPonteiroIndexado(nomeBase, campoBase, saida, ctx);
    ArgumentoIR indice = resolverExpressao(atr.indice, saida, ctx);
    ArgumentoIR valor = resolverExpressao(atr.valor, saida, ctx);
    gravarEmIndexacao(ponteiro, indice, valor, saida, ctx);
}

// gera IR pra "#chamada_sistema id, arg1, arg2, ...;" ou, com destino,
// "nome = #chamada_sistema id, arg1, arg2, ...;"(captura x0 apos o "svc"
// num slot ja existente, mesma ideia de resolverChamadaFuncaoComoValor
// pra "bl", so que "nome" precisa ja ter sido declarado antes: o
// slot(memoria) tem que existir pra receber o valor). "saida" recebe
// tanto os comandos auxiliares de cada argumento(ex: um "bl" se algum
// identificador for uma chamada de função) quanto o proprio comando final.
inline void GeradorIR::gerarChamadaSistemaCrua(const ChamadaSistemaCrua& chamada, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    if(chamada.identificadores.tam == 0) {
        throw std::runtime_error("#chamada_sistema precisa de ao menos o número da chamada de sistema");
    }
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::CHAMADA_SISTEMA_CRUA;
    comandoIR.chamadaSistemaCrua.iniciar();

    for(int i = 0; i < chamada.identificadores.tam; i++) {
        comandoIR.chamadaSistemaCrua.argumentos.empurrar(resolverExpressao(chamada.identificadores[i], saida, ctx));
    }
    if(chamada.temDestino) {
        ArgumentoIR existente;
        int slot;
        if(!ctx.variaveisLocais.buscar(chamada.nomeDestino, &existente, &slot)) {
            std::string nomeStr = chamada.nomeDestino;
            throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
        }
        if(existente.tipo == ArgumentoIR::Tipo::IMEDIATO) {
            std::string nomeStr = chamada.nomeDestino;
            throw std::runtime_error("Variável '" + nomeStr + "' é final e não pode ser alterada");
        }
        comandoIR.chamadaSistemaCrua.capturaResultado = true;
        comandoIR.chamadaSistemaCrua.slotDestino = slot;
    }
    saida.empurrar(comandoIR);
}

// gera IR pra "se(esquerda OP direita) { então } senao { senaoCorpo }".
// "saida" recebe os comandos auxiliares da condição(ex: chamada de
// função usada como operando) ANTES do proprio ComandoIR::SE, e por
// fim o ComandoIR::SE em si.
inline void GeradorIR::gerarComandoSe(const ComandoSe& comandoSe, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::SE;
    comandoIR.instrucaoSe.iniciar();
    resolverCondicao(comandoSe.condicao, saida, ctx, comandoIR.instrucaoSe.esquerda, comandoIR.instrucaoSe.operador, comandoIR.instrucaoSe.direita);
    
    int marcaEntao = ctx.variaveisLocais.abrirEscopo();
    int marcaLibEntao = ctx.liberarSlot.tam;
    gerarBlocoComandos(*comandoSe.entao, *comandoIR.instrucaoSe.entao, ctx);
    emitirLiberacoesDeEscopo(marcaLibEntao, true, *comandoIR.instrucaoSe.entao, ctx);
    ctx.variaveisLocais.fecharEscopo(marcaEntao);

    if(comandoSe.temSenao) {
        comandoIR.instrucaoSe.temSenao = true;
        comandoIR.instrucaoSe.senaoCorpo = new Vetor<ComandoIR>();
        comandoIR.instrucaoSe.senaoCorpo->iniciar();
        int marcaSenao = ctx.variaveisLocais.abrirEscopo();
        int marcaLibSenao = ctx.liberarSlot.tam;
        gerarBlocoComandos(*comandoSe.senaoCorpo, *comandoIR.instrucaoSe.senaoCorpo, ctx);
        emitirLiberacoesDeEscopo(marcaLibSenao, true, *comandoIR.instrucaoSe.senaoCorpo, ctx);
        ctx.variaveisLocais.fecharEscopo(marcaSenao);
    }
    saida.empurrar(comandoIR);
}

// gera IR pra "enquanto(esquerda OP direita) { corpo }". Diferente de gerarComandoSe, as instruções auxiliares da
// condição (ex: LEITURA_INDEXADA de "txt[i]") vão em comandosCondicao (reemitidas a cada iteração pela arquitetura),
// NÃO em "saida": senão a condição seria avaliada só uma vez antes do laço e nunca refletiria o "i" atualizado no corpo.
inline void GeradorIR::gerarComandoEnquanto(const ComandoEnquanto& comandoEnquanto, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::ENQUANTO;
    comandoIR.instrucaoEnquanto.iniciar();
    resolverCondicao(comandoEnquanto.condicao, *comandoIR.instrucaoEnquanto.comandosCondicao, ctx, comandoIR.instrucaoEnquanto.esquerda, comandoIR.instrucaoEnquanto.operador, comandoIR.instrucaoEnquanto.direita);
    
    int marca = ctx.variaveisLocais.abrirEscopo();
    int marcaLib = ctx.liberarSlot.tam;
    gerarBlocoComandos(*comandoEnquanto.corpo, *comandoIR.instrucaoEnquanto.corpo, ctx);
    emitirLiberacoesDeEscopo(marcaLib, true, *comandoIR.instrucaoEnquanto.corpo, ctx);
    ctx.variaveisLocais.fecharEscopo(marca);
    saida.empurrar(comandoIR);
}

// gera IR pra cada comando de um bloco("então" ou "senão" de um "se", ou
// corpo de um "enquanto"), empurrando na lista de saida já alocada pelo
// chamador. "ctx.funcao" e a declaracao da funcao que contem este bloco,
// usada so pra validar "retorne" aninhado(tipo bate com a assinatura).
inline void GeradorIR::gerarBlocoComandos(const Vetor<Comando>& corpo, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    for(int i = 0; i < corpo.tam; i++) {
        const Comando& comando = corpo[i];
        int marcaTemp = ctx.tempSlot.tam;
        if(comando.tipo == Comando::Tipo::DECLARACAO_VARIAVEL) {
            gerarDeclaracaoVariavel(comando.declaracaoVariavel, saida, ctx);
        } else if(comando.tipo == Comando::Tipo::REATRIBUICAO) {
            gerarReatribuicao(comando.reatribuicao, saida, ctx);
        } else if(comando.tipo == Comando::Tipo::CHAMADA_SISTEMA_CRUA) {
            gerarChamadaSistemaCrua(comando.chamadaSistemaCrua, saida, ctx);
        } else if(comando.tipo == Comando::Tipo::SE) {
            gerarComandoSe(comando.comandoSe, saida, ctx);
        } else if(comando.tipo == Comando::Tipo::ENQUANTO) {
            gerarComandoEnquanto(comando.comandoEnquanto, saida, ctx);
        } else if(comando.tipo == Comando::Tipo::POR) {
            gerarComandoPor(comando.comandoPor, saida, ctx);
        } else if(comando.tipo == Comando::Tipo::RETORNE) {
            gerarRetorne(comando.comandoRetorne, saida, ctx);
        } else if(comando.tipo == Comando::Tipo::ATRIBUICAO_INDEXADA) {
            gerarAtribuicaoIndexada(comando.atribuicaoIndexada, saida, ctx);
        } else if(comando.tipo == Comando::Tipo::ATRIBUICAO_CAMPO) {
            gerarAtribuicaoCampo(comando.atribuicaoCampo, saida, ctx);
        } else if(comando.tipo == Comando::Tipo::APAGAR) {
            gerarApagar(comando.comandoApagar, saida, ctx);
        } else if(comando.tipo == Comando::Tipo::EXPRESSAO) {
            resolverExpressao(comando.expressao, saida, ctx);
        } else {
            // "p.metodo(args);" com "p" campo de "este": reescreve como "este.p.metodo(args);"
            const ChamadaFuncao& original = comando.chamadaFuncao;
            const char* ponto = strchr(original.nome, '.');
            ArgumentoIR este;
            bool reescrever = false;
            std::string base;
            if(ponto && ponto != original.nome) {
                base = std::string(original.nome, ponto - original.nome);
                ArgumentoIR local;
                reescrever = !ctx.variaveisLocais.buscar(base.c_str(), &local) && ctx.variaveisLocais.buscar("este", &este) && este.idEspaco >= 0 && existeCampo(este.idEspaco, base.c_str());
            }
            if(reescrever) {
                Expressao identificadorEste;
                identificadorEste.tipo = Expressao::Tipo::IDENTIFICADOR;
                identificadorEste.valor = strdup("este");
                Expressao receptor;
                receptor.tipo = Expressao::Tipo::ACESSO_CAMPO;
                receptor.valor = strdup(base.c_str());
                receptor.esquerda = new Expressao(identificadorEste);
                receptor.direita = nullptr;
                ChamadaFuncao reescrita;
                reescrita.iniciar();
                reescrita.nome = strdup(ponto);
                reescrita.argumentos.empurrar(receptor);
                for(int a = 0; a < original.argumentos.tam; a++) reescrita.argumentos.empurrar(original.argumentos[a]);
                gerarChamadaFuncao(reescrita, saida, ctx);
                free(reescrita.nome);
                receptor.liberar();
                reescrita.argumentos.liberar();
            } else {
                gerarChamadaFuncao(comando.chamadaFuncao, saida, ctx);
            }
        }
        emitirLiberacaoTemporarios(marcaTemp, true, saida, ctx);
    }
}

// gera IR pra "por(inicio; condicao; passo) { corpo }" desdobrando em
// "inicio; enquanto(condicao) { corpo; passo; }": reaproveita o ENQUANTO do IR, então
// alocador de registradores e arquitetura não precisam saber que o "por" existe.
// Sem "continue" na linguagem, colocar o passo no fim do corpo é equivalente.
// A variável declarada no inicio só existe dentro do "por"(escopo de bloco).
inline void GeradorIR::gerarComandoPor(const ComandoPor& comandoPor, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    int marca = ctx.variaveisLocais.abrirEscopo(); // a variável do inicio só existe dentro do "por"
    int marcaPor = ctx.liberarSlot.tam;
    gerarBlocoComandos(*comandoPor.inicio, saida, ctx);
    int marcaCorpo = ctx.liberarSlot.tam;

    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::ENQUANTO;
    comandoIR.instrucaoEnquanto.iniciar();
    resolverCondicao(comandoPor.condicao, *comandoIR.instrucaoEnquanto.comandosCondicao, ctx, comandoIR.instrucaoEnquanto.esquerda, comandoIR.instrucaoEnquanto.operador, comandoIR.instrucaoEnquanto.direita);

    gerarBlocoComandos(*comandoPor.corpo, *comandoIR.instrucaoEnquanto.corpo, ctx);
    emitirLiberacoesDeEscopo(marcaCorpo, true, *comandoIR.instrucaoEnquanto.corpo, ctx);
    gerarBlocoComandos(*comandoPor.passo, *comandoIR.instrucaoEnquanto.corpo, ctx);
    ctx.variaveisLocais.fecharEscopo(marca);
    saida.empurrar(comandoIR);
    emitirLiberacoesDeEscopo(marcaPor, true, saida, ctx);
}
// "apagar alvo;": chama "Nome_liberar(alvo)"(se existir) e devolve a casca com "mliberar". Se o alvo é um temporário
// (ex: "apagar a + b;"), tira ele da lista de temporários: senão o fim do comando liberaria a mesma casca duas vezes.
inline void GeradorIR::gerarApagar(const ComandoApagar& comando, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ArgumentoIR alvo = resolverExpressao(comando.alvo, saida, ctx);
    if(alvo.tipo != ArgumentoIR::Tipo::REGISTRADOR || alvo.idEspaco < 0) {
        throw std::runtime_error("'apagar' só vale para um objeto (Nome*)");
    }
    if(alvo.espacoPorValor) {
        throw std::runtime_error("'apagar' em variável por valor: o liberar já é automático no fim do escopo");
    }
    if(alvo.ehTemporario) removerTemporario(alvo.registrador, ctx);
    emitirLiberar(alvo, alvo.idEspaco, saida);
    emitirMliberar(alvo, saida);
}