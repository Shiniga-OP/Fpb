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
    if(expressao.tipo == Expressao::Tipo::ACESSO_CAMPO) {
        return resolverAcessoCampo(expressao, saida, ctx);
    }
    if(expressao.tipo == Expressao::Tipo::ENDERECO) {
        return resolverEndereco(expressao, saida, ctx);
    }
    if(expressao.tipo == Expressao::Tipo::INDEXACAO) {
        return resolverIndexacao(expressao, saida, ctx);
    }
    // só sobra BINARIA daqui pra baixo.
    ArgumentoIR esquerda = resolverExpressao(*expressao.esquerda, saida, ctx);
    ArgumentoIR direita = resolverExpressao(*expressao.direita, saida, ctx);
    if(esquerda.tipo == ArgumentoIR::Tipo::REGISTRADOR && esquerda.idEspaco >= 0) {
        // operando esquerdo é um espaço(ou ponteiro pra um): "a + b" vira "Nome_somar(a, b)", e assim por diante.
        return resolverOperadorEspaco(expressao.operadorAritmetico, esquerda, direita, saida, ctx);
    }

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
    int larguraResultado = (esquerda.larguraBytes > direita.larguraBytes) ? esquerda.larguraBytes : direita.larguraBytes;
    // deslocamento: a largura do resultado e a do operando da esquerda(a quantidade deslocada nao promove nada). Tem que bater com a regra de emitirOperacaoAritmetica.
    if(expressao.operadorAritmetico == OperadorAritmetico::DESL_ESQ || expressao.operadorAritmetico == OperadorAritmetico::DESL_DIR) larguraResultado = esquerda.larguraBytes;
    if(larguraResultado < 4) larguraResultado = 4; // promoção pra int, como em Java: byte/curto/car/bool operados viram int
    referenciaSlot.larguraBytes = larguraResultado;
    return referenciaSlot;
}

// nome do método que implementa cada operador num espaço/classe: "a + b" chama "Nome_somar(a, b)".
inline const char* nomeMetodoOperador(OperadorAritmetico op) {
    if(op == OperadorAritmetico::SOMA) return "somar";
    if(op == OperadorAritmetico::SUBTRACAO) return "subtrair";
    if(op == OperadorAritmetico::MULTIPLICACAO) return "multiplicar";
    if(op == OperadorAritmetico::DIVISAO) return "dividir";
    if(op == OperadorAritmetico::MODULO) return "modulo";
    if(op == OperadorAritmetico::E_BIT) return "e_bit";
    if(op == OperadorAritmetico::OU_BIT) return "ou_bit";
    if(op == OperadorAritmetico::XOU) return "xou";
    if(op == OperadorAritmetico::DESL_ESQ) return "desl_esq";
    return "desl_dir";
}

// "esquerda OP direita" quando esquerda é um espaço: emite a chamada de "Nome_operador(esquerda, direita)"(o método tem "este" + 1 parâmetro).
// Se o método devolve "Nome*", o resultado carrega o idEspaco, então "x = a + b" copia campo a campo e "Nome x = a + b;" também.
inline ArgumentoIR GeradorIR::resolverOperadorEspaco(OperadorAritmetico op, ArgumentoIR esquerda, ArgumentoIR direita, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    int idEspaco = esquerda.idEspaco;
    std::string nomeEspacoStr = espacos.declaracoes[idEspaco]->nome;
    std::string nomeFuncao = nomeEspacoStr + "_" + nomeMetodoOperador(op);
    int totalParametros;
    if(!funcoesUsuario.buscar(nomeFuncao.c_str(), &totalParametros)) {
        throw std::runtime_error("Espaço '" + nomeEspacoStr + "' não tem o método '" + nomeFuncao + "' (necessário pra esse operador)");
    }
    if(totalParametros != 2) {
        throw std::runtime_error("O método '" + nomeFuncao + "' precisa ter exatamente 1 parâmetro além de 'este'");
    }
    TipoRetorno tipoRetorno;
    funcoesUsuario.buscarTipoRetorno(nomeFuncao.c_str(), &tipoRetorno);
    if(tipoRetorno.ehNulo) {
        throw std::runtime_error("O método '" + nomeFuncao + "' é 'nulo' e não pode ser usado como operador");
    }
    const DeclaracaoFuncao* declaracao = nullptr;
    corposFuncoesUsuario.buscar(nomeFuncao.c_str(), &declaracao);
    if(declaracao && declaracao->parametros[1].tipo == TipoVariavel::ESPACO) {
        int idParametro = obterIdEspaco(declaracao->parametros[1].nomeEspaco);
        if(direita.tipo != ArgumentoIR::Tipo::REGISTRADOR || direita.idEspaco != idParametro) {
            throw std::runtime_error("O operando direito de '" + nomeFuncao + "' precisa ser um espaço do tipo esperado");
        }
        direita = criarCopiaEspaco(direita, idParametro, saida, ctx);
    }
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO;
    comandoIR.chamadaFuncaoUsuario.iniciar();
    comandoIR.chamadaFuncaoUsuario.nome = strdup(nomeFuncao.c_str());
    comandoIR.chamadaFuncaoUsuario.argumentos.empurrar(esquerda);
    comandoIR.chamadaFuncaoUsuario.argumentos.empurrar(direita);
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
    referenciaSlot.sinalizado = tipoEhSinalizado(tipoRetorno.tipo);
    referenciaSlot.elementoSinalizado = elementoEhSinalizado(tipoRetorno.tipo);
    referenciaSlot.larguraElemento = larguraElementoApontado(tipoRetorno.tipo);
    referenciaSlot.ehPonteiro = tipoEhPonteiro(tipoRetorno.tipo);
    if(tipoRetorno.tipo == TipoVariavel::ESPACO_PONTEIRO) referenciaSlot.idEspaco = idEspaco;
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
        if(variaveisLocais.buscar(expressao.bytesNomeVariavel, &encontrado, nullptr)) {
            // espaço por valor: o tamanho do corpo, não os 8 bytes do slot que guarda o endereço
            if(encontrado.espacoPorValor && encontrado.idEspaco >= 0) tamanho = tamanhoDoEspaco(encontrado.idEspaco);
            else tamanho = encontrado.larguraBytes;
        } else {
            // nome de espaço/classe: "bytes(Texto)"
            int idEspaco = espacos.buscar(expressao.bytesNomeVariavel);
            if(idEspaco < 0) {
                std::string nomeStr = expressao.bytesNomeVariavel;
                throw std::runtime_error("'" + nomeStr + "' não é uma variável nem um espaço (usado em 'bytes(...)')");
            }
            tamanho = tamanhoDoEspaco(idEspaco);
        }
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
    referenciaSlot.sinalizado = ponteiro.elementoSinalizado;
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
    if(expressao.tipoConversao == TipoVariavel::BOOL) {
        // (bool)x normaliza pra 0 ou 1(x != 0). Imediato resolve em tempo de compilação; o resto vira "slot = 0; se(x != 0) { slot = 1; }".
        if(valor.tipo == ArgumentoIR::Tipo::IMEDIATO) {
            valor.imediato = (valor.imediato != 0) ? 1 : 0;
            valor.larguraBytes = 1;
            return valor;
        }
        int slot = ctx.variaveisLocais.proximoSlot++;
        atribuirMarcacao(slot, 0, saida);
        ComandoIR comandoSe;
        comandoSe.tipo = ComandoIR::Tipo::SE;
        comandoSe.instrucaoSe.iniciar();
        comandoSe.instrucaoSe.esquerda = valor;
        comandoSe.instrucaoSe.operador = OperadorComparacaoIR::DIFERENTE;
        comandoSe.instrucaoSe.direita = imediatoInt(0);
        atribuirMarcacao(slot, 1, *comandoSe.instrucaoSe.entao);
        saida.empurrar(comandoSe);
        ArgumentoIR referenciaSlot;
        referenciaSlot.tipo = ArgumentoIR::Tipo::REGISTRADOR;
        referenciaSlot.simbolo = nullptr;
        referenciaSlot.imediato = 0;
        referenciaSlot.registrador = slot;
        referenciaSlot.larguraBytes = 1;
        return referenciaSlot;
    }
    int larguraNova = larguraDoTipo(expressao.tipoConversao);
    bool sinalNovo = tipoEhSinalizado(expressao.tipoConversao);
    if(valor.tipo == ArgumentoIR::Tipo::REGISTRADOR && larguraNova > valor.larguraBytes) {
        // alargar: relabelar so a largura leria bytes que nunca foram gravados no slot de origem. Copia pra um slot novo(a arquitetura carrega com a largura e o sinal de origem, estendendo certo, e grava na largura nova).
        int slot = ctx.variaveisLocais.proximoSlot++;
        ComandoIR comandoCopia;
        comandoCopia.tipo = ComandoIR::Tipo::REATRIBUICAO;
        comandoCopia.instrucaoReatribuicao.slotDestino = slot;
        comandoCopia.instrucaoReatribuicao.valor = valor;
        comandoCopia.instrucaoReatribuicao.larguraDestino = larguraNova;
        saida.empurrar(comandoCopia);
        valor.registrador = slot;
    }
    valor.larguraBytes = larguraNova;
    valor.sinalizado = sinalNovo;
    valor.elementoSinalizado = elementoEhSinalizado(expressao.tipoConversao);
    valor.larguraElemento = larguraElementoApontado(expressao.tipoConversao); // a conversão muda o tipo do ponteiro: "nome[indice]" após a conversão precisa usar o passo do tipo NOVO, nao do tipo antigo(ex: (int*)baseCar precisa indexar de 4 em 4, mesmo que baseCar fosse car*)
    if(valor.tipo == ArgumentoIR::Tipo::IMEDIATO) {
        // trunca em tempo de compilação, estendendo o sinal quando o tipo é com sinal(ex: (byte)200 vale -56).
        if(valor.larguraBytes == 1) valor.imediato = sinalNovo ? (long)(signed char)valor.imediato : (valor.imediato & 0xFF);
        else if(valor.larguraBytes == 2) valor.imediato = sinalNovo ? (long)(short)valor.imediato : (valor.imediato & 0xFFFF);
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
    std::string nomeMetodo;
    Vetor<Expressao> novos;
    novos.iniciar();
    if(desdobrarChamadaMetodo(expressao.nomeFuncao, *expressao.argumentosFuncao, nomeMetodo, novos, ctx)) {
        Expressao reescrita;
        reescrita.tipo = Expressao::Tipo::CHAMADA_FUNCAO;
        reescrita.valor = nullptr;
        reescrita.nomeFuncao = strdup(nomeMetodo.c_str());
        reescrita.argumentosFuncao = &novos;
        ArgumentoIR resultado = resolverChamadaFuncaoComoValor(reescrita, saida, ctx);
        free(reescrita.nomeFuncao);
        liberarDesdobramento(novos, expressao.argumentosFuncao->tam);
        return resultado;
    }
    novos.liberar();
    std::string nomeStr = expressao.nomeFuncao;
    int totalParametrosEsperado;
    if(!funcoesUsuario.buscar(expressao.nomeFuncao, &totalParametrosEsperado)) {
        // não é função de usuário: pode ser uma variável que guarda um endereço("f()" com "nulo* f = @(endereco);"). O retorno é tratado como longo(8 bytes).
        ComandoIR comandoIndireto;
        comandoIndireto.tipo = ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO;
        comandoIndireto.chamadaFuncaoUsuario.iniciar();
        if(!prepararChamadaIndireta(expressao.nomeFuncao, *expressao.argumentosFuncao, comandoIndireto.chamadaFuncaoUsuario, saida, ctx)) {
            throw std::runtime_error("Função '" + nomeStr + "' não foi declarada (faltou #incluir a biblioteca correta?)");
        }
        int slotIndireto = ctx.variaveisLocais.proximoSlot++;
        comandoIndireto.chamadaFuncaoUsuario.capturaResultado = true;
        comandoIndireto.chamadaFuncaoUsuario.slotDestino = slotIndireto;
        comandoIndireto.chamadaFuncaoUsuario.resultadoLargura = 8;
        saida.empurrar(comandoIndireto);

        ArgumentoIR resultadoIndireto;
        resultadoIndireto.tipo = ArgumentoIR::Tipo::REGISTRADOR;
        resultadoIndireto.simbolo = nullptr;
        resultadoIndireto.imediato = 0;
        resultadoIndireto.registrador = slotIndireto;
        resultadoIndireto.larguraBytes = 8;
        return resultadoIndireto;
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
    resolverArgumentosChamada(expressao.nomeFuncao, *expressao.argumentosFuncao, comandoIR.chamadaFuncaoUsuario.argumentos, saida, ctx);
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
    referenciaSlot.sinalizado = tipoEhSinalizado(tipoRetorno.tipo);
    referenciaSlot.elementoSinalizado = elementoEhSinalizado(tipoRetorno.tipo);
    referenciaSlot.larguraElemento = larguraElementoApontado(tipoRetorno.tipo); // relevante se o retorno for usado como ponteiro indexado(ex: "malocar(10)[0]")
    return referenciaSlot;
}
// resolve "@(expr)" e "@nome".
// "@(expr)": o valor de expr passa a ser tratado como endereço(8 bytes), sem instrução nova: serve pra "nulo* f = @(0x1000);" e pra converter um número em endereço.
// "@nome": se "nome" é ponteiro ou espaço, o slot dele já guarda um endereço, então o valor é o próprio conteúdo do slot. Se é escalar(int, car, longo...), o valor
// é o endereço do slot na pilha: emite ENDERECO_SLOT, e o alocador de registradores deixa esse slot sempre na pilha(registrador físico não tem endereço).
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
    ArgumentoIR variavel;
    int slotVariavel;
    if(!ctx.variaveisLocais.buscar(expressao.esquerda->valor, &variavel, &slotVariavel)) {
        std::string nomeStr = expressao.esquerda->valor;
        throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
    }
    if(variavel.ehPonteiro || variavel.idEspaco >= 0) {
        ArgumentoIR endereco = copiarArgumento(variavel);
        endereco.larguraBytes = 8;
        endereco.espacoPorValor = false;
        endereco.ehPonteiro = true;
        return endereco;
    }
    int slotDestino = ctx.variaveisLocais.proximoSlot++;
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::ENDERECO_SLOT;
    comandoIR.instrucaoEnderecoSlot.slotDestino = slotDestino;
    comandoIR.instrucaoEnderecoSlot.slotOrigem = slotVariavel;
    saida.empurrar(comandoIR);

    ArgumentoIR referenciaSlot;
    referenciaSlot.tipo = ArgumentoIR::Tipo::REGISTRADOR;
    referenciaSlot.simbolo = nullptr;
    referenciaSlot.imediato = 0;
    referenciaSlot.registrador = slotDestino;
    referenciaSlot.larguraBytes = 8;
    referenciaSlot.ehPonteiro = true;
    referenciaSlot.larguraElemento = variavel.larguraBytes; // "nome[i]" sobre o endereço de um escalar anda de largura em largura do próprio escalar
    referenciaSlot.elementoSinalizado = variavel.sinalizado;
    return referenciaSlot;
}