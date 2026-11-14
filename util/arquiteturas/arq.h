// util/arquiteturas/arq.h
#pragma once
#include <string>
#include "../ir/ir.h"

// para adicionar uma nova arquitetura: crie uma classe que implemente
// esta interface e registre-a em fpbc.cpp. Nenhum outro arquivo do compilador precisa ser alterado.
class Arquitetura {
public:
    virtual ~Arquitetura() = default;
    virtual std::string nome() const = 0;
    virtual std::string gerarAssembly(const ProgramaIR& programa) = 0;
    
    std::string escaparParaAsm(const char* texto, int tamanho) {
        std::string resultado;
        for(int i = 0; i < tamanho; i++) {
            char c = texto[i];
            switch(c) {
                case '\n': resultado += "\\n"; break;
                case '\t': resultado += "\\t"; break;
                case '"': resultado += "\\\""; break;
                case '\\': resultado += "\\\\"; break;
                default: resultado += c; break;
            }
        }
        return resultado;
    }
};