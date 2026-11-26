// util/preprocessador.h
#pragma once
#include <string>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "lexer.h"
#include "diagnostico.h"

// raiz das bibliotecas: variável de ambiente FPB_RAIZ, ou "." se não existir.
inline std::string raizBiblisPadrao() {
    const char* env = getenv("FPB_RAIZ");
    if(env && env[0]) return env;
    return ".";
}

// única fonte de verdade pra resolver o caminho de um "#incluir"(usada aqui e no CarregadorBiblis).
// "~caminho" e "~/caminho" são relativos a "raizBiblis"; qualquer outro é relativo a "diretorioAtual".
inline std::filesystem::path resolverCaminhoInclusao(const char* caminhoRelativo, const std::string& diretorioAtual, const std::string& raizBiblis) {
    if(caminhoRelativo[0] == '~') {
        const char* resto = caminhoRelativo + 1;
        while(*resto == '/') resto++;
        return std::filesystem::path(raizBiblis) / resto;
    }
    return std::filesystem::path(diretorioAtual) / caminhoRelativo;
}

// roda entre o Lexer e o Analisador: guarda cada "#macro NOME ... #macrofim" numa tabela, troca cada uso de NOME pelo corpo e junta textos adjacentes ("a" "b" vira "ab").
// a tabela é única: o arquivo principal e todas as bibliotecas usam a mesma instância, então uma macro definida numa biblioteca vale em quem a inclui.
// pra isso, ao achar "#incluir", o pré-processador varre o arquivo incluído só pra coletar as macros(os tokens dele são descartados, o CarregadorBiblis lê o arquivo de novo).
class Preprocessador {
public:
    VetorStr nomes;
    Vetor<Vetor<Token>> corpos; // dono: Preprocessador. paralelo a nomes
    VetorStr arquivosVistos; // caminhos absolutos já varridos
    VetorStr pilha; // macros sendo expandidas agora, pra barrar recursão
    std::string raizBiblis = raizBiblisPadrao(); // raiz das bibliotecas, usada nos "~caminho"

    Preprocessador() {
        nomes.iniciar();
        corpos.iniciar();
        arquivosVistos.iniciar();
        pilha.iniciar();
    }

    Preprocessador(const Preprocessador&) = delete;
    Preprocessador& operator=(const Preprocessador&) = delete;

    ~Preprocessador() {
        nomes.liberar();
        for(int i = 0; i < corpos.tam; i++) {
            for(int j = 0; j < corpos[i].tam; j++) corpos[i][j].liberar();
            corpos[i].liberar();
        }
        corpos.liberar();
        arquivosVistos.liberar();
        pilha.liberar();
    }

    // consome "entrada"(os tokens são movidos ou liberados aqui) e devolve o vetor já expandido. "diretorio" resolve os "#incluir" relativos.
    Vetor<Token> processar(Vetor<Token> entrada, const std::string& diretorio) {
        Vetor<Token> saida;
        saida.iniciar();
        int pos = 0;
        
        struct BlocoCond {
            bool paiAtivo;
            bool tomou;
            bool temSenao;
            Token origem;
        };
        Vetor<BlocoCond> conds;
        conds.iniciar();
        bool ativo = true;
        
        while(pos < entrada.tam) {
            Token t = entrada[pos];
            
            if(t.tipo == TipoToken::PRE_SE) {
                Token origem = t;
                origem.valor = nullptr;
                t.liberar();
                pos++;
                bool cond = lerCondicao(entrada, pos, origem);
                conds.empurrar({ativo, ativo && cond, false, origem});
                ativo = ativo && cond;
                continue;
            }
            if(t.tipo == TipoToken::PRE_SENAOSE) {
                if(conds.tam == 0 || conds[conds.tam - 1].temSenao) throw erroNoToken(ERRO_PRE, "'#senaose' inesperado", t);
                Token origem = t;
                origem.valor = nullptr;
                t.liberar();
                pos++;
                bool cond = lerCondicao(entrada, pos, origem);
                BlocoCond& topo = conds[conds.tam - 1];
                bool toma = topo.paiAtivo && !topo.tomou && cond;
                topo.tomou = topo.tomou || toma;
                ativo = toma;
                continue;
            }
            if(t.tipo == TipoToken::PRE_SENAO) {
                if(conds.tam == 0 || conds[conds.tam - 1].temSenao) throw erroNoToken(ERRO_PRE, "'#senao' inesperado", t);
                BlocoCond& topo = conds[conds.tam - 1];
                topo.temSenao = true;
                ativo = topo.paiAtivo && !topo.tomou;
                t.liberar();
                pos++;
                continue;
            }
            if(t.tipo == TipoToken::PRE_FIM) {
                if(conds.tam == 0) throw erroNoToken(ERRO_PRE, "'#sefim' sem '#se'", t);
                ativo = conds[conds.tam - 1].paiAtivo;
                conds.tam--;
                t.liberar();
                pos++;
                continue;
            }
            if(!ativo) {
                t.liberar();
                pos++;
                continue;
            }
            if(t.tipo == TipoToken::MACRO) {
                pos = lerMacro(entrada, pos);
                continue;
            }
            if(t.tipo == TipoToken::MACROFIM) {
                throw erroNoToken(ERRO_PRE, "'#macrofim' sem '#macro'", t);
            }
            if(t.tipo == TipoToken::INCLUIR && pos + 1 < entrada.tam && entrada[pos + 1].tipo == TipoToken::TEXTO) {
                varrerInclusao(entrada[pos + 1].valor, diretorio);
                saida.empurrar(t);
                saida.empurrar(entrada[pos + 1]);
                pos += 2;
                continue;
            }
            if(t.tipo == TipoToken::IDENTIFICADOR) {
                int indice = buscar(t.valor);
                if(indice >= 0) {
                    pilha.empurrar(t.valor);
                    expandir(corpos[indice], t, saida);
                    pilha.tam--;
                    t.liberar();
                    pos++;
                    continue;
                }
            }
            saida.empurrar(t);
            pos++;
        }
        if(conds.tam > 0) throw erroNoToken(ERRO_PRE, "'#se' sem '#sefim'", conds[conds.tam - 1].origem);
        conds.liberar();
        entrada.liberar(); // só o array: cada token já foi movido pra saida ou liberado
        return juntarTextos(saida);
    }

    // índice da macro "nome", ou -1.
    int buscar(const char* nome) const {
        return nomes.buscar(nome);
    }
    
    // macro do compilador: corpo é só o número "valor". Um "#macro" do usuário com o mesmo nome e corpo diferente dá erro de redefinição.
    void defPredefinida(const char* nome, long valor) {
        Vetor<Token> corpo;
        corpo.iniciar();
        Token t;
        t.tipo = TipoToken::NUMERO;
        t.valor = strdup(std::to_string(valor).c_str());
        t.linha = 0;
        corpo.empurrar(t);
        Token origem;
        origem.tipo = TipoToken::MACRO;
        origem.valor = nullptr;
        origem.linha = 0;
        def(nome, corpo, origem);
    }
private:
    // "(NOME)" ou "(!NOME)" a partir de "pos", logo depois da diretiva. Libera os tokens e deixa "pos" depois do ')'.
    [[noreturn]] static void falharCondicao(const Vetor<Token>& e, int i, const Token& diretiva, const std::string& mensagem) {
        if(i < e.tam) throw erroNoToken(ERRO_PRE, mensagem, e[i], notaEncontrado(e[i]));
        throw erroNoToken(ERRO_PRE, mensagem, diretiva);
    }

    bool lerCondicao(Vetor<Token>& e, int& pos, const Token& diretiva) {
        std::string formato = "esperado '(NOME)' ou '(!NOME)' após a diretiva";
        int i = pos;
        if(i >= e.tam || e[i].tipo != TipoToken::ABRE_PARENTESE) falharCondicao(e, i, diretiva, formato);
        i++;
        bool nega = i < e.tam && e[i].tipo == TipoToken::EXCLAMACAO;
        if(nega) i++;
        if(i >= e.tam || e[i].tipo != TipoToken::IDENTIFICADOR) falharCondicao(e, i, diretiva, formato);
        bool definido = buscar(e[i].valor) >= 0;
        i++;
        if(i >= e.tam || e[i].tipo != TipoToken::FECHA_PARENTESE) falharCondicao(e, i, diretiva, formato);
        i++;
        for(int k = pos; k < i; k++) e[k].liberar();
        pos = i;
        return definido != nega;
    }
    
    static Token copiarToken(const Token& t, const Token& uso) {
        Token c;
        c.tipo = t.tipo;
        c.valor = strdup(t.valor);
        c.linha = uso.linha;
        c.coluna = uso.coluna;
        c.tam = uso.tam;
        c.arquivo = uso.arquivo;
        return c;
    }

    static bool mesmoCorpo(const Vetor<Token>& a, const Vetor<Token>& b) {
        if(a.tam != b.tam) return false;
        for(int i = 0; i < a.tam; i++) {
            if(a[i].tipo != b[i].tipo || strcmp(a[i].valor, b[i].valor) != 0) return false;
        }
        return true;
    }

    bool pilhaTem(const char* nome) const {
        return pilha.buscar(nome) >= 0;
    }

    // lê "#macro NOME corpo #macrofim" a partir de "pos"(no #macro), registra e devolve a posição depois do #macrofim.
    int lerMacro(Vetor<Token>& entrada, int pos) {
        Token origem = entrada[pos];
        origem.valor = nullptr;
        entrada[pos].liberar();
        pos++;
        if(pos >= entrada.tam || entrada[pos].tipo != TipoToken::IDENTIFICADOR) {
            throw erroNoToken(ERRO_PRE, "esperado nome da macro após '#macro'", pos < entrada.tam ? entrada[pos] : origem);
        }
        std::string nome = entrada[pos].valor;
        entrada[pos].liberar();
        pos++;

        Vetor<Token> corpo;
        corpo.iniciar();
        while(true) {
            if(pos >= entrada.tam || entrada[pos].tipo == TipoToken::FIM_ARQUIVO) {
                for(int i = 0; i < corpo.tam; i++) corpo[i].liberar();
                corpo.liberar();
                throw erroNoToken(ERRO_PRE, "macro '" + nome + "' não foi fechada: falta '#macrofim'", origem);
            }
            if(entrada[pos].tipo == TipoToken::MACRO) {
                Token interno = entrada[pos];
                for(int i = 0; i < corpo.tam; i++) corpo[i].liberar();
                corpo.liberar();
                throw erroNoToken(ERRO_PRE, "'#macro' dentro da macro '" + nome + "'", interno);
            }
            if(entrada[pos].tipo == TipoToken::MACROFIM) break;
            corpo.empurrar(entrada[pos]);
            pos++;
        }
        entrada[pos].liberar();
        pos++;
        def(nome, corpo, origem);
        return pos;
    }

    // registra a macro. Repetir a mesma definição(biblioteca lida duas vezes) é aceito; corpo diferente é erro.
    void def(const std::string& nome, Vetor<Token>& corpo, const Token& origem) {
        int indice = buscar(nome.c_str());
        if(indice >= 0) {
            bool igual = mesmoCorpo(corpos[indice], corpo);
            for(int i = 0; i < corpo.tam; i++) corpo[i].liberar();
            corpo.liberar();
            if(!igual) {
                throw erroNoToken(ERRO_PRE, "macro '" + nome + "' redefinida com corpo diferente", origem);
            }
            return;
        }
        nomes.empurrar(nome.c_str());
        corpos.empurrar(corpo);
    }

    // copia o corpo pra "saida", expandindo macros dentro dele. Os tokens ganham a linha do ponto de uso, pra mensagens de erro apontarem pro lugar certo.
    void expandir(const Vetor<Token>& corpo, const Token& uso, Vetor<Token>& saida) {
        for(int i = 0; i < corpo.tam; i++) {
            const Token& t = corpo[i];
            if(t.tipo == TipoToken::IDENTIFICADOR) {
                int indice = buscar(t.valor);
                if(indice >= 0) {
                    if(pilhaTem(t.valor)) {
                        throw erroNoToken(ERRO_PRE, std::string("macro '") + t.valor + "' se referencia a si mesma", uso);
                    }
                    pilha.empurrar(t.valor);
                    expandir(corpos[indice], uso, saida);
                    pilha.tam--;
                    continue;
                }
            }
            saida.empurrar(copiarToken(t, uso));
        }
    }

    // junta TEXTO adjacente: "a" "b" vira "ab". Consome "entrada".
    Vetor<Token> juntarTextos(Vetor<Token>& entrada) {
        Vetor<Token> saida;
        saida.iniciar();
        for(int i = 0; i < entrada.tam; i++) {
            Token t = entrada[i];
            if(t.tipo == TipoToken::TEXTO && saida.tam > 0 && saida[saida.tam - 1].tipo == TipoToken::TEXTO) {
                Token& anterior = saida[saida.tam - 1];
                size_t a = strlen(anterior.valor);
                size_t b = strlen(t.valor);
                anterior.valor = (char*)realloc(anterior.valor, a + b + 1);
                memcpy(anterior.valor + a, t.valor, b + 1);
                t.liberar();
                continue;
            }
            saida.empurrar(t);
        }
        entrada.liberar();
        return saida;
    }

    // lê o arquivo incluído só pra registrar as macros dele(e das que ele inclui). Arquivo que não abre é ignorado aqui: o CarregadorBiblis reporta o erro.
    void varrerInclusao(const char* caminhoRelativo, const std::string& diretorio) {
        std::error_code erroCaminho;
        std::filesystem::path caminhoAbsoluto = std::filesystem::canonical(resolverCaminhoInclusao(caminhoRelativo, diretorio, raizBiblis), erroCaminho);
        if(erroCaminho) return;
        std::string absolutoStr = caminhoAbsoluto.string();
        
        if(arquivosVistos.buscar(absolutoStr.c_str()) >= 0) return;
        arquivosVistos.empurrar(absolutoStr.c_str());

        std::ifstream arquivo(caminhoAbsoluto);
        if(!arquivo) return;
        std::stringstream conteudoStream;
        conteudoStream << arquivo.rdbuf();
        std::string conteudo = conteudoStream.str();

        int idArquivo = RegistroFontes::instancia().registrar(absolutoStr, conteudo, true);
        Lexer lexer(conteudo.c_str(), idArquivo);
        Vetor<Token> tokens = lexer.tokenizar();
        Vetor<Token> resultado = processar(tokens, caminhoAbsoluto.parent_path().string());
        for(int i = 0; i < resultado.tam; i++) resultado[i].liberar();
        resultado.liberar();
    }
};