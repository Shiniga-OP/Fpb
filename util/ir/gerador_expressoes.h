// util/ir/gerador_expressoes.h
#pragma once
#include "geradorir.h"

// resolve qualquer Expressao (literal, identificador, chamada de função, conversão, indexação, bytes ou binária) pra um
// ArgumentoIR com seu valor final. "saida" recebe as instruções auxiliares que a resolução precisar emitir ANTES do
// valor ficar pronto. É a ÚNICA rota de resolução de expressão do gerador.
inline ArgumentoIR GeradorIR::resolverExpressao(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    if(expressao.tipo == Expressao::Tipo::TERNARIA) {
        return resolverTernaria(expressao, saida, ctx);
    }
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
    if(expressao.tipo == Expressao::Tipo::DECIMAL_LITERAL) {
        // sem sufixo e dobro(64 bits); com "f" no fim e flu(32 bits). O imediato guarda o padrao de bits.
        const char* texto = expressao.valor;
        bool ehFlu = texto[strlen(texto) - 1] == 'f';
        ArgumentoIR arg;
        arg.tipo = ArgumentoIR::Tipo::IMEDIATO;
        arg.simbolo = nullptr;
        arg.ehFlutuante = true;
        if(ehFlu) {
            float f = strtof(texto, nullptr);
            unsigned int bits;
            memcpy(&bits, &f, 4);
            arg.imediato = (long)bits;
            arg.larguraBytes = 4;
        } else {
            double d = strtod(texto, nullptr);
            memcpy(&arg.imediato, &d, 8);
            arg.larguraBytes = 8;
        }
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
    if(expressao.tipo == Expressao::Tipo::ATRIBUICAO) {
        return resolverAtribuicao(expressao, saida, ctx);
    }
    // so sobra BINARIA daqui pra baixo.
    ArgumentoIR esquerda = resolverExpressao(*expressao.esquerda, saida, ctx);
    ArgumentoIR direita = resolverExpressao(*expressao.direita, saida, ctx);
    if(esquerda.tipo == ArgumentoIR::Tipo::REGISTRADOR && esquerda.idEspaco >= 0) {
        // operando esquerdo é um espaço(ou ponteiro pra um): "a + b" vira "Nome_somar(a, b)", e assim por diante.
        return resolverOperadorEspaco(expressao.operadorAritmetico, esquerda, direita, saida, ctx);
    }
    promoverFlutuantes(esquerda, direita, saida, ctx);
    bool ehFlutuante = esquerda.ehFlutuante;
    if(ehFlutuante) {
        OperadorAritmetico opFlu = expressao.operadorAritmetico;
        if(!(opFlu == OperadorAritmetico::SOMA || opFlu == OperadorAritmetico::SUBTRACAO || opFlu == OperadorAritmetico::MULTIPLICACAO || opFlu == OperadorAritmetico::DIVISAO)) {
            throw std::runtime_error("Operador não suportado para flu/dobro (só +, -, * e /)");
        }
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
    referenciaSlot.ehFlutuante = ehFlutuante;
    return referenciaSlot;
}

// converte "valor" pro tipo numerico do destino(flutuante ou inteiro, na largura dada). Imediato converte em tempo de compilacao;
// registrador emite CONVERSAO_NUMERICA(scvtf, fcvtzs, fcvt) num slot novo. Inteiro pra inteiro nao faz nada aqui(a gravacao ja trunca).
inline ArgumentoIR GeradorIR::converterParaTipo(ArgumentoIR valor, bool destinoFlutuante, int larguraDestino, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    if(valor.tipo == ArgumentoIR::Tipo::SIMBOLO_TEXTO) return valor;
    if(!destinoFlutuante && !valor.ehFlutuante) return valor;
    if(destinoFlutuante && valor.ehFlutuante && valor.larguraBytes == larguraDestino) return valor;
    if(valor.tipo == ArgumentoIR::Tipo::IMEDIATO) {
        double d;
        if(!valor.ehFlutuante) d = (double)valor.imediato;
        else if(valor.larguraBytes == 4) {
            unsigned int bitsOrigem = (unsigned int)valor.imediato;
            float f;
            memcpy(&f, &bitsOrigem, 4);
            d = f;
        } else memcpy(&d, &valor.imediato, 8);
        if(destinoFlutuante && larguraDestino == 4) {
            float f = (float)d;
            unsigned int bits;
            memcpy(&bits, &f, 4);
            valor.imediato = (long)bits;
        } else if(destinoFlutuante) {
            memcpy(&valor.imediato, &d, 8);
        } else {
            valor.imediato = (long)d;
        }
        valor.ehFlutuante = destinoFlutuante;
        valor.larguraBytes = larguraDestino;
        return valor;
    }
    int slot = ctx.variaveisLocais.proximoSlot++;
    ComandoIR comando;
    comando.tipo = ComandoIR::Tipo::CONVERSAO_NUMERICA;
    comando.instrucaoConversaoNumerica.slotDestino = slot;
    comando.instrucaoConversaoNumerica.origem = valor;
    comando.instrucaoConversaoNumerica.larguraDestino = larguraDestino;
    comando.instrucaoConversaoNumerica.destinoFlutuante = destinoFlutuante;
    saida.empurrar(comando);
    ArgumentoIR resultado;
    resultado.tipo = ArgumentoIR::Tipo::REGISTRADOR;
    resultado.simbolo = nullptr;
    resultado.imediato = 0;
    resultado.registrador = slot;
    resultado.larguraBytes = larguraDestino;
    resultado.ehFlutuante = destinoFlutuante;
    return resultado;
}

// se um dos lados e flutuante, converte os dois pro mesmo tipo: dobro se algum for dobro, senao flu.
inline void GeradorIR::promoverFlutuantes(ArgumentoIR& esq, ArgumentoIR& dir, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    if(!esq.ehFlutuante && !dir.ehFlutuante) return;
    bool dobro = (esq.ehFlutuante && esq.larguraBytes == 8) || (dir.ehFlutuante && dir.larguraBytes == 8);
    int largura = dobro ? 8 : 4;
    esq = converterParaTipo(esq, true, largura, saida, ctx);
    dir = converterParaTipo(dir, true, largura, saida, ctx);
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
    std::string nomeFuncao;
    if(!buscarMetodo(idEspaco, nomeMetodoOperador(op), nomeFuncao)) {
        throw std::runtime_error("Espaço '" + nomeEspacoStr + "' não tem o método '" + nomeEspacoStr + "_" + nomeMetodoOperador(op) + "' (necessário pra esse operador)");
    }
    int totalParametros;
    funcoesUsuario.buscar(nomeFuncao.c_str(), &totalParametros);
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
    if(declaracao && declaracao->parametros[1].nomeEspaco) {
        int idParametro = obterIdEspaco(declaracao->parametros[1].nomeEspaco);
        bool ehDoTipo = direita.tipo == ArgumentoIR::Tipo::REGISTRADOR && ehSubespaco(direita.idEspaco, idParametro);
        if(!ehDoTipo) {
            if(direita.tipo == ArgumentoIR::Tipo::REGISTRADOR && direita.idEspaco >= 0) {
                throw std::runtime_error("O operando direito de '" + nomeFuncao + "' é de outro espaço");
            }
            direita = criarTemporarioDeValor(idParametro, direita, saida, ctx); // texto + "literal"
        } else if(declaracao->parametros[1].tipo == TipoVariavel::ESPACO) {
            direita = criarCopiaEspaco(direita, idParametro, saida, ctx);
        }
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
    comandoIR.chamadaFuncaoUsuario.resultadoFlutuante = tipoEhFlutuante(tipoRetorno.tipo);
    saida.empurrar(comandoIR);

    ArgumentoIR referenciaSlot;
    referenciaSlot.tipo = ArgumentoIR::Tipo::REGISTRADOR;
    referenciaSlot.simbolo = nullptr;
    referenciaSlot.imediato = 0;
    referenciaSlot.registrador = slot;
    referenciaSlot.larguraBytes = larguraResultado;
    referenciaSlot.ehFlutuante = tipoEhFlutuante(tipoRetorno.tipo);
    referenciaSlot.sinalizado = tipoEhSinalizado(tipoRetorno.tipo);
    referenciaSlot.elementoSinalizado = elementoEhSinalizado(tipoRetorno.tipo);
    referenciaSlot.larguraElemento = larguraElementoApontado(tipoRetorno.tipo);
    referenciaSlot.ehPonteiro = tipoEhPonteiro(tipoRetorno.tipo);
    if(tipoRetorno.tipo == TipoVariavel::ESPACO_PONTEIRO) {
        referenciaSlot.idEspaco = idEspaco;
        referenciaSlot.ehTemporario = true;
        registrarTemporario(slot, idEspaco, true, ctx);
    }
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

// devolve o ponteiro sendo indexado: a variável "nome" ou, se "campo" não é nullptr, o valor do campo ponteiro "nome.campo".
// Pro campo, a largura do elemento vem do tipo declarado do campo.
inline ArgumentoIR GeradorIR::resolverPonteiroIndexado(const char* nome, const char* campo, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ArgumentoIR base;
    if(!ctx.variaveisLocais.buscar(nome, &base)) {
        std::string nomeStr = nome;
        throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
    }
    if(!campo) return base;
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
    return ponteiro;
}

// ponteiro sendo indexado em "alvo[i]": variável, "nome.campo" ou "qualquer.coisa.campo".
inline ArgumentoIR GeradorIR::resolverPonteiroDaExpressao(const Expressao& e, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    if(e.tipo == Expressao::Tipo::IDENTIFICADOR) return resolverPonteiroIndexado(e.valor, nullptr, saida, ctx);
    if(e.tipo != Expressao::Tipo::ACESSO_CAMPO) return resolverExpressao(e, saida, ctx);
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
    return ponteiro;
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
    referenciaSlot.ehFlutuante = ponteiro.elementoFlutuante;
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
            if(valor.ehFlutuante) {
                ArgumentoIR comoDobro = converterParaTipo(valor, true, 8, saida, ctx);
                double d;
                memcpy(&d, &comoDobro.imediato, 8);
                valor.imediato = (d != 0.0) ? 1 : 0;
                valor.ehFlutuante = false;
            } else {
                valor.imediato = (valor.imediato != 0) ? 1 : 0;
            }
            valor.larguraBytes = 1;
            return valor;
        }
        ArgumentoIR zero = imediatoInt(0);
        promoverFlutuantes(valor, zero, saida, ctx);
        int slot = ctx.variaveisLocais.proximoSlot++;
        atribuirMarcacao(slot, 0, saida);
        ComandoIR comandoSe;
        comandoSe.tipo = ComandoIR::Tipo::SE;
        comandoSe.instrucaoSe.iniciar();
        comandoSe.instrucaoSe.esquerda = valor;
        comandoSe.instrucaoSe.operador = OperadorComparacaoIR::DIFERENTE;
        comandoSe.instrucaoSe.direita = zero;
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
    bool destinoFlutuante = tipoEhFlutuante(expressao.tipoConversao);
    if(destinoFlutuante || valor.ehFlutuante) {
        ArgumentoIR convertido = converterParaTipo(valor, destinoFlutuante, larguraDoTipo(expressao.tipoConversao), saida, ctx);
        convertido.sinalizado = tipoEhSinalizado(expressao.tipoConversao);
        convertido.elementoSinalizado = elementoEhSinalizado(expressao.tipoConversao);
        convertido.larguraElemento = larguraElementoApontado(expressao.tipoConversao);
        convertido.elementoFlutuante = elementoEhFlutuante(expressao.tipoConversao);
        return convertido;
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
    valor.elementoFlutuante = elementoEhFlutuante(expressao.tipoConversao);
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
    if(expressao.nomeFuncao[0] == '.') {
        Vetor<Expressao> restantes;
        restantes.iniciar();
        std::string nomeOculto = ocultarReceptor(expressao.nomeFuncao, (*expressao.argumentosFuncao)[0], saida, ctx);
        for(int i = 1; i < expressao.argumentosFuncao->tam; i++) restantes.empurrar((*expressao.argumentosFuncao)[i]);
        Expressao reescritaOculta;
        reescritaOculta.tipo = Expressao::Tipo::CHAMADA_FUNCAO;
        reescritaOculta.valor = nullptr;
        reescritaOculta.nomeFuncao = strdup(nomeOculto.c_str());
        reescritaOculta.argumentosFuncao = &restantes;
        ArgumentoIR resultadoOculto = resolverChamadaFuncaoComoValor(reescritaOculta, saida, ctx);
        free(reescritaOculta.nomeFuncao);
        restantes.liberar();
        return resultadoOculto;
    }
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
    comandoIR.chamadaFuncaoUsuario.resultadoFlutuante = tipoEhFlutuante(tipoRetorno.tipo);
    saida.empurrar(comandoIR);

    ArgumentoIR referenciaSlot;
    referenciaSlot.tipo = ArgumentoIR::Tipo::REGISTRADOR;
    referenciaSlot.simbolo = nullptr;
    referenciaSlot.imediato = 0;
    referenciaSlot.registrador = slot;
    referenciaSlot.larguraBytes = larguraResultado;
    referenciaSlot.ehFlutuante = tipoEhFlutuante(tipoRetorno.tipo);
    referenciaSlot.sinalizado = tipoEhSinalizado(tipoRetorno.tipo);
    referenciaSlot.elementoSinalizado = elementoEhSinalizado(tipoRetorno.tipo);
    referenciaSlot.larguraElemento = larguraElementoApontado(tipoRetorno.tipo); // relevante se o retorno for usado como ponteiro indexado(ex: "malocar(10)[0]")
    referenciaSlot.elementoFlutuante = elementoEhFlutuante(tipoRetorno.tipo);
    return referenciaSlot;
}

// resolve "cond ? a : b": emite um SE cujos dois blocos gravam o valor escolhido no mesmo slot temporario novo. Os dois lados vão pro mesmo tipo(flutuante se algum for, senao a MAIOR largura, no minimo 4 como nas operacoes aritmeticas). A conversao de cada lado é emitida dentro do bloco dele, então só o lado escolhido executa.
inline ArgumentoIR GeradorIR::resolverTernaria(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ComandoIR comandoSe;
    comandoSe.tipo = ComandoIR::Tipo::SE;
    comandoSe.instrucaoSe.iniciar();
    resolverCondicao(*expressao.condicaoTernaria, saida, ctx, comandoSe.instrucaoSe.esquerda, comandoSe.instrucaoSe.operador, comandoSe.instrucaoSe.direita);
    comandoSe.instrucaoSe.temSenao = true;
    comandoSe.instrucaoSe.senaoCorpo = new Vetor<ComandoIR>();
    comandoSe.instrucaoSe.senaoCorpo->iniciar();
    Vetor<ComandoIR>& blocoEntao = *comandoSe.instrucaoSe.entao;
    Vetor<ComandoIR>& blocoSenao = *comandoSe.instrucaoSe.senaoCorpo;

    ArgumentoIR valorEntao = resolverExpressao(*expressao.esquerda, blocoEntao, ctx);
    ArgumentoIR valorSenao = resolverExpressao(*expressao.direita, blocoSenao, ctx);
    bool flutuante = valorEntao.ehFlutuante || valorSenao.ehFlutuante;
    int largura;
    if(flutuante) {
        bool dobro = (valorEntao.ehFlutuante && valorEntao.larguraBytes == 8) || (valorSenao.ehFlutuante && valorSenao.larguraBytes == 8);
        largura = dobro ? 8 : 4;
        valorEntao = converterParaTipo(valorEntao, true, largura, blocoEntao, ctx);
        valorSenao = converterParaTipo(valorSenao, true, largura, blocoSenao, ctx);
    } else {
        largura = (valorEntao.larguraBytes > valorSenao.larguraBytes) ? valorEntao.larguraBytes : valorSenao.larguraBytes;
        if(largura < 4) largura = 4;
    }
    // imediato monta direto na largura do resultado; registrador mantém largura e sinal de origem pra ser estendido certo
    if(valorEntao.tipo == ArgumentoIR::Tipo::IMEDIATO) valorEntao.larguraBytes = largura;
    if(valorSenao.tipo == ArgumentoIR::Tipo::IMEDIATO) valorSenao.larguraBytes = largura;

    int slot = ctx.variaveisLocais.proximoSlot++;
    ComandoIR atribuicaoEntao;
    atribuicaoEntao.tipo = ComandoIR::Tipo::REATRIBUICAO;
    atribuicaoEntao.instrucaoReatribuicao.slotDestino = slot;
    atribuicaoEntao.instrucaoReatribuicao.valor = valorEntao;
    atribuicaoEntao.instrucaoReatribuicao.larguraDestino = largura;
    blocoEntao.empurrar(atribuicaoEntao);
    ComandoIR atribuicaoSenao;
    atribuicaoSenao.tipo = ComandoIR::Tipo::REATRIBUICAO;
    atribuicaoSenao.instrucaoReatribuicao.slotDestino = slot;
    atribuicaoSenao.instrucaoReatribuicao.valor = valorSenao;
    atribuicaoSenao.instrucaoReatribuicao.larguraDestino = largura;
    blocoSenao.empurrar(atribuicaoSenao);
    saida.empurrar(comandoSe);

    ArgumentoIR referenciaSlot;
    referenciaSlot.tipo = ArgumentoIR::Tipo::REGISTRADOR;
    referenciaSlot.simbolo = nullptr;
    referenciaSlot.imediato = 0;
    referenciaSlot.registrador = slot;
    referenciaSlot.larguraBytes = largura;
    referenciaSlot.ehFlutuante = flutuante;
    // se algum lado é ponteiro/espaço, o resultado herda o passo de indexação e o espaço dele
    const ArgumentoIR& modelo = (valorEntao.ehPonteiro || valorEntao.idEspaco >= 0) ? valorEntao : valorSenao;
    referenciaSlot.ehPonteiro = modelo.ehPonteiro;
    referenciaSlot.larguraElemento = modelo.larguraElemento;
    referenciaSlot.elementoSinalizado = modelo.elementoSinalizado;
    referenciaSlot.idEspaco = modelo.idEspaco;
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
        
        ArgumentoIR referenciaCampo;
        referenciaCampo.tipo = ArgumentoIR::Tipo::REGISTRADOR;
        referenciaCampo.simbolo = nullptr;
        referenciaCampo.imediato = 0;
        referenciaCampo.registrador = slotCampo;
        referenciaCampo.larguraBytes = 8;
        referenciaCampo.ehPonteiro = true;
        referenciaCampo.larguraElemento = larguraDoTipo(tipoCampo);
        referenciaCampo.elementoSinalizado = tipoEhSinalizado(tipoCampo);
        referenciaCampo.elementoFlutuante = tipoEhFlutuante(tipoCampo);
        return referenciaCampo;
    }
    ArgumentoIR variavel;
    int slotVariavel;
    if(!ctx.variaveisLocais.buscar(expressao.esquerda->valor, &variavel, &slotVariavel)) {
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

    ArgumentoIR referenciaSlot;
    referenciaSlot.tipo = ArgumentoIR::Tipo::REGISTRADOR;
    referenciaSlot.simbolo = nullptr;
    referenciaSlot.imediato = 0;
    referenciaSlot.registrador = slotDestino;
    referenciaSlot.larguraBytes = 8;
    referenciaSlot.ehPonteiro = true;
    referenciaSlot.larguraElemento = variavel.larguraBytes; // "nome[i]" sobre o endereço de um escalar anda de largura em largura do próprio escalar
    referenciaSlot.elementoSinalizado = variavel.sinalizado;
    referenciaSlot.elementoFlutuante = variavel.ehFlutuante;
    return referenciaSlot;
}

// copia um valor que mora em slot de variável pra um slot novo, senão "(a = b) + (b = 5)" leria o b já alterado.
// Temporário já é slot exclusivo, não precisa copiar.
inline ArgumentoIR GeradorIR::congelarValor(ArgumentoIR valor, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    if(valor.tipo != ArgumentoIR::Tipo::REGISTRADOR || valor.ehTemporario) return valor;
    int slot = ctx.variaveisLocais.proximoSlot++;
    ComandoIR copia;
    copia.tipo = ComandoIR::Tipo::REATRIBUICAO;
    copia.instrucaoReatribuicao.slotDestino = slot;
    copia.instrucaoReatribuicao.valor = valor;
    copia.instrucaoReatribuicao.larguraDestino = valor.larguraBytes;
    saida.empurrar(copia);
    valor.registrador = slot;
    return valor;
}

// resolve "destino = expr" usada como valor: avalia expr UMA vez, grava no destino e devolve o valor gravado.
inline ArgumentoIR GeradorIR::resolverAtribuicao(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    const Expressao& destino = *expressao.esquerda;
    if(destino.tipo == Expressao::Tipo::INDEXACAO) {
        // mesma ordem do comando: ponteiro, índice, valor
        ArgumentoIR ponteiro = resolverPonteiroDaExpressao(*destino.esquerda, saida, ctx);
        ArgumentoIR indice = resolverExpressao(*destino.direita, saida, ctx);
        ArgumentoIR valor = congelarValor(resolverExpressao(*expressao.direita, saida, ctx), saida, ctx);
        return gravarEmIndexacao(ponteiro, indice, valor, saida, ctx);
    }
    if(destino.tipo == Expressao::Tipo::IDENTIFICADOR) {
        ArgumentoIR existente;
        if(ctx.variaveisLocais.buscar(destino.valor, &existente) && existente.espacoPorValor) {
            throw std::runtime_error("Atribuição de espaço por valor só vale como comando(não como valor de expressão)");
        }
        ArgumentoIR valor = congelarValor(resolverExpressao(*expressao.direita, saida, ctx), saida, ctx);
        return gravarEmVariavel(destino.valor, valor, saida, ctx);
    }
    if(destino.esquerda->tipo != Expressao::Tipo::IDENTIFICADOR) {
        ArgumentoIR baseComposta = resolverExpressao(*destino.esquerda, saida, ctx);
        ArgumentoIR valorComposto = congelarValor(resolverExpressao(*expressao.direita, saida, ctx), saida, ctx);
        return gravarEmCampoBase(baseComposta, destino.valor, valorComposto, saida, ctx);
    }
    // ACESSO_CAMPO("nome.campo" ou "Classe.estatico")
    ArgumentoIR valor = congelarValor(resolverExpressao(*expressao.direita, saida, ctx), saida, ctx);
    return gravarEmCampo(destino.esquerda->valor, destino.valor, valor, saida, ctx);
}