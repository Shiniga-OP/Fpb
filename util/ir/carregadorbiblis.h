// util/ir/carregadorbiblis.h
#pragma once

#include <string>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string.h>
#include <stdlib.h>
#include "../config.h"
#include "../ast.h"
#include "../lexer.h"
#include "../analisador.h"
#include "../preprocessador.h"

// Localiza, le e faz o parsing dos arquivos .fpb incluidos via "#incluir".
// Não sabe nada sobre IR nem sobre tabelas de funções: só devolve as ASTs
// (Programa) das bibliotecas, na ordem em que foram carregadas(pre-ordem:
// uma biblioteca vem antes das que ela mesma inclui).
//
// DONO das ASTs: o CarregadorBiblis precisa viver até o fim de
// GeradorIR::gerar(), porque GeradorIR::corposFuncoesUsuario guarda
// ponteiros pra dentro dos Programa mantidos aqui.
class CarregadorBiblis {
public:
    VetorStr caminhosJaIncluidos; // caminhos absolutos ja processados, pra não incluir o mesmo arquivo duas vezes
    Vetor<Programa*> programas; // dono: CarregadorBiblis. mesma ordem de "origens"
    VetorStr origens; // caminho relativo, como escrito no #incluir, de cada item de "programas"(usado só em mensagens de erro)
    Preprocessador* preprocessador = nullptr;
    std::string raizBiblis = raizBiblisPadrao(); // raiz das bibliotecas, usada nos "~caminho"

    CarregadorBiblis() {
        caminhosJaIncluidos.iniciar();
        programas.iniciar();
        origens.iniciar();
    }

    // o carregador é dono de ponteiros crus: copiar causaria delete duplo.
    CarregadorBiblis(const CarregadorBiblis&) = delete;
    CarregadorBiblis& operator=(const CarregadorBiblis&) = delete;

    ~CarregadorBiblis() {
        caminhosJaIncluidos.liberar();
        origens.liberar();
        for(int i = 0; i < programas.tam; i++) {
            programas[i]->liberar();
            delete programas[i];
        }
        programas.liberar();
    }

    // caminho relativo(como escrito no #incluir) da biblioteca de indice "i".
    std::string origem(int i) {
        int tam;
        const char* texto = origens.obter(i, &tam);
        return std::string(texto, tam);
    }

    // carrega uma biblioteca(#incluir "caminho.fpb"): "diretorioAtual" e o
    // diretório a partir do qual "caminhoRelativo" deve ser resolvido(o
    // diretório do arquivo que contem o #incluir, não sempre o diretório
    // raiz: uma biblioteca em "biblis/impressao.fpb" que faz
    // #incluir "./tex.fpb" espera achar "biblis/tex.fpb", não "tex.fpb" na
    // raiz). Resolve o caminho absoluto e ignora silenciosamente se já foi
    // incluida antes(mesmo arquivo, independente de como o caminho foi
    // escrito), já que não ha cenario util pra incluir o mesmo arquivo duas
    // vezes. Inclusões feitas pela própria biblioteca são carregadas em
    // seguida, recursivamente.
    void carregar(const char* caminhoRelativo, const std::string& diretorioAtual) {
        std::filesystem::path caminho = resolverCaminhoInclusao(caminhoRelativo, diretorioAtual, raizBiblis);
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
        if(preprocessador) tokens = preprocessador->processar(tokens, caminhoAbsoluto.parent_path().string());
        Analisador analisador(tokens);

        // aloca no heap e mantem em "programas": quem registrou as funções
        // guarda ponteiros pras DeclaracaoFuncao de dentro deste Programa,
        // entao ele precisa sobreviver até o fim de gerar()(não pode ser
        // liberado aqui).
        Programa* bibliotecaPrograma = new Programa();
        *bibliotecaPrograma = analisador.analisar();

        programas.empurrar(bibliotecaPrograma);
        origens.empurrar(caminhoRelativo);

        // bibliotecas também podem incluir outras bibliotecas: resolvidas
        // relativo ao próprio diretório desta biblioteca(caminhoAbsoluto),
        // não ao diretório do arquivo raiz.
        std::string diretorioDestaBiblioteca = caminhoAbsoluto.parent_path().string();
        for(int i = 0; i < bibliotecaPrograma->inclusoes.tam; i++) {
            carregar(bibliotecaPrograma->inclusoes[i].caminho, diretorioDestaBiblioteca);
        }
        // libera só os tokens(a AST da biblioteca continua viva em "programas").
        for(int i = 0; i < tokens.tam; i++) tokens[i].liberar();
        tokens.liberar();
    }
};