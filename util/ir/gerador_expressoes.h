// util/ir/gerador_expressoes.h
#pragma once
#include "geradorir.h"

// resolve qualquer Expressao (literal, identificador, chamada de função, conversão, indexação, bytes ou binária) pra um
// ArgumentoIR com seu valor final. "saida" recebe as instruções auxiliares que a resolução precisar emitir ANTES do
// valor ficar pronto. É a ÚNICA rota de resolução de expressão do gerador.
inline ArgumentoIR GeradorIR::resolverExpressao(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    if(expressao.tipo == Expressao::Tipo::CONVERSAO) {
        return resolverConversao(expressao, saida, ctx);
    }
    if(expressao.tipo == Expressao::Tipo::CHAMADA_FUNCAO) {
        return resolverChamadaFuncaoComoValor(expressao, saida, ctx);
    }
    if(expressao.tipo == Expressao::Tipo::CHAMADA_SISTEMA) {
        return resolverChamadaSistemaComoValor(expressao, saida, ctx);
    }
    if(expressao.tipo == Expressao::Tipo::BYTES) {
        return resolverBytes(expressao, ctx.variaveisLocais);
    }
    if(expressao.tipo == Expressao::Tipo::TEXTO_LITERAL) {
        char* simbolo = gerarNomeSimbolo(expressao.valor);

        DadoEstaticoIR dado;
        dado.simbolo = strdup(simbolo);
        dado.valor = strdup(expressao.valor);
        dado.tamanho = (int)strlen(expressao.valor);
        ctx.dados.empurrar(dado);

        ArgumentoIR arg;
        arg.tipo = ArgumentoIR::Tipo::SIMBOLO_TEXTO;
        arg.simbolo = simbolo; // transfere posse pro ArgumentoIR
        arg.imediato = 0;
        return arg;
    }
    if(expressao.tipo == Expressao::Tipo::NUMERO_LITERAL) {
        ArgumentoIR arg;
        arg.tipo = ArgumentoIR::Tipo::IMEDIATO;
        arg.simbolo = nullptr;
        arg.imediato = atol(expressao.valor);
        // literal numerico solto(ex: "10" em "quociente * 10") e' tratado como "int"(32 bits) por padrao, o tipo "natural" da linguagem: se combinado com um "longo"(64 bits) numa operacao, a regra de promocao em resolverExpressao(BINARIA) ja expande o resultado pra 64 bits. Excecao: se o valor nao cabe em 32 bits com sinal, vira 8 bytes, senao o arm64 truncaria o imediato pra 32 bits ao montar com movz/movk em w
        arg.larguraBytes = (arg.imediato >= -2147483648L && arg.imediato <= 2147483647L) ? 4 : 8;
        return arg;
    }
    if(expressao.tipo == Expressao::Tipo::IDENTIFICADOR) {
        // identificador: precisa ser uma variável local já declarada.
        ArgumentoIR encontrado;
        if(!ctx.variaveisLocais.buscar(expressao.valor, &encontrado)) {
            std::string nomeStr = expressao.valor;
            throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
        }
        // cópia independente: quem chamou resolverExpressao e dono do resultado.
        return copiarArgumento(encontrado);
    }
    if(expressao.tipo == Expressao::Tipo::INDEXACAO) {
        return resolverIndexacao(expressao, saida, ctx);
    }
    // só sobra BINARIA daqui pra baixo.
    ArgumentoIR esquerda = resolverExpressao(*expressao.esquerda, saida, ctx);
    ArgumentoIR direita = resolverExpressao(*expressao.direita, saida, ctx);

    int slot = ctx.variaveisLocais.proximoSlot++;
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::OPERACAO_ARITMETICA;
    comandoIR.instrucaoOperacaoAritmetica.slotDestino = slot;
    comandoIR.instrucaoOperacaoAritmetica.esquerda = esquerda;
    comandoIR.instrucaoOperacaoAritmetica.operador = traduzirOperadorAritmetico(expressao.operadorAritmetico);
    comandoIR.instrucaoOperacaoAritmetica.direita = direita;
    saida.empurrar(comandoIR);

    // o resultado de uma operação aritmética usa a MAIOR largura entre
    // os dois operandos(regra de promoção): car(1)+int(4)
    // vira int(4), int(4)+longo(8) vira longo(8), evitando truncar um
    // calculo por engano quando um dos lados e mais largo que o outro.
    ArgumentoIR referenciaSlot;
    referenciaSlot.tipo = ArgumentoIR::Tipo::REGISTRADOR;
    referenciaSlot.simbolo = nullptr;
    referenciaSlot.imediato = 0;
    referenciaSlot.registrador = slot;
    referenciaSlot.larguraBytes = (esquerda.larguraBytes > direita.larguraBytes) ? esquerda.larguraBytes : direita.larguraBytes;
    return referenciaSlot;
}

// resolve "bytes(tipo)" ou "bytes(variavel)" pro tamanho em bytes,
// conhecido em tempo de compilação. Usa larguraDoTipo() como fonte
// unica de verdade sobre tamanho de tipo(ver comentario da funcao).
// vira sempre um IMEDIATO, igual um literal numerico escrito a mão.
inline ArgumentoIR GeradorIR::resolverBytes(const Expressao& expressao, TabelaVariaveisLocais& variaveisLocais) {
    long tamanho;
    if(expressao.bytesEhTipoLiteral) {
        tamanho = larguraDoTipo(expressao.bytesTipoLiteral);
    } else {
        ArgumentoIR encontrado;
        if(!variaveisLocais.buscar(expressao.bytesNomeVariavel, &encontrado, nullptr)) {
            std::string nomeStr = expressao.bytesNomeVariavel;
            throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada (usada em 'bytes(...)')");
        }
        tamanho = encontrado.larguraBytes;
    }
    ArgumentoIR arg;
    arg.tipo = ArgumentoIR::Tipo::IMEDIATO;
    arg.simbolo = nullptr;
    arg.imediato = tamanho;
    return arg;
}

// resolve "ponteiro[indice]" como valor de leitura: le "larguraElemento"
// bytes(1, 8, ou qualquer largura futura), decidido pelo tipo declarado
// do ponteiro(ver larguraElementoApontado()). Emite InstrucaoLeituraIndexada
// (carrega em tempo de execucao o valor no endereco ponteiro+(indice*larguraElemento))
// num slot temporario novo, e devolve uma referencia REGISTRADOR pra esse
// slot(mesmo padrao de resolverExpressao pra BINARIA/chamada de funcao).
// "ponteiro" precisa ser uma variavel já declarada(parâmetro ou local); indice pode ser qualquer expressão.
inline ArgumentoIR GeradorIR::resolverIndexacao(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ArgumentoIR ponteiro;
    if(!ctx.variaveisLocais.buscar(expressao.esquerda->valor, &ponteiro)) {
        std::string nomeStr = expressao.esquerda->valor;
        throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
    }
    ArgumentoIR indice = resolverExpressao(*expressao.direita, saida, ctx);

    int slot = ctx.variaveisLocais.proximoSlot++;
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::LEITURA_INDEXADA;
    comandoIR.instrucaoLeituraIndexada.slotDestino = slot;
    comandoIR.instrucaoLeituraIndexada.ponteiro = copiarArgumento(ponteiro);
    comandoIR.instrucaoLeituraIndexada.indice = indice;
    saida.empurrar(comandoIR);

    ArgumentoIR referenciaSlot;
    referenciaSlot.tipo = ArgumentoIR::Tipo::REGISTRADOR;
    referenciaSlot.simbolo = nullptr;
    referenciaSlot.imediato = 0;
    referenciaSlot.registrador = slot;
    referenciaSlot.larguraBytes = ponteiro.larguraElemento; // largura do valor lido segue a largura do elemento apontado pelo ponteiro
    return referenciaSlot;
}

// resolve uma conversão de tipo(conversão): (car)expr ou (int)expr. Resolve a
// subexpressao normalmente e devolve uma copia com "ehCar" ajustado pro
// tipo alvo da conversão; a truncagem de fato pra 1 byte acontece na
// arquitetura na hora de materializar/gravar esse valor(usa w/strb em
// vez de x/str quando ehCar == true), então aqui so remarcamos o tipo.
inline ArgumentoIR GeradorIR::resolverConversao(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ArgumentoIR valor = resolverExpressao(*expressao.esquerda, saida, ctx);
    if(valor.tipo == ArgumentoIR::Tipo::SIMBOLO_TEXTO) {
        throw std::runtime_error("Erro: nao e possivel converter um texto literal para outro tipo");
    }
    valor.larguraBytes = larguraDoTipo(expressao.tipoConversao);
    valor.larguraElemento = larguraElementoApontado(expressao.tipoConversao); // a conversão muda o tipo do ponteiro: "nome[indice]" após a conversão precisa usar o passo do tipo NOVO, nao do tipo antigo(ex: (int*)baseCar precisa indexar de 4 em 4, mesmo que baseCar fosse car*)
    if(valor.tipo == ArgumentoIR::Tipo::IMEDIATO) {
        // trunca em tempo de compilação quando o valor ja e conhecido,
        // mesma semântica do truncamento em tempo de execucao(w/strb/str32) usado pra registradores. longo/ponteiro(8 bytes) nao trunca.
        if(valor.larguraBytes == 1) valor.imediato = valor.imediato & 0xFF;
        else if(valor.larguraBytes == 4) valor.imediato = valor.imediato & 0xFFFFFFFFL;
    }
    return valor;
}

// resolve "#chamada_sistema id, ..." usada DENTRO de uma expressão (ex: "retorne #chamada_sistema ...;"), diferente de
// gerarChamadaSistemaCrua (comando de topo). Captura x0 num slot temporário novo e devolve REGISTRADOR (8 bytes).
inline ArgumentoIR GeradorIR::resolverChamadaSistemaComoValor(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    const ChamadaSistemaCrua& chamada = *expressao.chamadaSistema;
    if(chamada.identificadores.tam == 0) {
        throw std::runtime_error("#chamada_sistema precisa de ao menos o número da chamada de sistema");
    }
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::CHAMADA_SISTEMA_CRUA;
    comandoIR.chamadaSistemaCrua.iniciar();
    for(int i = 0; i < chamada.identificadores.tam; i++) {
        comandoIR.chamadaSistemaCrua.argumentos.empurrar(resolverExpressao(chamada.identificadores[i], saida, ctx));
    }
    int slot = ctx.variaveisLocais.proximoSlot++;
    comandoIR.chamadaSistemaCrua.capturaResultado = true;
    comandoIR.chamadaSistemaCrua.slotDestino = slot;
    saida.empurrar(comandoIR);

    ArgumentoIR referenciaSlot;
    referenciaSlot.tipo = ArgumentoIR::Tipo::REGISTRADOR;
    referenciaSlot.simbolo = nullptr;
    referenciaSlot.imediato = 0;
    referenciaSlot.registrador = slot;
    referenciaSlot.larguraBytes = 8; // retorno de chamada de sistema(ex: ponteiro do mmap, ou codigo de retorno) e sempre tratado como valor de 8 bytes
    return referenciaSlot;
}

// resolve "f(arg1, ...)" usada DENTRO de uma expressão (ex: "int x = f();"), diferente de gerarChamadaFuncao (comando de
// topo, valor descartado). Exige função de usuário não-"nulo". Emite "bl nome" com capturaResultado=true num slot novo
// e devolve uma referência REGISTRADOR pra esse slot.
inline ArgumentoIR GeradorIR::resolverChamadaFuncaoComoValor(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    std::string nomeStr = expressao.nomeFuncao;
    int totalParametrosEsperado;
    if(!funcoesUsuario.buscar(expressao.nomeFuncao, &totalParametrosEsperado)) {
        throw std::runtime_error("Função '" + nomeStr + "' não foi declarada (faltou #incluir a biblioteca correta?)");
    }
    if(expressao.argumentosFuncao->tam != totalParametrosEsperado) {
        throw std::runtime_error("Função '" + nomeStr + "' espera " + std::to_string(totalParametrosEsperado) + " argumento(s), recebeu " + std::to_string(expressao.argumentosFuncao->tam));
    }
    TipoRetorno tipoRetorno;
    funcoesUsuario.buscarTipoRetorno(expressao.nomeFuncao, &tipoRetorno);
    if(tipoRetorno.ehNulo) {
        throw std::runtime_error("Função '" + nomeStr + "' é 'nulo' e não pode ser usada como valor de expressão");
    }
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO;
    comandoIR.chamadaFuncaoUsuario.iniciar();
    comandoIR.chamadaFuncaoUsuario.nome = strdup(expressao.nomeFuncao);
    for(int j = 0; j < expressao.argumentosFuncao->tam; j++) {
        comandoIR.chamadaFuncaoUsuario.argumentos.empurrar(resolverExpressao((*expressao.argumentosFuncao)[j], saida, ctx));
    }
    int slot = ctx.variaveisLocais.proximoSlot++;
    int larguraResultado = larguraDoTipo(tipoRetorno.tipo);
    comandoIR.chamadaFuncaoUsuario.capturaResultado = true;
    comandoIR.chamadaFuncaoUsuario.slotDestino = slot;
    comandoIR.chamadaFuncaoUsuario.resultadoLargura = larguraResultado;
    saida.empurrar(comandoIR);

    ArgumentoIR referenciaSlot;
    referenciaSlot.tipo = ArgumentoIR::Tipo::REGISTRADOR;
    referenciaSlot.simbolo = nullptr;
    referenciaSlot.imediato = 0;
    referenciaSlot.registrador = slot;
    referenciaSlot.larguraBytes = larguraResultado;
    referenciaSlot.larguraElemento = larguraElementoApontado(tipoRetorno.tipo); // relevante se o retorno for usado como ponteiro indexado(ex: "malocar(10)[0]")
    return referenciaSlot;
}