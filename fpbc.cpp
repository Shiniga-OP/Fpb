// fpbc.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <cstdlib>
#include "util/lexer.h"
#include "util/analisador.h"
#include "util/ir/geradorir.h"
#include "util/arquiteturas/arq.h"
#include "util/arquiteturas/arm64.h"

// informações:
static std::string versaoAtual = "0.0.1";
static std::string sistemaAtual = "Linux";

// registro de arquiteturas disponiveis. Para adicionar uma nova arquitetura,
// basta incluir o header dela e adicionar uma linha aqui.
static std::unique_ptr<Arquitetura> obterArquitetura(const std::string& nomeArquitetura) {
    if(nomeArquitetura == "arm64") return std::make_unique<ARM64>();
    return nullptr;
}

static std::string lerArquivo(const std::string& caminho) {
    std::ifstream arquivo(caminho);
    if(!arquivo) {
        throw std::runtime_error("Não foi possível abrir o arquivo: " + caminho);
    }
    std::stringstream conteudo;
    conteudo << arquivo.rdbuf();
    return conteudo.str();
}

static int executar(const std::string& comando) {
    return std::system(comando.c_str());
}

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "Uso: fpbc <arquivo.fpb> [--alvo arm64] [-s saida] [-asm]\n";
        std::cerr << "  -s saida      caminho do executavel final (padrão: nome do fonte sem extensão)\n";
        std::cerr << "  -asm  gera so o .asm, não monta nem linka\n";
        return 1;
    }
    std::string caminhoFonte = argv[1];
    std::string nomeArquitetura = "arm64";
    std::string caminhoSaida;
    bool manterAsm = false;

    for(int i = 1; i < argc; i++) {
        std::string argumento = argv[i];
        if(argumento == "-v") {
            std::cout << "Versão: v" << versaoAtual << '\n' <<
            "Arquitetura: " << nomeArquitetura << '\n' <<
            "Sistema: " << sistemaAtual << '\n' <<
            "Implementação: Foca-do Estúdios\n";
            return 0;
        } else if(argumento == "--alvo") {
            nomeArquitetura = argv[++i];
        } else if(argumento == "-s") {
            caminhoFonte = argv[i-1];
            caminhoSaida = argv[++i];
        } else if(argumento == "-asm") {
            manterAsm = true;
        }
    }
    std::filesystem::path caminhoBase(caminhoFonte);
    std::string nomeBase = caminhoBase.stem().string();
    std::string caminhoAsm = nomeBase + ".asm";
    std::string caminhoObjeto = nomeBase + ".o";
    if(caminhoSaida.empty()) caminhoSaida = nomeBase;

    try {
        std::string codigoFonte = lerArquivo(caminhoFonte);

        Lexer lexer(codigoFonte.c_str());
        auto tokens = lexer.tokenizar();

        Analisador analisador(tokens);
        auto programa = analisador.analisar();

        std::string diretorioBase = std::filesystem::path(caminhoFonte).parent_path().string();
        if(diretorioBase.empty()) diretorioBase = ".";
        GeradorIR geradorIR(diretorioBase);
        auto programaIR = geradorIR.gerar(programa);

        // libera memoria manual da AST/tokens(sem RAII, entao liberamos explicitamente)
        programa.liberar();
        for(int i = 0; i < tokens.tam; i++) tokens[i].liberar();
        tokens.liberar();

        auto arquitetura = obterArquitetura(nomeArquitetura);
        if(!arquitetura) {
            std::cerr << "Arquitetura desconhecida: " << nomeArquitetura << "\n";
            return 1;
        }
        std::string assembly = arquitetura->gerarAssembly(programaIR);

        std::ofstream saidaAsm(caminhoAsm);
        saidaAsm << assembly;
        saidaAsm.close();

        programaIR.liberar();

        // monta e linka de verdade, pra sair um binario executavel.
        if(executar("as " + caminhoAsm + " -o " + caminhoObjeto) != 0) {
            std::cerr << "Falha ao montar (as) o arquivo " << caminhoAsm << "\n";
            return 1;
        }
        if(executar("ld -e inicio " + caminhoObjeto + " -o " + caminhoSaida) != 0) {
            std::cerr << "Falha ao linkar (ld) o arquivo " << caminhoObjeto << "\n";
            return 1;
        }
        if(!manterAsm) std::filesystem::remove(caminhoAsm);
        std::filesystem::remove(caminhoObjeto);

        return 0;
    } catch(const std::exception& e) {
        std::cerr << "Erro de compilacao: " << e.what() << "\n";
        return 1;
    }
}