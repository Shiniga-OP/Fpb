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
        TipoVariavel tipoRetorno = ctx.funcao.tipoRetorno.tipo;
        comandoIR.instrucaoRetorne.valor = converterParaTipo(valorRetorno, tipoEhFlutuante(tipoRetorno), larguraDoTipo(tipoRetorno), saida, ctx);
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
    referenciaSlot.ehPonteiro = tipoEhPonteiro(decl.tipo);
    referenciaSlot.ehFlutuante = tipoEhFlutuante(decl.tipo);
    referenciaSlot.elementoFlutuante = elementoEhFlutuante(decl.tipo);
    
    if(decl.nomeEspaco) referenciaSlot.idEspaco = obterIdEspaco(decl.nomeEspaco);
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
    if(decl.tipo == TipoVariavel::ESPACO) {
        // espaço por valor: o corpo mora na area de arrays do frame(mesmo mecanismo dos arrays locais) e o slot guarda o endereço base.
        // Cada valor da lista vira uma escrita em base + posicao do campo.
        int id = referenciaSlot.idEspaco;
        const DeclaracaoEspaco& espaco = *espacos.declaracoes[id];
        if(decl.temLista && decl.listaInicial.tam != espaco.campos.tam) {
            std::string nomeStr = espaco.nome;
            throw std::runtime_error("Espaço '" + nomeStr + "' tem " + std::to_string(espaco.campos.tam) + " campo(s), mas a lista de inicialização tem " + std::to_string(decl.listaInicial.tam) + " valor(es)");
        }
        referenciaSlot.espacoPorValor = true;
        int bytes = ((tamanhoDoEspaco(id) + 15) / 16) * 16;
        int posArray = bytesArraysAtual;
        bytesArraysAtual += bytes;
        ctx.variaveisLocais.definirComSlot(decl.nome, referenciaSlot, slot);
        ComandoIR comandoEndereco;
        comandoEndereco.tipo = ComandoIR::Tipo::ENDERECO_FRAME;
        comandoEndereco.instrucaoEnderecoFrame.slotDestino = slot;
        comandoEndereco.instrucaoEnderecoFrame.posArray = posArray;
        saida.empurrar(comandoEndereco);
        if(!decl.temLista && decl.listaInicial.tam == 1) {
            // "Nome x = expr;": se expr é um espaço do mesmo tipo, copia campo a campo; senão chama o construtor "Nome_criar(x, expr)".
            ArgumentoIR valor = resolverExpressao(decl.listaInicial[0], saida, ctx);
            if(valor.tipo == ArgumentoIR::Tipo::REGISTRADOR && valor.idEspaco == id) {
                copiarEspaco(valor, referenciaSlot, id, saida, ctx);
                return;
            }
            std::string nomeEspacoStr = espaco.nome;
            std::string nomeCriar = nomeEspacoStr + "_criar";
            int totalParametros;
            if(!funcoesUsuario.buscar(nomeCriar.c_str(), &totalParametros)) {
                throw std::runtime_error("Espaço '" + nomeEspacoStr + "' não tem o construtor '" + nomeCriar + "' (necessário pra '" + nomeEspacoStr + " x = valor;')");
            }
            if(totalParametros != 2) {
                throw std::runtime_error("O construtor '" + nomeCriar + "' precisa ter exatamente 1 parâmetro além de 'este'");
            }
            ComandoIR comandoCriar;
            comandoCriar.tipo = ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO;
            comandoCriar.chamadaFuncaoUsuario.iniciar();
            comandoCriar.chamadaFuncaoUsuario.nome = strdup(nomeCriar.c_str());
            comandoCriar.chamadaFuncaoUsuario.argumentos.empurrar(copiarArgumento(referenciaSlot));
            comandoCriar.chamadaFuncaoUsuario.argumentos.empurrar(valor);
            saida.empurrar(comandoCriar);
            return;
        }
        if(!decl.temLista) return;
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
    if(referenciaSlot.idEspaco >= 0 && valor.tipo == ArgumentoIR::Tipo::REGISTRADOR && valor.idEspaco >= 0 && valor.idEspaco != referenciaSlot.idEspaco) {
        std::string nomeStr = decl.nome;
        throw std::runtime_error("Variável '" + nomeStr + "' recebe um ponteiro de outro espaço");
    }
    valor = converterParaTipo(valor, referenciaSlot.ehFlutuante, larguraDoTipo(decl.tipo), saida, ctx);
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
    if(existente.espacoPorValor) {
        // "a = b" entre espaços: copia campo a campo pro corpo de "a"(o slot de "a" continua guardando o mesmo endereço base).
        std::string nomeStr = reat.nome;
        ArgumentoIR origem = resolverExpressao(reat.valor, saida, ctx);
        if(origem.tipo != ArgumentoIR::Tipo::REGISTRADOR || origem.idEspaco != existente.idEspaco) {
            throw std::runtime_error("Variável '" + nomeStr + "' só pode receber um espaço do mesmo tipo");
        }
        copiarEspaco(origem, existente, existente.idEspaco, saida, ctx);
        return;
    }
    ArgumentoIR novoValor = resolverExpressao(reat.valor, saida, ctx);
    novoValor = converterParaTipo(novoValor, existente.ehFlutuante, existente.larguraBytes, saida, ctx);
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
    valor = converterParaTipo(valor, ponteiro.elementoFlutuante, ponteiro.larguraElemento, saida, ctx);
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
        } else if(comando.tipo == Comando::Tipo::ATRIBUICAO_CAMPO) {
            gerarAtribuicaoCampo(comando.atribuicaoCampo, saida, ctx);
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
    std::string nomeMetodo;
    ChamadaFuncao reescrita;
    reescrita.iniciar();
    if(desdobrarChamadaMetodo(chamada.nome, chamada.argumentos, nomeMetodo, reescrita.argumentos, ctx)) {
        reescrita.nome = strdup(nomeMetodo.c_str());
        gerarChamadaFuncao(reescrita, saida, ctx);
        free(reescrita.nome);
        liberarDesdobramento(reescrita.argumentos, chamada.argumentos.tam);
        return;
    }
    reescrita.argumentos.liberar();
    int totalParametrosEsperado;
    if(!funcoesUsuario.buscar(chamada.nome, &totalParametrosEsperado)) {
        // não é função de usuário: pode ser uma variável que guarda um endereço("f();" com "nulo* f = @(endereco);")
        ComandoIR comandoIndireto;
        comandoIndireto.tipo = ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO;
        comandoIndireto.chamadaFuncaoUsuario.iniciar();
        if(prepararChamadaIndireta(chamada.nome, chamada.argumentos, comandoIndireto.chamadaFuncaoUsuario, saida, ctx)) {
            saida.empurrar(comandoIndireto);
            return;
        }
        std::string nomeStr = chamada.nome;
        throw std::runtime_error("Função '" + nomeStr + "' não foi declarada (faltou #incluir a biblioteca correta?)");
    }
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO;
    comandoIR.chamadaFuncaoUsuario.iniciar();
    comandoIR.chamadaFuncaoUsuario.nome = strdup(chamada.nome);
    resolverArgumentosChamada(chamada.nome, chamada.argumentos, comandoIR.chamadaFuncaoUsuario.argumentos, saida, ctx);
    saida.empurrar(comandoIR);
}

// "nome.metodo(args)": se "nome" é uma variável de espaço, vira "Classe_metodo(nome, args)"; se "nome" é o nome de um espaço(classe), vira
// "Classe_metodo(args)"(método estático). Devolve false se "nome" não tem '.'(chamada normal). "novos" já deve estar iniciado: recebe cópias
// rasas das expressões(não é dono delas, só do identificador injetado, ver liberarDesdobramento).
inline bool GeradorIR::desdobrarChamadaMetodo(const char* nome, const Vetor<Expressao>& argumentos, std::string& nomeFuncao, Vetor<Expressao>& novos, ContextoFuncao& ctx) {
    const char* ponto = strchr(nome, '.');
    if(!ponto) return false;
    std::string base(nome, ponto - nome);
    std::string metodo = ponto + 1;
    ArgumentoIR variavel;
    if(ctx.variaveisLocais.buscar(base.c_str(), &variavel)) {
        if(variavel.idEspaco < 0) {
            throw std::runtime_error("Variável '" + base + "' não é um espaço, não tem métodos");
        }
        std::string nomeClasse = espacos.declaracoes[variavel.idEspaco]->nome;
        nomeFuncao = nomeClasse + "_" + metodo;
        Expressao este;
        este.tipo = Expressao::Tipo::IDENTIFICADOR;
        este.valor = strdup(base.c_str());
        novos.empurrar(este);
    } else {
        if(espacos.buscar(base.c_str()) < 0) {
            throw std::runtime_error("'" + base + "' não é uma variável nem uma classe");
        }
        nomeFuncao = base + "_" + metodo;
    }
    for(int i = 0; i < argumentos.tam; i++) novos.empurrar(argumentos[i]);
    return true;
}

// libera o que desdobrarChamadaMetodo criou: o identificador injetado(se houve) e o vetor. As demais expressões são do chamador.
inline void GeradorIR::liberarDesdobramento(Vetor<Expressao>& novos, int totalOriginal) {
    if(novos.tam > totalOriginal) free(novos[0].valor);
    novos.liberar();
}

// valida a contagem de argumentos contra a assinatura de "nome" e resolve todos pra "destino"(usada por
// gerarChamadaFuncao e resolverChamadaFuncaoComoValor). Função variádica("longo... args" no fim): os argumentos fixos
// são resolvidos normalmente, e os extras são empacotados num array de longo(8 bytes cada) na área de arrays do frame;
// o endereço desse array vira o último argumento. Cada extra é gravado logo depois de resolvido, então chamadas dentro
// dos argumentos funcionam. Os comandos auxiliares vão em "saida", antes do comando da chamada.
inline void GeradorIR::resolverArgumentosChamada(const char* nome, const Vetor<Expressao>& argumentos, Vetor<ArgumentoIR>& destino, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    std::string nomeStr = nome;
    int totalParametros = 0;
    funcoesUsuario.buscar(nome, &totalParametros);
    bool variadica = funcoesUsuario.ehVariadica(nome);
    int fixos = variadica ? totalParametros - 1 : totalParametros;
    bool contagemOk = variadica ? (argumentos.tam >= fixos) : (argumentos.tam == fixos);
    if(!contagemOk) {
        throw std::runtime_error("Função '" + nomeStr + "' espera " + std::string(variadica ? "pelo menos " : "") + std::to_string(fixos) + " argumento(s), recebeu " + std::to_string(argumentos.tam));
    }
    const DeclaracaoFuncao* declaracao = nullptr;
    corposFuncoesUsuario.buscar(nome, &declaracao);
    for(int j = 0; j < fixos; j++) {
        ArgumentoIR arg = resolverExpressao(argumentos[j], saida, ctx);
        if(declaracao) arg = converterParaTipo(arg, tipoEhFlutuante(declaracao->parametros[j].tipo), larguraDoTipo(declaracao->parametros[j].tipo), saida, ctx);
        if(declaracao && declaracao->parametros[j].tipo == TipoVariavel::ESPACO) {
            // parâmetro "Nome x"(por valor, como em C): a função recebe o endereço de uma CÓPIA no frame de quem chama.
            int idParametro = obterIdEspaco(declaracao->parametros[j].nomeEspaco);
            if(arg.tipo != ArgumentoIR::Tipo::REGISTRADOR || arg.idEspaco != idParametro) {
                throw std::runtime_error("Função '" + nomeStr + "': o argumento " + std::to_string(j + 1) + " precisa ser um espaço do tipo esperado");
            }
            arg = criarCopiaEspaco(arg, idParametro, saida, ctx);
        }
        destino.empurrar(arg);
    }
    if(!variadica) return;

    int extras = argumentos.tam - fixos;
    int bytes = ((extras * 8 + 15) / 16) * 16; // mesma regra de arredondamento dos arrays declarados(gerarDeclaracaoVariavel)
    int posArray = bytesArraysAtual;
    bytesArraysAtual += bytes;
    int slot = ctx.variaveisLocais.proximoSlot++;

    ComandoIR comandoEndereco;
    comandoEndereco.tipo = ComandoIR::Tipo::ENDERECO_FRAME;
    comandoEndereco.instrucaoEnderecoFrame.slotDestino = slot;
    comandoEndereco.instrucaoEnderecoFrame.posArray = posArray;
    saida.empurrar(comandoEndereco);

    ArgumentoIR pacote; // ponteiro pro array de longo: serve de base das escritas e de último argumento da chamada
    pacote.tipo = ArgumentoIR::Tipo::REGISTRADOR;
    pacote.simbolo = nullptr;
    pacote.imediato = 0;
    pacote.registrador = slot;
    pacote.larguraBytes = 8;
    pacote.larguraElemento = 8;
    pacote.elementoSinalizado = true;

    for(int i = 0; i < extras; i++) {
        ArgumentoIR valor = resolverExpressao(argumentos[fixos + i], saida, ctx);
        if(valor.ehFlutuante) valor = converterParaTipo(valor, true, 8, saida, ctx); // como em C: flu nos extras vira dobro
        if(valor.tipo == ArgumentoIR::Tipo::IMEDIATO) valor.larguraBytes = 8; // monta o imediato direto em 64 bits
        ComandoIR comandoEscrita;
        comandoEscrita.tipo = ComandoIR::Tipo::ESCRITA_INDEXADA;
        comandoEscrita.instrucaoEscritaIndexada.ponteiro = copiarArgumento(pacote);
        comandoEscrita.instrucaoEscritaIndexada.indice = imediatoInt(i);
        comandoEscrita.instrucaoEscritaIndexada.valor = valor;
        saida.empurrar(comandoEscrita);
    }
    destino.empurrar(pacote);
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

        ArgumentoIR valor;
        valor.tipo = ArgumentoIR::Tipo::REGISTRADOR;
        valor.simbolo = nullptr;
        valor.imediato = 0;
        valor.registrador = slot;
        valor.larguraBytes = largura;
        valor.sinalizado = tipoEhSinalizado(tipoCampo);
        valor.elementoSinalizado = elementoEhSinalizado(tipoCampo);
        valor.larguraElemento = larguraElementoApontado(tipoCampo);
        emitirEscritaCampo(destino, posCampo, tipoCampo, valor, saida);
    }
}

// reserva um corpo novo do espaço "idEspaco" na área de arrays do frame de quem chama(mesmo mecanismo de gerarDeclaracaoVariavel),
// copia o conteúdo de "origem" pra lá e devolve o argumento com o endereço base da cópia. É o que o "Nome x" por valor recebe.
inline ArgumentoIR GeradorIR::criarCopiaEspaco(const ArgumentoIR& origem, int idEspaco, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    int bytes = ((tamanhoDoEspaco(idEspaco) + 15) / 16) * 16;
    int posArray = bytesArraysAtual;
    bytesArraysAtual += bytes;
    int slot = ctx.variaveisLocais.proximoSlot++;

    ComandoIR comandoEndereco;
    comandoEndereco.tipo = ComandoIR::Tipo::ENDERECO_FRAME;
    comandoEndereco.instrucaoEnderecoFrame.slotDestino = slot;
    comandoEndereco.instrucaoEnderecoFrame.posArray = posArray;
    saida.empurrar(comandoEndereco);

    ArgumentoIR copia = copiarArgumento(origem);
    copia.registrador = slot;
    copia.espacoPorValor = true;
    copiarEspaco(origem, copia, idEspaco, saida, ctx);
    return copia;
}

// monta a chamada de uma função por endereço: "nome" é uma variável local de 8 bytes que guarda o endereço da função(ex: "nulo* f = @(endereco); f();").
// Devolve false se "nome" não é variável. Os argumentos vão em x0..x5, como no "bl"; não há assinatura pra conferir, então só o máximo de 6 é validado.
// O endereço fica em "enderecoAlvo" e a arquitetura emite "blr" em vez de "bl nome".
inline bool GeradorIR::prepararChamadaIndireta(const char* nome, const Vetor<Expressao>& argumentos, InstrucaoChamadaFuncaoUsuario& chamada, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ArgumentoIR variavel;
    if(!ctx.variaveisLocais.buscar(nome, &variavel)) return false;
    std::string nomeStr = nome;
    if(variavel.larguraBytes != 8) {
        throw std::runtime_error("Variável '" + nomeStr + "' não pode ser chamada como função: precisa guardar um endereço de 8 bytes");
    }
    int totalInteiros = 0;
    int totalFlutuantes = 0;
    if(argumentos.tam > 16) {
        throw std::runtime_error("Chamada por '" + nomeStr + "' aceita no máximo 8 argumentos inteiros e 8 flutuantes");
    }
    chamada.indireta = true;
    chamada.enderecoAlvo = copiarArgumento(variavel);
    chamada.enderecoAlvo.ehPonteiro = variavel.ehPonteiro;
    for(int i = 0; i < argumentos.tam; i++) {
        ArgumentoIR arg = resolverExpressao(argumentos[i], saida, ctx);
        if(arg.ehFlutuante) totalFlutuantes++;
        else totalInteiros++;
        chamada.argumentos.empurrar(arg);
    }
    if(totalInteiros > 8 || totalFlutuantes > 8) {
        throw std::runtime_error("Chamada por '" + nomeStr + "' aceita no máximo 8 argumentos inteiros e 8 flutuantes");
    }
    return true;
}