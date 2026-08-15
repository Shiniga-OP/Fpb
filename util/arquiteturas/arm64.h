// util/arquiteturas/arm64.h
#pragma once
#include <sstream>
#include <string>
#include <stdexcept>
#include <string.h>
#include <vector>
#include "arq.h"

// traduz a IR genérica para assembly ARM64, convenção de
// chamada do Linux(numero da chamada em x8, args em x0-x2, svc 0).
class ARM64 : public Arquitetura {
public:
    std::string nome() const override { return "arm64"; }
    int contadorRotulos = 0; // usado pra gerar labels unicos(ex: loops de strlen)

    // nomes dos registradores fisicos disponiveis pro alocador de
    // "liberacao por ultimo uso"(indice i == x{19+i}). Restrito a x19..x27
    // de proposito: sao os unicos registradores verdadeiramente
    // callee-saved da AAPCS64(ver comentario de TOTAL_REGISTRADORES_FISICOS
    // em ir.h). Setado por registradorDoSlot em ir.h.
    static const char* nomeRegistradorFisico(int indice) {
        static const char* nomes[TOTAL_REGISTRADORES_FISICOS] = {
            "x19", "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27"
        };
        return nomes[indice];
    }

    // funcao IR sendo emitida no momento: setada no inicio de cada função 
    // em gerarAssembly() e consultada por emitirCarregarSlot/emitirGravarSlot/
    // emitirCarregarParametro pra saber se um slot tem registrador fisico
    // atribuido(liberacao por ultimo uso) ou se ainda mora na pilha.
    const FuncaoIR* funcaoAtual = nullptr;

    // calcula quais indices de registrador fisico(0..TOTAL_REGISTRADORES_FISICOS-1,
    // ver nomeRegistradorFisico) a funcao de fato usa, varrendo
    // registradorDoSlot. Usado tanto pra dimensionar o frame(espaco extra
    // pra salvar cada um) quanto pra emitir os proprios puxar/pegar no
    // prologo/epilogo. Devolve os indices em
    // ordem crescente(preenche "saida", que o chamador aloca com tamanho
    // TOTAL_REGISTRADORES_FISICOS e le so os primeiros "*totalUsados").
    void calcularRegistradoresUsados(const FuncaoIR& funcao, int* saida, int* totalUsados) {
        bool usado[TOTAL_REGISTRADORES_FISICOS];
        for(int i = 0; i < TOTAL_REGISTRADORES_FISICOS; i++) usado[i] = false;
        for(int s = 0; s < funcao.registradorDoSlot.tam; s++) {
            int r = funcao.registradorDoSlot[s];
            if(r != DONO_SLOT_NENHUM) usado[r] = true;
        }
        *totalUsados = 0;
        for(int i = 0; i < TOTAL_REGISTRADORES_FISICOS; i++) {
            if(usado[i]) saida[(*totalUsados)++] = i;
        }
    }

    std::string gerarAssembly(const ProgramaIR& programa) override {
        std::ostringstream saida;

        saida << ".global inicio\n";
        saida << ".section .rodata\n";
        for(int i = 0; i < programa.dados.tam; i++) {
            const DadoEstaticoIR& dado = programa.dados[i];
            saida << dado.simbolo << ": .asciz \"" << escaparParaAsm(dado.valor, dado.tamanho) << "\"\n";
        }
        saida << "\n.section .text\n";
        for(int i = 0; i < programa.funcoes.tam; i++) {
            const FuncaoIR& funcao = programa.funcoes[i];
            funcaoAtual = &funcao;
            saida << funcao.nome << ":\n";

            int registradoresUsados[TOTAL_REGISTRADORES_FISICOS];
            int totalRegistradoresUsados;
            calcularRegistradoresUsados(funcao, registradoresUsados, &totalRegistradoresUsados);
            // espaco extra no frame pra salvar cada callee-saved usado(16
            // bytes por par, igual stp/ldp de x29/x30: arredonda pra cima
            // se for impar, mantendo o sp alinhado a 16 como a AAPCS64 exige).
            int espacoCalleeSaved = ((totalRegistradoresUsados + 1) / 2) * 16;

            // tamanho do frame desta funcao: 16 bytes fixos pra x29/x30(registrador link, necessario porque o corpo pode chamar outras funcoes via 'bl', o que sobrescreve x30), mais 16 bytes por
            // slot(8 usados + 8 de espaço, mantendo o sp alinhado a 16 como a AAPCS64 exige), mais espacoCalleeSaved(callee-saved usados por esta funcao, ver acima). Slot inclui parametros e variaveis
            // locais declaradas no corpo, nessa ordem(totalSlots >= totalParametros). Slots que ganharam registrador fisico
            // via "liberacao por ultimo uso" nao usam esse espaço pra leitura/escrita normal, mas o espaço continua reservado
            // no frame(simplifica o calculo de tamFrame e mantem os indices de slot estaveis).
            int tamFrame = 16 + funcao.totalSlots * 16 + espacoCalleeSaved;

            if(!funcao.ehInicio) {
                saida << "sub sp, sp, #" << tamFrame << "\n";
                saida << "stp x29, x30, [sp, #" << (tamFrame - 16) << "]\n";
                saida << "add x29, sp, #" << (tamFrame - 16) << "\n";
                emitirPuxarPegarSalvo(saida, registradoresUsados, totalRegistradoresUsados, funcao.totalSlots * 16, true);
                // materializa cada parametro recebido em x0..x5 no seu slot
                // logico: se o slot ganhou registrador fisico(liberacao por
                // ultimo uso), move pra la; senao grava na pilha como antes.
                // A partir daqui, nenhum uso do parametro confia mais no
                // registrador original x0-x5(pode ser sujado por qualquer chamada dentro do corpo).
                static const char* registradoresParametro[] = { "x0", "x1", "x2", "x3", "x4", "x5" };
                for(int p = 0; p < funcao.totalParametros; p++) {
                    emitirGravarSlot(saida, registradoresParametro[p], p, funcao.parametrosLargura[p]);
                }
                saida << "\n";
            }
            emitirBlocoComandos(saida, funcao.instrucoes, programa);
            // recebe prologo/epilogo nem "ret". Toda outra funcao pode ser
            // chamada via "bl" e precisa desfazer o frame e retornar.
            if(!funcao.ehInicio) {
                // rotulo de fim: alvo de qualquer "retorne" no meio do corpo
                // (inclusive dentro de "se"/"enquanto"), que salta direto pra
                // ca via "b _fim_<nome>" em vez de continuar o resto do corpo.
                // emitido sempre(mesmo que a funcao nao use "retorne" em
                // nenhum ponto), pra manter o calculo do frame/epilogo simples.
                // O restore dos salvo TEM que acontecer aqui(unico
                // ponto de saida), nunca antes: um "retorne" no meio do corpo
                // salta direto pra ca via "b", pulando qualquer codigo entre
                // o retorne e o fim do corpo textual da funcao.
                saida << "_fim_" << funcao.nome << ":\n";
                emitirPuxarPegarSalvo(saida, registradoresUsados, totalRegistradoresUsados, funcao.totalSlots * 16, false);
                saida << "ldp x29, x30, [sp, #" << (tamFrame - 16) << "]\n";
                saida << "add sp, sp, #" << tamFrame << "\n";
                saida << "ret\n\n";
            }
            funcaoAtual = nullptr;
        }
        std::string resultado = saida.str();
        return otimizarMovsRedundantes(resultado);
    }

    // otimizacao de registradores: funde "mov A, B" seguido imediatamente de
    // "mov C, A" em "mov C, B", quando C != B(evita virar um "mov X, X" inutil).
    // isso elimina copias redundantes registrador-a-registrador que aparecem
    // quando um valor passa por um slot temporario antes de ser gravado na
    // variavel final(ex: "int total = a + b;" grava o resultado da soma num
    // slot temporario, depois copia pro slot de "total" — ambos podem ganhar
    // registrador fisico, e a copia vira "mov" em vez de acesso a pilha).
    // so funde as DUAS linhas adjacentes(sem rotulo/instrucao no meio), pra
    // não arriscar remover um "mov" que um salto possa pular por cima e
    // pousar exatamente entre as duas linhas(o que quebraria a fusão).
    std::string otimizarMovsRedundantes(const std::string& asm_) {
        std::vector<std::string> linhas;
        std::istringstream entrada(asm_);
        std::string linha;
        while(std::getline(entrada, linha)) linhas.push_back(linha);

        std::vector<std::string> saida;
        for(size_t i = 0; i < linhas.size(); i++) {
            if(i + 1 < linhas.size()) {
                std::string destinoA, origemB;
                std::string destinoC, origemA2;
                if(ehMovSimples(linhas[i], destinoA, origemB) && ehMovSimples(linhas[i + 1], destinoC, origemA2)) {
                    if(origemA2 == destinoA && destinoC != origemB) {
                        saida.push_back("mov " + destinoC + ", " + origemB);
                        i++; // consome as duas linhas originais, so a fundida vai pra saida
                        continue;
                    }
                }
            }
            saida.push_back(linhas[i]);
        }

        std::ostringstream resultado;
        for(size_t i = 0; i < saida.size(); i++) resultado << saida[i] << "\n";
        return resultado.str();
    }

    // reconhece uma linha "mov DESTINO, ORIGEM" onde ORIGEM e' um
    // registrador(não um imediato numérico): preenche destino/origem e
    // devolve true. Usado so pela fusao acima; não aceita "mov x9, 8"(imediato), so "mov x9, x21"(registrador-a-registrador), que e' o único 
    // caso seguro de fundir(fundir com imediato exigiria reanalisar largura w/x, fora do escopo desta otimização simples).
    bool ehMovSimples(const std::string& linha, std::string& destino, std::string& origem) {
        if(linha.compare(0, 4, "mov ") != 0) return false;
        size_t virgula = linha.find(',');
        if(virgula == std::string::npos) return false;
        destino = linha.substr(4, virgula - 4);
        std::string resto = linha.substr(virgula + 1);
        size_t inicio = resto.find_first_not_of(' ');
        if(inicio == std::string::npos) return false;
        origem = resto.substr(inicio);
        if(origem.empty() || !(origem[0] == 'x' || origem[0] == 'w')) return false; // so registrador, não imediato
        for(size_t i = 1; i < origem.size(); i++) {
            if(!isdigit((unsigned char)origem[i])) return false;
        }
        return true;
    }

    // emite os stp/ldp que salvam(salvar==true, no prologo) ou restauram
    // (salvar==false, no epilogo) os registradores callee-saved que esta
    // funcao usa, num espaco dedicado do frame logo APOS a area de slots
    // (posição "basePos" = totalSlots*16, ANTES da area de x29/x30) e
    // ANTES da area de x29/x30(posição tamFrame-16). Nao pode ocupar
    // [sp, #0..totalSlots*16), pois essa faixa e' usada por
    // emitirCarregarSlot/emitirGravarSlot pra slots sem registrador fisico
    // (ver "slot * 16" nessas funcoes). Emite em pares(stp/ldp) quando
    // sobra 2, e um unico str/ldr se sobrar 1 impar. Essencial pra corrigir
    // o bug de slots(variaveis) perdendo seu valor apos uma chamada de
    // função de usuário(bl): sem isso, a FUNÇÃO CHAMADA e' livre pra reusar
    // o mesmo registrador fisico(ex: x19) pros PRÓPRIOS slots dela, sem
    // nenhuma obrigação de preservar o valor que o CHAMADOR tinha guardado la.
    void emitirPuxarPegarSalvo(std::ostringstream& saida, const int* registradoresUsados, int totalRegistradoresUsados, int basePos, bool salvar) {
        int i = 0;
        while(i < totalRegistradoresUsados) {
            int pos = basePos + i * 8;
            if(i + 1 < totalRegistradoresUsados) {
                const char* r1 = nomeRegistradorFisico(registradoresUsados[i]);
                const char* r2 = nomeRegistradorFisico(registradoresUsados[i + 1]);
                if(salvar) saida << "stp " << r1 << ", " << r2 << ", [sp, #" << pos << "]\n";
                else saida << "ldp " << r1 << ", " << r2 << ", [sp, #" << pos << "]\n";
                i += 2;
            } else {
                const char* r1 = nomeRegistradorFisico(registradoresUsados[i]);
                if(salvar) saida << "str " << r1 << ", [sp, #" << pos << "]\n";
                else saida << "ldr " << r1 << ", [sp, #" << pos << "]\n";
                i += 1;
            }
        }
    }
    // consulta se um slot(logico) ganhou registrador fisico via "liberacao
    // por ultimo uso". Retorna o nome do registrador fisico(ex: "x19") ou
    // nullptr se o slot nao ganhou registrador e continua na pilha.
    const char* registradorFisicoDoSlot(int slot) {
        if(!funcaoAtual) return nullptr;
        if(slot < 0 || slot >= funcaoAtual->registradorDoSlot.tam) return nullptr;
        int r = funcaoAtual->registradorDoSlot[slot];
        if(r == DONO_SLOT_NENHUM) return nullptr;
        return nomeRegistradorFisico(r);
    }

    // carrega o valor de um parametro(guardado na pilha pelo prologo da funcao atual, ou no proprio registrador fisico se o
    // slot ganhou um via "liberacao por ultimo uso") no registrador de destino. Nunca assume que o valor ainda esta no
    // registrador xN original de chamada(x0-x5): qualquer chamada dentro do corpo(svc, bl) pode ter sujado x0-x5 nesse meio tempo.
    void emitirCarregarParametro(std::ostringstream& saida, const char* registradorDestino, int indiceParametro) {
        const char* fisico = registradorFisicoDoSlot(indiceParametro);
        if(fisico) {
            if(std::string(fisico) != registradorDestino) saida << "mov " << registradorDestino << ", " << fisico << "\n";
            return;
        }
        saida << "ldr " << registradorDestino << ", [sp, #" << (indiceParametro * 16) << "]\n";
    }

    // converte o nome de um registrador de 64 bits("x9") pro seu equivalente
    // de 32 bits("w9"), usado quando o valor envolvido e do tipo "car"(1
    // byte): a ALU do ARM64 so opera em w(32 bits) ou x(64 bits) inteiros,
    // então pra um valor de 1 byte usamos w e as instruções de memória
    // estreitas(ldrb/strb) em vez de x/ldr/str de 8 bytes.
    std::string paraRegistrador32(const char* registradorX) {
        std::string r = registradorX;
        if(!r.empty() && r[0] == 'x') r[0] = 'w';
        return r;
    }

    // carrega um slot(parametro ou variavel) no registrador de destino. Se o slot ganhou registrador físico via "liberacao por
    // ultimo uso", e um simples mov entre registradores(sem tocar memoria; usa w quando largura <= 4, x quando largura == 8).
    // Senão, respeita a largura do tipo na pilha: largura==1(car) usa ldrb num registrador w(zero-estendido, trunca pra 1
    // byte), largura==4(int) usa ldr num registrador w(32 bits, sem truncar), largura==8(longo/ponteiro) usa ldr num
    // registrador x(64 bits).
    void emitirCarregarSlot(std::ostringstream& saida, const char* registradorDestino, int slot, int larguraBytes) {
        const char* fisico = registradorFisicoDoSlot(slot);
        bool usaW = (larguraBytes <= 4);
        if(fisico) {
            std::string destinoStr = usaW ? paraRegistrador32(registradorDestino) : std::string(registradorDestino);
            std::string origemStr = usaW ? paraRegistrador32(fisico) : std::string(fisico);
            if(destinoStr != origemStr) saida << "mov " << destinoStr << ", " << origemStr << "\n";
            return;
        }
        if(larguraBytes == 1) {
            saida << "ldrb " << paraRegistrador32(registradorDestino) << ", [sp, #" << (slot * 16) << "]\n";
        } else if(larguraBytes == 4) {
            saida << "ldr " << paraRegistrador32(registradorDestino) << ", [sp, #" << (slot * 16) << "]\n";
        } else {
            saida << "ldr " << registradorDestino << ", [sp, #" << (slot * 16) << "]\n";
        }
    }

    // grava o registrador de origem num slot. Se o slot ganhou registrador fisico via "liberacao por ultimo uso", e um mov
    // entre registradores(sem tocar memoria; usa w quando largura <= 4, x quando largura == 8). Senao, respeita a largura
    // do tipo na pilha: largura==1(car) usa strb(so o byte baixo e gravado), largura==4(int) usa str num registrador
    // w(32 bits), largura==8(longo/ponteiro) usa str num registrador x(64 bits).
    void emitirGravarSlot(std::ostringstream& saida, const char* registradorOrigem, int slot, int larguraBytes) {
        const char* fisico = registradorFisicoDoSlot(slot);
        bool usaW = (larguraBytes <= 4);
        if(fisico) {
            std::string destinoStr = usaW ? paraRegistrador32(fisico) : std::string(fisico);
            std::string origemStr = usaW ? paraRegistrador32(registradorOrigem) : std::string(registradorOrigem);
            if(destinoStr != origemStr) saida << "mov " << destinoStr << ", " << origemStr << "\n";
            return;
        }
        if(larguraBytes == 1) {
            saida << "strb " << paraRegistrador32(registradorOrigem) << ", [sp, #" << (slot * 16) << "]\n";
        } else if(larguraBytes == 4) {
            saida << "str " << paraRegistrador32(registradorOrigem) << ", [sp, #" << (slot * 16) << "]\n";
        } else {
            saida << "str " << registradorOrigem << ", [sp, #" << (slot * 16) << "]\n";
        }
    }

    // mapa: id abstrato de chamada de sistema -> numero real da chamada de sistema no Linux/ARM64
    long numSistema(ChamadaSistemaId id) {
        switch(id) {
            case ChamadaSistemaId::ESCREVER: return 64;
            case ChamadaSistemaId::SAIR: return 93;
            case ChamadaSistemaId::ALOCAR: return 222; // mmap
            case ChamadaSistemaId::LIBERAR: return 215; // munmap
            default: throw std::runtime_error("Arquitetura ARM64: chamada de sistema sem mapeamento");
        }
    }

    // gera assembly que calcula, em tempo de execução, o tamanho de um texto
    // terminada em '\0' apontada pelo ponteiro guardado no slot de
    // "argumentoPonteiro"(pode ser parametro OU variavel/temporario, ex: o
    // resultado de uma função que retorna car*), e deixa
    // x0=1(fd stdout), x1=ponteiro, x2=tamanho, prontos pra chamada de sistema
    // de escrever. Usa um rotulo unico por chamada pra evitar colisão 
    // quando escrever(parâmetro) aparece mais de uma vez no programa.
    // NOTA: x9/w9 e reservado como registrador de rascunho desta arquitetura(fora da faixa x0-x5 usada por parâmetros/args, então seguro).
    // nenhuma outra função de emissao deste arquivo deve usar x9.
    void emitirTexTam(std::ostringstream& saida, const ArgumentoIR& argumentoPonteiro) {
        int idRotulo = contadorRotulos++;
        std::string rotuloLoop = "_textam_loop_" + std::to_string(idRotulo);
        std::string rotuloFim = "_textam_fim_" + std::to_string(idRotulo);

        // x1 = ponteiro. emitirCarregarSlot(não emitirCarregarParametro) pois
        // "registrador" aqui e' um indice de slot genérico(parâmetro OU
        // variavel/temporário), não necessariamente 0..5.
        emitirCarregarSlot(saida, "x1", argumentoPonteiro.registrador, 8);
        saida << "mov x2, 0\n"; // x2 = contador de bytes
        saida << rotuloLoop << ":\n";
        saida << "ldrb w9, [x1, x2]\n"; // w9 = byte atual (x1 + x2)
        saida << "cbz w9, " << rotuloFim << "\n"; // achou '\0': para
        saida << "add x2, x2, 1\n";
        saida << "b " << rotuloLoop << "\n";
        saida << rotuloFim << ":\n";
        saida << "mov x0, 1\n"; // fd stdout
    }

    // monta qualquer imediato de 32("usaW"=true) ou 64 bits no registrador
    // de destino via movz(primeiro bloco de 16 bits, zera o resto) seguido
    // de movk(demais blocos de 16 bits, preserva o resto ja montado). "mov"
    // sozinho so aceita um subconjunto restrito de imediatos(cabe numa
    // constante de 16 bits, possivelmente deslocada/invertida pelo proprio
    // assembler) - qualquer valor fora desse subconjunto(ex: 333334, que
    // não cabe em 16 bits). movz/movk cobre QUALQUER imediato de
    // 32/64 bits, sem excecao, as custas de ate 2(w) ou 4(x) instruções.
    // valores negativos são tratados via o bit paterno de complemento de
    // dois do proprio "longo"(imediato ja chega truncado pra largura certa
    // pelo GeradorIR, ver resolverConversao), reinterpretado como sem sinal
    // pra fatiar em blocos de 16 bits sem estouro de sinal em shift.
    void emitirMovImediato(std::ostringstream& saida, const char* registradorDestino, long valor, bool usaW) {
        std::string destino = usaW ? paraRegistrador32(registradorDestino) : std::string(registradorDestino);
        unsigned long bruto = usaW ? (unsigned long)(unsigned int)valor : (unsigned long)valor;
        int totalBlocos = usaW ? 2 : 4;

        if(bruto == 0) {
            saida << "movz " << destino << ", #0\n";
            return;
        }
        bool primeiro = true;
        for(int i = 0; i < totalBlocos; i++) {
            unsigned int bloco = (unsigned int)((bruto >> (i * 16)) & 0xFFFF);
            if(bloco == 0) continue; // pula blocos zerados: o caso "valor inteiro == 0" ja foi tratado acima
            if(primeiro) {
                saida << "movz " << destino << ", #" << bloco;
                if(i > 0) saida << ", lsl #" << (i * 16);
                saida << "\n";
                primeiro = false;
            } else {
                saida << "movk " << destino << ", #" << bloco << ", lsl #" << (i * 16) << "\n";
            }
        }
    }

    // carrega um ArgumentoIR(imediato ou parametro/variavel-registrador)
    // no registrador de destino. Não aceita SIMBOLO_TEXTO: comparar
    // ponteiros de texto com "==" não faz sentido nesta linguagem ainda.
    void emitirCarregarValorEm(std::ostringstream& saida, const char* registradorDestino, const ArgumentoIR& arg) {
        emitirCarregarValorEmComLargura(saida, registradorDestino, arg, arg.larguraBytes);
    }

    // mesma coisa que emitirCarregarValorEm, so que "larguraOperacao" pode
    // ser MAIOR que arg.larguraBytes(ex: um "int"(4 bytes) somado a um
    // ponteiro(8 bytes) numa operacao de 64 bits, caso classico de
    // "ponteiro + deslocamento_int"). Quando isso acontece, carrega o
    // valor normalmente em w(32 bits) e da "sxtw" pra estender o SINAL
    // pros 32 bits altos de x: sem isso, um "mov w10, -4" seguido de um
    // "add x9, x9, x10" de 64 bits usaria x10 = 0x00000000FFFFFFFC(quase
    // 4 bilhoes) em vez de -4, porque escrever em w SEMPRE zera os bits
    // altos de x(comportamento do ARM64), nunca estende o sinal sozinho.
    void emitirCarregarValorEmComLargura(std::ostringstream& saida, const char* registradorDestino, const ArgumentoIR& arg, int larguraOperacao) {
        bool precisaEstenderSinal = (larguraOperacao == 8 && arg.larguraBytes <= 4);
        std::string destino32 = paraRegistrador32(registradorDestino);
        if(arg.tipo == ArgumentoIR::Tipo::IMEDIATO) {
            // um imediato "car"/"int" já chega truncado pelo GeradorIR; usamos o registrador x/w normalmente(movz/movk aceita ambos), so a leitura/gravacao de memoria precisa da largura estreita.
            bool usaW = (arg.larguraBytes <= 4);
            emitirMovImediato(saida, registradorDestino, arg.imediato, usaW);
        } else if(arg.tipo == ArgumentoIR::Tipo::REGISTRADOR) {
            emitirCarregarSlot(saida, registradorDestino, arg.registrador, arg.larguraBytes);
        } else {
            throw std::runtime_error("Arquitetura ARM64: esta operação não suporta texto(SIMBOLO_TEXTO) como operando");
        }
        if(precisaEstenderSinal) {
            saida << "sxtw " << registradorDestino << ", " << destino32 << "\n";
        }
    }

    // devolve o mnemonico de salto CONDICIONAL INVERTIDO: "se a condicao OP
    // for FALSA, salta". Usado tanto por emitirSe(pula pro "senao"/fim)
    // quanto por emitirEnquanto(pula pro fim do laço). Usa as condicoes
    // SIGNED do ARM64(lt/gt/le/ge), coerente com o resto do compilador
    // tratar todo inteiro como signed(ex: "sdiv" em vez de "udiv" na
    // divisao em emitirOperacaoAritmetica).
    const char* mnemonicoSaltoSeFalso(OperadorComparacaoIR operador) {
        switch(operador) {
            case OperadorComparacaoIR::IGUAL: return "b.ne";
            case OperadorComparacaoIR::DIFERENTE: return "b.eq";
            case OperadorComparacaoIR::MAIOR: return "b.le";
            case OperadorComparacaoIR::MENOR: return "b.ge";
            case OperadorComparacaoIR::MAIOR_IGUAL: return "b.lt";
            default: return "b.gt"; // MENOR_IGUAL
        }
    }

    // emite "se(esquerda OP direita) { então } senao { senaoCorpo }".
    // usa x9/x10 como registradores de rascunho(fora da faixa x0-x5, mesma
    // reserva documentada em emitirTexTam) e rotulos unicos por chamada.
    void emitirSe(std::ostringstream& saida, const InstrucaoSe& instrucao, const ProgramaIR& programa) {
        int idRotulo = contadorRotulos++;
        std::string rotuloSenao = "_se_senao_" + std::to_string(idRotulo);
        std::string rotuloFim = "_se_fim_" + std::to_string(idRotulo);

        emitirCarregarValorEmComLargura(saida, "x9", instrucao.esquerda, 8);
        emitirCarregarValorEmComLargura(saida, "x10", instrucao.direita, 8);
        saida << "cmp x9, x10\n";

        bool temSenao = instrucao.temSenao;
        const char* saltoSeFalso = mnemonicoSaltoSeFalso(instrucao.operador);
        saida << saltoSeFalso << " " << (temSenao ? rotuloSenao : rotuloFim) << "\n\n";

        emitirBlocoComandos(saida, *instrucao.entao, programa);

        if(temSenao) {
            saida << "b " << rotuloFim << "\n\n";
            saida << rotuloSenao << ":\n";
            emitirBlocoComandos(saida, *instrucao.senaoCorpo, programa);
        }
        saida << rotuloFim << ":\n\n";
    }

    // emite "enquanto(esquerda OP direita) { corpo }": testa a condição no
    // topo do laço(rotuloTopo), salta pro fim(rotuloFim) se falsa, senão 
    // executa o corpo e volta a testar de novo. Mesma reserva de x9/x10
    // documentada em emitirSe.
    // emite "enquanto(esquerda OP direita) { corpo }": testa a condição no
    // topo do laço(rotuloTopo), salta pro fim(rotuloFim) se falsa, senão 
    // executa o corpo e volta a testar de novo. Mesma reserva de x9/x10
    // documentada em emitirSe. IMPORTANTE: comandosCondicao(instrucoes
    // auxiliares que materializam esquerda/direita, ex: LEITURA_INDEXADA de
    // "txt[i]") sao reemitidas a CADA volta do rotuloTopo, nao uma unica vez
    // antes do laço: a condicao pode depender de estado alterado pelo corpo
    // (ex: "i" mudando a cada iteracao em "enq(txt[i] != 0)"), entao emitir
    // so uma vez antes congelaria o resultado da primeira avaliação e o
    // laço nunca releria a condição de verdade.
    void emitirEnquanto(std::ostringstream& saida, const InstrucaoEnquanto& instrucao, const ProgramaIR& programa) {
        int idRotulo = contadorRotulos++;
        std::string rotuloTopo = "_enquanto_topo_" + std::to_string(idRotulo);
        std::string rotuloFim = "_enquanto_fim_" + std::to_string(idRotulo);

        saida << rotuloTopo << ":\n";
        emitirBlocoComandos(saida, *instrucao.comandosCondicao, programa);
        emitirCarregarValorEmComLargura(saida, "x9", instrucao.esquerda, 8);
        emitirCarregarValorEmComLargura(saida, "x10", instrucao.direita, 8);
        saida << "cmp x9, x10\n";

        const char* saltoSeFalso = mnemonicoSaltoSeFalso(instrucao.operador);
        saida << saltoSeFalso << " " << rotuloFim << "\n\n";

        emitirBlocoComandos(saida, *instrucao.corpo, programa);

        saida << "b " << rotuloTopo << "\n";
        saida << rotuloFim << ":\n\n";
    }

    // emite cada ComandoIR de um bloco("entao" ou "senao" de um "se");
    // usado tanto pelo corpo normal de uma funcao quanto recursivamente
    // por blocos de "se" aninhados.
    void emitirBlocoComandos(std::ostringstream& saida, const Vetor<ComandoIR>& bloco, const ProgramaIR& programa) {
        for(int i = 0; i < bloco.tam; i++) {
            const ComandoIR& comando = bloco[i];
            if(comando.tipo == ComandoIR::Tipo::CHAMADA_SISTEMA_CRUA) {
                emitirChamadaSistemaCrua(saida, comando.chamadaSistemaCrua);
            } else if(comando.tipo == ComandoIR::Tipo::CHAMADA_FUNCAO_USUARIO) {
                emitirChamadaFuncaoUsuario(saida, comando.chamadaFuncaoUsuario);
            } else if(comando.tipo == ComandoIR::Tipo::SE) {
                emitirSe(saida, comando.instrucaoSe, programa);
            } else if(comando.tipo == ComandoIR::Tipo::ENQUANTO) {
                emitirEnquanto(saida, comando.instrucaoEnquanto, programa);
            } else if(comando.tipo == ComandoIR::Tipo::REATRIBUICAO) {
                emitirReatribuicao(saida, comando.instrucaoReatribuicao);
            } else if(comando.tipo == ComandoIR::Tipo::OPERACAO_ARITMETICA) {
                emitirOperacaoAritmetica(saida, comando.instrucaoOperacaoAritmetica);
            } else if(comando.tipo == ComandoIR::Tipo::LEITURA_INDEXADA) {
                emitirLeituraIndexada(saida, comando.instrucaoLeituraIndexada);
            } else if(comando.tipo == ComandoIR::Tipo::ESCRITA_INDEXADA) {
                emitirEscritaIndexada(saida, comando.instrucaoEscritaIndexada);
            } else {
                emitirRetorne(saida, comando.instrucaoRetorne);
            }
        }
    }

    // emite #chamada_sistema id, arg1, arg2, ...; cru: o primeiro identificador
    // e o ID ABSTRATO da chamada de sistema (mesmo espaco de ChamadaSistemaId,
    // ex: ESCREVER=1, SAIR=2), traduzido aqui pro numero real da chamada de
    // sistema desta arquitetura via numSistema(). Os demais argumentos vao
    // posicionalmente pra x0, x1, x2... na ordem em que foram escritos.
    // suporta ate 6 argumentos posicionais(x0-x5), limite da convenção ARM64.
    void emitirChamadaSistemaCrua(std::ostringstream& saida, const InstrucaoChamadaSistemaCrua& instrucao) {
        static const char* registradores[] = { "x0", "x1", "x2", "x3", "x4", "x5" };
        int totalArgumentosPosicionais = instrucao.argumentos.tam - 1;
        if(totalArgumentosPosicionais > 6) {
            throw std::runtime_error("Arquitetura ARM64: #chamada_sistema aceita no máximo 6 argumentos posicionais");
        }
        for(int i = 1; i < instrucao.argumentos.tam; i++) {
            const ArgumentoIR& arg = instrucao.argumentos[i];
            const char* registrador = registradores[i - 1];
            if(arg.tipo == ArgumentoIR::Tipo::SIMBOLO_TEXTO) {
                saida << "ldr " << registrador << ", =" << arg.simbolo << "\n";
            } else if(arg.tipo == ArgumentoIR::Tipo::REGISTRADOR) {
                // ldrb/ldr(w) num registrador w zera os bits altos do x correspondente, entao o x0-x5 completo chega correto pra chamada de sistema mesmo pra um valor "car"(1 byte) ou "int"(4 bytes).
                emitirCarregarSlot(saida, registrador, arg.registrador, arg.larguraBytes);
            } else {
                bool usaW = (arg.larguraBytes <= 4);
                emitirMovImediato(saida, registrador, arg.imediato, usaW);
            }
        }
        const ArgumentoIR& idAbstrato = instrucao.argumentos[0];
        if(idAbstrato.tipo != ArgumentoIR::Tipo::IMEDIATO) {
            throw std::runtime_error("Arquitetura ARM64: o ID da #chamada_sistema precisa ser um valor imediato (int)");
        }
        ChamadaSistemaId id = static_cast<ChamadaSistemaId>(idAbstrato.imediato);
        saida << "mov x8, " << numSistema(id) << "\n";
        saida << "svc 0\n";
        if(instrucao.capturaResultado) {
            emitirGravarSlot(saida, "x0", instrucao.slotDestino, 8); // resultado de syscall(ex: ponteiro do mmap) e sempre 8 bytes
        }
        saida << "\n";
    }

    // emite "retorne;" ou "retorne valor;": materializa 'valor'(se houver)
    // diretamente em x0(registrador de retorno da convencao AAPCS64) e
    // desvia pro rotulo de fim da funcao atual(_fim_<nome>), que desfaz o
    // frame e emite "ret". Funciona de qualquer ponto do corpo, inclusive
    // dentro de blocos aninhados de "se"/"enquanto", ja que "b" e um salto
    // incondicional direto ao rotulo.
    void emitirRetorne(std::ostringstream& saida, const InstrucaoRetorne& instrucao) {
        if(instrucao.temValor) {
            const ArgumentoIR& valor = instrucao.valor;
            if(valor.tipo == ArgumentoIR::Tipo::SIMBOLO_TEXTO) {
                saida << "ldr x0, =" << valor.simbolo << "\n";
            } else {
                emitirCarregarValorEm(saida, "x0", valor);
            }
        }
        saida << "b _fim_" << funcaoAtual->nome << "\n\n";
    }

    // emite "slot = valor": materializa "valor" em x9(registrador de rascunho, mesma reserva documentada em emitirTexTam) e grava no slot
    // de 16 bytes da variável(seja declaração inicial ou reatribuição).
    void emitirReatribuicao(std::ostringstream& saida, const InstrucaoReatribuicao& instrucao) {
        const ArgumentoIR& valor = instrucao.valor;
        if(valor.tipo == ArgumentoIR::Tipo::SIMBOLO_TEXTO) {
            saida << "ldr x9, =" << valor.simbolo << "\n";
        } else if(valor.tipo == ArgumentoIR::Tipo::REGISTRADOR) {
            emitirCarregarSlot(saida, "x9", valor.registrador, valor.larguraBytes);
        } else {
            bool usaW = (valor.larguraBytes <= 4);
            emitirMovImediato(saida, "x9", valor.imediato, usaW);
        }
        emitirGravarSlot(saida, "x9", instrucao.slotDestino, valor.larguraBytes);
    }
    // emite "slot = esquerda OP direita": carrega os dois operandos em
    // x9/x10(mesmos registradores de rascunho documentados em emitirTexTam),
    // calcula em x9 e grava no slot de 16 bytes do destino(temporário ou
    // variável, tanto faz: mesmo espaço de slots de emitirReatribuicao).
    // DIVISAO e MODULO checam o divisor antes de usar "sdiv": divisao por
    // zero e matematicamente indefinida, e o ARM64 não gera nenhuma
    // excecao de hardware pra isso(sdiv por zero silenciosamente devolve 0),
    // entao a checagem e emitida aqui em software, abortando o processo
    // com "sair(1)" se o divisor for zero, em vez de deixar passar um
    // resultado incorreto sem aviso nenhum.
    void emitirOperacaoAritmetica(std::ostringstream& saida, const InstrucaoOperacaoAritmetica& instrucao) {
        // regra de promocao: usa a MAIOR largura entre os dois operandos, decide se opera em w(32 bits, largura<=4) ou x(64 bits, largura==8)
        int largura = (instrucao.esquerda.larguraBytes > instrucao.direita.larguraBytes) ? instrucao.esquerda.larguraBytes : instrucao.direita.larguraBytes;
        bool usaW = (largura <= 4);
        emitirCarregarValorEmComLargura(saida, "x9", instrucao.esquerda, largura);
        emitirCarregarValorEmComLargura(saida, "x10", instrucao.direita, largura);
        if(instrucao.operador == OperadorAritmeticoIR::SOMA) {
            saida << (usaW ? "add w9, w9, w10\n" : "add x9, x9, x10\n");
        } else if(instrucao.operador == OperadorAritmeticoIR::SUBTRACAO) {
            saida << (usaW ? "sub w9, w9, w10\n" : "sub x9, x9, x10\n");
        } else if(instrucao.operador == OperadorAritmeticoIR::MULTIPLICACAO) {
            saida << (usaW ? "mul w9, w9, w10\n" : "mul x9, x9, x10\n");
        } else if(instrucao.operador == OperadorAritmeticoIR::DIVISAO) {
            emitirChecagemDivisorZero(saida);
            saida << (usaW ? "sdiv w9, w9, w10\n" : "sdiv x9, x9, x10\n");
        } else { // MODULO: resto = dividendo - (dividendo/divisor)*divisor.
            // x9=dividendo, x10=divisor. x28 e reservado(fora do pool de
            // TOTAL_REGISTRADORES_FISICOS, ver ir.h) especificamente pra este
            // caso: guarda o dividendo enquanto x9 vira o quociente(sdiv),
            // pro msub calcular "dividendo - quociente*divisor" em seguida.
            // MSUB Xd,Xn,Xm,Xa calcula Xd = Xa - (Xn*Xm): Xa(minuendo) =
            // x28(dividendo original), Xn*Xm = quociente*divisor.
            emitirChecagemDivisorZero(saida);
            saida << "mov x28, x9\n";
            saida << (usaW ? "sdiv w9, w9, w10\n" : "sdiv x9, x9, x10\n");
            saida << (usaW ? "msub w9, w9, w10, w28\n" : "msub x9, x9, x10, x28\n");
        }
        emitirGravarSlot(saida, "x9", instrucao.slotDestino, largura);
    }

    // emite a checagem "se x10(divisor) == 0, aborta o processo": usada por
    // DIVISAO e MODULO antes de qualquer "sdiv". Usa um rotulo unico por
    // chamada(mesmo padrao de emitirSe/emitirEnquanto/emitirTexTam) pra nao
    // colidir quando o programa tem mais de uma divisao/modulo. O código de
    // saída 1 segue a convenção Unix de "erro genérico"(0 == sucesso).
    void emitirChecagemDivisorZero(std::ostringstream& saida) {
        int idRotulo = contadorRotulos++;
        std::string rotuloContinua = "_divisor_ok_" + std::to_string(idRotulo);
        saida << "cbnz x10, " << rotuloContinua << "\n";
        saida << "mov x0, 1\n"; // codigo de saida: erro(divisao por zero)
        saida << "mov x8, " << numSistema(ChamadaSistemaId::SAIR) << "\n";
        saida << "svc 0\n";
        saida << rotuloContinua << ":\n";
    }

    // emite "slot = ponteiro[indice]": carrega o ponteiro base em x9 e o
    // índice em x10(mesmos registradores de rascunho de emitirOperacaoAritmetica). Multiplica o indice pela largura de cada elemento
    // apontado(ponteiro.larguraElemento: 1 pra car*, 8 pra int*, ou qualquer
    // largura futura) via "mul"(generico, funciona pra qualquer largura, não
    // so potência de 2), soma ao ponteiro pra achar o endereco final, e le
    // essa mesma largura de bytes desse endereco(ldrb se largura==1, ldr se
    // largura==8), gravando o resultado no slot destino.
    // emite "slot = ponteiro[indice]": carrega o ponteiro base em x9 e o
    // índice em x10(mesmos registradores de rascunho de emitirOperacaoAritmetica). Multiplica o indice pela largura de cada elemento
    // apontado(ponteiro.larguraElemento: 1 pra car*, 4 pra int*, 8 pra
    // longo*, ou qualquer largura futura) via "mul"(generico, funciona pra
    // qualquer largura, não só potência de 2), soma ao ponteiro pra achar o
    // endereco final, e le essa mesma largura de bytes desse endereco
    // (ldrb se largura==1, ldr w se largura==4, ldr x se largura==8),
    // gravando o resultado no slot destino.
    void emitirLeituraIndexada(std::ostringstream& saida, const InstrucaoLeituraIndexada& instrucao) {
        emitirCarregarValorEmComLargura(saida, "x9", instrucao.ponteiro, 8);
        emitirCarregarValorEmComLargura(saida, "x10", instrucao.indice, 8);
        int largura = instrucao.ponteiro.larguraElemento;
        saida << "mov x11, " << largura << "\n";
        saida << "mul x10, x10, x11\n"; // x10 = indice * largura do elemento
        saida << "add x9, x9, x10\n"; // x9 = endereco final(ponteiro + indice*largura)
        if(largura == 1) {
            saida << "ldrb w9, [x9]\n";
            emitirGravarSlot(saida, "x9", instrucao.slotDestino, 1);
        } else if(largura == 4) {
            saida << "ldr w9, [x9]\n";
            emitirGravarSlot(saida, "x9", instrucao.slotDestino, 4);
        } else {
            saida << "ldr x9, [x9]\n";
            emitirGravarSlot(saida, "x9", instrucao.slotDestino, 8);
        }
    }

    // emite "ponteiro[indice] = valor": usa x9/x10/x11(mesma reserva de
    // rascunho documentada em emitirTexTam; x19..x27 sao fisicos de slots e
    // NÃO podem ser tocados aqui). Multiplica o indice pela largura de cada
    // elemento apontado(ponteiro.larguraElemento) via "mul"(generico, funciona
    // pra qualquer largura), soma ao ponteiro pra achar o endereco final, move
    // pra x10(libera x9), materializa "valor" em x9 e grava essa mesma
    // largura de bytes no endereco(strb se largura==1, str se largura==8).
    // emite "ponteiro[indice] = valor": usa x9/x10/x11(mesma reserva de
    // rascunho documentada em emitirTexTam; x19..x27 sao fisicos de slots e
    // NÃO podem ser tocados aqui). Multiplica o indice pela largura de cada
    // elemento apontado(ponteiro.larguraElemento) via "mul"(generico, funciona
    // pra qualquer largura), soma ao ponteiro pra achar o endereco final, move
    // pra x10(libera x9), materializa "valor" em x9 e grava essa mesma
    // largura de bytes no endereço: largura==1(car*) usa strb, largura==4
    // (int*) usa str num registrador w(32 bits), largura==8(longo*) usa str
    // num registrador x(64 bits).
    void emitirEscritaIndexada(std::ostringstream& saida, const InstrucaoEscritaIndexada& instrucao) {
        emitirCarregarValorEmComLargura(saida, "x9", instrucao.ponteiro, 8);
        emitirCarregarValorEmComLargura(saida, "x10", instrucao.indice, 8);
        int largura = instrucao.ponteiro.larguraElemento;
        saida << "mov x11, " << largura << "\n";
        saida << "mul x10, x10, x11\n"; // x10 = indice * largura do elemento
        saida << "add x9, x9, x10\n"; // x9 = endereco final(ponteiro + indice*largura)
        saida << "mov x10, x9\n"; // move o endereco pra x10, libera x9 pro valor
        emitirCarregarValorEm(saida, "x9", instrucao.valor);
        if(largura == 1) {
            saida << "strb w9, [x10]\n";
        } else if(largura == 4) {
            saida << "str w9, [x10]\n";
        } else {
            saida << "str x9, [x10]\n";
        }
    }

    // emite uma chamada real a uma função de usuário: nome(arg1, arg2, ...);
    // argumentos vao posicionalmente pra x0, x1, x2...(convencao AAPCS64),
    // seguido de "bl nome". Suporta até 6 argumentos(x0-x5). Se a chamada
    // for usada como valor de expressão(capturaResultado == true, ex:
    // "int x = f();"), grava x0(valor de retorno, convenção AAPCS64) no
    // slot destino logo apos o bl, antes que qualquer outra chamada possa sujar x0.
    void emitirChamadaFuncaoUsuario(std::ostringstream& saida, const InstrucaoChamadaFuncaoUsuario& instrucao) {
        static const char* registradores[] = { "x0", "x1", "x2", "x3", "x4", "x5" };
        if(instrucao.argumentos.tam > 6) {
            throw std::runtime_error("Arquitetura ARM64: chamada de função aceita no máximo 6 argumentos");
        }
        for(int i = 0; i < instrucao.argumentos.tam; i++) {
            const ArgumentoIR& arg = instrucao.argumentos[i];
            const char* registrador = registradores[i];
            if(arg.tipo == ArgumentoIR::Tipo::SIMBOLO_TEXTO) {
                saida << "ldr " << registrador << ", =" << arg.simbolo << "\n";
            } else if(arg.tipo == ArgumentoIR::Tipo::REGISTRADOR) {
                emitirCarregarSlot(saida, registrador, arg.registrador, arg.larguraBytes);
            } else {
                bool usaW = (arg.larguraBytes <= 4);
                emitirMovImediato(saida, registrador, arg.imediato, usaW);
            }
        }
        saida << "bl " << instrucao.nome << "\n";
        if(instrucao.capturaResultado) {
            emitirGravarSlot(saida, "x0", instrucao.slotDestino, instrucao.resultadoLargura);
        }
        saida << "\n";
    }

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