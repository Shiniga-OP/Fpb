// util/ir/geradorir.h
#pragma once
#include <string>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../config.h"
#include "../ast.h"
#include "ir.h"
#include "../lexer.h"
#include "../analisador.h"
#include "tabelasir.h"
#include "alocadorreg.h"

class GeradorIR {
public:
    std::string diretorioBase;
    TabelaFuncoesUsuario funcoesUsuario; // assinaturas: programa principal + todas as bibliotecas incluidas(direta ou transitivamente)
    TabelaCorposFuncoesUsuario corposFuncoesUsuario; // nome -> AST completa(corpo), usada pra eliminação sob demanda
    VetorStr caminhosJaIncluidos; // caminhos absolutos ja processados, pra não incluir o mesmo arquivo duas vezes
    Vetor<Programa*> programasBibliotecas; // dono: GeradorIR. mantidos vivos ate o fim de gerar(), pois corposFuncoesUsuario aponta pra dentro deles
    int contadorSimbolos = 0;

    // diretorioBase: diretório a partir do qual os "#incluir" são resolvidos.
    explicit GeradorIR(const std::string& diretorioBase) : diretorioBase(diretorioBase) {
        funcoesUsuario.iniciar();
        corposFuncoesUsuario.iniciar();
        caminhosJaIncluidos.iniciar();
        programasBibliotecas.iniciar();
    }

    ~GeradorIR() {
        funcoesUsuario.liberar();
        corposFuncoesUsuario.liberar();
        caminhosJaIncluidos.liberar();
        for(int i = 0; i < programasBibliotecas.tam; i++) {
            programasBibliotecas[i]->liberar();
            delete programasBibliotecas[i];
        }
        programasBibliotecas.liberar();
    }

    // largura em bytes que o proprio VALOR de um tipo ocupa, usada tanto por
    // resolverBytes("bytes(tipo)") quanto pra popular ArgumentoIR.larguraBytes(decide w32/x64 e a instrução de memória usada pela arquitetura). Ponto
    // UNICO de verdade sobre "quantos bytes este tipo ocupa": adicionar um
    // tipo novo no futuro exige mudar só aqui.
    // car=1, int=4(32 bits), longo=8(64 bits), qualquer ponteiro=8(endereco).
    int larguraDoTipo(TipoVariavel tipo) {
        if(tipo == TipoVariavel::CAR) return 1;
        if(tipo == TipoVariavel::INT) return 4;
        return 8; // LONGO, CAR_PONTEIRO, LONGO_PONTEIRO, NULO_PONTEIRO
    }

    // largura em bytes de CADA ELEMENTO apontado por um tipo ponteiro,
    // usada por ArgumentoIR.larguraElemento(indexacao nome[indice]). Ponto
    // UNICO de verdade sobre "o que um ponteiro deste tipo aponta": adicionar
    // um tipo ponteiro novo no futuro exige mudar só aqui, não espalhar
    // "if"/"switch" por resolverIndexacao/gerarAtribuicaoIndexada/arm64.h.
    // Nao confundir com larguraDoTipo() acima: o PONTEIRO em si sempre
    // ocupa 8 bytes(endereco de 64 bits); aqui e' a largura do que ele APONTA.
    int larguraElementoApontado(TipoVariavel tipoPonteiro) {
        if(tipoPonteiro == TipoVariavel::CAR_PONTEIRO) return 1;
        if(tipoPonteiro == TipoVariavel::INT_PONTEIRO) return 4;
        if(tipoPonteiro == TipoVariavel::LONGO_PONTEIRO) return 8;
        return 1; // tipo não-ponteiro(ou NULO_PONTEIRO, generico) nunca deveria ser indexado sem conversão antes; 1 e o valor historico/seguro
    }


    ProgramaIR gerar(const Programa& programa) {
        // 1. registra a assinatura + corpo de cada função do programa principal.
        registrarFuncoes(programa);
        // 2. resolve cada "#incluir"(dedupe por caminho absoluto), carregando
        //    assinatura + corpo de cada função das bibliotecas(ex: biblis/impressao.fpb).
        for(int i = 0; i < programa.inclusoes.tam; i++) {
            carregarBiblioteca(programa.inclusoes[i].caminho, diretorioBase);
        }
        // 3. gera FuncaoIR só pras funções alcancaveis a partir
        //    do programa principal(direta ou indiretamente via chamadas "bl"),
        //    evitando emitir codigo morto de bibliotecas incluídas mas não usadas.
        ProgramaIR ir;
        ir.iniciar();
        VetorStr jaGeradas;
        jaGeradas.iniciar();
        for(int i = 0; i < programa.funcoes.tam; i++) {
            gerarFuncaoAlcancavel(programa.funcoes[i].nome, ir, jaGeradas);
        }
        jaGeradas.liberar();
        return ir;
    }

    // registra assinatura(nome/parâmetros/retorno) e corpo(AST) de cada
    // funcao de "programa" nas tabelas globais, checando nome duplicado
    // contra qualquer função já registrada(programa principal ou biblioteca
    // anterior). "origem" e usada so na mensagem de erro.
    void registrarFuncoes(const Programa& programa, const std::string& origem = "programa principal") {
        for(int i = 0; i < programa.funcoes.tam; i++) {
            const DeclaracaoFuncao& funcao = programa.funcoes[i];
            int totalParametrosIgnorado;
            if(funcoesUsuario.buscar(funcao.nome, &totalParametrosIgnorado)) {
                std::string nomeStr = funcao.nome;
                throw std::runtime_error("Função '" + nomeStr + "' definida mais de uma vez (conflito ao incluir '" + origem + "')");
            }
            funcoesUsuario.definir(funcao.nome, funcao.parametros.tam, funcao.tipoRetorno);
            corposFuncoesUsuario.definir(funcao.nome, &funcao);
        }
    }

    // gera(se ainda não foi gerada) a FuncaoIR de "nome" e, recursivamente,
    // de toda função de usuario chamada(direta ou indiretamente) a partir
    // dela. "jaGeradas" evita reprocessar a mesma função mais de uma vez
    // (tanto por seguranca contra recursão infinita quanto por eficiência).
    void gerarFuncaoAlcancavel(const char* nome, ProgramaIR& ir, VetorStr& jaGeradas) {
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
    void coletarChamadasUsuario(const Vetor<ComandoIR>& bloco, VetorStr& saida) {
        for(int i = 0; i < bloco.tam; i++) {
            const ComandoIR& c = bloco[i];
            if(c.tipo == ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO) {
                saida.empurrar(c.chamadaFuncaoUsuario.nome);
            } else if(c.tipo == ComandoIR::Tipo::SE) {
                coletarChamadasUsuario(*c.instrucaoSe.entao, saida);
                if(c.instrucaoSe.temSenao) coletarChamadasUsuario(*c.instrucaoSe.senaoCorpo, saida);
            } else if(c.tipo == ComandoIR::Tipo::ENQUANTO) {
                coletarChamadasUsuario(*c.instrucaoEnquanto.comandosCondicao, saida);
                coletarChamadasUsuario(*c.instrucaoEnquanto.corpo, saida);
            }
        }
    }

    // carrega uma biblioteca(#incluir "caminho.fpb"): "diretorioAtual" e o
    // diretório a partir do qual "caminhoRelativo" deve ser resolvido(o
    // diretório do arquivo que contem o #incluir, não sempre diretorioBase:
    // uma biblioteca em "biblis/impressao.fpb" que faz #incluir "./tex.fpb"
    // espera achar "biblis/tex.fpb", não "tex.fpb" na raiz). Resolve o
    // caminho absoluto e ignora silenciosamente se já foi incluida antes
    // (mesmo arquivo, independente de como o caminho foi escrito), já que
    // não ha cenario util pra incluir o mesmo arquivo duas vezes. Toda
    // funcao da biblioteca precisa ter corpo real(ex: usando #chamada_sistema
    // cru dentro do corpo pra acessar o sistema); não existe mais a forma
    // "nome(...) #chamada_sistema(N);" sem corpo.
    void carregarBiblioteca(const char* caminhoRelativo, const std::string& diretorioAtual) {
        std::filesystem::path caminho = std::filesystem::path(diretorioAtual) / caminhoRelativo;
        std::error_code erroCaminho;
        std::filesystem::path caminhoAbsoluto = std::filesystem::canonical(caminho, erroCaminho);
        if(erroCaminho) {
            throw std::runtime_error("Não foi possível abrir biblioteca incluída: " + caminho.string());
        }
        std::string caminhoAbsolutoStr = caminhoAbsoluto.string();

        for(int i = 0; i < caminhosJaIncluidos.tam; i++) {
            int tamCandidato;
            const char* candidato = caminhosJaIncluidos.obter(i, &tamCandidato);
            if(tamCandidato == (int)caminhoAbsolutoStr.size() && memcmp(candidato, caminhoAbsolutoStr.c_str(), tamCandidato) == 0) {
                return; // já incluída antes, mesmo arquivo: ignora silenciosamente
            }
        }
        caminhosJaIncluidos.empurrar(caminhoAbsolutoStr.c_str());

        std::ifstream arquivo(caminhoAbsoluto);
        if(!arquivo) {
            throw std::runtime_error("Não foi possível abrir biblioteca incluída: " + caminhoAbsolutoStr);
        }
        std::stringstream conteudoStream;
        conteudoStream << arquivo.rdbuf();
        std::string conteudo = conteudoStream.str();

        Lexer lexer(conteudo.c_str());
        Vetor<Token> tokens = lexer.tokenizar();
        Analisador analisador(tokens);

        // aloca no heap e mantem em programasBibliotecas: corposFuncoesUsuario
        // guarda ponteiros pras DeclaracaoFuncao de dentro deste Programa,
        // entao ele precisa sobreviver até o fim de gerar()(não pode ser
        // liberado aqui, diferente do comportamento anterior).
        Programa* bibliotecaPrograma = new Programa();
        *bibliotecaPrograma = analisador.analisar();

        registrarFuncoes(*bibliotecaPrograma, caminhoRelativo);
        programasBibliotecas.empurrar(bibliotecaPrograma);

        // bibliotecas também podem incluir outras bibliotecas: resolvidas
        // relativo ao próprio diretório desta biblioteca(caminhoAbsoluto),
        // não ao diretório do arquivo raiz.
        std::string diretorioDestaBiblioteca = caminhoAbsoluto.parent_path().string();
        for(int i = 0; i < bibliotecaPrograma->inclusoes.tam; i++) {
            carregarBiblioteca(bibliotecaPrograma->inclusoes[i].caminho, diretorioDestaBiblioteca);
        }
        // libera só os tokens(a AST da biblioteca continua viva em programasBibliotecas).
        for(int i = 0; i < tokens.tam; i++) tokens[i].liberar();
        tokens.liberar();
    }

    FuncaoIR gerarFuncao(const DeclaracaoFuncao& funcao, Vetor<DadoEstaticoIR>& dados) {
        if(funcao.parametros.tam > 6) {
            std::string nomeStr = funcao.nome;
            throw std::runtime_error("Função '" + nomeStr + "' aceita no máximo 6 parâmetros");
        }
        FuncaoIR funcaoIR;
        funcaoIR.iniciar();
        funcaoIR.nome = strdup(funcao.nome);
        funcaoIR.totalParametros = funcao.parametros.tam;
        funcaoIR.ehInicio = (strcmp(funcao.nome, "inicio") == 0);
        funcaoIR.temRetornoTipado = !funcao.tipoRetorno.ehNulo;

        // tabela de variaveis locais: vale so durante a geracao desta funcao.
        // parametros ja chegam prontos em x0..x5(convencao AAPCS64), entao
        // são pre-registrados aqui como REGISTRADOR, sem copia nenhuma. Novas
        // variáveis declaradas no corpo continuam alocando slots a partir daqui.
        TabelaVariaveisLocais variaveisLocais;
        variaveisLocais.iniciar(funcao.parametros.tam);
        for(int i = 0; i < funcao.parametros.tam; i++) {
            ArgumentoIR arg;
            arg.tipo = ArgumentoIR::Tipo::REGISTRADOR;
            arg.simbolo = nullptr;
            arg.imediato = 0;
            arg.registrador = i;
            arg.larguraBytes = larguraDoTipo(funcao.parametros[i].tipo);
            arg.larguraElemento = larguraElementoApontado(funcao.parametros[i].tipo);
            variaveisLocais.definirComSlot(funcao.parametros[i].nome, arg, i);
            funcaoIR.parametrosLargura.empurrar(arg.larguraBytes);
        }
        bool terminaComSair = false;
        bool terminaComRetorne = false;
        for(int i = 0; i < funcao.corpo.tam; i++) {
            const Comando& comando = funcao.corpo[i];
            terminaComSair = false;
            terminaComRetorne = false;
            if(comando.tipo == Comando::Tipo::DECLARACAO_VARIAVEL) {
                gerarDeclaracaoVariavel(comando.declaracaoVariavel, funcaoIR.instrucoes, dados, variaveisLocais);
            } else if(comando.tipo == Comando::Tipo::REATRIBUICAO) {
                gerarReatribuicao(comando.reatribuicao, funcaoIR.instrucoes, dados, variaveisLocais);
            } else if(comando.tipo == Comando::Tipo::CHAMADA_SISTEMA_CRUA) {
                gerarChamadaSistemaCrua(comando.chamadaSistemaCrua, funcaoIR.instrucoes, dados, variaveisLocais);
                // o proprio ComandoIR::CHAMADA_SISTEMA_CRUA e sempre o
                // ULTIMO empurrado(qualquer auxiliar de argumento vem antes),
                // entao esta no topo da lista neste ponto.
                terminaComSair = comandoCrahamadaTerminaProcesso(funcaoIR.instrucoes[funcaoIR.instrucoes.tam - 1]);
            } else if(comando.tipo == Comando::Tipo::SE) {
                gerarComandoSe(comando.comandoSe, funcao, funcaoIR.instrucoes, dados, variaveisLocais);
            } else if(comando.tipo == Comando::Tipo::ENQUANTO) {
                gerarComandoEnquanto(comando.comandoEnquanto, funcao, funcaoIR.instrucoes, dados, variaveisLocais);
            } else if(comando.tipo == Comando::Tipo::RETORNE) {
                gerarRetorne(comando.comandoRetorne, funcao, funcaoIR.instrucoes, dados, variaveisLocais);
                terminaComRetorne = true;
            } else if(comando.tipo == Comando::Tipo::ATRIBUICAO_INDEXADA) {
                gerarAtribuicaoIndexada(comando.atribuicaoIndexada, funcaoIR.instrucoes, dados, variaveisLocais);
            } else {
                gerarChamadaFuncao(comando.chamadaFuncao, funcaoIR.instrucoes, dados, variaveisLocais);
            }
        }
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
        // total de slots de 16 bytes que o frame precisa: parâmetros +
        // variáveis declaradas(variaveisLocais.proximoSlot já contabiliza os dois).
        funcaoIR.totalSlots = variaveisLocais.proximoSlot;
        variaveisLocais.liberar();

        alocarRegistradores(funcaoIR);
        return funcaoIR;
    }

    // detecta se um ComandoIR de #chamada_sistema crua e a chamada SAIR,
    // usada pra saber se "inicio" já termina o processo por conta propria.
    bool comandoCrahamadaTerminaProcesso(const ComandoIR& comandoIR) {
        if(comandoIR.tipo != ComandoIR::Tipo::CHAMADA_SISTEMA_CRUA) return false;
        if(comandoIR.chamadaSistemaCrua.argumentos.tam == 0) return false;
        const ArgumentoIR& idAbstrato = comandoIR.chamadaSistemaCrua.argumentos[0];
        if(idAbstrato.tipo != ArgumentoIR::Tipo::IMEDIATO) return false;
        return static_cast<ChamadaSistemaId>(idAbstrato.imediato) == ChamadaSistemaId::SAIR;
    }

    // gera IR pra "retorne;" ou "retorne expr;", validando que a presença(ou
    // ausência) de valor bate com o tipo de retorno declarado da função.
    // "saida" e a lista de instruções da funcao(ou do bloco atual): recebe
    // quaisquer comandos auxiliares que a resolucao da expressao precisar
    // emitir ANTES do retorno(ex: uma chamada de funcao ou operacao
    // aritmetica dentro de "retorne f() + 1;"). O ComandoIR::RETORNE em si
    // e' empurrado por ultimo, igual todo outro "gerar*" deste arquivo.
    void gerarRetorne(const ComandoRetorne& comandoRetorne, const DeclaracaoFuncao& funcao, Vetor<ComandoIR>& saida, Vetor<DadoEstaticoIR>& dados, TabelaVariaveisLocais& variaveisLocais) {
        std::string nomeStr = funcao.nome;
        if(funcao.tipoRetorno.ehNulo && comandoRetorne.temValor) {
            throw std::runtime_error("Função '" + nomeStr + "' é 'nulo' e não pode retornar um valor");
        }
        if(!funcao.tipoRetorno.ehNulo && !comandoRetorne.temValor) {
            throw std::runtime_error("Função '" + nomeStr + "' precisa retornar um valor do tipo declarado");
        }
        ComandoIR comandoIR;
        comandoIR.tipo = ComandoIR::Tipo::RETORNE;
        comandoIR.instrucaoRetorne.temValor = comandoRetorne.temValor;
        if(comandoRetorne.temValor) {
            comandoIR.instrucaoRetorne.valor = resolverExpressao(comandoRetorne.valor, saida, dados, variaveisLocais);
        }
        saida.empurrar(comandoIR);
    }

    // monta "#chamada_sistema SAIR, 0" cru, usado como saída implícita de "inicio".
    ComandoIR gerarSairImplicito() {
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

    // registra "int nome = expr;" ou "car* nome = expr;": aloca um slot novo
    // na tabela de variáveis locais(próximo após parametros/variáveis anteriores) e empurra em "saida" as instruções que materializam o
    // valor nesse slot(pode ser mais de uma comando se "expr" for uma operação aritmética, que passa por slots temporários), ja que a
    // variável precisa existir de verdade pra poder ser reatribuida depois.
    // registra "int nome = expr;", "car* nome = expr;" ou, com valor vindo
    // de chamada de sistema, "nulo* nome = #chamada_sistema id, arg1, ...;"
    // (captura x0, retorno da chamada de sistema, direto no slot recem-criado da
    // variavel, sem passar por uma REATRIBUICAO extra: mesmo resultado de
    // "nulo* p; p = #chamada_sistema ...;" mas numa unica declaracao).
    // Aloca um slot novo na tabela de variáveis locais(próximo após parametros/variáveis anteriores) e empurra em "saida" as instruções que materializam o
    // valor nesse slot(pode ser mais de uma comando se "expr" for uma operação aritmética, que passa por slots temporários), ja que a
    // variável precisa existir de verdade pra poder ser reatribuida depois.
    void gerarDeclaracaoVariavel(const DeclaracaoVariavel& decl, Vetor<ComandoIR>& saida, Vetor<DadoEstaticoIR>& dados, TabelaVariaveisLocais& variaveisLocais) {
        int slot = variaveisLocais.proximoSlot;
        ArgumentoIR referenciaSlot;
        referenciaSlot.tipo = ArgumentoIR::Tipo::REGISTRADOR;
        referenciaSlot.simbolo = nullptr;
        referenciaSlot.imediato = 0;
        referenciaSlot.registrador = slot;
        referenciaSlot.larguraBytes = larguraDoTipo(decl.tipo);
        referenciaSlot.larguraElemento = larguraElementoApontado(decl.tipo); // so relevante se esta variavel for usada como ponteiro numa indexacao(nome[i]); ver comentario de larguraElementoApontado()

        if(decl.ehChamadaSistema) {
            // a entrada na tabela precisa existir ANTES de gerar a chamada de
            // sistema(mesmo raciocínio do caso normal abaixo: leitura futura
            // tem que enxergar o slot, nao um valor congelado), mas o slot em
            // si so e definido de fato pelo ComandoIR::CHAMADA_SISTEMA_CRUA emitido a seguir(capturaResultado grava x0 nele apos o "svc").
            variaveisLocais.definir(decl.nome, referenciaSlot);

            if(decl.chamadaSistemaInicial.identificadores.tam == 0) {
                throw std::runtime_error("#chamada_sistema precisa de ao menos o número da chamada de sistema");
            }
            ComandoIR comandoIR;
            comandoIR.tipo = ComandoIR::Tipo::CHAMADA_SISTEMA_CRUA;
            comandoIR.chamadaSistemaCrua.iniciar();
            for(int i = 0; i < decl.chamadaSistemaInicial.identificadores.tam; i++) {
                comandoIR.chamadaSistemaCrua.argumentos.empurrar(resolverExpressao(decl.chamadaSistemaInicial.identificadores[i], saida, dados, variaveisLocais));
            }
            comandoIR.chamadaSistemaCrua.capturaResultado = true;
            comandoIR.chamadaSistemaCrua.slotDestino = slot;
            saida.empurrar(comandoIR);
            return;
        }

        ArgumentoIR valor = resolverExpressao(decl.valorInicial, saida, dados, variaveisLocais);
        valor.larguraBytes = larguraDoTipo(decl.tipo); // a largura da gravacao no slot segue o tipo declarado da variavel, não o tipo da expressão(um "longo" pode virar "car" via cast, e o valor resolvido ja reflete isso, mas fixamos aqui pra garantir consistencia mesmo em literais crus)

        // a entrada na tabela precisa ser sempre REGISTRADOR(lê o slot em tempo de execução), nunca o literal resolvido(SIMBOLO_TEXTO/IMEDIATO): o
        // slot pode ser sobrescrito por uma reatribuição mais adiante, e um
        // uso futuro da variável tem que enxergar o valor atual, não o valor
        // congelado da declaração. O "valor" resolvido serve so pra
        // materializar o conteúdo inicial no slot, no ComandoIR abaixo.
        variaveisLocais.definir(decl.nome, referenciaSlot);

        ComandoIR comandoIR;
        comandoIR.tipo = ComandoIR::Tipo::REATRIBUICAO;
        comandoIR.instrucaoReatribuicao.slotDestino = slot;
        comandoIR.instrucaoReatribuicao.valor = valor;
        saida.empurrar(comandoIR);
    }

    // "nome = expr;": busca o slot já alocado na declaracao e empurra em
    // "saida" as instruções que sobrescrevem o valor nesse mesmo slot.
    void gerarReatribuicao(const Reatribuicao& reat, Vetor<ComandoIR>& saida, Vetor<DadoEstaticoIR>& dados, TabelaVariaveisLocais& variaveisLocais) {
        ArgumentoIR existente;
        int slot;
        if(!variaveisLocais.buscar(reat.nome, &existente, &slot)) {
            std::string nomeStr = reat.nome;
            throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
        }
        ArgumentoIR novoValor = resolverExpressao(reat.valor, saida, dados, variaveisLocais);
        novoValor.larguraBytes = existente.larguraBytes; // a largura da gravacao no slot segue o tipo do slot já declarado, não o tipo da expressão do lado direito

        ComandoIR comandoIR;
        comandoIR.tipo = ComandoIR::Tipo::REATRIBUICAO;
        comandoIR.instrucaoReatribuicao.slotDestino = slot;
        comandoIR.instrucaoReatribuicao.valor = novoValor;
        saida.empurrar(comandoIR);
    }

    // "nome[indice] = expr;": grava "larguraElemento" bytes(1, 8, ou qualquer
    // largura futura) no endereço nome+(indice*larguraElemento), decidido pelo
    // tipo declarado do ponteiro(ver larguraElementoApontado()). "nome"
    // precisa ser uma variável ja declarada(o ponteiro sendo indexado).
    void gerarAtribuicaoIndexada(const AtribuicaoIndexada& atr, Vetor<ComandoIR>& saida, Vetor<DadoEstaticoIR>& dados, TabelaVariaveisLocais& variaveisLocais) {
        ArgumentoIR ponteiro;
        if(!variaveisLocais.buscar(atr.nome, &ponteiro)) {
            std::string nomeStr = atr.nome;
            throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
        }
        ArgumentoIR indice = resolverExpressao(atr.indice, saida, dados, variaveisLocais);
        ArgumentoIR valor = resolverExpressao(atr.valor, saida, dados, variaveisLocais);
        valor.larguraBytes = ponteiro.larguraElemento; // largura da gravacao segue a largura do elemento apontado pelo ponteiro

        ComandoIR comandoIR;
        comandoIR.tipo = ComandoIR::Tipo::ESCRITA_INDEXADA;
        comandoIR.instrucaoEscritaIndexada.ponteiro = copiarArgumento(ponteiro);
        comandoIR.instrucaoEscritaIndexada.indice = indice;
        comandoIR.instrucaoEscritaIndexada.valor = valor;
        saida.empurrar(comandoIR);
    }

    // traduz o operador aritmético da AST(OperadorAritmetico) pro equivalente
    // no IR(OperadorAritmeticoIR): mesmos nomes, só pra o IR não depender de ast.h.
    OperadorAritmeticoIR traduzirOperadorAritmetico(OperadorAritmetico op) {
        switch(op) {
            case OperadorAritmetico::SOMA: return OperadorAritmeticoIR::SOMA;
            case OperadorAritmetico::SUBTRACAO: return OperadorAritmeticoIR::SUBTRACAO;
            case OperadorAritmetico::MULTIPLICACAO: return OperadorAritmeticoIR::MULTIPLICACAO;
            case OperadorAritmetico::DIVISAO: return OperadorAritmeticoIR::DIVISAO;
            default: return OperadorAritmeticoIR::MODULO;
        }
    }

    // traduz o operador de comparação da AST(OperadorComparacao) pro
    // equivalente no IR(OperadorComparacaoIR): mesmos nomes, só pra o IR
    // não depender de ast.h. Usada por gerarComandoSe e gerarComandoEnquanto,
    // evita duplicar o mapeamento em cada uma.
    OperadorComparacaoIR traduzirOperadorComparacao(OperadorComparacao op) {
        switch(op) {
            case OperadorComparacao::IGUAL: return OperadorComparacaoIR::IGUAL;
            case OperadorComparacao::DIFERENTE: return OperadorComparacaoIR::DIFERENTE;
            case OperadorComparacao::MAIOR: return OperadorComparacaoIR::MAIOR;
            case OperadorComparacao::MENOR: return OperadorComparacaoIR::MENOR;
            case OperadorComparacao::MAIOR_IGUAL: return OperadorComparacaoIR::MAIOR_IGUAL;
            default: return OperadorComparacaoIR::MENOR_IGUAL;
        }
    }

    // resolve uma Expressao(literal, identificador ou binaria) para um
    // resolve qualquer Expressao(literal, identificador, chamada de função, conversão de tipo ou binaria) para um ArgumentoIR que representa seu
    // valor final. "saida" recebe qualquer instrução auxiliar que a
    // resolução precisar emitir ANTES do valor ficar pronto(chamada de função via "bl", ou operação aritmética com slot temporário): por
    // isso esta e a ÚNICA rota de resolução de expressao no gerador,
    // usada tanto em declaração/reatribuição/retorne quanto em condição de
    // "se"/"enquanto", #chamada_sistema e argumento de chamada de função.
    ArgumentoIR resolverExpressao(const Expressao& expressao, Vetor<ComandoIR>& saida, Vetor<DadoEstaticoIR>& dados, TabelaVariaveisLocais& variaveisLocais) {
        if(expressao.tipo == Expressao::Tipo::CONVERSAO) {
            return resolverConversao(expressao, saida, dados, variaveisLocais);
        }
        if(expressao.tipo == Expressao::Tipo::CHAMADA_FUNCAO) {
            return resolverChamadaFuncaoComoValor(expressao, saida, dados, variaveisLocais);
        }
        if(expressao.tipo == Expressao::Tipo::CHAMADA_SISTEMA) {
            return resolverChamadaSistemaComoValor(expressao, saida, dados, variaveisLocais);
        }
        if(expressao.tipo == Expressao::Tipo::BYTES) {
            return resolverBytes(expressao, variaveisLocais);
        }
        if(expressao.tipo == Expressao::Tipo::TEXTO_LITERAL) {
            char* simbolo = gerarNomeSimbolo(expressao.valor);

            DadoEstaticoIR dado;
            dado.simbolo = strdup(simbolo);
            dado.valor = strdup(expressao.valor);
            dado.tamanho = (int)strlen(expressao.valor);
            dados.empurrar(dado);

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
            arg.larguraBytes = 4; // literal numerico solto(ex: "10" em "quociente * 10") e' tratado como "int"(32 bits) por padrao, o tipo "natural" da linguagem: se combinado com um "longo"(64 bits) numa operacao, a regra de promocao em resolverExpressao(BINARIA) ja expande o resultado pra 64 bits: nao precisa que o literal em si comece largo
            return arg;
        }
        if(expressao.tipo == Expressao::Tipo::IDENTIFICADOR) {
            // identificador: precisa ser uma variável local já declarada.
            ArgumentoIR encontrado;
            if(!variaveisLocais.buscar(expressao.valor, &encontrado)) {
                std::string nomeStr = expressao.valor;
                throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
            }
            // cópia independente: quem chamou resolverExpressao e dono do resultado.
            return copiarArgumento(encontrado);
        }
        if(expressao.tipo == Expressao::Tipo::INDEXACAO) {
            return resolverIndexacao(expressao, saida, dados, variaveisLocais);
        }
        // só sobra BINARIA daqui pra baixo.
        ArgumentoIR esquerda = resolverExpressao(*expressao.esquerda, saida, dados, variaveisLocais);
        ArgumentoIR direita = resolverExpressao(*expressao.direita, saida, dados, variaveisLocais);

        int slot = variaveisLocais.proximoSlot++;
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
    ArgumentoIR resolverBytes(const Expressao& expressao, TabelaVariaveisLocais& variaveisLocais) {
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
    ArgumentoIR resolverIndexacao(const Expressao& expressao, Vetor<ComandoIR>& saida, Vetor<DadoEstaticoIR>& dados, TabelaVariaveisLocais& variaveisLocais) {
        ArgumentoIR ponteiro;
        if(!variaveisLocais.buscar(expressao.esquerda->valor, &ponteiro)) {
            std::string nomeStr = expressao.esquerda->valor;
            throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
        }
        ArgumentoIR indice = resolverExpressao(*expressao.direita, saida, dados, variaveisLocais);

        int slot = variaveisLocais.proximoSlot++;
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
    ArgumentoIR resolverConversao(const Expressao& expressao, Vetor<ComandoIR>& saida, Vetor<DadoEstaticoIR>& dados, TabelaVariaveisLocais& variaveisLocais) {
        ArgumentoIR valor = resolverExpressao(*expressao.esquerda, saida, dados, variaveisLocais);
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

    // cópia independente de um ArgumentoIR(duplica "simbolo" se houver),
    // usada quando o mesmo valor resolvido precisa viver em dois lugares
    // donos diferentes(ex: na tabela de variáveis e no ComandoIR emitido).
    ArgumentoIR copiarArgumento(const ArgumentoIR& original) {
        ArgumentoIR copia;
        copia.tipo = original.tipo;
        copia.imediato = original.imediato;
        copia.registrador = original.registrador;
        copia.simbolo = original.simbolo ? strdup(original.simbolo) : nullptr;
        copia.larguraBytes = original.larguraBytes;
        copia.larguraElemento = original.larguraElemento;
        return copia;
    }

    // gera IR pra "#chamada_sistema id, arg1, arg2, ...;" ou, com destino,
    // "nome = #chamada_sistema id, arg1, arg2, ...;"(captura x0 apos o "svc"
    // num slot ja existente, mesma ideia de resolverChamadaFuncaoComoValor
    // pra "bl", so que "nome" precisa ja ter sido declarado antes: o
    // slot(memoria) tem que existir pra receber o valor). "saida" recebe
    // tanto os comandos auxiliares de cada argumento(ex: um "bl" se algum
    // identificador for uma chamada de função) quanto o proprio comando final.
    void gerarChamadaSistemaCrua(const ChamadaSistemaCrua& chamada, Vetor<ComandoIR>& saida, Vetor<DadoEstaticoIR>& dados, TabelaVariaveisLocais& variaveisLocais) {
        if(chamada.identificadores.tam == 0) {
            throw std::runtime_error("#chamada_sistema precisa de ao menos o número da chamada de sistema");
        }
        ComandoIR comandoIR;
        comandoIR.tipo = ComandoIR::Tipo::CHAMADA_SISTEMA_CRUA;
        comandoIR.chamadaSistemaCrua.iniciar();

        for(int i = 0; i < chamada.identificadores.tam; i++) {
            comandoIR.chamadaSistemaCrua.argumentos.empurrar(resolverExpressao(chamada.identificadores[i], saida, dados, variaveisLocais));
        }
        if(chamada.temDestino) {
            ArgumentoIR existente;
            int slot;
            if(!variaveisLocais.buscar(chamada.nomeDestino, &existente, &slot)) {
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
    void gerarComandoSe(const ComandoSe& comandoSe, const DeclaracaoFuncao& funcao, Vetor<ComandoIR>& saida, Vetor<DadoEstaticoIR>& dados, TabelaVariaveisLocais& variaveisLocais) {
        ComandoIR comandoIR;
        comandoIR.tipo = ComandoIR::Tipo::SE;
        comandoIR.instrucaoSe.iniciar();
        comandoIR.instrucaoSe.esquerda = resolverExpressao(comandoSe.condicao.esquerda, saida, dados, variaveisLocais);
        comandoIR.instrucaoSe.direita = resolverExpressao(comandoSe.condicao.direita, saida, dados, variaveisLocais);
        comandoIR.instrucaoSe.operador = traduzirOperadorComparacao(comandoSe.condicao.operador);

        gerarBlocoComandos(*comandoSe.entao, *comandoIR.instrucaoSe.entao, funcao, dados, variaveisLocais);

        if(comandoSe.temSenao) {
            comandoIR.instrucaoSe.temSenao = true;
            comandoIR.instrucaoSe.senaoCorpo = new Vetor<ComandoIR>();
            comandoIR.instrucaoSe.senaoCorpo->iniciar();
            gerarBlocoComandos(*comandoSe.senaoCorpo, *comandoIR.instrucaoSe.senaoCorpo, funcao, dados, variaveisLocais);
        }
        saida.empurrar(comandoIR);
    }

    // gera IR pra "enquanto(esquerda OP direita) { corpo }". Mesma ideia de
    // gerarComandoSe: comandos auxiliares da condição vao em "saida" antes
    // do ComandoIR::ENQUANTO.
    // gera IR pra "enquanto(esquerda OP direita) { corpo }". Diferente de
    // gerarComandoSe, as instruções auxiliares da condição(ex: LEITURA_INDEXADA
    // de "txt[i]") vao em comandosCondicao(reemitidas a cada interação pela
    // arquitetura), NÃO em "saida": se fossem resolvidas só uma vez antes do
    // laço, uma condição como "txt[i] != 0" nunca refletiria o "i" atualizado
    // dentro do corpo, e o laço rodaria pra sempre(ou nunca) dependendo do
    // valor inicial. "esquerda"/"direita" continuam avaliadas nesse
    // comandosCondicao isolado, com seu proprio slot temporário se precisar.
    void gerarComandoEnquanto(const ComandoEnquanto& comandoEnquanto, const DeclaracaoFuncao& funcao, Vetor<ComandoIR>& saida, Vetor<DadoEstaticoIR>& dados, TabelaVariaveisLocais& variaveisLocais) {
        ComandoIR comandoIR;
        comandoIR.tipo = ComandoIR::Tipo::ENQUANTO;
        comandoIR.instrucaoEnquanto.iniciar();
        comandoIR.instrucaoEnquanto.esquerda = resolverExpressao(comandoEnquanto.condicao.esquerda, *comandoIR.instrucaoEnquanto.comandosCondicao, dados, variaveisLocais);
        comandoIR.instrucaoEnquanto.direita = resolverExpressao(comandoEnquanto.condicao.direita, *comandoIR.instrucaoEnquanto.comandosCondicao, dados, variaveisLocais);
        comandoIR.instrucaoEnquanto.operador = traduzirOperadorComparacao(comandoEnquanto.condicao.operador);

        gerarBlocoComandos(*comandoEnquanto.corpo, *comandoIR.instrucaoEnquanto.corpo, funcao, dados, variaveisLocais);
        saida.empurrar(comandoIR);
    }

    // gera IR pra cada comando de um bloco("então" ou "senão" de um "se", ou
    // corpo de um "enquanto"), empurrando na lista de saida já alocada pelo
    // chamador. "função" e a declaracao da funcao que contem este bloco,
    // usada so pra validar "retorne" aninhado(tipo bate com a assinatura).
    void gerarBlocoComandos(const Vetor<Comando>& corpo, Vetor<ComandoIR>& saida, const DeclaracaoFuncao& funcao, Vetor<DadoEstaticoIR>& dados, TabelaVariaveisLocais& variaveisLocais) {
        for(int i = 0; i < corpo.tam; i++) {
            const Comando& comando = corpo[i];
            if(comando.tipo == Comando::Tipo::DECLARACAO_VARIAVEL) {
                gerarDeclaracaoVariavel(comando.declaracaoVariavel, saida, dados, variaveisLocais);
            } else if(comando.tipo == Comando::Tipo::REATRIBUICAO) {
                gerarReatribuicao(comando.reatribuicao, saida, dados, variaveisLocais);
            } else if(comando.tipo == Comando::Tipo::CHAMADA_SISTEMA_CRUA) {
                gerarChamadaSistemaCrua(comando.chamadaSistemaCrua, saida, dados, variaveisLocais);
            } else if(comando.tipo == Comando::Tipo::SE) {
                gerarComandoSe(comando.comandoSe, funcao, saida, dados, variaveisLocais);
            } else if(comando.tipo == Comando::Tipo::ENQUANTO) {
                gerarComandoEnquanto(comando.comandoEnquanto, funcao, saida, dados, variaveisLocais);
            } else if(comando.tipo == Comando::Tipo::RETORNE) {
                gerarRetorne(comando.comandoRetorne, funcao, saida, dados, variaveisLocais);
            } else if(comando.tipo == Comando::Tipo::ATRIBUICAO_INDEXADA) {
                gerarAtribuicaoIndexada(comando.atribuicaoIndexada, saida, dados, variaveisLocais);
            } else {
                gerarChamadaFuncao(comando.chamadaFuncao, saida, dados, variaveisLocais);
            }
        }
    }

    // gera IR pra uma chamada de função de usuário usada como COMANDO de
    // topo(valor de retorno, se houver, e descartado): "bl nome". "saida"
    // recebe os comandos auxiliares de cada argumento(ex: chamada de funcao
    // aninhada) antes do comando final.
    void gerarChamadaFuncao(const ChamadaFuncao& chamada, Vetor<ComandoIR>& saida, Vetor<DadoEstaticoIR>& dados, TabelaVariaveisLocais& variaveisLocais) {
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
            comandoIR.chamadaFuncaoUsuario.argumentos.empurrar(resolverExpressao(chamada.argumentos[j], saida, dados, variaveisLocais));
        }
        saida.empurrar(comandoIR);
    }

    // resolve "f(arg1, arg2, ...)" usada DENTRO de uma expressao(ex:
    // "int x = f();" ou "se(f() == 0)"), diferente de gerarChamadaFuncao(que
    // trata a chamada como COMANDO de topo, valor descartado). Só aceita
    // função de usuario, e exige que ela nao seja "nulo". Emite "bl nome"
    // com capturaResultado=true(grava x0 num slot novo logo apos o bl) e
    // devolve uma referencia REGISTRADOR pra esse slot, mesmo padrao de
    // resolverExpressao pra BINARIA.
    // resolve "#chamada_sistema id, arg1, arg2, ...;" usada DENTRO de uma
    // expressao(ex: "retorne #chamada_sistema ...;", "escrever(#chamada_sistema ...)"), diferente de gerarChamadaSistemaCrua(que trata a chamada
    // como COMANDO de topo). Emite o ComandoIR::CHAMADA_SISTEMA_CRUA com
    // capturaResultado=true num slot TEMPORARIO novo(mesmo padrao de
    // resolverChamadaFuncaoComoValor pra "bl") e devolve uma referencia
    // REGISTRADOR pra esse slot: o valor da expressao e o retorno da chamada de sistema(x0), capturado logo apos o "svc".
    ArgumentoIR resolverChamadaSistemaComoValor(const Expressao& expressao, Vetor<ComandoIR>& saida, Vetor<DadoEstaticoIR>& dados, TabelaVariaveisLocais& variaveisLocais) {
        const ChamadaSistemaCrua& chamada = *expressao.chamadaSistema;
        if(chamada.identificadores.tam == 0) {
            throw std::runtime_error("#chamada_sistema precisa de ao menos o número da chamada de sistema");
        }
        ComandoIR comandoIR;
        comandoIR.tipo = ComandoIR::Tipo::CHAMADA_SISTEMA_CRUA;
        comandoIR.chamadaSistemaCrua.iniciar();
        for(int i = 0; i < chamada.identificadores.tam; i++) {
            comandoIR.chamadaSistemaCrua.argumentos.empurrar(resolverExpressao(chamada.identificadores[i], saida, dados, variaveisLocais));
        }

        int slot = variaveisLocais.proximoSlot++;
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

    ArgumentoIR resolverChamadaFuncaoComoValor(const Expressao& expressao, Vetor<ComandoIR>& saida, Vetor<DadoEstaticoIR>& dados, TabelaVariaveisLocais& variaveisLocais) {
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
            comandoIR.chamadaFuncaoUsuario.argumentos.empurrar(resolverExpressao((*expressao.argumentosFuncao)[j], saida, dados, variaveisLocais));
        }
        int slot = variaveisLocais.proximoSlot++;
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

    char* gerarNomeSimbolo(const char* textoOriginal) {
        // gera um nome de símbolo legível a partir do conteúdo do texto,
        // ex: "Ola Mundo\n" -> "_ola_mundo_0"
        BufAcumulador base;
        base.iniciar();
        for(const char* p = textoOriginal; *p; p++) {
            unsigned char c = (unsigned char)*p;
            if(isalnum(c)) {
                base.empurrar((char)tolower(c));
            } else if(c == ' ' && base.tam > 0 && base.dados[base.tam - 1] != '_') {
                base.empurrar('_');
            }
        }
        while(base.tam > 0 && base.dados[base.tam - 1] == '_') {
            base.tam--;
            base.dados[base.tam] = '\0';
        }
        if(base.tam == 0) {
            base.empurrar('s'); base.empurrar('t'); base.empurrar('r');
        }
        std::string resultado = "_" + std::string(base.dados) + "_" + std::to_string(contadorSimbolos++);
        base.liberar();
        return strdup(resultado.c_str());
    }
};