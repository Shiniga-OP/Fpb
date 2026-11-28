// util/ir/gerador_ponteiros.h
#pragma once
#include "geradorir.h"

// "xs[i]" com "xs" de uma classe que tem campo "representante": indexa esse campo(vira "xs.dados[i]"). Sem representante devolve o ponteiro intacto.
inline ArgumentoIR GeradorIR::indexarRepresentante(const ArgumentoIR& ponteiro, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    if(ponteiro.idEspaco < 0) return ponteiro;
    const CampoEspaco* representante = campoRepresentante(ponteiro.idEspaco);
    if(!representante) return ponteiro;
    ArgumentoIR resultado = lerCampoDeBase(ponteiro, representante->nome, saida, ctx);
    resultado.ehPonteiro = true;
    resultado.larguraElemento = larguraElementoApontado(representante->tipo);
    resultado.elementoSinalizado = elementoEhSinalizado(representante->tipo);
    resultado.elementoFlutuante = elementoEhFlutuante(representante->tipo);
    return resultado;
}

// devolve o ponteiro sendo indexado: a variável "nome" ou, se "campo" não é nullptr, o valor do campo ponteiro "nome.campo".
// Pro campo, a largura do elemento vem do tipo declarado do campo.
inline ArgumentoIR GeradorIR::resolverPonteiroIndexado(const char* nome, const char* campo, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ArgumentoIR base;
    if(!ctx.variaveisLocais.buscar(nome, &base)) {
        // "p[i]" com "p" global escalar(car*, int*...): lê o ponteiro da memória
        CampoEstatico global;
        std::string simbolo;
        if(!campo && buscarGlobalEscalar(nome, global, simbolo)) return resolverAcessoEstatico(simbolo, global, saida, ctx);
        std::string nomeStr = nome;
        throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
    }
    if(!campo) return indexarRepresentante(base, saida, ctx);
    if(base.idEspaco < 0) {
        std::string nomeStr = nome;
        throw std::runtime_error("Variável '" + nomeStr + "' não é um espaço, não tem campos");
    }
    int posCampo;
    TipoVariavel tipoCampo;
    posicaoDoCampo(base.idEspaco, campo, &posCampo, &tipoCampo);
    if(!tipoEhPonteiro(tipoCampo)) {
        std::string campoStr = campo;
        throw std::runtime_error("O campo '" + campoStr + "' não é um ponteiro, não pode ser indexado");
    }
    // expressões temporárias na pilha: os char* são emprestados, então não chama liberar()
    Expressao identificador;
    identificador.tipo = Expressao::Tipo::IDENTIFICADOR;
    identificador.valor = (char*)nome;
    identificador.esquerda = nullptr;
    identificador.direita = nullptr;
    Expressao acesso;
    acesso.tipo = Expressao::Tipo::ACESSO_CAMPO;
    acesso.valor = (char*)campo;
    acesso.esquerda = &identificador;
    acesso.direita = nullptr;
    ArgumentoIR ponteiro = resolverExpressao(acesso, saida, ctx);
    ponteiro.ehPonteiro = true;
    ponteiro.larguraElemento = larguraElementoApontado(tipoCampo);
    ponteiro.elementoSinalizado = elementoEhSinalizado(tipoCampo);
    ponteiro.elementoFlutuante = elementoEhFlutuante(tipoCampo);
    const CampoEspaco* representante = campoRepresentante(base.idEspaco);
    if(representante && strcmp(representante->nome, campo) == 0) return ponteiro; // "xs.dados[i]": o próprio representante é o array cru
    return indexarRepresentante(ponteiro, saida, ctx);
}

// ponteiro sendo indexado em "alvo[i]": variável, "nome.campo" ou "qualquer.coisa.campo".
inline ArgumentoIR GeradorIR::resolverPonteiroDaExpressao(const Expressao& e, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    if(e.tipo == Expressao::Tipo::IDENTIFICADOR) return resolverPonteiroIndexado(e.valor, nullptr, saida, ctx);
    if(e.tipo != Expressao::Tipo::ACESSO_CAMPO) return indexarRepresentante(resolverExpressao(e, saida, ctx), saida, ctx);
    if(e.esquerda->tipo == Expressao::Tipo::IDENTIFICADOR) return resolverPonteiroIndexado(e.esquerda->valor, e.valor, saida, ctx);
    ArgumentoIR base = resolverExpressao(*e.esquerda, saida, ctx);
    ArgumentoIR ponteiro = lerCampoDeBase(base, e.valor, saida, ctx);
    int posCampo;
    TipoVariavel tipoCampo;
    posicaoDoCampo(base.idEspaco, e.valor, &posCampo, &tipoCampo);
    if(!tipoEhPonteiro(tipoCampo)) {
        std::string campoStr = e.valor;
        throw std::runtime_error("O campo '" + campoStr + "' não é um ponteiro, não pode ser indexado");
    }
    ponteiro.ehPonteiro = true;
    ponteiro.larguraElemento = larguraElementoApontado(tipoCampo);
    ponteiro.elementoSinalizado = elementoEhSinalizado(tipoCampo);
    ponteiro.elementoFlutuante = elementoEhFlutuante(tipoCampo);
    const CampoEspaco* representante = campoRepresentante(base.idEspaco);
    if(representante && strcmp(representante->nome, e.valor) == 0) return ponteiro;
    return indexarRepresentante(ponteiro, saida, ctx);
}

// "p[i]" com "p" apontando pra espaço(Nome*): os corpos ficam um depois do outro, então o elemento "i" mora em p + i * tamDoEspaco.
// Devolve o endereço dele como um Nome*(mesmo idEspaco do ponteiro): serve pra leitura("a.b[i].campo") e pra escrita(cópia do corpo em gravarEmIndexacao).
inline ArgumentoIR GeradorIR::enderecoDoElementoEspaco(const ArgumentoIR& ponteiro, const ArgumentoIR& indice, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    long tamanho = tamDoEspaco(ponteiro.idEspaco);
    ArgumentoIR deslocamento;
    if(indice.tipo == ArgumentoIR::Tipo::IMEDIATO && !indice.ehFlutuante) {
        deslocamento = imediatoInt(indice.imediato * tamanho);
    } else {
        ArgumentoIR passo = imediatoInt(tamanho);
        passo.larguraBytes = 8; // promove o produto pra 64 bits
        int slotProduto = ctx.variaveisLocais.proximoSlot++;
        ComandoIR comandoProduto;
        comandoProduto.tipo = ComandoIR::Tipo::OPERACAO_ARITMETICA;
        comandoProduto.instrucaoOperacaoAritmetica.slotDestino = slotProduto;
        comandoProduto.instrucaoOperacaoAritmetica.esquerda = indice;
        comandoProduto.instrucaoOperacaoAritmetica.operador = traduzirOperadorAritmetico(OperadorAritmetico::MULTIPLICACAO);
        comandoProduto.instrucaoOperacaoAritmetica.direita = passo;
        saida.empurrar(comandoProduto);
        deslocamento = argumentoSlotLargura(slotProduto, 8);
    }
    deslocamento.larguraBytes = 8;
    ArgumentoIR base = copiarArgumento(ponteiro);
    base.larguraBytes = 8;
    base.espacoPorValor = false;
    base.ehPonteiro = true;
    int slot = ctx.variaveisLocais.proximoSlot++;
    ComandoIR comandoSoma;
    comandoSoma.tipo = ComandoIR::Tipo::OPERACAO_ARITMETICA;
    comandoSoma.instrucaoOperacaoAritmetica.slotDestino = slot;
    comandoSoma.instrucaoOperacaoAritmetica.esquerda = base;
    comandoSoma.instrucaoOperacaoAritmetica.operador = OperadorAritmeticoIR::SOMA;
    comandoSoma.instrucaoOperacaoAritmetica.direita = deslocamento;
    saida.empurrar(comandoSoma);

    ArgumentoIR elemento = argumentoSlotLargura(slot, 8);
    elemento.ehPonteiro = true;
    elemento.idEspaco = ponteiro.idEspaco;
    return elemento;
}

// resolve "ponteiro[indice]" como valor de leitura: le "larguraElemento"
// bytes(1, 8, ou qualquer largura futura), decidido pelo tipo declarado
// do ponteiro(ver larguraElementoApontado()). Emite InstrucaoLeituraIndexada
// (carrega em tempo de execucao o valor no endereco ponteiro+(indice*larguraElemento))
// num slot temporario novo, e devolve uma referencia REGISTRADOR pra esse
// slot(mesmo padrao de resolverExpressao pra BINARIA/chamada de funcao).
// "ponteiro" precisa ser uma variavel já declarada(parâmetro ou local); indice pode ser qualquer expressão.
inline ArgumentoIR GeradorIR::resolverIndexacao(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ArgumentoIR ponteiro = resolverPonteiroDaExpressao(*expressao.esquerda, saida, ctx);
    ArgumentoIR indice = resolverExpressao(*expressao.direita, saida, ctx);
    if(ponteiro.idEspaco >= 0) return enderecoDoElementoEspaco(ponteiro, indice, saida, ctx); // ponteiro pra espaço: o elemento é o corpo inteiro, não um valor de 8 bytes

    int slot = ctx.variaveisLocais.proximoSlot++;
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::LEITURA_INDEXADA;
    comandoIR.instrucaoLeituraIndexada.slotDestino = slot;
    comandoIR.instrucaoLeituraIndexada.ponteiro = copiarArgumento(ponteiro);
    comandoIR.instrucaoLeituraIndexada.indice = indice;
    saida.empurrar(comandoIR);

    ArgumentoIR referenciaSlot = argumentoSlotLargura(slot, ponteiro.larguraElemento, ponteiro.elementoFlutuante); // largura do valor lido segue a largura do elemento apontado pelo ponteiro
    referenciaSlot.sinalizado = ponteiro.elementoSinalizado;
    return referenciaSlot;
}

// resolve "@(expr)" e "@nome".
// "@(expr)": o valor de expr passa a ser tratado como endereço(8 bytes), sem instrução nova: serve pra "nulo* f = @(0x1000);" e pra converter um número em endereço.
// "@nome": se "nome" é ponteiro ou espaço, o slot dele já guarda um endereço, então o valor é o próprio conteúdo do slot. Se é escalar(int, car, longo...), o valor
// é o endereço do slot na pilha: emite ENDERECO_SLOT, e o alocador de registradores deixa esse slot sempre na pilha(registrador físico não tem endereço).
// Se "nome" é global escalar, o valor é o endereço dela no .data.
inline ArgumentoIR GeradorIR::resolverEndereco(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    if(expressao.enderecoDeValor) {
        ArgumentoIR valor = resolverExpressao(*expressao.esquerda, saida, ctx);
        if(valor.tipo == ArgumentoIR::Tipo::REGISTRADOR && valor.larguraBytes < 8) {
            // alargar pra 64 bits: copia pra um slot novo, estendendo pelo sinal/largura de origem(mesmo caminho de resolverConversao)
            int slot = ctx.variaveisLocais.proximoSlot++;
            ComandoIR comandoCopia;
            comandoCopia.tipo = ComandoIR::Tipo::REATRIBUICAO;
            comandoCopia.instrucaoReatribuicao.slotDestino = slot;
            comandoCopia.instrucaoReatribuicao.valor = valor;
            comandoCopia.instrucaoReatribuicao.larguraDestino = 8;
            saida.empurrar(comandoCopia);
            valor.registrador = slot;
        }
        valor.larguraBytes = 8;
        valor.idEspaco = -1;
        valor.espacoPorValor = false;
        valor.ehPonteiro = true;
        return valor;
    }
    if(expressao.esquerda->tipo == Expressao::Tipo::ACESSO_CAMPO) {
        // "@nome.campo": endereço base do espaço + posição do campo, via SOMA(mesmo caminho de "ponteiro + int")
        const Expressao& acesso = *expressao.esquerda;
        ArgumentoIR base;
        if(!ctx.variaveisLocais.buscar(acesso.esquerda->valor, &base)) {
            std::string nomeStr = acesso.esquerda->valor;
            throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
        }
        if(base.idEspaco < 0) {
            std::string nomeStr = acesso.esquerda->valor;
            throw std::runtime_error("Variável '" + nomeStr + "' não é um espaço, não tem campos");
        }
        int posCampo;
        TipoVariavel tipoCampo;
        posicaoDoCampo(base.idEspaco, acesso.valor, &posCampo, &tipoCampo);
        ArgumentoIR enderecoBase = copiarArgumento(base);
        enderecoBase.larguraBytes = 8;
        enderecoBase.espacoPorValor = false;
        enderecoBase.ehPonteiro = true;
        int slotCampo = ctx.variaveisLocais.proximoSlot++;
        ComandoIR comandoSoma;
        comandoSoma.tipo = ComandoIR::Tipo::OPERACAO_ARITMETICA;
        comandoSoma.instrucaoOperacaoAritmetica.slotDestino = slotCampo;
        comandoSoma.instrucaoOperacaoAritmetica.esquerda = enderecoBase;
        comandoSoma.instrucaoOperacaoAritmetica.operador = OperadorAritmeticoIR::SOMA;
        comandoSoma.instrucaoOperacaoAritmetica.direita = imediatoInt(posCampo);
        saida.empurrar(comandoSoma);
        
        ArgumentoIR referenciaCampo = argumentoSlotLargura(slotCampo, 8);
        referenciaCampo.ehPonteiro = true;
        referenciaCampo.larguraElemento = larguraDoTipo(tipoCampo);
        referenciaCampo.elementoSinalizado = tipoEhSinalizado(tipoCampo);
        referenciaCampo.elementoFlutuante = tipoEhFlutuante(tipoCampo);
        return referenciaCampo;
    }
    ArgumentoIR variavel;
    int slotVariavel;
    if(!ctx.variaveisLocais.buscar(expressao.esquerda->valor, &variavel, &slotVariavel)) {
        CampoEstatico global;
        std::string simbolo;
        if(buscarGlobalEscalar(expressao.esquerda->valor, global, simbolo)) {
            // "@global": endereço dela no .data, que "nome[i]" percorre de largura em largura do tipo dela
            ArgumentoIR endereco = enderecoDeGlobal(simbolo, global, saida, ctx);
            endereco.elementoSinalizado = tipoEhSinalizado(global.tipo);
            endereco.elementoFlutuante = tipoEhFlutuante(global.tipo);
            return endereco;
        }
        int totalParametrosIgnorado;
        if(funcoesUsuario.buscar(expressao.esquerda->valor, &totalParametrosIgnorado)) {
            // "@funcao": endereço do símbolo da função(adrp/add, o mesmo caminho do endereço de um texto literal)
            ArgumentoIR enderecoFuncao;
            enderecoFuncao.tipo = ArgumentoIR::Tipo::SIMBOLO_TEXTO;
            enderecoFuncao.simbolo = strdup(expressao.esquerda->valor);
            enderecoFuncao.imediato = 0;
            enderecoFuncao.larguraBytes = 8;
            enderecoFuncao.ehPonteiro = true;
            enderecoFuncao.ehFuncao = true;
            return enderecoFuncao;
        }
        std::string nomeStr = expressao.esquerda->valor;
        throw std::runtime_error("'" + nomeStr + "' não é uma variável nem uma função");
    }
    if(variavel.ehPonteiro || variavel.idEspaco >= 0) {
        ArgumentoIR endereco = copiarArgumento(variavel);
        endereco.larguraBytes = 8;
        endereco.espacoPorValor = false;
        endereco.ehPonteiro = true;
        return endereco;
    }
    if(variavel.tipo == ArgumentoIR::Tipo::IMEDIATO) {
        std::string nomeStr = expressao.esquerda->valor;
        throw std::runtime_error("Variável '" + nomeStr + "' é final: constante não tem endereço");
    }
    int slotDestino = ctx.variaveisLocais.proximoSlot++;
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::ENDERECO_SLOT;
    comandoIR.instrucaoEnderecoSlot.slotDestino = slotDestino;
    comandoIR.instrucaoEnderecoSlot.slotOrigem = slotVariavel;
    saida.empurrar(comandoIR);

    ArgumentoIR referenciaSlot = argumentoSlotLargura(slotDestino, 8);
    referenciaSlot.ehPonteiro = true;
    referenciaSlot.larguraElemento = variavel.larguraBytes; // "nome[i]" sobre o endereço de um escalar anda de largura em largura do próprio escalar
    referenciaSlot.elementoSinalizado = variavel.sinalizado;
    referenciaSlot.elementoFlutuante = variavel.ehFlutuante;
    return referenciaSlot;
}