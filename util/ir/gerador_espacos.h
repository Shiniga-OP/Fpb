// util/ir/gerador_espacos.h
#pragma once
#include "geradorir.h"

// registra cada #espaco de "programa" na tabela global, checando nome de espaço repetido(programa principal ou biblioteca
// anterior) e nome de campo repetido dentro do mesmo espaço. "origem" é usada só na mensagem de erro.
inline void GeradorIR::registrarEspacos(const Programa& programa, const std::string& origem) {
    for(int i = 0; i < programa.espacos.tam; i++) {
        const DeclaracaoEspaco& espaco = programa.espacos[i];
        std::string nomeStr = espaco.nome;
        if(espacos.buscar(espaco.nome) >= 0) {
            throw std::runtime_error("Espaço '" + nomeStr + "' definido mais de uma vez (conflito ao incluir '" + origem + "')");
        }
        for(int a = 0; a < espaco.campos.tam; a++) {
            for(int b = a + 1; b < espaco.campos.tam; b++) {
                if(strcmp(espaco.campos[a].nome, espaco.campos[b].nome) == 0) {
                    std::string campoStr = espaco.campos[a].nome;
                    throw std::runtime_error("Espaço '" + nomeStr + "' tem o campo '" + campoStr + "' repetido");
                }
            }
        }
        espacos.definir(espaco.nome, &espaco);
    }
}

// id do espaço de nome "nome"(erro se não existir).
inline int GeradorIR::obterIdEspaco(const char* nome) {
    int id = espacos.buscar(nome);
    if(id < 0) {
        std::string nomeStr = nome;
        throw std::runtime_error("Espaço '" + nomeStr + "' não foi declarado (faltou #incluir a biblioteca correta?)");
    }
    return id;
}

// id da base de "idEspaco"(#classe Nome herda Base), ou -1 se não herda. validarHeranca já garantiu que a base existe.
inline int GeradorIR::baseDoEspaco(int idEspaco) {
    const char* base = espacos.declaracoes[idEspaco]->base;
    if(!base) return -1;
    return espacos.buscar(base);
}

// total de campos de instância, contando os da base primeiro.
inline int GeradorIR::totalCampos(int idEspaco) {
    int idBase = baseDoEspaco(idEspaco);
    int total = espacos.declaracoes[idEspaco]->campos.tam;
    if(idBase >= 0) total += totalCampos(idBase);
    return total;
}

// campo de índice "indice" na ordem do layout: primeiro os da base, depois os próprios.
inline const CampoEspaco& GeradorIR::campoDoIndice(int idEspaco, int indice) {
    int idBase = baseDoEspaco(idEspaco);
    int totalBase = idBase >= 0 ? totalCampos(idBase) : 0;
    if(indice < totalBase) return campoDoIndice(idBase, indice);
    return espacos.declaracoes[idEspaco]->campos[indice - totalBase];
}

// true se o espaço tem o campo de instância "campo"(próprio ou da base).
inline bool GeradorIR::existeCampo(int idEspaco, const char* campo) {
    int total = totalCampos(idEspaco);
    for(int i = 0; i < total; i++) {
        if(strcmp(campoDoIndice(idEspaco, i).nome, campo) == 0) return true;
    }
    return false;
}

// true se "idEspaco" é "idBase" ou herda dele(direta ou indiretamente): um Derivada* serve onde se espera Base*.
inline bool GeradorIR::ehSubespaco(int idEspaco, int idBase) {
    int id = idEspaco;
    while(id >= 0) {
        if(id == idBase) return true;
        id = baseDoEspaco(id);
    }
    return false;
}

// acha a função "Classe_metodo" subindo da classe pela cadeia de bases. Sem despacho virtual: vale a classe do tipo declarado da variável.
inline bool GeradorIR::buscarMetodo(int idEspaco, const char* metodo, std::string& nomeFuncao) {
    int id = idEspaco;
    while(id >= 0) {
        std::string candidato = std::string(espacos.declaracoes[id]->nome) + "_" + metodo;
        int totalParametros;
        if(funcoesUsuario.buscar(candidato.c_str(), &totalParametros)) {
            nomeFuncao = candidato;
            return true;
        }
        id = baseDoEspaco(id);
    }
    return false;
}

// campo estatico "campo" da classe ou de uma base. "simbolo" recebe o nome da variável global(da classe que declarou o campo).
inline const CampoEstatico* GeradorIR::buscarEstatico(int idEspaco, const char* campo, std::string& simbolo) {
    int id = idEspaco;
    while(id >= 0) {
        const DeclaracaoEspaco& espaco = *espacos.declaracoes[id];
        for(int i = 0; i < espaco.estaticos.tam; i++) {
            if(strcmp(espaco.estaticos[i].nome, campo) == 0) {
                simbolo = std::string("_estatico_") + espaco.nome + "_" + campo;
                return &espaco.estaticos[i];
            }
        }
        id = baseDoEspaco(id);
    }
    return nullptr;
}

// "nomeBase.campo" é um campo estatico? "nomeBase" pode ser o nome de uma classe ou uma variável de espaço(que não tenha um campo de instância com esse nome).
inline bool GeradorIR::localizarEstatico(const char* nomeBase, const char* campo, ContextoFuncao& ctx, std::string& simbolo, const CampoEstatico** estatico) {
    int id;
    ArgumentoIR variavel;
    if(ctx.variaveisLocais.buscar(nomeBase, &variavel)) {
        if(variavel.idEspaco < 0) return false;
        if(existeCampo(variavel.idEspaco, campo)) return false;
        id = variavel.idEspaco;
    } else {
        id = espacos.buscar(nomeBase);
        if(id < 0) return false;
    }
    *estatico = buscarEstatico(id, campo, simbolo);
    return *estatico != nullptr;
}

// guarda a variável global na lista que vai pro ProgramaIR(só as usadas são emitidas, sem repetir).
inline void GeradorIR::registrarGlobalUsada(const std::string& simbolo, int largura, long valorInicial) {
    for(int i = 0; i < globais.tam; i++) {
        if(simbolo == globais[i].simbolo) return;
    }
    VariavelGlobalIR global;
    global.simbolo = strdup(simbolo.c_str());
    global.largura = largura;
    global.valorInicial = valorInicial;
    globais.empurrar(global);
}

// coloca o endereço da variável global num slot novo(mesmo caminho do endereço de um texto literal) e devolve um REGISTRADOR que serve de base pra
// LEITURA_INDEXADA/ESCRITA_INDEXADA com índice 0.
inline ArgumentoIR GeradorIR::enderecoDeGlobal(const std::string& simbolo, const CampoEstatico& estatico, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    int largura = larguraDoTipo(estatico.tipo);
    registrarGlobalUsada(simbolo, largura, estatico.valorInicial);
    ArgumentoIR textoSimbolo;
    textoSimbolo.tipo = ArgumentoIR::Tipo::SIMBOLO_TEXTO;
    textoSimbolo.simbolo = strdup(simbolo.c_str());
    textoSimbolo.imediato = 0;
    textoSimbolo.larguraBytes = 8;
    textoSimbolo.ehPonteiro = true;
    int slot = ctx.variaveisLocais.proximoSlot++;
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::REATRIBUICAO;
    comandoIR.instrucaoReatribuicao.slotDestino = slot;
    comandoIR.instrucaoReatribuicao.valor = textoSimbolo;
    comandoIR.instrucaoReatribuicao.larguraDestino = 8;
    saida.empurrar(comandoIR);

    ArgumentoIR endereco = argumentoSlotLargura(slot, 8);
    endereco.ehPonteiro = true;
    endereco.larguraElemento = largura;
    return endereco;
}

// "Classe.campo" estatico como valor de leitura: LEITURA_INDEXADA no endereço da variável global, igual a um campo de instância.
inline ArgumentoIR GeradorIR::resolverAcessoEstatico(const std::string& simbolo, const CampoEstatico& estatico, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    if(estatico.ehFinal) return constanteDoTipo(estatico.valorInicial, estatico.tipo); // constante: sem global, sem leitura
    ArgumentoIR endereco = enderecoDeGlobal(simbolo, estatico, saida, ctx);
    int slot = ctx.variaveisLocais.proximoSlot++;
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::LEITURA_INDEXADA;
    comandoIR.instrucaoLeituraIndexada.slotDestino = slot;
    comandoIR.instrucaoLeituraIndexada.ponteiro = endereco;
    comandoIR.instrucaoLeituraIndexada.indice = imediatoInt(0);
    saida.empurrar(comandoIR);

    return argumentoSlot(slot, estatico.tipo);
}

// confere a herança depois que todos os espaços(do programa e das bibliotecas) foram registrados: base existe, sem ciclo, sem campo repetido
// na cadeia, campo estatico sem conflito e "@sobrepor" batendo com um método da base("criar" é construtor: não precisa de "@sobrepor").
inline void GeradorIR::validarHeranca() {
    int total = espacos.declaracoes.tam;
    for(int i = 0; i < total; i++) {
        const DeclaracaoEspaco& espaco = *espacos.declaracoes[i];
        if(!espaco.base) continue;
        std::string nomeStr = espaco.nome;
        std::string baseStr = espaco.base;
        if(espacos.buscar(espaco.base) < 0) {
            throw std::runtime_error("Classe '" + nomeStr + "' herda de '" + baseStr + "', que não foi declarada (faltou #incluir a biblioteca correta?)");
        }
        int id = i;
        int passos = 0;
        while(id >= 0) {
            if(++passos > total) throw std::runtime_error("Herança circular envolvendo '" + nomeStr + "'");
            id = baseDoEspaco(id);
        }
    }
    for(int i = 0; i < total; i++) {
        const DeclaracaoEspaco& espaco = *espacos.declaracoes[i];
        std::string nomeStr = espaco.nome;
        int idBase = baseDoEspaco(i);
        for(int c = 0; c < espaco.campos.tam; c++) {
            if(idBase >= 0 && existeCampo(idBase, espaco.campos[c].nome)) {
                std::string campoStr = espaco.campos[c].nome;
                throw std::runtime_error("O campo '" + campoStr + "' de '" + nomeStr + "' já existe numa classe base");
            }
        }
        for(int e = 0; e < espaco.estaticos.tam; e++) {
            std::string estaticoStr = espaco.estaticos[e].nome;
            if(existeCampo(i, espaco.estaticos[e].nome)) {
                throw std::runtime_error("'" + nomeStr + "." + estaticoStr + "' é campo estatico e campo de instância ao mesmo tempo");
            }
            for(int f = e + 1; f < espaco.estaticos.tam; f++) {
                if(strcmp(espaco.estaticos[e].nome, espaco.estaticos[f].nome) == 0) {
                    throw std::runtime_error("Classe '" + nomeStr + "' tem o campo estatico '" + estaticoStr + "' repetido");
                }
            }
        }
        for(int m = 0; m < espaco.metodos.tam; m++) {
            const MetodoDeclarado& metodo = espaco.metodos[m];
            std::string metodoStr = metodo.nome;
            bool achou = false;
            std::string donoStr;
            int ancestral = idBase;
            while(ancestral >= 0 && !achou) {
                const DeclaracaoEspaco& espacoAncestral = *espacos.declaracoes[ancestral];
                for(int k = 0; k < espacoAncestral.metodos.tam; k++) {
                    if(strcmp(espacoAncestral.metodos[k].nome, metodo.nome) == 0) {
                        achou = true;
                        donoStr = espacoAncestral.nome;
                        break;
                    }
                }
                ancestral = baseDoEspaco(ancestral);
            }
            bool ehConstrutor = metodo.nome[0] == '.'; // construtor e destrutor não precisam de @sobrepor
            if(achou && !metodo.ehSobrepor && !ehConstrutor) {
                throw std::runtime_error("O método '" + nomeStr + "." + metodoStr + "' já existe em '" + donoStr + "': marque com @sobrepor");
            }
            if(!achou && metodo.ehSobrepor) {
                throw std::runtime_error("'@sobrepor' em '" + nomeStr + "." + metodoStr + "', mas nenhuma classe base tem esse método");
            }
        }
    }
}

// posição(em bytes, a partir do endereço base) e tipo do campo "campo" do espaço "idEspaco". Layout calculado na hora, sem guardar: os campos da base
// vêm primeiro(mesmas posições na derivada, então um Derivada* serve como Base*) e cada campo é alinhado à própria largura(larguraDoTipo), então a posição
// é sempre múltipla da largura. Isso é o que permite reusar LEITURA_INDEXADA/ESCRITA_INDEXADA com índice imediato = pos / largura, sem instrução nova no IR.
inline void GeradorIR::posicaoDoCampo(int idEspaco, const char* campo, int* posSaida, TipoVariavel* tipoSaida) {
    int total = totalCampos(idEspaco);
    int pos = 0;
    for(int i = 0; i < total; i++) {
        const CampoEspaco& atual = campoDoIndice(idEspaco, i);
        int largura = larguraDoTipo(atual.tipo);
        pos = ((pos + largura - 1) / largura) * largura;
        if(strcmp(atual.nome, campo) == 0) {
            *posSaida = pos;
            *tipoSaida = atual.tipo;
            return;
        }
        pos += largura;
    }
    std::string nomeStr = espacos.declaracoes[idEspaco]->nome;
    std::string campoStr = campo;
    throw std::runtime_error("Espaço '" + nomeStr + "' não tem o campo '" + campoStr + "'");
}

// tamanho total em bytes do espaço(com os campos da base), arredondado pro maior alinhamento entre os campos.
inline int GeradorIR::tamanhoDoEspaco(int idEspaco) {
    int total = totalCampos(idEspaco);
    int pos = 0;
    int maiorLargura = 1;
    for(int i = 0; i < total; i++) {
        int largura = larguraDoTipo(campoDoIndice(idEspaco, i).tipo);
        pos = ((pos + largura - 1) / largura) * largura + largura;
        if(largura > maiorLargura) maiorLargura = largura;
    }
    return ((pos + maiorLargura - 1) / maiorLargura) * maiorLargura;
}

// id do espaço apontado pelo campo "Nome* campo" (ou -1 se o campo não aponta pra espaço).
inline int GeradorIR::idEspacoDoCampo(int idEspaco, const char* campo) {
    int total = totalCampos(idEspaco);
    for(int i = 0; i < total; i++) {
        const CampoEspaco& atual = campoDoIndice(idEspaco, i);
        if(strcmp(atual.nome, campo) != 0) continue;
        if(!atual.nomeEspaco) return -1;
        return obterIdEspaco(atual.nomeEspaco);
    }
    return -1;
}

// lê o campo "campo" do espaço cujo endereço base está em "base": emite LEITURA_INDEXADA(base + pos do campo) num slot novo e devolve
// REGISTRADOR com a largura e o sinal do campo. Campo "Nome*" devolve o idEspaco, pra permitir a.b.c.
inline ArgumentoIR GeradorIR::lerCampoDeBase(const ArgumentoIR& base, const char* campo, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    if(base.idEspaco < 0) {
        throw std::runtime_error(std::string("O valor à esquerda de '.") + campo + "' não é um espaço, não tem campos");
    }
    int posCampo;
    TipoVariavel tipoCampo;
    posicaoDoCampo(base.idEspaco, campo, &posCampo, &tipoCampo);
    int largura = larguraDoTipo(tipoCampo);

    int slot = ctx.variaveisLocais.proximoSlot++;
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::LEITURA_INDEXADA;
    comandoIR.instrucaoLeituraIndexada.slotDestino = slot;
    comandoIR.instrucaoLeituraIndexada.ponteiro = copiarArgumento(base);
    comandoIR.instrucaoLeituraIndexada.ponteiro.larguraElemento = largura;
    comandoIR.instrucaoLeituraIndexada.indice = imediatoInt(posCampo / largura);
    saida.empurrar(comandoIR);

    ArgumentoIR referenciaSlot = argumentoSlot(slot, tipoCampo);
    if(tipoCampo == TipoVariavel::ESPACO_PONTEIRO) {
        referenciaSlot.idEspaco = idEspacoDoCampo(base.idEspaco, campo);
    }
    return referenciaSlot;
}

// resolve "nome.campo" (ou "qualquer.coisa.campo") como valor de leitura. "nome" pode ser um espaço por valor ou um parâmetro/variável "Nome*": nos dois o
// slot guarda o endereço base, então o acesso é o mesmo.
inline ArgumentoIR GeradorIR::resolverAcessoCampo(const Expressao& expressao, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    if(expressao.esquerda->tipo != Expressao::Tipo::IDENTIFICADOR) {
        ArgumentoIR baseComposta = resolverExpressao(*expressao.esquerda, saida, ctx);
        return lerCampoDeBase(baseComposta, expressao.valor, saida, ctx);
    }
    std::string simbolo;
    const CampoEstatico* estatico = nullptr;
    if(localizarEstatico(expressao.esquerda->valor, expressao.valor, ctx, simbolo, &estatico)) {
        return resolverAcessoEstatico(simbolo, *estatico, saida, ctx);
    }
    ArgumentoIR base;
    ArgumentoIR esteBase;
    bool achouBase = ctx.variaveisLocais.buscar(expressao.esquerda->valor, &base);
    if(!achouBase && ctx.variaveisLocais.buscar("este", &esteBase) && esteBase.idEspaco >= 0 && existeCampo(esteBase.idEspaco, expressao.esquerda->valor)) {
        // "p.campo" com "p" campo de "este": vira "este.p.campo"
        base = lerCampoDeBase(esteBase, expressao.esquerda->valor, saida, ctx);
        achouBase = true;
    }
    if(!achouBase) {
        std::string nomeStr = expressao.esquerda->valor;
        if(espacos.buscar(expressao.esquerda->valor) >= 0) {
            throw std::runtime_error("Classe '" + nomeStr + "' não tem o campo estatico '" + expressao.valor + "'");
        }
        throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
    }
    if(base.idEspaco < 0) {
        std::string nomeStr = expressao.esquerda->valor;
        throw std::runtime_error("Variável '" + nomeStr + "' não é um espaço, não tem campos");
    }
    return lerCampoDeBase(base, expressao.valor, saida, ctx);
}

// emite ESCRITA_INDEXADA de "valor" no campo(posCampo, tipoCampo) do espaço cujo endereço base está em "base". Usada pela lista de
// inicialização e por "nome.campo = expr;". Imediato é montado direto na largura do campo; registrador mantém a largura de origem.
inline void GeradorIR::emitirEscritaCampo(const ArgumentoIR& base, int posCampo, TipoVariavel tipoCampo, ArgumentoIR valor, Vetor<ComandoIR>& saida) {
    int largura = larguraDoTipo(tipoCampo);
    if(valor.tipo == ArgumentoIR::Tipo::IMEDIATO) valor.larguraBytes = largura;
    ComandoIR comandoIR;
    comandoIR.tipo = ComandoIR::Tipo::ESCRITA_INDEXADA;
    comandoIR.instrucaoEscritaIndexada.ponteiro = copiarArgumento(base);
    comandoIR.instrucaoEscritaIndexada.ponteiro.larguraElemento = largura;
    comandoIR.instrucaoEscritaIndexada.indice = imediatoInt(posCampo / largura);
    comandoIR.instrucaoEscritaIndexada.valor = valor;
    saida.empurrar(comandoIR);
}

// grava "valor"(já resolvido) em "nome.campo": campo estático(global) ou de instância(base + posição).
inline ArgumentoIR GeradorIR::gravarEmCampo(const char* nome, const char* campo, ArgumentoIR valor, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    std::string simbolo;
    const CampoEstatico* estatico = nullptr;
    if(localizarEstatico(nome, campo, ctx, simbolo, &estatico)) {
        if(estatico->ehFinal) {
            std::string nomeStr = nome;
            std::string campoStr = campo;
            throw std::runtime_error("'" + nomeStr + "." + campoStr + "' é final e não pode ser alterado");
        }
        ArgumentoIR endereco = enderecoDeGlobal(simbolo, *estatico, saida, ctx);
        valor = converterParaTipo(valor, tipoEhFlutuante(estatico->tipo), larguraDoTipo(estatico->tipo), saida, ctx);
        if(valor.tipo == ArgumentoIR::Tipo::IMEDIATO) valor.larguraBytes = larguraDoTipo(estatico->tipo);
        emitirEscritaCampo(endereco, 0, estatico->tipo, valor, saida);
        return valor;
    }
    ArgumentoIR base;
    ArgumentoIR esteBase;
    bool achouBase = ctx.variaveisLocais.buscar(nome, &base);
    if(!achouBase && ctx.variaveisLocais.buscar("este", &esteBase) && esteBase.idEspaco >= 0 && existeCampo(esteBase.idEspaco, nome)) {
        // "p.campo = e" com "p" campo de "este": vira "este.p.campo = e"
        base = lerCampoDeBase(esteBase, nome, saida, ctx);
        achouBase = true;
    }
    if(!achouBase) {
        std::string nomeStr = nome;
        if(espacos.buscar(nome) >= 0) {
            std::string campoStr = campo;
            throw std::runtime_error("Classe '" + nomeStr + "' não tem o campo estatico '" + campoStr + "'");
        }
        throw std::runtime_error("Variável '" + nomeStr + "' não foi declarada");
    }
    if(base.idEspaco < 0) {
        std::string nomeStr = nome;
        throw std::runtime_error("Variável '" + nomeStr + "' não é um espaço, não tem campos");
    }
    return gravarEmCampoBase(base, campo, valor, saida, ctx);
}

// grava "valor"(já resolvido) no campo "campo" do espaço cujo endereço base está em "base".
inline ArgumentoIR GeradorIR::gravarEmCampoBase(const ArgumentoIR& base, const char* campo, ArgumentoIR valor, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    if(base.idEspaco < 0) {
        throw std::runtime_error(std::string("O valor à esquerda de '.") + campo + "' não é um espaço, não tem campos");
    }
    int posCampo;
    TipoVariavel tipoCampo;
    posicaoDoCampo(base.idEspaco, campo, &posCampo, &tipoCampo);
    valor = converterParaTipo(valor, tipoEhFlutuante(tipoCampo), larguraDoTipo(tipoCampo), saida, ctx);
    if(valor.tipo == ArgumentoIR::Tipo::IMEDIATO) valor.larguraBytes = larguraDoTipo(tipoCampo);
    emitirEscritaCampo(base, posCampo, tipoCampo, valor, saida);
    return valor;
}

// "nome.campo = expr;"
inline void GeradorIR::gerarAtribuicaoCampo(const AtribuicaoCampo& atr, Vetor<ComandoIR>& saida, ContextoFuncao& ctx) {
    ArgumentoIR valor = resolverExpressao(atr.valor, saida, ctx);
    gravarEmCampo(atr.nome, atr.campo, valor, saida, ctx);
}