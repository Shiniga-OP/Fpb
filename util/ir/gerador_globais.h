#pragma once
#include "geradorir.h"

inline std::string simboloGlobal(const char* nome) {
    return std::string("_global_") + nome;
}

inline CampoEstatico campoDoPonteiroGlobal() {
    CampoEstatico c;
    c.tipo = TipoVariavel::NULO_PONTEIRO;
    c.nome = nullptr;
    c.valorInicial = 0;
    c.ehFinal = false;
    return c;
}

// global escalar(int, car, flu, car*...): nem espaço nem final. Mora no .data e é lida/gravada direto na memória a cada uso, sem cache em slot.
inline bool ehGlobalEscalar(const DeclaracaoVariavel& g) {
    return g.tipo != TipoVariavel::ESPACO && !g.ehFinal;
}

inline void GeradorIR::registrarGlobaisDeclaradas(const Programa& programa) {
    for(int i = 0; i < programa.variaveisGlobais.tam; i++) {
        const DeclaracaoVariavel& decl = programa.variaveisGlobais[i];
        std::string nomeStr = decl.nome;
        if(decl.ehArray) {
            throw std::runtime_error("Variável global '" + nomeStr + "': array global ainda não é suportado");
        }
        if(decl.tipo == TipoVariavel::ESPACO_PONTEIRO && !decl.ehFinal) {
            throw std::runtime_error("Variável global '" + nomeStr + "': 'estatico Nome* x' ainda não é suportado, use 'estatico Nome x'");
        }
        if(ehGlobalEscalar(decl)) {
            bool numero = !decl.ehChamadaSistema && (decl.valorInicial.tipo == Expressao::Tipo::NUMERO_LITERAL || decl.valorInicial.tipo == Expressao::Tipo::DECIMAL_LITERAL);
            bool texto = !decl.ehChamadaSistema && decl.valorInicial.tipo == Expressao::Tipo::TEXTO_LITERAL;
            if(texto && !tipoEhPonteiro(decl.tipo)) throw std::runtime_error("Variável global '" + nomeStr + "': texto só pode inicializar um ponteiro(car*)");
            if(!numero && !texto) throw std::runtime_error("Variável global '" + nomeStr + "': o valor inicial precisa ser um número ou um texto constante");
        }
        for(int k = 0; k < globaisDeclarados.tam; k++) {
            if(nomeStr == globaisDeclarados[k]->nome) throw std::runtime_error("Variável global '" + nomeStr + "' definida mais de uma vez");
        }
        globaisDeclarados.empurrar(&decl);
    }
}

// valor inicial de uma global escalar no formato do .data: inteiro, ou padrão de bits pra flu/dobro.
// Texto vale 0 aqui: o endereço dele só é gravado em _iniciar_globais.
inline long GeradorIR::valorInicialGlobal(const DeclaracaoVariavel& g) {
    if(g.valorInicial.tipo == Expressao::Tipo::TEXTO_LITERAL) return 0;
    const char* texto = g.valorInicial.valor;
    if(!tipoEhFlutuante(g.tipo)) return atol(texto);
    double d = strtod(texto, nullptr);
    if(larguraDoTipo(g.tipo) == 4) {
        float f = (float)d;
        unsigned int bits;
        memcpy(&bits, &f, 4);
        return (long)bits;
    }
    long bits;
    memcpy(&bits, &d, 8);
    return bits;
}

// global escalar "nome" vista como campo estatico: leitura e escrita reaproveitam resolverAcessoEstatico/enderecoDeGlobal. "simbolo" recebe o nome no .data.
inline bool GeradorIR::buscarGlobalEscalar(const char* nome, CampoEstatico& campo, std::string& simbolo) {
    for(int i = 0; i < globaisDeclarados.tam; i++) {
        const DeclaracaoVariavel& g = *globaisDeclarados[i];
        if(!ehGlobalEscalar(g) || strcmp(g.nome, nome) != 0) continue;
        campo.tipo = g.tipo;
        campo.nome = g.nome;
        campo.valorInicial = valorInicialGlobal(g);
        campo.ehFinal = false;
        simbolo = simboloGlobal(g.nome);
        return true;
    }
    return false;
}

// no começo de cada função: liga cada global de espaço(ou final) a um slot local com o mesmo nome. Escalar fica de fora: é buscada por buscarGlobalEscalar a cada uso.
inline void GeradorIR::vincularGlobais(ContextoFuncao& ctx, Vetor<ComandoIR>& saida) {
    for(int i = 0; i < globaisDeclarados.tam; i++) {
        const DeclaracaoVariavel& g = *globaisDeclarados[i];
        if(ehGlobalEscalar(g)) continue;
        ArgumentoIR jaExiste;
        if(ctx.variaveisLocais.buscar(g.nome, &jaExiste)) continue; // parâmetro sombreia a global
        int slot = ctx.variaveisLocais.proximoSlot++;
        if(g.ehFinal) {
            ArgumentoIR valor = resolverExpressao(g.valorInicial, saida, ctx);
            valor = converterParaTipo(valor, tipoEhFlutuante(g.tipo), larguraDoTipo(g.tipo), saida, ctx);
            if(valor.tipo != ArgumentoIR::Tipo::IMEDIATO) {
                std::string nomeStr = g.nome;
                throw std::runtime_error("'final' exige um valor constante em tempo de compilação (global '" + nomeStr + "')");
            }
            ctx.variaveisLocais.definirComSlot(g.nome, constanteDoTipo(valor.imediato, g.tipo), slot);
            continue;
        }
        int id = obterIdEspaco(g.nomeEspaco);
        CampoEstatico campo = campoDoPonteiroGlobal();
        ArgumentoIR endereco = enderecoDeGlobal(simboloGlobal(g.nome), campo, saida, ctx);
        ComandoIR leitura;
        leitura.tipo = ComandoIR::Tipo::LEITURA_INDEXADA;
        leitura.instrucaoLeituraIndexada.slotDestino = slot;
        leitura.instrucaoLeituraIndexada.ponteiro = endereco;
        leitura.instrucaoLeituraIndexada.indice = imediatoInt(0);
        saida.empurrar(leitura);
        ctx.variaveisLocais.definirComSlot(g.nome, argumentoEspaco(slot, id), slot);
    }
}

// função sintética: aloca e constrói cada objeto global e guarda o ponteiro na variável global, e grava o endereço dos textos nas globais car*. Final e escalar numérica não passam por aqui.
inline FuncaoIR GeradorIR::gerarIniciarGlobais(Vetor<DadoEstaticoIR>& dados) {
    DeclaracaoFuncao decl;
    decl.iniciar();
    decl.nome = strdup("_iniciar_globais");
    FuncaoIR f;
    f.iniciar();
    bytesArraysAtual = 0;
    f.nome = strdup(decl.nome);
    ContextoFuncao ctx(decl, dados);
    ctx.variaveisLocais.iniciar(0);
    for(int i = 0; i < globaisDeclarados.tam; i++) {
        const DeclaracaoVariavel& g = *globaisDeclarados[i];
        if(ehGlobalEscalar(g) && g.valorInicial.tipo == Expressao::Tipo::TEXTO_LITERAL) {
            // "estatico car* x = "texto";": o endereço do texto não é constante no .data(PIE), então é gravado aqui
            CampoEstatico global;
            std::string simbolo;
            buscarGlobalEscalar(g.nome, global, simbolo);
            ArgumentoIR endereco = enderecoDeGlobal(simbolo, global, f.instrucoes, ctx);
            ArgumentoIR texto = resolverExpressao(g.valorInicial, f.instrucoes, ctx);
            emitirEscritaCampo(endereco, 0, g.tipo, texto, f.instrucoes);
            continue;
        }
        if(g.tipo != TipoVariavel::ESPACO) continue;
        int marcaTemp = ctx.tempSlot.tam;
        int id = obterIdEspaco(g.nomeEspaco);
        int slot = ctx.variaveisLocais.proximoSlot++;
        ArgumentoIR ref = argumentoEspaco(slot, id);
        emitirAlocar(slot, id, f.instrucoes);
        if(g.temLista) throw std::runtime_error(std::string("Variável global '") + g.nome + "': lista de inicialização ainda não é suportada, use 'estatico Nome x = expr;'");
        if(g.listaInicial.tam == 1) {
            ArgumentoIR valor = resolverExpressao(g.listaInicial[0], f.instrucoes, ctx);
            if(valor.tipo == ArgumentoIR::Tipo::REGISTRADOR && valor.idEspaco == id) {
                if(valor.ehTemporario) moverEspaco(valor, ref, id, f.instrucoes, ctx);
                else copiarObjeto(valor, ref, id, f.instrucoes, ctx);
            } else {
                emitirCriar(ref, valor, id, f.instrucoes);
            }
        } else {
            zerarEspaco(ref, id, f.instrucoes);
        }
        CampoEstatico campo = campoDoPonteiroGlobal();
        ArgumentoIR endereco = enderecoDeGlobal(simboloGlobal(g.nome), campo, f.instrucoes, ctx);
        emitirEscritaCampo(endereco, 0, TipoVariavel::NULO_PONTEIRO, copiarArgumento(ref), f.instrucoes);
        emitirLiberacaoTemporarios(marcaTemp, true, f.instrucoes, ctx);
    }
    f.totalSlots = ctx.variaveisLocais.proximoSlot;
    f.bytesArrays = bytesArraysAtual;
    ctx.variaveisLocais.liberar();
    ctx.liberar();
    decl.liberar();
    alocarRegistradores(f);
    return f;
}