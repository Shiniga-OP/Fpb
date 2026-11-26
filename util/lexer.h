// util/lexer.h
#pragma once
#include <stdexcept>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "config.h"
#include "diagnostico.h"

// buffer de acumulação de texto cru(usado so durante "lexear" de um token).
// não tem dono compartilhado: cada chamada de lerXxx cria o seu, acumula,
// e no fim gera um car* proprio via strdup para o Token.
struct BufAcumulador {
    char* dados;
    int tam;
    int cap;

    void iniciar() {
        dados = nullptr;
        tam = 0;
        cap = 0;
    }
    void liberar() {
        free(dados);
        iniciar();
    }
    void empurrar(char c) {
        if(tam + 1 >= cap) { // +1 pra sempre sobrar espaco pro '\0' final
            cap = cap ? cap * 2 : 16;
            dados = (char*)realloc(dados, cap);
        }
        dados[tam++] = c;
        dados[tam] = '\0';
    }
    bool igual(const char* s) const {
        return strcmp(dados, s) == 0;
    }
    char* paraTokenTexto() const {
        // se nunca empurrou nada, retorna texto vazio valida(não nullptr)
        return strdup(dados ? dados : "");
    }
};

inline bool ehInicioIdentificador(char c) {
    unsigned char u = (unsigned char)c;
    return isalpha(u) || u == '_' || u >= 128;
}

inline bool ehCorpoIdentificador(char c) {
    unsigned char u = (unsigned char)c;
    return isalnum(u) || u == '_' || u >= 128;
}

class Lexer {
public:
    char* fonte; // não possui: apenas le, quem chamou e dono
    size_t tamFonte;
    size_t pos = 0;
    int linha = 1;
    size_t inicioLinha = 0;
    int colunaToken = 1;
    int arquivo;

    explicit Lexer(const char* fonte, int arquivo = -1) {
        this->fonte = (char*)fonte;
        this->tamFonte = strlen(fonte);
        this->arquivo = arquivo;
    }

    int colunaAtual() {
        return (int)(pos - inicioLinha) + 1;
    }

    [[noreturn]] void erro(const std::string& mensagem, int linhaErro, int colunaErro, int tamTrecho) {
        throw ErroCompilacao(ERRO_LEXICO, mensagem, arquivo, linhaErro, colunaErro, tamTrecho);
    }

    Vetor<Token> tokenizar() {
        Vetor<Token> tokens;
        tokens.iniciar();
        
        while(!fimDoArquivo()) {
            pularEspacosEComentarios();
            if(fimDoArquivo()) break;

            char c = atual();
            int linhaAtual = linha;
            colunaToken = colunaAtual();
            size_t posInicio = pos;
            Token t;
            if(c == '#') {
                t = lerDiretiva(linhaAtual);
            } else if(ehInicioIdentificador(c)) {
                t = lerIdentificadorOuPalavraChave(linhaAtual);
            } else if(isdigit((unsigned char)c)) {
                t = lerNumero(linhaAtual);
            } else if(c == '"') {
                t = lerTexto(linhaAtual);
            } else if(c == '\'') {
                t = lerCaractere(linhaAtual);
            } else {
                t = lerSimbolo(linhaAtual);
            }
            t.coluna = colunaToken;
            t.tam = (int)(pos - posInicio);
            t.arquivo = arquivo;
            tokens.empurrar(t);
        }
        Token fim;
        fim.tipo = TipoToken::FIM_ARQUIVO;
        fim.valor = strdup("");
        fim.arquivo = arquivo;
        fim.tam = 1;
        if(tokens.tam > 0) {
            // aponta pro fim do último token real, onde o que faltava deveria estar
            fim.linha = tokens[tokens.tam - 1].linha;
            fim.coluna = tokens[tokens.tam - 1].coluna + tokens[tokens.tam - 1].tam;
        } else {
            fim.linha = linha;
            fim.coluna = 1;
        }
        tokens.empurrar(fim);
        return tokens;
    }

    bool fimDoArquivo() {
        return pos >= tamFonte;
    }
    char atual() {
        return fonte[pos];
    }

    char avancar() {
        char c = fonte[pos++];
        if(c == '\n') {
            linha++;
            inicioLinha = pos;
        }
        return c;
    }

    void pularEspacosEComentarios() {
        while(!fimDoArquivo()) {
            char c = atual();
            if(c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                avancar();
            } else if(c == '/' && pos + 1 < tamFonte && fonte[pos + 1] == '/') {
                while(!fimDoArquivo() && atual() != '\n') avancar();
            } else if(c == '/' && pos + 1 < tamFonte && fonte[pos + 1] == '*') {
                int linhaAbertura = linha;
                int colunaAbertura = colunaAtual();
                avancar(); // '/'
                avancar(); // '*'
                bool fechou = false;
                while(!fimDoArquivo()) {
                    if(atual() == '*' && pos + 1 < tamFonte && fonte[pos + 1] == '/') {
                        avancar(); // '*'
                        avancar(); // '/'
                        fechou = true;
                        break;
                    }
                    avancar(); // usa avancar() pra manter "linha" correta mesmo atravessando '\n' dentro do comentario
                }
                if(!fechou) {
                    erro("comentário de bloco não fechado", linhaAbertura, colunaAbertura, 2);
                }
            } else {
                break;
            }
        }
    }

    Token lerDiretiva(int linhaAtual) {
        BufAcumulador buf;
        buf.iniciar();
        buf.empurrar(avancar()); // '#'
        while(!fimDoArquivo() && (isalnum((unsigned char)atual()) || atual() == '_')) {
            buf.empurrar(avancar());
        }
        Token t;
        t.linha = linhaAtual;
        if(buf.igual("#incluir")) {
            t.tipo = TipoToken::INCLUIR;
            t.valor = buf.paraTokenTexto();
            buf.liberar();
            return t;
        }
        if(buf.igual("#macro")) {
            t.tipo = TipoToken::MACRO;
            t.valor = buf.paraTokenTexto();
            buf.liberar();
            return t;
        }
        if(buf.igual("#macrofim")) {
            t.tipo = TipoToken::MACROFIM;
            t.valor = buf.paraTokenTexto();
            buf.liberar();
            return t;
        }
        if(buf.igual("#se")) {
            t.tipo = TipoToken::PRE_SE;
            t.valor = buf.paraTokenTexto();
            buf.liberar();
            return t;
        }
        if(buf.igual("#senaose")) {
            t.tipo = TipoToken::PRE_SENAOSE;
            t.valor = buf.paraTokenTexto();
            buf.liberar();
            return t;
        }
        if(buf.igual("#senao")) {
            t.tipo = TipoToken::PRE_SENAO;
            t.valor = buf.paraTokenTexto();
            buf.liberar();
            return t;
        }
        if(buf.igual("#sefim")) {
            t.tipo = TipoToken::PRE_FIM;
            t.valor = buf.paraTokenTexto();
            buf.liberar();
            return t;
        }
        if(buf.igual("#espaco")) {
            t.tipo = TipoToken::ESPACO;
            t.valor = buf.paraTokenTexto();
            buf.liberar();
            return t;
        }
        if(buf.igual("#classe")) {
            t.tipo = TipoToken::CLASSE;
            t.valor = buf.paraTokenTexto();
            buf.liberar();
            return t;
        }
        if(buf.igual("#chamada_sistema")) {
            t.tipo = TipoToken::CHAMADA_SISTEMA;
            t.valor = buf.paraTokenTexto();
            buf.liberar();
            return t;
        }
        if(buf.igual("#ligar") || buf.igual("#alvo")) {
            t.tipo = buf.igual("#ligar") ? TipoToken::LIGAR : TipoToken::ALVO;
            t.valor = buf.paraTokenTexto();
            buf.liberar();
            return t;
        }
        std::string nome = buf.dados ? buf.dados : "";
        buf.liberar();
        erro("diretiva desconhecida '" + nome + "'", linhaAtual, colunaToken, (int)nome.size());
    }

    Token lerIdentificadorOuPalavraChave(int linhaAtual) {
        BufAcumulador buf;
        buf.iniciar();
        while(!fimDoArquivo() && ehCorpoIdentificador(atual())) {
            buf.empurrar(avancar());
        }
        Token t;
        t.linha = linhaAtual;
        // literais booleanos viram numero na hora: verdade = 1, falso = 0
        if(buf.igual("verdade") || buf.igual("falso")) {
            t.tipo = TipoToken::NUMERO;
            t.valor = strdup(buf.igual("verdade") ? "1" : "0");
            buf.liberar();
            return t;
        }
        if(buf.igual("nulo")) t.tipo = TipoToken::NULO;
        else if(buf.igual("int")) t.tipo = TipoToken::INT;
        else if(buf.igual("longo")) t.tipo = TipoToken::LONGO;
        else if(buf.igual("car")) t.tipo = TipoToken::CAR;
        else if(buf.igual("bool")) t.tipo = TipoToken::BOOL;
        else if(buf.igual("byte")) t.tipo = TipoToken::BYTE;
        else if(buf.igual("curto")) t.tipo = TipoToken::CURTO;
        else if(buf.igual("flu")) t.tipo = TipoToken::FLUTUANTE;
        else if(buf.igual("dobro")) t.tipo = TipoToken::DOBRO;
        else if(buf.igual("se")) t.tipo = TipoToken::SE;
        else if(buf.igual("senao") || buf.igual("senão")) t.tipo = TipoToken::SENAO;
        else if(buf.igual("enq") || buf.igual("enquanto")) t.tipo = TipoToken::ENQUANTO;
        else if(buf.igual("por")) t.tipo = TipoToken::POR;
        else if(buf.igual("retorne")) t.tipo = TipoToken::RETORNE;
        else if(buf.igual("sem_sinal")) t.tipo = TipoToken::SEM_SINAL;
        else if(buf.igual("apagar")) t.tipo = TipoToken::APAGAR;
        else if(buf.igual("herda")) t.tipo = TipoToken::HERDA;
        else if(buf.igual("modelo")) t.tipo = TipoToken::MODELO;
        else if(buf.igual("estatico")) t.tipo = TipoToken::ESTATICO;
        else if(buf.igual("final")) t.tipo = TipoToken::FINAL;
        else t.tipo = TipoToken::IDENTIFICADOR;
        t.valor = buf.paraTokenTexto();
        buf.liberar();
        return t;
    }

    Token lerNumero(int linhaAtual) {
        if(atual() == '0' && pos + 1 < tamFonte && (fonte[pos + 1] == 'x' || fonte[pos + 1] == 'X')) {
            // hexadecimal(ex: 0x1F00): vira o número decimal equivalente, pra o resto do compilador só enxergar NUMERO decimal
            avancar();
            avancar();
            BufAcumulador digitos;
            digitos.iniciar();
            while(!fimDoArquivo() && isxdigit((unsigned char)atual())) {
                digitos.empurrar(avancar());
            }
            if(digitos.tam == 0) {
                digitos.liberar();
                erro("número hexadecimal sem dígitos", linhaAtual, colunaToken, 2);
            }
            char bufNumero[32];
            snprintf(bufNumero, sizeof(bufNumero), "%ld", (long)strtoull(digitos.dados, nullptr, 16));
            digitos.liberar();
            Token tHex;
            tHex.tipo = TipoToken::NUMERO;
            tHex.valor = strdup(bufNumero);
            tHex.linha = linhaAtual;
            return tHex;
        }
        BufAcumulador buf;
        buf.iniciar();
        while(!fimDoArquivo() && isdigit((unsigned char)atual())) {
            buf.empurrar(avancar());
        }
        bool decimal = false;
        if(!fimDoArquivo() && atual() == '.' && pos + 1 < tamFonte && isdigit((unsigned char)fonte[pos + 1])) {
            decimal = true;
            buf.empurrar(avancar());
            while(!fimDoArquivo() && isdigit((unsigned char)atual())) buf.empurrar(avancar());
        }
        if(!fimDoArquivo() && atual() == 'f') {
            decimal = true;
            buf.empurrar(avancar());
        }
        Token t;
        t.tipo = decimal ? TipoToken::DECIMAL : TipoToken::NUMERO;
        t.valor = buf.paraTokenTexto();
        t.linha = linhaAtual;
        buf.liberar();
        return t;
    }

    Token lerTexto(int linhaAtual) {
        avancar(); // abre aspas
        BufAcumulador buf;
        buf.iniciar();
        while(!fimDoArquivo() && atual() != '"') {
            char c = avancar();
            if(c == '\\' && !fimDoArquivo()) {
                char escapado = avancar();
                switch(escapado) {
                    case 'n': buf.empurrar('\n'); break;
                    case 't': buf.empurrar('\t'); break;
                    case '"': buf.empurrar('"'); break;
                    case '\\': buf.empurrar('\\'); break;
                    default: buf.empurrar(escapado); break;
                }
            } else {
                buf.empurrar(c);
            }
        }
        if(fimDoArquivo()) {
            buf.liberar();
            erro("texto não fechado: faltou '\"' para fechar", linhaAtual, colunaToken, 1);
        }
        avancar(); // fecha aspas
        Token t;
        t.tipo = TipoToken::TEXTO;
        t.valor = buf.paraTokenTexto();
        t.linha = linhaAtual;
        buf.liberar();
        return t;
    }

    // le um literal de caractere: 'x' ou '\n', '\t', '\\', '\''. Vira um
    // token NUMERO com o valor numerico do byte como texto(ex: 'A' -> "65"),
    // assim o parser/AST nao precisam de um tipo de expressao novo: um
    // literal de caractere e so um jeito mais legivel de escrever um
    // numero, igual (car)65 e 'A' produzem o mesmo IMEDIATO.
    Token lerCaractere(int linhaAtual) {
        avancar(); // abre aspas simples
        if(fimDoArquivo()) {
            erro("literal de caractere não fechado", linhaAtual, colunaToken, 1);
        }
        unsigned char valor;
        char c = avancar();
        if(c == '\\' && !fimDoArquivo()) {
            char escapado = avancar();
            switch(escapado) {
                case 'n': valor = '\n'; break;
                case 't': valor = '\t'; break;
                case '0': valor = '\0'; break;
                case '\'': valor = '\''; break;
                case '"': valor = '"'; break;
                case '\\': valor = '\\'; break;
                default: valor = (unsigned char)escapado; break;
            }
        } else {
            valor = (unsigned char)c;
        }
        if(fimDoArquivo() || atual() != '\'') {
            erro("esperado apenas um caractere entre aspas simples", linhaAtual, colunaToken, 1);
        }
        avancar(); // fecha aspas simples

        Token t;
        t.tipo = TipoToken::NUMERO;
        char bufNumero[16];
        snprintf(bufNumero, sizeof(bufNumero), "%d", (int)valor);
        t.valor = strdup(bufNumero);
        t.linha = linhaAtual;
        return t;
    }

    Token lerSimbolo(int linhaAtual) {
        char c = avancar();
        Token t;
        t.linha = linhaAtual;
        switch(c) {
            case '(':
                t.tipo = TipoToken::ABRE_PARENTESE;
                t.valor = strdup("(");
                return t;
            case ')':
                t.tipo = TipoToken::FECHA_PARENTESE;
                t.valor = strdup(")");
                return t;
            case '{':
                t.tipo = TipoToken::ABRE_CHAVE;
                t.valor = strdup("{");
                return t;
            case '}':
                t.tipo = TipoToken::FECHA_CHAVE;
                t.valor = strdup("}");
                return t;
            case '[':
                t.tipo = TipoToken::ABRE_COLCHETE;
                t.valor = strdup("[");
                return t;
            case ']':
                t.tipo = TipoToken::FECHA_COLCHETE;
                t.valor = strdup("]");
                return t;
            case ',':
                t.tipo = TipoToken::VIRGULA;
                t.valor = strdup(",");
                return t;
            case ';':
                t.tipo = TipoToken::PONTO_VIRGULA;
                t.valor = strdup(";");
                return t;
            case '=':
                if(!fimDoArquivo() && atual() == '=') {
                    avancar();
                    t.tipo = TipoToken::IGUAL_IGUAL;
                    t.valor = strdup("==");
                    return t;
                }
                t.tipo = TipoToken::IGUAL;
                t.valor = strdup("=");
                return t;
            case '+':
                if(!fimDoArquivo() && atual() == '+') {
                    avancar();
                    t.tipo = TipoToken::MAIS_MAIS;
                    t.valor = strdup("++");
                    return t;
                }
                if(!fimDoArquivo() && atual() == '=') {
                    avancar();
                    t.tipo = TipoToken::MAIS_IGUAL;
                    t.valor = strdup("+=");
                    return t;
                }
                t.tipo = TipoToken::MAIS;
                t.valor = strdup("+");
                return t;
            case '-':
                if(!fimDoArquivo() && atual() == '-') {
                    avancar();
                    t.tipo = TipoToken::MENOS_MENOS;
                    t.valor = strdup("--");
                    return t;
                }
                if(!fimDoArquivo() && atual() == '=') {
                    avancar();
                    t.tipo = TipoToken::MENOS_IGUAL;
                    t.valor = strdup("-=");
                    return t;
                }
                t.tipo = TipoToken::MENOS;
                t.valor = strdup("-");
                return t;
            case '*':
                if(!fimDoArquivo() && atual() == '=') {
                    avancar();
                    t.tipo = TipoToken::VEZES_IGUAL;
                    t.valor = strdup("*=");
                    return t;
                }
                t.tipo = TipoToken::VEZES;
                t.valor = strdup("*");
                return t;
            case '/':
                if(!fimDoArquivo() && atual() == '=') {
                    avancar();
                    t.tipo = TipoToken::DIVIDIR_IGUAL;
                    t.valor = strdup("/=");
                    return t;
                }
                t.tipo = TipoToken::DIVIDIR;
                t.valor = strdup("/");
                return t;
            case '%':
                if(!fimDoArquivo() && atual() == '=') {
                    avancar();
                    t.tipo = TipoToken::PORCENTAGEM_IGUAL;
                    t.valor = strdup("%=");
                    return t;
                }
                t.tipo = TipoToken::PORCENTAGEM;
                t.valor = strdup("%");
                return t;
            case '!':
                if(!fimDoArquivo() && atual() == '=') {
                    avancar();
                    t.tipo = TipoToken::DIFERENTE;
                    t.valor = strdup("!=");
                    return t;
                }
                t.tipo = TipoToken::EXCLAMACAO;
                t.valor = strdup("!");
                return t;
            case '&':
                if(!fimDoArquivo() && atual() == '&') {
                    avancar();
                    t.tipo = TipoToken::TAMBEM_TAMBEM;
                    t.valor = strdup("&&");
                    return t;
                }
                if(!fimDoArquivo() && atual() == '=') {
                    avancar();
                    t.tipo = TipoToken::TAMBEM_IGUAL;
                    t.valor = strdup("&=");
                    return t;
                }
                t.tipo = TipoToken::TAMBEM;
                t.valor = strdup("&");
                return t;
            case '|':
                if(!fimDoArquivo() && atual() == '|') {
                    avancar();
                    t.tipo = TipoToken::OU_OU;
                    t.valor = strdup("||");
                    return t;
                }
                if(!fimDoArquivo() && atual() == '=') {
                    avancar();
                    t.tipo = TipoToken::OU_IGUAL;
                    t.valor = strdup("|=");
                    return t;
                }
                t.tipo = TipoToken::OU;
                t.valor = strdup("|");
                return t;
            case '>':
                if(!fimDoArquivo() && atual() == '=') {
                    avancar();
                    t.tipo = TipoToken::MAIOR_IGUAL;
                    t.valor = strdup(">=");
                    return t;
                }
                if(!fimDoArquivo() && atual() == '>') {
                    avancar();
                    if(!fimDoArquivo() && atual() == '=') {
                        avancar();
                        t.tipo = TipoToken::MAIOR_MAIOR_IGUAL;
                        t.valor = strdup(">>=");
                        return t;
                    }
                    t.tipo = TipoToken::MAIOR_MAIOR;
                    t.valor = strdup(">>");
                    return t;
                }
                t.tipo = TipoToken::MAIOR;
                t.valor = strdup(">");
                return t;
            case '<':
                if(!fimDoArquivo() && atual() == '=') {
                    avancar();
                    t.tipo = TipoToken::MENOR_IGUAL;
                    t.valor = strdup("<=");
                    return t;
                }
                if(!fimDoArquivo() && atual() == '<') {
                    avancar();
                    if(!fimDoArquivo() && atual() == '=') {
                        avancar();
                        t.tipo = TipoToken::MENOR_MENOR_IGUAL;
                        t.valor = strdup("<<=");
                        return t;
                    }
                    t.tipo = TipoToken::MENOR_MENOR;
                    t.valor = strdup("<<");
                    return t;
                }
                t.tipo = TipoToken::MENOR;
                t.valor = strdup("<");
                return t;
            case '^':
                if(!fimDoArquivo() && atual() == '=') {
                    avancar();
                    t.tipo = TipoToken::CIRCUNFLEXO_IGUAL;
                    t.valor = strdup("^=");
                    return t;
                }
                t.tipo = TipoToken::CIRCUNFLEXO;
                t.valor = strdup("^");
                return t;
            case '.':
                if(!fimDoArquivo() && atual() == '.' && pos + 1 < tamFonte && fonte[pos + 1] == '.') {
                    avancar();
                    avancar();
                    t.tipo = TipoToken::RETICENCIAS;
                    t.valor = strdup("...");
                    return t;
                }
                t.tipo = TipoToken::PONTO;
                t.valor = strdup(".");
                return t;
            case '~':
                t.tipo = TipoToken::TIL;
                t.valor = strdup("~");
                return t;
            case '@':
                t.tipo = TipoToken::ARROBA;
                t.valor = strdup("@");
                return t;
            case '?':
                t.tipo = TipoToken::INTERROGACAO;
                t.valor = strdup("?");
                return t;
            case ':':
                t.tipo = TipoToken::DOIS_PONTOS;
                t.valor = strdup(":");
                return t;
            default:
                erro(std::string("caractere inesperado '") + c + "'", linhaAtual, colunaToken, 1);
        }
    }
};