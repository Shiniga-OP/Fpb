// util/ir/gerador_chamadas.h
#pragma once
#include "geradorir.h"

// gera IR pra uma chamada de função de usuário usada como COMANDO de
// topo(valor de retorno, se houver, e descartado): "bl nome". "saida"
// recebe os comandos auxiliares de cada argumento(ex: chamada de funcao
// aninhada) antes do comando final.
inline void GeradorIR::gerarChamadaFuncao(const ChamadaFuncao& chamada, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    if(chamada.nome[0] == '.') {
        ChamadaFuncao oculta;
        oculta.iniciar();
        std::string nomeOculto = ocultarReceptor(chamada.nome, chamada.argumentos[0], saida, ctx);
        oculta.nome = strdup(nomeOculto.c_str());
        for(int i = 1; i < chamada.argumentos.tam; i++) oculta.argumentos.empurrar(chamada.argumentos[i]);
        gerarChamadaFuncao(oculta, saida, ctx);
        free(oculta.nome);
        oculta.argumentos.liberar();
        return;
    }
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
    ComandoIR comandoIR = montarChamada(chamada.nome, {});
    resolverArgumentosChamada(chamada.nome, chamada.argumentos, comandoIR.chamadaFuncaoUsuario.argumentos, saida, ctx);
    saida.empurrar(comandoIR);
}

// "X.metodo(args)" com X qualquer expressão (o parser manda ".metodo" e X em argumentos[0]): resolve X uma vez, guarda numa
// variável oculta e devolve "oculta.metodo", que segue o caminho normal de desdobrarChamadaMetodo.
inline std::string GeradorIR::ocultarReceptor(const char* nome, const Expressao& receptor, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ArgumentoIR valor = resolverExpressao(receptor, saida, ctx);
    if(valor.tipo != ArgumentoIR::Tipo::REGISTRADOR || valor.idEspaco < 0) {
        throw std::runtime_error(std::string("O valor à esquerda de '") + nome + "' não é um espaço, não tem métodos");
    }
    std::string oculto = "$r" + std::to_string(valor.registrador);
    ctx.variaveisLocais.definirComSlot(oculto.c_str(), valor, valor.registrador);
    return oculto + nome;
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
        if(!buscarMetodo(variavel.idEspaco, metodo.c_str(), nomeFuncao)) {
            nomeFuncao = std::string(espacos.declaracoes[variavel.idEspaco]->nome) + "_" + metodo;
        }
        Expressao este;
        este.tipo = Expressao::Tipo::IDENTIFICADOR;
        este.valor = strdup(base.c_str());
        novos.empurrar(este);
    } else {
        int idClasse = espacos.buscar(base.c_str());
        if(idClasse < 0) {
            throw std::runtime_error("'" + base + "' não é uma variável nem uma classe");
        }
        if(!buscarMetodo(idClasse, metodo.c_str(), nomeFuncao)) nomeFuncao = base + "_" + metodo;
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
    int slot = ctx.variaveisLocais.proximoSlot++;
    reservarNoFrame(extras * 8, slot, saida); // 8 bytes por extra, como nos arrays declarados

    ArgumentoIR pacote = argumentoSlotLargura(slot, 8); // ponteiro pro array de longo: serve de base das escritas e de último argumento da chamada
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

        return argumentoSlotLargura(slotIndireto, 8);
    }
    TipoRetorno tipoRetorno;
    funcoesUsuario.buscarTipoRetorno(expressao.nomeFuncao, &tipoRetorno);
    if(tipoRetorno.ehNulo) {
        throw std::runtime_error("Função '" + nomeStr + "' é 'nulo' e não pode ser usada como valor de expressão");
    }
    ComandoIR comandoIR = montarChamada(expressao.nomeFuncao, {});
    resolverArgumentosChamada(expressao.nomeFuncao, *expressao.argumentosFuncao, comandoIR.chamadaFuncaoUsuario.argumentos, saida, ctx);
    return capturarChamada(comandoIR, tipoRetorno.tipo, saida, ctx); // relevante se o retorno for usado como ponteiro indexado(ex: "malocar(10)[0]")
}