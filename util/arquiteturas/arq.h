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
};