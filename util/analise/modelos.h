// util/analise/modelos.h
#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <string.h>
#include <stdlib.h>
#include "../config.h"

struct ModeloClasse {
    std::string nome;
    std::vector<std::string> parametros;
    std::vector<Token> corpo; // "#classe Nome ..." sem o "<A, B>", até o "}" final
};

inline Token copiarToken(const Token& t) {
    Token c = t;
    c.valor = strdup(t.valor);
    return c;
}

inline Token novoIdentificador(const std::string& nome, const Token& base) {
    Token t = base;
    t.tipo = TipoToken::IDENTIFICADOR;
    t.valor = strdup(nome.c_str());
    return t;
}

struct Expansor {
    std::vector<ModeloClasse> modelos;
    std::vector<std::string> instanciados;
    std::vector<std::vector<Token>> fila;

    ModeloClasse* buscarModelo(const std::string& nome) {
        for(size_t i = 0; i < modelos.size(); i++) {
            if(modelos[i].nome == nome) return &modelos[i];
        }
        return nullptr;
    }

    // tira os "#classe Nome<...> { }" do fluxo e guarda em "modelos"
    std::vector<Token> extrairModelos(Vetor<Token>& entrada) {
        std::vector<Token> resto;
        int i = 0;
        while(i < entrada.tam) {
            bool ehModelo = entrada[i].tipo == TipoToken::CLASSE && i + 2 < entrada.tam && entrada[i + 1].tipo == TipoToken::IDENTIFICADOR && entrada[i + 2].tipo == TipoToken::MENOR;
            if(!ehModelo) {
                resto.push_back(copiarToken(entrada[i]));
                i++;
                continue;
            }
            ModeloClasse m;
            m.nome = entrada[i + 1].valor;
            m.corpo.push_back(copiarToken(entrada[i]));
            m.corpo.push_back(copiarToken(entrada[i + 1]));
            int j = i + 3;
            while(true) {
                if(j >= entrada.tam || entrada[j].tipo != TipoToken::IDENTIFICADOR) {
                    throw std::runtime_error("esperado nome do parâmetro de tipo em '" + m.nome + "'");
                }
                m.parametros.push_back(entrada[j].valor);
                j++;
                if(j < entrada.tam && entrada[j].tipo == TipoToken::VIRGULA) {
                    j++;
                    continue;
                }
                break;
            }
            if(j >= entrada.tam || entrada[j].tipo != TipoToken::MAIOR) {
                throw std::runtime_error("esperado '>' após os parâmetros de tipo de '" + m.nome + "'");
            }
            j++;
            int chaves = 0;
            bool abriu = false;
            while(j < entrada.tam && entrada[j].tipo != TipoToken::FIM_ARQUIVO) {
                if(entrada[j].tipo == TipoToken::ABRE_CHAVE) {
                    chaves++;
                    abriu = true;
                } else if(entrada[j].tipo == TipoToken::FECHA_CHAVE) {
                    chaves--;
                }
                m.corpo.push_back(copiarToken(entrada[j]));
                j++;
                if(abriu && chaves == 0) break;
            }
            if(!abriu || chaves != 0) {
                throw std::runtime_error("classe '" + m.nome + "' não foi fechada");
            }
            modelos.push_back(m);
            i = j;
        }
        return resto;
    }

    // lê um tipo em t[j]: palavra de tipo ou nome (com <...> se for modelo), mais "*" opcional.
    // "tipo" recebe os tokens e "parte" o pedaço do nome mangled
    bool lerTipo(std::vector<Token>& t, size_t& j, std::vector<Token>& tipo, std::string& parte) {
        if(j >= t.size()) return false;
        TipoToken tp = t[j].tipo;
        bool ehPalavra = tp == TipoToken::INT || tp == TipoToken::LONGO || tp == TipoToken::CAR || tp == TipoToken::BOOL || tp == TipoToken::BYTE || tp == TipoToken::CURTO || tp == TipoToken::FLUTUANTE || tp == TipoToken::DOBRO;
        if(ehPalavra) {
            tipo.push_back(copiarToken(t[j]));
            parte = t[j].valor;
            j++;
        } else if(tp == TipoToken::IDENTIFICADOR) {
            if(j + 1 < t.size() && t[j + 1].tipo == TipoToken::MENOR && buscarModelo(t[j].valor)) {
                size_t k = j;
                if(!lerUso(t, k, parte)) return false;
                tipo.push_back(novoIdentificador(parte, t[j]));
                j = k;
            } else {
                tipo.push_back(copiarToken(t[j]));
                parte = t[j].valor;
                j++;
            }
        } else {
            return false;
        }
        if(j < t.size() && t[j].tipo == TipoToken::VEZES) {
            tipo.push_back(copiarToken(t[j]));
            parte += "Ptr";
            j++;
        }
        return true;
    }

    // t[i] é o nome do modelo e t[i + 1] é '<'. Em caso de sucesso, "i" aponta pro token depois do '>'
    bool lerUso(std::vector<Token>& t, size_t& i, std::string& mangled) {
        std::string nome = t[i].valor;
        size_t j = i + 2;
        std::vector<std::vector<Token>> args;
        mangled = nome;
        while(true) {
            std::vector<Token> tipo;
            std::string parte;
            if(!lerTipo(t, j, tipo, parte)) return false;
            args.push_back(tipo);
            mangled += "_" + parte;
            if(j < t.size() && t[j].tipo == TipoToken::VIRGULA) {
                j++;
                continue;
            }
            break;
        }
        if(j >= t.size()) return false;
        if(t[j].tipo == TipoToken::MAIOR) {
            j++;
        } else if(t[j].tipo == TipoToken::MAIOR_MAIOR) {
            // "Par<Lista<int>>": o '>>' vira um '>' que sobra pro modelo de fora
            free(t[j].valor);
            t[j].valor = strdup(">");
            t[j].tipo = TipoToken::MAIOR;
        } else {
            return false;
        }
        instanciar(*buscarModelo(nome), mangled, args);
        i = j;
        return true;
    }

    void instanciar(ModeloClasse& m, const std::string& mangled, std::vector<std::vector<Token>>& args) {
        if(args.size() != m.parametros.size()) {
            throw std::runtime_error("O modelo '" + m.nome + "' espera " + std::to_string(m.parametros.size()) + " tipo(s)");
        }
        for(size_t i = 0; i < instanciados.size(); i++) {
            if(instanciados[i] == mangled) return;
        }
        instanciados.push_back(mangled);
        std::vector<Token> expandido;
        for(size_t i = 0; i < m.corpo.size(); i++) {
            const Token& tk = m.corpo[i];
            if(tk.tipo == TipoToken::IDENTIFICADOR) {
                int p = -1;
                for(size_t k = 0; k < m.parametros.size(); k++) {
                    if(m.parametros[k] == tk.valor) p = (int)k;
                }
                if(p >= 0) {
                    for(size_t k = 0; k < args[p].size(); k++) expandido.push_back(copiarToken(args[p][k]));
                    continue;
                }
                bool proximoEhMenor = i + 1 < m.corpo.size() && m.corpo[i + 1].tipo == TipoToken::MENOR;
                if(m.nome == tk.valor && !proximoEhMenor) {
                    expandido.push_back(novoIdentificador(mangled, tk));
                    continue;
                }
            }
            expandido.push_back(copiarToken(tk));
        }
        fila.push_back(expandido);
    }

    // troca cada "Modelo<...>" por "Modelo_..." em "t" e joga o resultado em "saida"
    void reescrever(std::vector<Token>& t, std::vector<Token>& saida) {
        size_t i = 0;
        while(i < t.size()) {
            if(t[i].tipo == TipoToken::IDENTIFICADOR && i + 1 < t.size() && t[i + 1].tipo == TipoToken::MENOR && buscarModelo(t[i].valor)) {
                std::string mangled;
                size_t j = i;
                if(lerUso(t, j, mangled)) {
                    saida.push_back(novoIdentificador(mangled, t[i]));
                    i = j;
                    continue;
                }
            }
            saida.push_back(copiarToken(t[i]));
            i++;
        }
    }
};

inline Vetor<Token> montarVetor(std::vector<Token>& v) {
    Vetor<Token> r;
    r.iniciar();
    for(size_t k = 0; k < v.size(); k++) r.empurrar(v[k]);
    return r;
}

// o último de "arquivos" é o principal: todas as instâncias vão pra ele, sem repetir
inline void expandirModelosTodos(std::vector<Vetor<Token>*>& arquivos) {
    Expansor ex;
    std::vector<std::vector<Token>> restos;
    for(size_t i = 0; i < arquivos.size(); i++) restos.push_back(ex.extrairModelos(*arquivos[i]));
    if(ex.modelos.empty()) return;
    std::vector<Token> fins;
    std::vector<std::vector<Token>> saidas(arquivos.size());
    for(size_t i = 0; i < arquivos.size(); i++) {
        fins.push_back(restos[i].back());
        restos[i].pop_back();
        ex.reescrever(restos[i], saidas[i]);
    }
    std::vector<Token>& principal = saidas.back();
    // a fila cresce durante o laço: uma expansão pode instanciar outro modelo
    for(size_t k = 0; k < ex.fila.size(); k++) {
        std::vector<Token> expandido = ex.fila[k];
        ex.reescrever(expandido, principal);
    }
    for(size_t i = 0; i < arquivos.size(); i++) {
        saidas[i].push_back(fins[i]);
        Vetor<Token>& alvo = *arquivos[i];
        for(int k = 0; k < alvo.tam; k++) alvo[k].liberar();
        alvo.liberar();
        alvo = montarVetor(saidas[i]);
    }
}