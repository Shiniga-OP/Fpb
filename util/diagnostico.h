// util/diagnostico.h
#pragma once
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "config.h"

inline constexpr const char* ERRO_LEXICO = "erro léxico";
inline constexpr const char* ERRO_SINTAXE = "erro de sintaxe";
inline constexpr const char* ERRO_PRE = "erro de pré-processamento";
inline constexpr const char* ERRO_INCLUSAO = "erro de inclusão";

inline constexpr const char* COR_RESET = "\033[0m";
inline constexpr const char* COR_NEGRITO = "\033[1m";
inline constexpr const char* COR_VERMELHO = "\033[1;31m";
inline constexpr const char* COR_AZUL = "\033[1;34m";
inline constexpr const char* COR_CIANO = "\033[1;36m";
inline constexpr const char* COR_MAGENTA = "\033[1;35m";

// devolve o código de cor só se o stderr for um terminal(e NO_COLOR não existir); senão devolve "" e a saída fica limpa em logs/pipes.
inline const char* cor(const char* codigo) {
    static const bool colorido = isatty(fileno(stderr)) && !getenv("NO_COLOR");
    return colorido ? codigo : "";
}

struct ArquivoFonte {
    std::string caminho;
    std::string conteudo;
    bool ehBiblioteca;
};

// guarda o texto de cada arquivo lido, pra mostrar o trecho do erro depois. Cada Token carrega só o id do arquivo de onde veio.
class RegistroFontes {
public:
    std::vector<ArquivoFonte> arquivos;

    static RegistroFontes& instancia() {
        static RegistroFontes registro;
        return registro;
    }

    // registrar o mesmo caminho duas vezes devolve o mesmo id.
    int registrar(const std::string& caminho, const std::string& conteudo, bool ehBiblioteca) {
        for(size_t i = 0; i < arquivos.size(); i++) {
            if(arquivos[i].caminho == caminho) return (int)i;
        }
        arquivos.push_back({caminho, conteudo, ehBiblioteca});
        return (int)arquivos.size() - 1;
    }

    const ArquivoFonte* obter(int id) const {
        if(id < 0 || id >= (int)arquivos.size()) return nullptr;
        return &arquivos[id];
    }
};

// erro com posição. "coluna" é em bytes, base 1; "tam" é o tamanho do trecho em bytes; "nota" vai ao lado do sublinhado.
struct ErroCompilacao : std::runtime_error {
    std::string tipo;
    int arquivo;
    int linha;
    int coluna;
    int tam;
    std::string nota;

    ErroCompilacao(const std::string& tipo, const std::string& mensagem, int arquivo, int linha, int coluna, int tam, const std::string& nota = "")
        : std::runtime_error(mensagem), tipo(tipo), arquivo(arquivo), linha(linha), coluna(coluna), tam(tam), nota(nota) {}
};

// "encontrado 'x'" pro token dado, com texto longo cortado e quebras de linha escapadas.
inline std::string notaEncontrado(const Token& t) {
    if(t.tipo == TipoToken::FIM_ARQUIVO) return "fim do arquivo";
    std::string valor = t.valor ? t.valor : "";
    bool cortou = false;
    if(valor.size() > 24) {
        size_t corte = 24;
        while(corte > 0 && (((unsigned char)valor[corte]) & 0xC0) == 0x80) corte--;
        valor = valor.substr(0, corte);
        cortou = true;
    }
    std::string limpo;
    for(char c : valor) {
        if(c == '\n') limpo += "\\n";
        else if(c == '\t') limpo += "\\t";
        else limpo += c;
    }
    if(cortou) limpo += "...";
    if(t.tipo == TipoToken::TEXTO) return "encontrado \"" + limpo + "\"";
    return "encontrado '" + limpo + "'";
}

inline ErroCompilacao erroNoToken(const std::string& tipo, const std::string& mensagem, const Token& t, const std::string& nota = "") {
    return ErroCompilacao(tipo, mensagem, t.arquivo, t.linha, t.coluna, t.tam, nota);
}

inline std::string linhaDoFonte(const std::string& fonte, int numero) {
    size_t inicio = 0;
    for(int l = 1; l < numero; l++) {
        size_t quebra = fonte.find('\n', inicio);
        if(quebra == std::string::npos) return "";
        inicio = quebra + 1;
    }
    size_t fim = fonte.find('\n', inicio);
    if(fim == std::string::npos) fim = fonte.size();
    std::string texto = fonte.substr(inicio, fim - inicio);
    if(!texto.empty() && texto.back() == '\r') texto.pop_back();
    return texto;
}

inline std::string expandirTabs(const std::string& linha) {
    std::string resultado;
    for(char c : linha) {
        if(c == '\t') resultado += "    ";
        else resultado += c;
    }
    return resultado;
}

// largura em colunas do terminal dos bytes [de, ate): tab vale 4 e byte de continuação UTF-8 vale 0.
inline int larguraVisual(const std::string& linha, int de, int ate) {
    int largura = 0;
    for(int i = de; i < ate && i < (int)linha.size(); i++) {
        unsigned char c = linha[i];
        if(c == '\t') largura += 4;
        else if((c & 0xC0) != 0x80) largura++;
    }
    return largura;
}

inline std::string caminhoCurto(const std::string& caminho) {
    std::error_code erro;
    std::string relativo = std::filesystem::proximate(caminho, erro).string();
    if(erro) return caminho;
    return relativo.size() < caminho.size() ? relativo : caminho;
}

inline void imprimirErro(const ErroCompilacao& e) {
    std::cerr << cor(COR_VERMELHO) << e.tipo << cor(COR_RESET) << cor(COR_NEGRITO) << ": " << e.what() << cor(COR_RESET) << "\n";
    const ArquivoFonte* fonte = RegistroFontes::instancia().obter(e.arquivo);
    if(!fonte || e.linha < 1) {
        if(e.linha > 0) std::cerr << cor(COR_AZUL) << "--> " << cor(COR_RESET) << "linha " << e.linha << "\n";
        return;
    }
    int largura = (int)std::to_string(e.linha).size();
    std::string margem(largura, ' ');
    const char* rotulo = fonte->ehBiblioteca ? "biblioteca" : "arquivo principal";
    const char* corRotulo = fonte->ehBiblioteca ? COR_MAGENTA : COR_CIANO;
    std::cerr << margem << cor(COR_AZUL) << "--> " << cor(COR_RESET) << cor(corRotulo) << "[" << rotulo << "]" << cor(COR_RESET) << " " << caminhoCurto(fonte->caminho) << ":" << e.linha << ":" << e.coluna << "\n";
    std::cerr << margem << cor(COR_AZUL) << " |" << cor(COR_RESET) << "\n";

    auto imprimirLinha = [&](int numero, const std::string& texto) {
        char formatado[16];
        snprintf(formatado, sizeof(formatado), "%*d", largura, numero);
        std::cerr << cor(COR_AZUL) << formatado << " | " << cor(COR_RESET) << expandirTabs(texto) << "\n";
    };
    if(e.linha > 1) {
        std::string anterior = linhaDoFonte(fonte->conteudo, e.linha - 1);
        if(anterior.find_first_not_of(" \t") != std::string::npos) imprimirLinha(e.linha - 1, anterior);
    }
    std::string texto = linhaDoFonte(fonte->conteudo, e.linha);
    imprimirLinha(e.linha, texto);
    if(e.coluna < 1) return;

    int recuo = larguraVisual(texto, 0, e.coluna - 1);
    int tamanho = larguraVisual(texto, e.coluna - 1, e.coluna - 1 + e.tam);
    if(tamanho < 1) tamanho = 1;
    std::cerr << margem << cor(COR_AZUL) << " | " << cor(COR_RESET) << std::string(recuo, ' ') << cor(COR_VERMELHO) << std::string(tamanho, '^');
    if(!e.nota.empty()) std::cerr << " " << e.nota;
    std::cerr << cor(COR_RESET) << "\n";
}

// erros sem posição(ex: os do GeradorIR por enquanto).
inline void imprimirErroSimples(const char* mensagem) {
    std::cerr << cor(COR_VERMELHO) << "erro" << cor(COR_RESET) << cor(COR_NEGRITO) << ": " << mensagem << cor(COR_RESET) << "\n";
}