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
        comandoIR.instrucaoRetorne.valor = resolverExpressao(comandoRetorne.valor, saida, ctx);
    }
    saida.empurrar(comandoIR);
}

// "int nome = expr;", "car* nome = expr;", array local ("int[10] nome;") ou, com valor de chamada de sistema,
// "nulo* p = #chamada_sistema id, ...;". Aloca um slot novo na tabela de variáveis locais e empurra em "saida" as
// instruções que materializam o valor nesse slot. A entrada na tabela é sempre REGISTRADOR (lê o slot em tempo de
// execução), nunca o literal resolvido: reatribuições futuras precisam ser enxergadas.
inline void GeradorIR::gerarDeclaracaoVariavel(const DeclaracaoVariavel& decl, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    int slot = ctx.variaveisLocais.proximoSlot++; // reserva já: temporários da expressão não podem pegar o mesmo slot
    ArgumentoIR referenciaSlot;
    referenciaSlot.tipo = ArgumentoIR::Tipo::REGISTRADOR;
    referenciaSlot.simbolo = nullptr;
    referenciaSlot.imediato = 0;
    referenciaSlot.registrador = slot;
    referenciaSlot.larguraBytes = larguraDoTipo(decl.tipo);
    referenciaSlot.sinalizado = tipoEhSinalizado(decl.tipo);
    referenciaSlot.elementoSinalizado = elementoEhSinalizado(decl.tipo);
    referenciaSlot.larguraElemento = larguraElementoApontado(decl.tipo); // so relevante se esta variavel for usada como ponteiro numa indexacao(nome[i]); ver comentario de larguraElementoApontado()
    
    if(decl.ehArray) {
        if(decl.tamArray <= 0) throw std::runtime_error("Array precisa ter tamanho maior que zero");
        int bytes = decl.tamArray * larguraElementoApontado(decl.tipo);
        bytes = ((bytes + 15) / 16) * 16;
        int posArray = bytesArraysAtual;
        bytesArraysAtual += bytes;
        ctx.variaveisLocais.definirComSlot(decl.nome, referenciaSlot, slot);
        ComandoIR comandoArray;
        comandoArray.tipo = ComandoIR::Tipo::ENDERECO_FRAME;
        comandoArray.instrucaoEnderecoFrame.slotDestino = slot;
        comandoArray.instrucaoEnderecoFrame.posArray = posArray;
        saida.empurrar(comandoArray);
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

// "nome = expr;": busca o slot já alocado na declaracao e empurra em
// "saida" as instruções que sobrescrevem o valor nesse mesmo slot.
inline void GeradorIR::gerarReatribuicao(const Reatribuicao& reat, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ArgumentoIR existente;
    int slot;
    if(!ctx.variaveisLocais.buscar(reat.nome, &existente, &slot)) {
        std::string nomeStr = reat.nome;
        throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
    }
    ArgumentoIR novoValor = resolverExpressao(reat.valor, saida, ctx);
    if(novoValor.tipo == ArgumentoIR::Tipo::IMEDIATO) novoValor.larguraBytes = existente.larguraBytes; // a gravação segue o tipo do slot já declarado, não o da expressão do lado direito(ver gerarDeclaracaoVariavel)

    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::REATRIBUICAO;
    comandoIR.instrucaoReatribuicao.slotDestino = slot;
    comandoIR.instrucaoReatribuicao.valor = novoValor;
    comandoIR.instrucaoReatribuicao.larguraDestino = existente.larguraBytes;
    saida.empurrar(comandoIR);
}

// "nome[indice] = expr;": grava "larguraElemento" bytes(1, 8, ou qualquer
// largura futura) no endereço nome+(indice*larguraElemento), decidido pelo
// tipo declarado do ponteiro(ver larguraElementoApontado()). "nome"
// precisa ser uma variável ja declarada(o ponteiro sendo indexado).
inline void GeradorIR::gerarAtribuicaoIndexada(const AtribuicaoIndexada& atr, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ArgumentoIR ponteiro;
    if(!ctx.variaveisLocais.buscar(atr.nome, &ponteiro)) {
        std::string nomeStr = atr.nome;
        throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
    }
    ArgumentoIR indice = resolverExpressao(atr.indice, saida, ctx);
    ArgumentoIR valor = resolverExpressao(atr.valor, saida, ctx);
    if(valor.tipo == ArgumentoIR::Tipo::IMEDIATO) valor.larguraBytes = ponteiro.larguraElemento; // registrador mantém a largura de origem: a arquitetura estende e grava só a largura do elemento

    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::ESCRITA_INDEXADA;
    comandoIR.instrucaoEscritaIndexada.ponteiro = copiarArgumento(ponteiro);
    comandoIR.instrucaoEscritaIndexada.indice = indice;
    comandoIR.instrucaoEscritaIndexada.valor = valor;
    saida.empurrar(comandoIR);
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
    gerarBlocoComandos(*comandoSe.entao, *comandoIR.instrucaoSe.entao, ctx);
    ctx.variaveisLocais.fecharEscopo(marcaEntao);

    if(comandoSe.temSenao) {
        comandoIR.instrucaoSe.temSenao = true;
        comandoIR.instrucaoSe.senaoCorpo = new Vetor<ComandoIR>();
        comandoIR.instrucaoSe.senaoCorpo->iniciar();
        int marcaSenao = ctx.variaveisLocais.abrirEscopo();
        gerarBlocoComandos(*comandoSe.senaoCorpo, *comandoIR.instrucaoSe.senaoCorpo, ctx);
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
    gerarBlocoComandos(*comandoEnquanto.corpo, *comandoIR.instrucaoEnquanto.corpo, ctx);
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
        } else {
            gerarChamadaFuncao(comando.chamadaFuncao, saida, ctx);
        }
    }
}

// gera IR pra uma chamada de função de usuário usada como COMANDO de
// topo(valor de retorno, se houver, e descartado): "bl nome". "saida"
// recebe os comandos auxiliares de cada argumento(ex: chamada de funcao
// aninhada) antes do comando final.
inline void GeradorIR::gerarChamadaFuncao(const ChamadaFuncao& chamada, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    int totalParametrosEsperado;
    if(!funcoesUsuario.buscar(chamada.nome, &totalParametrosEsperado)) {
        std::string nomeStr = chamada.nome;
        throw std::runtime_error("Função '" + nomeStr + "' não foi declarada (faltou #incluir a biblioteca correta?)");
    }
    if(chamada.argumentos.tam != totalParametrosEsperado) {
        std::string nomeStr = chamada.nome;
        throw std::runtime_error("Função '" + nomeStr + "' espera " + std::to_string(totalParametrosEsperado) + " argumento(s), recebeu " + std::to_string(chamada.argumentos.tam));
    }
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO;
    comandoIR.chamadaFuncaoUsuario.iniciar();
    comandoIR.chamadaFuncaoUsuario.nome = strdup(chamada.nome);
    for(int j = 0; j < chamada.argumentos.tam; j++) {
        comandoIR.chamadaFuncaoUsuario.argumentos.empurrar(resolverExpressao(chamada.argumentos[j], saida, ctx));
    }
    saida.empurrar(comandoIR);
}

// gera IR pra "por(inicio; condicao; passo) { corpo }" desdobrando em
// "inicio; enquanto(condicao) { corpo; passo; }": reaproveita o ENQUANTO do IR, então
// alocador de registradores e arquitetura não precisam saber que o "por" existe.
// Sem "continue" na linguagem, colocar o passo no fim do corpo é equivalente.
// A variável declarada no inicio só existe dentro do "por"(escopo de bloco).
inline void GeradorIR::gerarComandoPor(const ComandoPor& comandoPor, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    int marca = ctx.variaveisLocais.abrirEscopo(); // a variável do inicio só existe dentro do "por"
    gerarBlocoComandos(*comandoPor.inicio, saida, ctx);

    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::ENQUANTO;
    comandoIR.instrucaoEnquanto.iniciar();
    resolverCondicao(comandoPor.condicao, *comandoIR.instrucaoEnquanto.comandosCondicao, ctx, comandoIR.instrucaoEnquanto.esquerda, comandoIR.instrucaoEnquanto.operador, comandoIR.instrucaoEnquanto.direita);

    gerarBlocoComandos(*comandoPor.corpo, *comandoIR.instrucaoEnquanto.corpo, ctx);
    gerarBlocoComandos(*comandoPor.passo, *comandoIR.instrucaoEnquanto.corpo, ctx);
    ctx.variaveisLocais.fecharEscopo(marca);
    saida.empurrar(comandoIR);
}