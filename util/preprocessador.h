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

// roda entre o Lexer e o Analisador: guarda cada "#macro NOME ... #macrofim" numa tabela, troca cada uso de NOME pelo corpo e junta textos adjacentes ("a" "b" vira "ab").
// a tabela é única: o arquivo principal e todas as bibliotecas usam a mesma instância, então uma macro definida numa biblioteca vale em quem a inclui.
// pra isso, ao achar "#incluir", o pré-processador varre o arquivo incluído só pra coletar as macros(os tokens dele são descartados, o CarregadorBiblis lê o arquivo de novo).
class Preprocessador {
public:
    VetorStr nomes;
    Vetor<Vetor<Token>> corpos; // dono: Preprocessador. paralelo a nomes
    VetorStr arquivosVistos; // caminhos absolutos já varridos
    VetorStr pilha; // macros sendo expandidas agora, pra barrar recursão

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
        while(pos < entrada.tam) {
            Token t = entrada[pos];
            if(t.tipo == TipoToken::MACRO) {
                pos = lerMacro(entrada, pos);
                continue;
            }
            if(t.tipo == TipoToken::MACROFIM) {
                throw std::runtime_error("'#macrofim' sem '#macro' na linha " + std::to_string(t.linha));
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
                    expandir(corpos[indice], t.linha, saida);
                    pilha.tam--;
                    t.liberar();
                    pos++;
                    continue;
                }
            }
            saida.empurrar(t);
            pos++;
        }
        entrada.liberar(); // só o array: cada token já foi movido pra saida ou liberado
        return juntarTextos(saida);
    }

    // índice da macro "nome", ou -1.
    int buscar(const char* nome) const {
        int tamNome = (int)strlen(nome);
        for(int i = 0; i < nomes.tam; i++) {
            int tamCandidato;
            const char* candidato = nomes.obter(i, &tamCandidato);
            if(tamCandidato == tamNome && memcmp(candidato, nome, tamNome) == 0) return i;
        }
        return -1;
    }

private:
    static Token copiarToken(const Token& t, int linha) {
        Token c;
        c.tipo = t.tipo;
        c.valor = strdup(t.valor);
        c.linha = linha;
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
        int tamNome = (int)strlen(nome);
        for(int i = 0; i < pilha.tam; i++) {
            int tamCandidato;
            const char* candidato = pilha.obter(i, &tamCandidato);
            if(tamCandidato == tamNome && memcmp(candidato, nome, tamNome) == 0) return true;
        }
        return false;
    }

    // lê "#macro NOME corpo #macrofim" a partir de "pos"(no #macro), registra e devolve a posição depois do #macrofim.
    int lerMacro(Vetor<Token>& entrada, int pos) {
        int linha = entrada[pos].linha;
        entrada[pos].liberar();
        pos++;
        if(pos >= entrada.tam || entrada[pos].tipo != TipoToken::IDENTIFICADOR) {
            throw std::runtime_error("Esperado nome da macro após '#macro' na linha " + std::to_string(linha));
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
                throw std::runtime_error("Macro '" + nome + "' aberta na linha " + std::to_string(linha) + " não foi fechada");
            }
            if(entrada[pos].tipo == TipoToken::MACRO) {
                int linhaInterna = entrada[pos].linha;
                for(int i = 0; i < corpo.tam; i++) corpo[i].liberar();
                corpo.liberar();
                throw std::runtime_error("'#macro' dentro da macro '" + nome + "' na linha " + std::to_string(linhaInterna));
            }
            if(entrada[pos].tipo == TipoToken::MACROFIM) break;
            corpo.empurrar(entrada[pos]);
            pos++;
        }
        entrada[pos].liberar();
        pos++;
        definir(nome, corpo, linha);
        return pos;
    }

    // registra a macro. Repetir a mesma definição(biblioteca lida duas vezes) é aceito; corpo diferente é erro.
    void definir(const std::string& nome, Vetor<Token>& corpo, int linha) {
        int indice = buscar(nome.c_str());
        if(indice >= 0) {
            bool igual = mesmoCorpo(corpos[indice], corpo);
            for(int i = 0; i < corpo.tam; i++) corpo[i].liberar();
            corpo.liberar();
            if(!igual) {
                throw std::runtime_error("Macro '" + nome + "' redefinida com corpo diferente na linha " + std::to_string(linha));
            }
            return;
        }
        nomes.empurrar(nome.c_str());
        corpos.empurrar(corpo);
    }

    // copia o corpo pra "saida", expandindo macros dentro dele. Os tokens ganham a linha do ponto de uso, pra mensagens de erro apontarem pro lugar certo.
    void expandir(const Vetor<Token>& corpo, int linhaUso, Vetor<Token>& saida) {
        for(int i = 0; i < corpo.tam; i++) {
            const Token& t = corpo[i];
            if(t.tipo == TipoToken::IDENTIFICADOR) {
                int indice = buscar(t.valor);
                if(indice >= 0) {
                    if(pilhaTem(t.valor)) {
                        throw std::runtime_error(std::string("Macro '") + t.valor + "' se referencia a si mesma (linha " + std::to_string(linhaUso) + ")");
                    }
                    pilha.empurrar(t.valor);
                    expandir(corpos[indice], linhaUso, saida);
                    pilha.tam--;
                    continue;
                }
            }
            saida.empurrar(copiarToken(t, linhaUso));
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
        std::filesystem::path caminhoAbsoluto = std::filesystem::canonical(std::filesystem::path(diretorio) / caminhoRelativo, erroCaminho);
        if(erroCaminho) return;
        std::string absolutoStr = caminhoAbsoluto.string();
        int tamAbsoluto = (int)absolutoStr.size();
        for(int i = 0; i < arquivosVistos.tam; i++) {
            int tamCandidato;
            const char* candidato = arquivosVistos.obter(i, &tamCandidato);
            if(tamCandidato == tamAbsoluto && memcmp(candidato, absolutoStr.c_str(), tamAbsoluto) == 0) return;
        }
        arquivosVistos.empurrar(absolutoStr.c_str());

        std::ifstream arquivo(caminhoAbsoluto);
        if(!arquivo) return;
        std::stringstream conteudoStream;
        conteudoStream << arquivo.rdbuf();
        std::string conteudo = conteudoStream.str();

        Lexer lexer(conteudo.c_str());
        Vetor<Token> tokens = lexer.tokenizar();
        Vetor<Token> resultado = processar(tokens, caminhoAbsoluto.parent_path().string());
        for(int i = 0; i < resultado.tam; i++) resultado[i].liberar();
        resultado.liberar();
    }
};