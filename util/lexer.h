// util/lexer.h
#pragma once
#include <stdexcept>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "config.h"

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

class Lexer {
public:
    char* fonte; // não possui: apenas le, quem chamou e dono
    size_t tamFonte;
    size_t pos = 0;
    int linha = 1;

    explicit Lexer(const char* fonte) {
        this->fonte = (char*)fonte;
        this->tamFonte = strlen(fonte);
    }

    Vetor<Token> tokenizar() {
        Vetor<Token> tokens;
        tokens.iniciar();
        while(!fimDoArquivo()) {
            pularEspacosEComentarios();
            if(fimDoArquivo()) break;

            char c = atual();
            int linhaAtual = linha;

            if(c == '#') {
                tokens.empurrar(lerDiretiva(linhaAtual));
            } else if(isalpha((unsigned char)c) || c == '_') {
                tokens.empurrar(lerIdentificadorOuPalavraChave(linhaAtual));
            } else if(isdigit((unsigned char)c)) {
                tokens.empurrar(lerNumero(linhaAtual));
            } else if(c == '"') {
                tokens.empurrar(lerTexto(linhaAtual));
            } else if(c == '\'') {
                tokens.empurrar(lerCaractere(linhaAtual));
            } else {
                tokens.empurrar(lerSimbolo(linhaAtual));
            }
        }
        Token fim;
        fim.tipo = TipoToken::FIM_ARQUIVO;
        fim.valor = strdup("");
        fim.linha = linha;
        tokens.empurrar(fim);
        return tokens;
    }

    bool fimDoArquivo() {
        return pos >= tamFonte;
    }
    char atual() { return fonte[pos]; }

    char avancar() {
        char c = fonte[pos++];
        if(c == '\n') linha++;
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
                    throw std::runtime_error("Comentário de bloco não fechado (aberto na linha " + std::to_string(linhaAbertura) + ")");
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
        if(buf.igual("#chamada_sistema")) {
            t.tipo = TipoToken::CHAMADA_SISTEMA;
            t.valor = buf.paraTokenTexto();
            buf.liberar();
            return t;
        }
        std::string msg = "Diretiva desconhecida '";
        msg += buf.dados ? buf.dados : "";
        msg += "' na linha " + std::to_string(linhaAtual);
        buf.liberar();
        throw std::runtime_error(msg);
    }

    Token lerIdentificadorOuPalavraChave(int linhaAtual) {
        BufAcumulador buf;
        buf.iniciar();
        while(!fimDoArquivo() && (isalnum((unsigned char)atual()) || atual() == '_')) {
            buf.empurrar(avancar());
        }
        Token t;
        t.linha = linhaAtual;
        if(buf.igual("nulo")) t.tipo = TipoToken::NULO;
        else if(buf.igual("int")) t.tipo = TipoToken::INT;
        else if(buf.igual("longo")) t.tipo = TipoToken::LONGO;
        else if(buf.igual("car")) t.tipo = TipoToken::CAR;
        else if(buf.igual("se")) t.tipo = TipoToken::SE;
        else if(buf.igual("senao") || buf.igual("senão")) t.tipo = TipoToken::SENAO;
        else if(buf.igual("enq") || buf.igual("enquanto")) t.tipo = TipoToken::ENQUANTO;
        else if(buf.igual("por")) t.tipo = TipoToken::POR;
        else if(buf.igual("retorne")) t.tipo = TipoToken::RETORNE;
        else t.tipo = TipoToken::IDENTIFICADOR;
        t.valor = buf.paraTokenTexto();
        buf.liberar();
        return t;
    }

    Token lerNumero(int linhaAtual) {
        BufAcumulador buf;
        buf.iniciar();
        while(!fimDoArquivo() && isdigit((unsigned char)atual())) {
            buf.empurrar(avancar());
        }
        Token t;
        t.tipo = TipoToken::NUMERO;
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
            std::string msg = "Texto não fechado na linha " + std::to_string(linhaAtual);
            buf.liberar();
            throw std::runtime_error(msg);
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
            throw std::runtime_error("Literal de caractere não fechado na linha " + std::to_string(linhaAtual));
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
            throw std::runtime_error("Literal de caractere não fechado na linha " + std::to_string(linhaAtual) + " (esperado apenas um caractere entre aspas simples)");
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
                t.tipo = TipoToken::VEZES;
                t.valor = strdup("*");
                return t;
            case '/':
                t.tipo = TipoToken::DIVIDIR;
                t.valor = strdup("/");
                return t;
            case '%':
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
                t.tipo = TipoToken::MENOR;
                t.valor = strdup("<");
                return t;
            default:
                throw std::runtime_error(std::string("Caractere inesperado '") + c + "' na linha " + std::to_string(linhaAtual));
        }
    }
};