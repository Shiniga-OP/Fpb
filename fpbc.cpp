// fpbc.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <elf.h>
#include "util/lexer.h"
#include "util/analisador.h"
#include "util/preprocessador.h"
#include "util/ir/geradorir.h"
#include "util/arquiteturas/arq.h"
#include "util/arquiteturas/arm64/arm64.h"

// informações:
static std::string versaoAtual = "0.0.1";
#define ARQ_PADRAO "arm64"

#ifdef __ANDROID__
    static std::string sistemaAtual = "Android";
#elif defined(__linux__)
    static std::string sistemaAtual = "Linux";
#elif defined(_WIN32)
    static std::string sistemaAtual = "Windows";
#endif

static std::string montador = "as";
static std::string ligador = "ld";

#ifdef __ANDROID__
    static std::string alvoPadrao = ARQ_PADRAO "-android";
#elif defined(__linux__)
    static std::string alvoPadrao = ARQ_PADRAO "-linux";
#endif

// linker dinâmico dos executáveis gerados. Vazio = descobre sozinho(ver descobrirInterprete). "-interp" só serve de exceção.
static std::string interprete;
static std::string pastasExtras;

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

// lê o PT_INTERP de um ELF 64 bits: o linker dinâmico que o sistema usa pra rodar esse binário.
// usado no próprio fpbc("/proc/self/exe"): se ele roda aqui, o linker dele serve pros executáveis que ele gera.
// devolve "" se o binário for estático ou não for ELF 64 bits.
static std::string descobrirInterprete(const std::string& caminhoElf) {
    std::ifstream arquivo(caminhoElf, std::ios::binary);
    if(!arquivo) return "";
    Elf64_Ehdr cabecalho;
    if(!arquivo.read((char*)&cabecalho, sizeof(cabecalho))) return "";
    if(memcmp(cabecalho.e_ident, ELFMAG, SELFMAG) != 0 || cabecalho.e_ident[EI_CLASS] != ELFCLASS64) return "";
    for(int i = 0; i < cabecalho.e_phnum; i++) {
        Elf64_Phdr programa;
        arquivo.seekg(cabecalho.e_phoff + (std::streamoff)i * cabecalho.e_phentsize);
        if(!arquivo.read((char*)&programa, sizeof(programa))) return "";
        if(programa.p_type == PT_INTERP) {
            std::string caminho(programa.p_filesz, '\0');
            arquivo.seekg(programa.p_offset);
            if(!arquivo.read(&caminho[0], programa.p_filesz)) return "";
            return caminho.c_str();
        }
    }
    return "";
}

static std::string descobrirPastasBiblis() {
    std::string resultado;
    const char* ambBiblis = getenv("LD_LIBRARY_PATH");
    if(ambBiblis) {
        std::stringstream fluxo(ambBiblis);
        std::string pasta;
        while(std::getline(fluxo, pasta, ':')) {
            if(!pasta.empty()) resultado += "-L" + pasta + " ";
        }
    }
    const char* ambCaminho = getenv("PATH");
    if(ambCaminho) {
        std::stringstream fluxo(ambCaminho);
        std::string pasta;
        while(std::getline(fluxo, pasta, ':')) {
            if(pasta.empty()) continue;
            std::error_code erro;
            if(!std::filesystem::exists(std::filesystem::path(pasta) / ligador, erro)) continue;
            std::filesystem::path pastaLib = std::filesystem::path(pasta).parent_path() / "lib";
            if(std::filesystem::is_directory(pastaLib, erro)) resultado += "-L" + pastaLib.string() + " ";
            break;
        }
    }
    return resultado;
}

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "Uso: fpbc <arquivo.fpb> [--alvo arm64-linux] [-s saida] [-asm] [-pie] [-interp caminho]\n";
        std::cerr << "  -s saida      caminho do executavel final (padrão: nome do fonte sem extensão)\n";
        std::cerr << "  -asm          gera so o .asm, não monta nem linka\n";
        std::cerr << "  -pie          gera executável PIE (necessário pra rodar no Android)\n";
        std::cerr << "  -interp caminho  linker dinâmico do executável (padrão: o mesmo do próprio fpbc)\n";
        std::cerr << "  -rota pasta   pasta de bibliotecas: usada no link e gravada no executável pra achá-las ao rodar\n";
        return 1;
    }
    std::string caminhoFonte = argv[1];
    std::string nomeArquitetura = ARQ_PADRAO;
    std::string alvoCompleto = alvoPadrao;
    std::string caminhoSaida;
    bool manterAsm = false;
    bool usarPie = false;

    for(int i = 1; i < argc; i++) {
        std::string argumento = argv[i];
        if(argumento == "-v") {
            std::cout <<
            "FPBC — Fácil Programação Baixo nível Compilador" << '\n' <<
            "Versão: v" << versaoAtual << '\n' <<
            "Arquitetura: " << alvoPadrao << '\n' <<
            "Sistema: " << sistemaAtual << '\n' <<
            "Implementação: Foca-do Estúdios\n";
            return 0;
        } else if(argumento == "--alvo") {
            alvoCompleto = argv[++i];
        } else if(argumento == "-s") {
            caminhoFonte = argv[i-1];
            caminhoSaida = argv[++i];
        } else if(argumento == "-asm") {
            manterAsm = true;
        } else if(argumento == "-pie") {
            usarPie = true;
        } else if(argumento == "-interp") {
            interprete = argv[++i];
        } else if(argumento == "-rota") {
            std::string pasta = argv[++i];
            pastasExtras += "-L" + pasta + " -rpath " + pasta + " ";
        }
    }
    std::filesystem::path caminhoBase(caminhoFonte);
    std::string nomeBase = caminhoBase.stem().string();
    std::string caminhoAsm = nomeBase + ".asm";
    std::string caminhoObjeto = nomeBase + ".o";
    if(caminhoSaida.empty()) caminhoSaida = nomeBase;

    try {
        std::string codigoFonte = lerArquivo(caminhoFonte);

        std::string diretorioBase = std::filesystem::path(caminhoFonte).parent_path().string();
        if(diretorioBase.empty()) diretorioBase = ".";

        Lexer lexer(codigoFonte.c_str());
        auto tokens = lexer.tokenizar();

        // "#alvo" do fonte vale mais que o "--alvo": precisa ser lido antes do pré-processador, que define as macros do alvo
        for(int i = 0; i + 1 < tokens.tam; i++) {
            if(tokens[i].tipo == TipoToken::ALVO && tokens[i + 1].tipo == TipoToken::TEXTO) alvoCompleto = tokens[i + 1].valor;
        }
        size_t traco = alvoCompleto.find('-');
        nomeArquitetura = alvoCompleto.substr(0, traco);
        std::string sistemaAlvo = traco == std::string::npos ? "linux" : alvoCompleto.substr(traco + 1);

        Preprocessador preprocessador;
        std::string macroArquitetura = "__" + nomeArquitetura + "__";
        for(char& c : macroArquitetura) c = toupper((unsigned char)c);
        preprocessador.defPredefinida(macroArquitetura.c_str(), 1);
        bool alvoAndroid = sistemaAlvo.find("android") != std::string::npos;
        if(alvoAndroid) preprocessador.defPredefinida("__ANDROID__", 1);
        if(alvoAndroid || sistemaAlvo.find("linux") != std::string::npos) preprocessador.defPredefinida("__LINUX__", 1);
        if(sistemaAlvo.find("win") != std::string::npos) preprocessador.defPredefinida("__WIN__", 1);
        
        tokens = preprocessador.processar(tokens, diretorioBase);

        Analisador analisador(tokens);
        auto programa = analisador.analisar();
        GeradorIR geradorIR(diretorioBase, &preprocessador);
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

        bool ehBibli = programaIR.ehBibli;
        bool temExportada = false;
        for(int i = 0; i < programaIR.funcoes.tam; i++) {
            if(programaIR.funcoes[i].ehExportada) temExportada = true;
        }
        programaIR.liberar();

        // monta e linka de verdade, pra sair um binario executavel.
        if(executar(montador + " " + caminhoAsm + " -o " + caminhoObjeto) != 0) {
            std::cerr << "Falha ao montar (as) o arquivo " << caminhoAsm << "\n";
            return 1;
        }
        std::string comandoLigar = ligador + " ";
        std::string pasta;
        if(!ehBibli) {
            comandoLigar += "-e inicio -z max-page-size=16384 ";
            if(geradorIR.ligacoes.tam > 0) {
                comandoLigar += "--no-as-needed ";
                if(alvoAndroid) usarPie = true;
                if(interprete.empty()) interprete = descobrirInterprete("/proc/self/exe");
                if(!interprete.empty()) {
                    comandoLigar += "-dynamic-linker " + interprete + " ";
                    if(interprete.find("linker") != std::string::npos) usarPie = true;
                }
                comandoLigar += descobrirPastasBiblis();
                comandoLigar += pastasExtras;
            }
            if(usarPie) comandoLigar += "-pie ";
            if(temExportada) comandoLigar += "--export-dynamic ";
            for(int i = 0; i < geradorIR.ligacoes.tam; i++) {
                int tam;
                const char* texto = geradorIR.ligacoes.obter(i, &tam);
                comandoLigar += "-l" + std::string(texto, tam) + " ";
            }
        } else {
            pasta = nomeBase + "_ligacoes";
            std::filesystem::create_directories(pasta);
            std::ofstream(pasta + "/vazio.s").close();
            executar(montador + " " + pasta + "/vazio.s -o " + pasta + "/vazio.o");
            comandoLigar += "-shared -z max-page-size=16384 -L" + pasta + " --no-as-needed ";
            for(int i = 0; i < geradorIR.ligacoes.tam; i++) {
                int tam;
                const char* texto = geradorIR.ligacoes.obter(i, &tam);
                std::string nome(texto, tam);
                executar(ligador + " -shared -soname lib" + nome + ".so " + pasta + "/vazio.o -o " + pasta + "/lib" + nome + ".so");
                comandoLigar += "-l" + nome + " ";
            }
        }
        if(executar(comandoLigar + caminhoObjeto + " -o " + caminhoSaida) != 0) {
            std::cerr << "Falha ao linkar (ld) o arquivo " << caminhoObjeto << "\n";
            return 1;
        }
        if(!pasta.empty()) std::filesystem::remove_all(pasta);
        if(!manterAsm) std::filesystem::remove(caminhoAsm);
        std::filesystem::remove(caminhoObjeto);

        return 0;
    } catch(const std::exception& e) {
        std::cerr << "Erro de compilacao: " << e.what() << "\n";
        return 1;
    }
}