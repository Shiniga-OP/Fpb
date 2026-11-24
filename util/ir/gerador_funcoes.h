// util/ir/gerador_funcoes.h
#pragma once
#include "geradorir.h"

inline void coletarFuncaoReferenciada(const ArgumentoIR& arg, VetorStr& saida) {
    if(arg.tipo == ArgumentoIR::Tipo::SIMBOLO_TEXTO && arg.ehFuncao) saida.empurrar(arg.simbolo);
}

inline ProgramaIR GeradorIR::gerar(const Programa& programa) {
    // 1. registra a assinatura + corpo de cada função do programa principal.
    registrarFuncoes(programa);
    registrarEspacos(programa);
    registrarLigacoes(programa);
    // 2. o carregador resolve cada "#incluir"(dedupe por caminho absoluto) e
    //    devolve a AST de cada biblioteca(ex: biblis/impressao.fpb); aqui
    //    registramos assinatura + corpo de cada função delas.
    for(int i = 0; i < programa.inclusoes.tam; i++) {
        carregador.carregar(programa.inclusoes[i].caminho, diretorioBase);
    }
    for(int i = 0; i < carregador.programas.tam; i++) {
        for(int j = 0; j < carregador.programas[i]->funcoes.tam; j++) {
            const DeclaracaoFuncao& f = carregador.programas[i]->funcoes[j];
            if(f.ehExportada || strcmp(f.nome, "inicio") == 0) gerarFuncaoAlcancavel(f.nome, ir, jaGeradas);
        }
    }
    validarHeranca();
    // 3. gera FuncaoIR só pras funções alcancaveis a partir
    //    do programa principal(direta ou indiretamente via chamadas "bl"),
    //    evitando emitir codigo morto de bibliotecas incluídas mas não usadas.
    ProgramaIR ir;
    ir.iniciar();
    VetorStr jaGeradas;
    jaGeradas.iniciar();
    
    for(int i = 0; i < programa.funcoes.tam; i++) {
        const DeclaracaoFuncao& f = programa.funcoes[i];
        if(f.ehExportada || strcmp(f.nome, "inicio") == 0) gerarFuncaoAlcancavel(f.nome, ir, jaGeradas);
    }
    for(int i = 0; i < carregador.programas.tam; i++) {
        for(int j = 0; j < carregador.programas[i]->funcoes.tam; j++) {
            if(carregador.programas[i]->funcoes[j].ehExportada) gerarFuncaoAlcancavel(carregador.programas[i]->funcoes[j].nome, ir, jaGeradas);
        }
    }
    jaGeradas.liberar();
    for(int i = 0; i < globais.tam; i++) ir.globais.empurrar(globais[i]);
    globais.liberar(); // só o array: os símbolos agora são do ProgramaIR
    ir.ehBibli = true;
    for(int i = 0; i < carregador.programas.tam; i++) {
        for(int j = 0; j < carregador.programas[i]->funcoes.tam; j++) {
            if(strcmp(carregador.programas[i]->funcoes[j].nome, "inicio") == 0) ir.ehBibli = false;
        }
    }
    return ir;
}

// registra assinatura(nome/parâmetros/retorno) e corpo(AST) de cada
// funcao de "programa" nas tabelas globais, checando nome duplicado
// contra qualquer função já registrada(programa principal ou biblioteca
// anterior). "origem" e usada so na mensagem de erro.
inline void GeradorIR::registrarFuncoes(const Programa& programa, const std::string& origem) {
    for(int i = 0; i < programa.funcoes.tam; i++) {
        const DeclaracaoFuncao& funcao = programa.funcoes[i];
        int totalParametrosIgnorado;
        if(funcoesUsuario.buscar(funcao.nome, &totalParametrosIgnorado)) {
            std::string nomeStr = funcao.nome;
            throw std::runtime_error("Função '" + nomeStr + "' definida mais de uma vez (conflito ao incluir '" + origem + "')");
        }
        funcoesUsuario.definir(funcao.nome, funcao.parametros.tam, funcao.tipoRetorno, funcao.ehVariadica);
        corposFuncoesUsuario.definir(funcao.nome, &funcao);
    }
}

// gera(se ainda não foi gerada) a FuncaoIR de "nome" e, recursivamente,
// de toda função de usuario chamada(direta ou indiretamente) a partir
// dela. "jaGeradas" evita reprocessar a mesma função mais de uma vez
// (tanto por seguranca contra recursão infinita quanto por eficiência).
inline void GeradorIR::gerarFuncaoAlcancavel(const char* nome, ProgramaIR& ir, VetorStr& jaGeradas) {
    int tamNome = (int)strlen(nome);
    for(int i = 0; i < jaGeradas.tam; i++) {
        int tamCandidato;
        const char* candidato = jaGeradas.obter(i, &tamCandidato);
        if(tamCandidato == tamNome && memcmp(candidato, nome, tamNome) == 0) return; // ja gerada
    }
    jaGeradas.empurrar(nome);

    const DeclaracaoFuncao* corpo;
    if(!corposFuncoesUsuario.buscar(nome, &corpo)) {
        std::string nomeStr = nome;
        throw std::runtime_error("Função '" + nomeStr + "' não foi declarada (faltou #incluir a biblioteca correta?)");
    }
    if(corpo->ehExterna) return;
    FuncaoIR funcaoIR = gerarFuncao(*corpo, ir.dados);
    VetorStr chamadasEncontradas;
    chamadasEncontradas.iniciar();
    coletarChamadasUsuario(funcaoIR.instrucoes, chamadasEncontradas);
    ir.funcoes.empurrar(funcaoIR);
    for(int i = 0; i < chamadasEncontradas.tam; i++) {
        int tamChamada;
        const char* nomeChamada = chamadasEncontradas.obter(i, &tamChamada);
        std::string nomeChamadaStr(nomeChamada, tamChamada);
        gerarFuncaoAlcancavel(nomeChamadaStr.c_str(), ir, jaGeradas);
    }
    chamadasEncontradas.liberar();
}

// varre um bloco de ComandoIR ja gerado(incluindo recursivamente dentro
// de "se"/"enquanto") coletando o nome de toda funcao de usuario chamada
// via CHAMADA_FUNCAO_USUARIO. Usado pela eliminação sob demanda em gerarFuncaoAlcancavel.
inline void GeradorIR::coletarChamadasUsuario(const Vetor<ComandoIR>& bloco, VetorStr& saida) {
    for(int i = 0; i < bloco.tam; i++) {
        const ComandoIR& c = bloco[i];
        if(c.tipo == ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO) {
            if(!c.chamadaFuncaoUsuario.indireta) saida.empurrar(c.chamadaFuncaoUsuario.nome);
            for(int j = 0; j < c.chamadaFuncaoUsuario.argumentos.tam; j++) coletarFuncaoReferenciada(c.chamadaFuncaoUsuario.argumentos[j], saida);
        } else if(c.tipo == ComandoIR::Tipo::REATRIBUICAO) {
            coletarFuncaoReferenciada(c.instrucaoReatribuicao.valor, saida);
        } else if(c.tipo == ComandoIR::Tipo::ESCRITA_INDEXADA) {
            coletarFuncaoReferenciada(c.instrucaoEscritaIndexada.valor, saida);
        } else if(c.tipo == ComandoIR::Tipo::SE) {
            coletarChamadasUsuario(*c.instrucaoSe.entao, saida);
            if(c.instrucaoSe.temSenao) coletarChamadasUsuario(*c.instrucaoSe.senaoCorpo, saida);
        } else if(c.tipo == ComandoIR::Tipo::ENQUANTO) {
            coletarChamadasUsuario(*c.instrucaoEnquanto.comandosCondicao, saida);
            coletarChamadasUsuario(*c.instrucaoEnquanto.corpo, saida);
        }
    }
}

inline FuncaoIR GeradorIR::gerarFuncao(const DeclaracaoFuncao& funcao, Vetor<DadoEstaticoIR>& dados) {
    FuncaoIR funcaoIR;
    funcaoIR.iniciar();
    bytesArraysAtual = 0;
    funcaoIR.nome = strdup(funcao.nome);
    funcaoIR.ehExportada = funcao.ehExportada;
    funcaoIR.totalParametros = funcao.parametros.tam;
    funcaoIR.ehInicio = (strcmp(funcao.nome, "inicio") == 0);
    funcaoIR.temRetornoTipado = !funcao.tipoRetorno.ehNulo;

    // tabela de variaveis locais: vale so durante a geracao desta funcao.
    // parametros ja chegam prontos em x0..x5(convencao AAPCS64), entao
    // são pre-registrados aqui como REGISTRADOR, sem copia nenhuma. Novas
    // variáveis declaradas no corpo continuam alocando slots a partir daqui.
    ContextoFuncao ctx(funcao, dados);
    ctx.variaveisLocais.iniciar(funcao.parametros.tam);
    for(int i = 0; i < funcao.parametros.tam; i++) {
        ArgumentoIR arg;
        arg.tipo = ArgumentoIR::Tipo::REGISTRADOR;
        arg.simbolo = nullptr;
        arg.imediato = 0;
        arg.registrador = i;
        arg.larguraBytes = larguraDoTipo(funcao.parametros[i].tipo);
        arg.larguraElemento = larguraElementoApontado(funcao.parametros[i].tipo);
        arg.sinalizado = tipoEhSinalizado(funcao.parametros[i].tipo);
        arg.elementoSinalizado = elementoEhSinalizado(funcao.parametros[i].tipo);
        arg.ehPonteiro = tipoEhPonteiro(funcao.parametros[i].tipo);
        arg.ehFlutuante = tipoEhFlutuante(funcao.parametros[i].tipo);
        arg.elementoFlutuante = elementoEhFlutuante(funcao.parametros[i].tipo);
        if(funcao.parametros[i].nomeEspaco) arg.idEspaco = obterIdEspaco(funcao.parametros[i].nomeEspaco);
        if(funcao.parametros[i].tipo == TipoVariavel::ESPACO) arg.espacoPorValor = true; // "Nome x" por valor: o chamador passa o endereço de uma cópia, então "x = y" aqui copia campo a campo
        ctx.variaveisLocais.definirComSlot(funcao.parametros[i].nome, arg, i);
        if(funcao.parametros[i].tipo == TipoVariavel::ESPACO) registrarLiberacaoAutomatica(i, arg.idEspaco, ctx);
        funcaoIR.parametrosLargura.empurrar(arg.larguraBytes);
        funcaoIR.parametrosFlutuante.empurrar(arg.ehFlutuante);
    }
    emitirConstrutorBase(funcao, funcaoIR.instrucoes);
    funcaoIR.retornoFlutuante = !funcao.tipoRetorno.ehNulo && tipoEhFlutuante(funcao.tipoRetorno.tipo);
    funcaoIR.retornoLargura = funcao.tipoRetorno.ehNulo ? 8 : larguraDoTipo(funcao.tipoRetorno.tipo);
    bool terminaComSair = false;
    bool terminaComRetorne = false;
    for(int i = 0; i < funcao.corpo.tam; i++) {
        const Comando& comando = funcao.corpo[i];
        int marcaTemp = ctx.tempSlot.tam;
        terminaComSair = false;
        terminaComRetorne = false;
        if(comando.tipo == Comando::Tipo::DECLARACAO_VARIAVEL) {
            gerarDeclaracaoVariavel(comando.declaracaoVariavel, funcaoIR.instrucoes, ctx);
        } else if(comando.tipo == Comando::Tipo::REATRIBUICAO) {
            gerarReatribuicao(comando.reatribuicao, funcaoIR.instrucoes, ctx);
        } else if(comando.tipo == Comando::Tipo::CHAMADA_SISTEMA_CRUA) {
            gerarChamadaSistemaCrua(comando.chamadaSistemaCrua, funcaoIR.instrucoes, ctx);
            // o proprio ComandoIR::CHAMADA_SISTEMA_CRUA e sempre o
            // ULTIMO empurrado(qualquer auxiliar de argumento vem antes),
            // entao esta no topo da lista neste ponto.
            terminaComSair = comandoCrahamadaTerminaProcesso(funcaoIR.instrucoes[funcaoIR.instrucoes.tam - 1]);
        } else if(comando.tipo == Comando::Tipo::SE) {
            gerarComandoSe(comando.comandoSe, funcaoIR.instrucoes, ctx);
        } else if(comando.tipo == Comando::Tipo::ENQUANTO) {
            gerarComandoEnquanto(comando.comandoEnquanto, funcaoIR.instrucoes, ctx);
        } else if(comando.tipo == Comando::Tipo::POR) {
            gerarComandoPor(comando.comandoPor, funcaoIR.instrucoes, ctx);
        } else if(comando.tipo == Comando::Tipo::APAGAR) {
            gerarApagar(comando.comandoApagar, funcaoIR.instrucoes, ctx);
        } else if(comando.tipo == Comando::Tipo::RETORNE) {
            gerarRetorne(comando.comandoRetorne, funcaoIR.instrucoes, ctx);
            terminaComRetorne = true;
        } else if(comando.tipo == Comando::Tipo::ATRIBUICAO_INDEXADA) {
            gerarAtribuicaoIndexada(comando.atribuicaoIndexada, funcaoIR.instrucoes, ctx);
        } else if(comando.tipo == Comando::Tipo::ATRIBUICAO_CAMPO) {
            gerarAtribuicaoCampo(comando.atribuicaoCampo, funcaoIR.instrucoes, ctx);
        } else {
            gerarChamadaFuncao(comando.chamadaFuncao, funcaoIR.instrucoes, ctx);
        }
        emitirLiberacaoTemporarios(marcaTemp, true, funcaoIR.instrucoes, ctx);
    }
    if(!terminaComRetorne && !terminaComSair) emitirLiberacoesDeEscopo(0, false, funcaoIR.instrucoes, ctx);
    // pode emitir "ret". Se o programador não terminou com SAIR
    // explícito, força um sair(0) implícito pra não cair em lixo.
    if(funcaoIR.ehInicio && !terminaComSair) {
        funcaoIR.instrucoes.empurrar(gerarSairImplicito());
    }
    // funcao tipada(int/car/car*) precisa terminar com "retorne": sem
    // isso o corpo cairia direto no epilogo com um valor indefinido em
    // x0. "retorne" no meio do corpo(dentro de "se"/"enquanto") e
    // permitido e não passa por essa checagem, mas o ÚLTIMO comando do
    // corpo precisa ser "retorne" explicito.
    if(funcaoIR.temRetornoTipado && !terminaComRetorne) {
        std::string nomeStr = funcao.nome;
        throw std::runtime_error("Função '" + nomeStr + "' precisa terminar com 'retorne <valor>;'");
    }
    funcaoIR.bytesArrays = bytesArraysAtual;
    // total de slots de 16 bytes que o frame precisa: parâmetros +
    // variáveis declaradas(variaveisLocais.proximoSlot já contabiliza os dois).
    funcaoIR.totalSlots = ctx.variaveisLocais.proximoSlot;
    ctx.variaveisLocais.liberar();
    ctx.liberar();

    alocarRegistradores(funcaoIR);
    return funcaoIR;
}

// detecta se um ComandoIR de #chamada_sistema crua e a chamada SAIR,
// usada pra saber se "inicio" já termina o processo por conta propria.
inline bool GeradorIR::comandoCrahamadaTerminaProcesso(const ComandoIR& comandoIR) {
    if(comandoIR.tipo != ComandoIR::Tipo::CHAMADA_SISTEMA_CRUA) return false;
    if(comandoIR.chamadaSistemaCrua.argumentos.tam == 0) return false;
    const ArgumentoIR& idAbstrato = comandoIR.chamadaSistemaCrua.argumentos[0];
    if(idAbstrato.tipo != ArgumentoIR::Tipo::IMEDIATO) return false;
    return static_cast<ChamadaSistemaId>(idAbstrato.imediato) == ChamadaSistemaId::SAIR;
}

// monta "#chamada_sistema SAIR, 0" cru, usado como saída implícita de "inicio".
inline ComandoIR GeradorIR::gerarSairImplicito() {
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::CHAMADA_SISTEMA_CRUA;
    comandoIR.chamadaSistemaCrua.iniciar();

    ArgumentoIR idSair;
    idSair.tipo = ArgumentoIR::Tipo::IMEDIATO;
    idSair.simbolo = nullptr;
    idSair.imediato = static_cast<long>(ChamadaSistemaId::SAIR);
    comandoIR.chamadaSistemaCrua.argumentos.empurrar(idSair);

    ArgumentoIR codigo;
    codigo.tipo = ArgumentoIR::Tipo::IMEDIATO;
    codigo.simbolo = nullptr;
    codigo.imediato = 0;
    comandoIR.chamadaSistemaCrua.argumentos.empurrar(codigo);

    return comandoIR;
}

inline void GeradorIR::registrarLigacoes(const Programa& programa) {
    for(int i = 0; i < programa.ligacoes.tam; i++) {
        const char* nome = programa.ligacoes[i].caminho;
        int tamNome = (int)strlen(nome);
        bool jaTem = false;
        for(int j = 0; j < ligacoes.tam && !jaTem; j++) {
            int tamCandidato;
            const char* candidato = ligacoes.obter(j, &tamCandidato);
            jaTem = tamCandidato == tamNome && memcmp(candidato, nome, tamNome) == 0;
        }
        if(!jaTem) ligacoes.empurrar(nome);
    }
}
// "Nome_criar" de uma classe que herda: chama o "criar" da base(a mais próxima que tiver) antes do corpo, passando o próprio "este"(Nome* serve como Base*).
// É automático, então esse "criar" da base só pode ter o parâmetro "este". Classe sem "criar" próprio não chama o da base.
inline void GeradorIR::emitirConstrutorBase(const DeclaracaoFuncao& funcao, Vetor<ComandoIR>& saida) {
    if(funcao.parametros.tam == 0 || !funcao.parametros[0].nomeEspaco) return;
    if(strcmp(funcao.parametros[0].nome, "este") != 0) return;
    std::string nomeEspacoStr = funcao.parametros[0].nomeEspaco;
    if(nomeEspacoStr + "_criar" != funcao.nome) return;
    int id = espacos.buscar(nomeEspacoStr.c_str());
    if(id < 0) return;
    int idBase = baseDoEspaco(id);
    if(idBase < 0) return;
    std::string nomeCriar;
    if(!buscarMetodo(idBase, "criar", nomeCriar)) return;
    int totalParametros;
    funcoesUsuario.buscar(nomeCriar.c_str(), &totalParametros);
    if(totalParametros != 1) {
        throw std::runtime_error("O construtor '" + nomeCriar + "' da base de '" + nomeEspacoStr + "' tem parâmetros, mas o construtor da base é chamado automático e só pode ter 'este'");
    }
    ComandoIR comando;
    comando.tipo = ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO;
    comando.chamadaFuncaoUsuario.iniciar();
    comando.chamadaFuncaoUsuario.nome = strdup(nomeCriar.c_str());
    comando.chamadaFuncaoUsuario.argumentos.empurrar(argumentoEspaco(0, idBase));
    saida.empurrar(comando);
}