#pragma once
/*
* [FUNÇÃO]: Gerador de código.
* [IMPLEMENTAÇÃO]: @Shiniga-OP.
* [BASE]: Assembly.
* [SISTEMA OPERACIONAL]: ANDROID.
* [ARQUITETURA]: ARM64-LINUX-ANDROID(ARM64).
* [LINGUAGEM]: Português Brasil(PT-BR).
* [DATA]: 07/02/2026.
* [ATUAL]: 21/02/2026.
* [PONTEIRO]: dereferencia automatica, acesso a endereços apenas com "@ponteiro".
*/
// carregar
void carregar_valor(FILE* s, Variavel* var);
void carregar_const(FILE* s, int titulo);
// add
int add_tex(const char* valor);
int add_const(TipoToken tipo, const char* lex, double d_val, long l_val);
// gerar
void gerar_comparacao(FILE* s, TipoToken op, TipoToken tipo);
void gerar_operacao(FILE* s, TipoToken op, TipoToken tipo);
void gerar_prelude(FILE* s);
// util
void escrever_valor(FILE* s, TipoToken tipo);
// emissão segura (posset x29 fora do range -256..255 usa x9 como temporário)
void fp_str(FILE* s, const char* reg, int pos);
void fp_ldr(FILE* s, const char* reg, int pos);
void fp_strb(FILE* s, const char* reg, int pos);
void fp_ldrb(FILE* s, const char* reg, int pos);
void fp_add_fp(FILE* s, const char* dst, int pos);

#include "analisador.h"

// [EMISSÃO SEGURA]:
// ARM64 unscaled (STUR/LDUR): posset 9-bit signed → -256..255
// Para possets fora desse range usa x9 como scratch temporário.
// x9 é caller-saved e não interfere com x0..x8 usados para retorno/args.
static inline int _pos_ok(int pos) { return pos >= -256 && pos <= 255; }

void fp_str(FILE* s, const char* reg, int pos) {
    if(_pos_ok(pos)) fprintf(s, "  str %s, [x29, %d]\n", reg, pos);
    else { fprintf(s, "  add x9, x29, %d\n", pos); fprintf(s, "  str %s, [x9]\n", reg); }
}
void fp_ldr(FILE* s, const char* reg, int pos) {
    if(_pos_ok(pos)) fprintf(s, "  ldr %s, [x29, %d]\n", reg, pos);
    else { fprintf(s, "  add x9, x29, %d\n", pos); fprintf(s, "  ldr %s, [x9]\n", reg); }
}
void fp_strb(FILE* s, const char* reg, int pos) {
    if(_pos_ok(pos)) fprintf(s, "  strb %s, [x29, %d]\n", reg, pos);
    else { fprintf(s, "  add x9, x29, %d\n", pos); fprintf(s, "  strb %s, [x9]\n", reg); }
}
void fp_ldrb(FILE* s, const char* reg, int pos) {
    if(_pos_ok(pos)) fprintf(s, "  ldrb %s, [x29, %d]\n", reg, pos);
    else { fprintf(s, "  add x9, x29, %d\n", pos); fprintf(s, "  ldrb %s, [x9]\n", reg); }
}
void fp_add_fp(FILE* s, const char* dst, int pos) {
    // add dst, x29, pos — se pos cabe em 12 bits (0..4095 ou negativo via sub), ok
    // add imm12 aceita 0..4095; para negativo usa sub
    if(pos >= 0 && pos <= 4095) fprintf(s, "  add %s, x29, %d\n", dst, pos);
    else if(pos < 0 && (-pos) <= 4095) fprintf(s, "  sub %s, x29, %d\n", dst, -pos);
    else {
        // posset grande: usa mov + add
        fprintf(s, "  mov x9, %d\n", pos);
        fprintf(s, "  add %s, x29, x9\n", dst);
    }
}

// [GERAÇÃO]:
void gerar_prelude(FILE* s) {
    fprintf(s,".section .text\n");
}

void gerar_texs(FILE* s) {
    if(tex_cnt == 0) return;
    fprintf(s, ".section .rodata\n");
    fprintf(s, ".align 2\n");
    for(int i = 0; i < tex_cnt; i++) {
        fprintf(s, "%s: .asciz \"", texs[i].nome);
        
        // processa cada caractere pra escapar caracteres especiais
        const char* str = texs[i].valor;
        for(int j = 0; str[j] != '\0'; j++) {
            unsigned char c = str[j];
            switch(c) {
                case '\n': fprintf(s, "\\n"); break;
                case '\t': fprintf(s, "\\t"); break;
                case '\r': fprintf(s, "\\r"); break;
                case '\0': fprintf(s, "\\0"); break;
                case '\\': fprintf(s, "\\\\"); break;
                case '\"': fprintf(s, "\\\""); break;
                case '\a': fprintf(s, "\\a"); break;
                case '\b': fprintf(s, "\\b"); break;
                case '\v': fprintf(s, "\\v"); break;
                case '\f': fprintf(s, "\\f"); break;
                default:
                    // caracteres imprimiveis normais
                    if(c >= 32 && c <= 126) {
                        fputc(c, s);
                    } else {
                        // caracteres não imprimiveis: usa escape octal
                        fprintf(s, "\\%03o", c);
                    }
                break;
            }
        }
        fprintf(s, "\"\n");
    }
    fprintf(s, ".section .text\n\n");
}

void gerar_consts(FILE* s) {
    if(const_cnt == 0) return;
    
    fprintf(s, "  .section .rodata\n");
    fprintf(s, "  .align 8\n");
    for(int i = 0; i < const_cnt; i++) {
        fprintf(s, "const_%d:\n", i);
        if(constantes[i].tipo == T_INT) fprintf(s, "  .word %ld\n", constantes[i].l_val);
        else if(constantes[i].tipo == T_FLU) fprintf(s, "  .float %f\n", (float)constantes[i].d_val);
        else if(constantes[i].tipo == T_DOBRO) fprintf(s, "  .double %f\n", constantes[i].d_val);
        else if(constantes[i].tipo == T_LONGO) fprintf(s, "  .quad %ld\n", constantes[i].l_val);
    }
}

void gerar_operacao(FILE* s, TipoToken op, TipoToken tipo) {
    switch(op) {
        case T_MAIS: 
            if(tipo == T_pFLU) fprintf(s, "  fadd s0, s1, s0\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fadd d0, d1, d0\n");
            else if(tipo == T_pLONGO) fprintf(s, "  add x0, x1, x0\n");
            else if(tipo == T_PONTEIRO) {
                fprintf(s, "  lsl x0, x0, 3\n");
                fprintf(s, "  add x0, x1, x0\n");
            } else fprintf(s, "  add w0, w1, w0\n");
        break;
        case T_MENOS: 
            if(tipo == T_pFLU) fprintf(s, "  fsub s0, s1, s0\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fsub d0, d1, d0\n");
            else if(tipo == T_pLONGO) fprintf(s, "  sub x0, x1, x0\n");
            else if(tipo == T_PONTEIRO) {
                fprintf(s, "  lsl x0, x0, 3\n");
                fprintf(s, "  sub x0, x1, x0\n");
            } else fprintf(s, "  sub w0, w1, w0\n");
        break;
        case T_VEZES: 
            if(tipo == T_pFLU) fprintf(s, "  fmul s0, s1, s0\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fmul d0, d1, d0\n");
            else if(tipo == T_pLONGO) fprintf(s, "  mul x0, x1, x0\n");
            else if(tipo == T_PONTEIRO) {
                fprintf(s, "  lsl x0, x0, 3\n");
                fprintf(s, "  mul x0, x1, x0\n");
            } else fprintf(s, "  mul w0, w1, w0\n");
        break;
        case T_DIV: 
            if(tipo == T_pFLU) fprintf(s, "  fdiv s0, s1, s0\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fdiv d0, d1, d0\n");
            else if(tipo == T_pLONGO) fprintf(s, "  sdiv x0, x1, x0\n");
            else if(tipo == T_PONTEIRO) {
                fprintf(s, "  lsl x0, x0, 3\n");
                fprintf(s, "  sdiv x0, x1, x0\n");
            } else fprintf(s, "  sdiv w0, w1, w0\n");
        break;
        case T_PORCEN: 
            if(tipo == T_pFLU || tipo == T_pDOBRO) {
                fatal("[gerar_operacao] operador módulo não suportado para tipos flutuante");
            } else if(tipo == T_pLONGO || tipo == T_PONTEIRO) {
                fprintf(s, "  sdiv x2, x1, x0\n");
                fprintf(s, "  msub x0, x2, x0, x1\n");
            } else {
                fprintf(s, "  sdiv w2, w1, w0\n");
                fprintf(s, "  msub w0, w2, w0, w1\n");
            }
        break;
        case T_TAMBEM_TAMBEM:
            if(tipo == T_pFLU || tipo == T_pDOBRO) {
                fatal("[gerar_operacao] operador && não suportado para tipos flutuante");
            } else if(tipo == T_pLONGO || tipo == T_PONTEIRO) {
                fprintf(s, "  cmp x1, 0\n");
                fprintf(s, "  cset x1, ne\n");
                fprintf(s, "  cmp x0, 0\n");
                fprintf(s, "  cset x0, ne\n");
                fprintf(s, "  and x0, x1, x0\n");
            } else {
                fprintf(s, "  cmp w1, 0\n");
                fprintf(s, "  cset w1, ne\n");
                fprintf(s, "  cmp w0, 0\n");
                fprintf(s, "  cset w0, ne\n");
                fprintf(s, "  and w0, w1, w0\n");
            }
        break;
        case T_MENOR_MENOR: 
            if(tipo == T_pFLU) fprintf(s, "  flsl s0, s1, s0\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  flsl d0, d1, d0\n");
            else if(tipo == T_pLONGO) fprintf(s, "  lsl x0, x1, x0\n");
            else if(tipo == T_PONTEIRO) {
                fprintf(s, "  lsl x0, x0, 3\n");
                fprintf(s, "  lsl x0, x1, x0\n");
            } else fprintf(s, "  lsl w0, w1, w0\n");
        break;
        case T_MAIOR_MAIOR: 
            if(tipo == T_pFLU) fprintf(s, "  flsr s0, s1, s0\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  flsr d0, d1, d0\n");
            else if(tipo == T_pLONGO) fprintf(s, "  lsr x0, x1, x0\n");
            else if(tipo == T_PONTEIRO) {
                fprintf(s, "  lsr x0, x0, 3\n");
                fprintf(s, "  lsr x0, x1, x0\n");
            } else fprintf(s, "  lsr w0, w1, w0\n");
        break;
        case T_TAMBEM:
            if(tipo == T_pFLU || tipo == T_pDOBRO) fatal("[gerar_operacao] operador & não suportado pra tipos flutuante");
            else if(tipo == T_pLONGO || tipo == T_PONTEIRO) fprintf(s, "  and x0, x1, x0\n");
            else fprintf(s, "  and w0, w1, w0\n");
        break;
        case T_OU:
            if(tipo == T_pFLU || tipo == T_pDOBRO) fatal("[gerar_operacao] operador | não suportado pra tipos flutuante");
            else if(tipo == T_pLONGO || tipo == T_PONTEIRO) fprintf(s, "  orr x0, x1, x0\n");
            else fprintf(s, "  orr w0, w1, w0\n");
        break;
        default: fatal("[gerar_operacao] operador inválido");
    }
}

void gerar_comparacao(FILE* s, TipoToken op, TipoToken tipo) {
    switch(op) {
        case T_IGUAL_IGUAL:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, eq\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, eq\n");
            else if(tipo == T_pLONGO) fprintf(s, "  cmp x1, x0\n  cset x0, eq\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, eq\n");
        break;
        case T_DIFERENTE:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, ne\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, ne\n");
            else if(tipo == T_pLONGO) fprintf(s, "  cmp x1, x0\n  cset x0, ne\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, ne\n");
        break;
        case T_MAIOR:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, gt\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, gt\n");
            else if(tipo == T_pLONGO) fprintf(s, "  cmp x1, x0\n  cset x0, gt\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, gt\n");
        break;
        case T_MENOR:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, lt\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, lt\n");
            else if(tipo == T_pLONGO || tipo == T_PONTEIRO) fprintf(s, "  cmp x1, x0\n  cset x0, lt\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, lt\n");
        break;
        case T_MAIOR_IGUAL:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, ge\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, ge\n");
            else if(tipo == T_pLONGO) fprintf(s, "  cmp x1, x0\n  cset x0, ge\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, ge\n");
        break;
        case T_MENOR_IGUAL:
            if(tipo == T_pFLU) fprintf(s, "  fcmp s1, s0\n  cset w0, le\n");
            else if(tipo == T_pDOBRO) fprintf(s, "  fcmp d1, d0\n  cset w0, le\n");
            else if(tipo == T_pLONGO) fprintf(s, "  cmp x1, x0\n  cset x0, le\n");
            else fprintf(s, "  cmp w1, w0\n  cset w0, le\n");
        break;
        default: fatal("[gerar_comparacao] operador de comparação inválido");
    }
}

TipoToken converter_tipos(FILE* s, TipoToken tipo_anterior, TipoToken tipo_atual) {
    if(debug_o) {
        printf("[converter_tipos]: conversão de %s e %s\n", 
        token_str(tipo_anterior), token_str(tipo_atual));
    }
    if(tipo_anterior == tipo_atual) return tipo_anterior;
    if(tipo_anterior == T_PONTEIRO && (tipo_atual == T_pINT || tipo_atual == T_pLONGO)) {
        return T_PONTEIRO;
    }
    if(tipo_atual == T_PONTEIRO && (tipo_anterior == T_pINT || tipo_anterior == T_pLONGO)) {
        return T_PONTEIRO;
    }
    // caso 1: inteiro(esq) com flutuante(dir)
    // o inteiro ta em w1(anterior), o flutuante ta em s0(atual)
    if(tipo_anterior == T_pINT && tipo_atual == T_pFLU) {
        fprintf(s, "  scvtf s1, w1\n"); // converte w1 pra s1
        return T_pFLU;
    }
    // caso 2: flutuante(esq) com inteiro(dir)
    // O flutuante ta em s1(anterior), o inteiro ta em w0(atual)
    else if(tipo_anterior == T_pFLU && tipo_atual == T_pINT) {
        fprintf(s, "  scvtf s0, w0\n"); // converte w0 pra s0
        return T_pFLU;
    }
    // caso 3: inteiro e dobro
    else if(tipo_anterior == T_pINT && tipo_atual == T_pDOBRO) {
        fprintf(s, "  scvtf d1, w1\n"); // int w1 -> dobro d1
        return T_pDOBRO;
    } else if(tipo_anterior == T_pDOBRO && tipo_atual == T_pINT) {
        fprintf(s, "  scvtf d0, w0\n"); // int w0 -> dobro d0
        return T_pDOBRO;
    }
    // caso 4: flutuante e dobro
    else if(tipo_anterior == T_pFLU && tipo_atual == T_pDOBRO) {
        fprintf(s, "  fcvt d1, s1\n"); // flu s1 -> dobro d1
        return T_pDOBRO;
    } else if(tipo_anterior == T_pDOBRO && tipo_atual == T_pFLU) {
        fprintf(s, "  fcvt d0, s0\n"); // flu s0 -> dobro d0(promoção)
        return T_pDOBRO;
    }
    // caso 5: longo e flutuante/dobro
    else if(tipo_anterior == T_pLONGO && tipo_atual == T_pFLU) {
        fprintf(s, "  scvtf s1, x1\n");
        return T_pFLU;
    } else if(tipo_anterior == T_pFLU && tipo_atual == T_pLONGO) {
        fprintf(s, "  scvtf s0, x0\n");
        return T_pFLU;
    } else if(tipo_anterior == T_pLONGO && tipo_atual == T_pDOBRO) {
        fprintf(s, "  scvtf d1, x1\n");
        return T_pDOBRO;
    } else if(tipo_anterior == T_pDOBRO && tipo_atual == T_pLONGO) {
        fprintf(s, "  scvtf d0, x0\n");
        return T_pDOBRO;
    }
    // retorno padrão(mantem o maior tipo se não tiver conversão especifica)
    return (tam_tipo(tipo_atual) > tam_tipo(tipo_anterior)) ? tipo_atual : tipo_anterior;
}

void gerar_convert(FILE* s, TipoToken tipo_origem, TipoToken tipo_destino) {
    if(tipo_origem == tipo_destino) return; // desnecessario né meu, complicado meu
    // vonversões numericas
    if(tipo_destino == T_pINT) {
        if(tipo_origem == T_pFLU) fprintf(s, "  fcvtzs w0, s0\n");
        else if(tipo_origem == T_pDOBRO) fprintf(s, "  fcvtzs w0, d0\n");
        else if(tipo_origem == T_pLONGO) fprintf(s, "  mov w0, w0\n"); // Trunca
        else if(tipo_origem == T_pCAR) fprintf(s, "  sxtb w0, w0\n");
        else if(tipo_origem == T_pBOOL) fprintf(s, "  and w0, w0, 1\n");
    } else if(tipo_destino == T_pBYTE) {
        if(tipo_origem == T_pINT || tipo_origem == T_pLONGO) {
            fprintf(s, "  and w0, w0, 0xFF  // truncar para byte\n");
        } else if(tipo_origem == T_pFLU) {
            fprintf(s, "  fcvtzs w0, s0\n");
            fprintf(s, "  and w0, w0, 0xFF\n");
        } else if(tipo_origem == T_pDOBRO) {
            fprintf(s, "  fcvtzs w0, d0\n");
            fprintf(s, "  and w0, w0, 0xFF\n");
        }
    } else if(tipo_destino == T_pLONGO) {
        if(tipo_origem == T_pINT) fprintf(s, "  sxtw x0, w0\n");
        else if(tipo_origem == T_pFLU) fprintf(s, "  fcvtzs x0, s0\n");
        else if(tipo_origem == T_pDOBRO) fprintf(s, "  fcvtzs x0, d0\n");
        else if(tipo_origem == T_pCAR) fprintf(s, "  sxtb x0, w0\n");
        else if(tipo_origem == T_pBOOL) fprintf(s, "  and x0, x0, 1\n");
    } else if(tipo_destino == T_pFLU) {
        if(tipo_origem == T_pINT) fprintf(s, "  scvtf s0, w0\n");
        else if(tipo_origem == T_pLONGO) fprintf(s, "  scvtf s0, x0\n");
        else if(tipo_origem == T_pDOBRO) fprintf(s, "  fcvt s0, d0\n");
        else if(tipo_origem == T_pCAR) {
            fprintf(s, "  sxtb w0, w0\n");
            fprintf(s, "  scvtf s0, w0\n");
        }
    } else if(tipo_destino == T_pDOBRO) {
        if(tipo_origem == T_pINT) fprintf(s, "  scvtf d0, w0\n");
        else if(tipo_origem == T_pLONGO) fprintf(s, "  scvtf d0, x0\n");
        else if(tipo_origem == T_pFLU) fprintf(s, "  fcvt d0, s0\n");
        else if(tipo_origem == T_pCAR) {
            fprintf(s, "  sxtb w0, w0\n");
            fprintf(s, "  scvtf d0, w0\n");
        }
    } else if(tipo_destino == T_pCAR) {
        if(tipo_origem == T_pINT || tipo_origem == T_pLONGO) {
            fprintf(s, "  and w0, w0, 0xFF\n");
        } else if(tipo_origem == T_pFLU) {
            fprintf(s, "  fcvtzs w0, s0\n");
            fprintf(s, "  and w0, w0, 0xFF\n");
        } else if(tipo_origem == T_pDOBRO) {
            fprintf(s, "  fcvtzs w0, d0\n");
            fprintf(s, "  and w0, w0, 0xFF\n");
        }
    } else if(tipo_destino == T_pBOOL) {
        // qualquer valor != 0 vira 1(verdade), 0 permanece 0(falso)
        if(tipo_origem == T_pINT || tipo_origem == T_pLONGO || 
           tipo_origem == T_pCAR) {
            fprintf(s, "  cmp w0, 0\n");
            fprintf(s, "  cset w0, ne\n");
        } else if(tipo_origem == T_pFLU) {
            fprintf(s, "  fcmp s0, 0.0\n");
            fprintf(s, "  cset w0, ne\n");
        } else if(tipo_origem == T_pDOBRO) {
            fprintf(s, "  fcmp d0, 0.0\n");
            fprintf(s, "  cset w0, ne\n");
        }
    }
}

void gerar_globais(FILE* s) {
    if(global_cnt == 0) return;
    
    // primeiro, gera constantes(.rodata)
    int tem_constantes = 0;
    for(int i = 0; i < global_cnt; i++) {
        if(globais[i].eh_final) {
            tem_constantes = 1;
            break;
        }
    }
    if(tem_constantes) {
        fprintf(s, "\n.section .rodata\n");
        for(int i = 0; i < global_cnt; i++) {
            Variavel* var = &globais[i];
            if(!var->eh_final) continue;
            
            fprintf(s, ".align %d\n", var->alinhamento > 0 ? var->alinhamento : 3);
            fprintf(s, "global_%s:\n", var->nome);
            
            if(var->eh_array) {
                int total_bytes = var->bytes;
                if(var->tipo_base == T_pCAR && var->valor >= 0) {
                    const char* texto_valor = texs[var->valor].valor;
                    fprintf(s, "  .asciz \"%s\"\n", texto_valor);
                    int tam_texto = strlen(texto_valor) + 1;
                    if(tam_texto < total_bytes) {
                        fprintf(s, "  .space %d\n", total_bytes - tam_texto);
                    }
                } else fprintf(s, "  .space %d\n", total_bytes);
            } else if(var->eh_ponteiro) {
                if(var->valor >= 0) fprintf(s, "  .quad %s\n", texs[var->valor].nome);
                else fprintf(s, "  .quad 0\n");
            } else {
                switch(var->tipo_base) {
                    case T_pBYTE:
                    case T_pCAR:
                    case T_pBOOL:
                        fprintf(s, "  .byte %d\n", (int)var->valor);
                    break;
                    case T_pINT:
                        fprintf(s, "  .word %d\n", (int)var->valor);
                    break;
                    case T_pFLU:
                        fprintf(s, "  .float %f\n", (float)var->valor_f);
                    break;
                    case T_pDOBRO:
                        fprintf(s, "  .double %f\n", var->valor_f);
                    break;
                    case T_pLONGO:
                        fprintf(s, "  .quad %ld\n", var->valor);
                    break;
                    default:
                        fprintf(s, "  .space %d\n", var->bytes);
                }
            }
        }
    }
    // depois, gera variaveis mutaveis(.data)
    int tem_vars = 0;
    for(int i = 0; i < global_cnt; i++) {
        if(!globais[i].eh_final) {
            tem_vars = 1;
            break;
        }
    }
    if(tem_vars) {
        fprintf(s, "\n.section .data\n");
        
        for(int i = 0; i < global_cnt; i++) {
            Variavel* var = &globais[i];
            if(var->eh_final) continue;
            
            fprintf(s, ".align %d\n", var->alinhamento > 0 ? var->alinhamento : 3);
            fprintf(s, "global_%s:\n", var->nome);
            
            if(var->eh_array) {
                int total_bytes = var->bytes;
                if(var->tipo_base == T_pCAR && var->valor >= 0) {
                    const char* texto_valor = texs[var->valor].valor;
                    fprintf(s, "  .asciz \"");
                    for(int j = 0; texto_valor[j] != '\0'; j++) {
                        unsigned char c = texto_valor[j];
                        switch(c) {
                            case '\n': fprintf(s, "\\n"); break;
                            case '\t': fprintf(s, "\\t"); break;
                            case '\r': fprintf(s, "\\r"); break;
                            case '\0': fprintf(s, "\\0"); break;
                            case '\\': fprintf(s, "\\\\"); break;
                            case '\"': fprintf(s, "\\\""); break;
                            case '\a': fprintf(s, "\\a"); break;
                            case '\b': fprintf(s, "\\b"); break;
                            case '\v': fprintf(s, "\\v"); break;
                            case '\f': fprintf(s, "\\f"); break;
                            default:
                                if(c >= 32 && c <= 126) {
                                    fputc(c, s);
                                } else {
                                    fprintf(s, "\\%03o", c);
                                }
                            break;
                        }
                    }
                    fprintf(s, "\"\n");
                    int tam_texto = strlen(texto_valor) + 1;
                    if(tam_texto < total_bytes) {
                        fprintf(s, "  .space %d\n", total_bytes - tam_texto);
                    }
                } else fprintf(s, "  .space %d\n", total_bytes);
            } else if(var->eh_ponteiro) {
                if(var->valor >= 0) fprintf(s, "  .quad %s\n", texs[var->valor].nome);
                else fprintf(s, "  .quad 0\n");
            } else {
                switch(var->tipo_base) {
                    case T_pBYTE:
                        fprintf(s, "  .word %d\n", (int)var->valor);
                    break;
                    case T_pCAR:
                    case T_pBOOL:
                        fprintf(s, "  .byte %d\n", (int)var->valor);
                    break;
                    case T_pINT:
                        fprintf(s, "  .word %d\n", (int)var->valor);
                    break;
                    case T_pFLU:
                        fprintf(s, "  .float %f\n", (float)var->valor_f);
                    break;
                    case T_pDOBRO:
                        fprintf(s, "  .double %f\n", var->valor_f);
                    break;
                    case T_pLONGO:
                        fprintf(s, "  .quad %ld\n", var->valor);
                    break;
                    default:
                        fprintf(s, "  .space %d\n", var->bytes);
                }
            }
        }
    }
}

void escrever_valor(FILE* s, TipoToken tipo) {
    if(tipo == T_pFLU) fprintf(s, "  bl _escrever_flu\n");
    else if(tipo == T_pCAR) fprintf(s, "  bl _escrever_car\n");
    else if(tipo == T_pBOOL) fprintf(s, "  bl _escrever_bool\n");
    else if(tipo == T_TEX || tipo == T_PONTEIRO) fprintf(s, "  bl _escrever_tex\n");
    else if(tipo == T_pLONGO) fprintf(s, "  bl _escrever_longo\n");
    else fprintf(s, "  bl _escrever_int\n");
}

void carregar_valor(FILE* s, Variavel* var) {
    if(var->escopo == -1) { // variável global
        fprintf(s, "  ldr x0, = global_%s\n", var->nome);
        
        if(var->eh_ponteiro) {
            // pra ponteiros globais, carrega o valor do ponteiro
            fprintf(s, "  ldr x0, [x0]\n");
        } else if(var->eh_array) {}
        else {
            // pra variaveis simples globais, carrega o valor
            switch(tam_tipo(var->tipo_base)) {
                case 1: 
                    fprintf(s, "  ldrb w0, [x0]\n"); 
                break;
                case 4: 
                    if(var->tipo_base == T_pFLU) fprintf(s, "  ldr s0, [x0]\n");
                    else fprintf(s, "  ldr w0, [x0]\n");
                break;
                case 8:
                    if(var->tipo_base == T_pDOBRO) fprintf(s, "  ldr d0, [x0]\n");
                    else fprintf(s, "  ldr x0, [x0]\n");
                break;
            }
        }
    } else {
        if(var->eh_ponteiro) fp_ldr(s, "x0", var->pos);
        else if(var->eh_array) fp_add_fp(s, "x0", var->pos);
        else {
            switch(tam_tipo(var->tipo_base)) {
                case 1: 
                fp_ldrb(s, "w0", var->pos);
                break;
                case 4: 
                if(var->tipo_base == T_pFLU) fp_ldr(s, "s0", var->pos);
                else fp_ldr(s, "w0", var->pos);
                break;
                case 8:
                if(var->tipo_base == T_pDOBRO) fp_ldr(s, "d0", var->pos);
                else fp_ldr(s, "x0", var->pos);
                break;
            }
        }
    }
}

void armazenar_valor(FILE* s, Variavel* var) {
    if(var->escopo == -1) { // variavel global
        fprintf(s, "  ldr x1, = global_%s\n", var->nome);
        
        if(var->eh_ponteiro) {
            // armazena o valor do ponteiro(x0) na variavel global
            fprintf(s, "  str x0, [x1]\n");
        } else if(var->eh_array) {
            fatal("[armazenar_valor] não é possível armazenar valor direto em array global");
        } else {
            // armazena o valor na variavel global
            switch(tam_tipo(var->tipo_base)) {
                case 1: 
                    fprintf(s, "  strb w0, [x1]\n"); 
                break;
                case 4: 
                    if(var->tipo_base == T_pFLU) fprintf(s, "  str s0, [x1]\n");
                    else fprintf(s, "  str w0, [x1]\n");
                break;
                case 8:
                    if(var->tipo_base == T_pDOBRO) fprintf(s, "  str d0, [x1]\n");
                    else fprintf(s, "  str x0, [x1]\n");
                break;
            }
        }
    } else { // variavel local
        if(var->eh_ponteiro) {
            fp_str(s, "x0", var->pos);
        } else if(var->eh_array) {
            fatal("[armazenar_valor] não é possível armazenar valor direto em array");
        } else {
            switch(tam_tipo(var->tipo_base)) {
                case 1: 
                    fp_strb(s, "w0", var->pos);
                break;
                case 4:
                    if(var->tipo_base == T_pFLU) fp_str(s, "s0", var->pos);
                    else fp_str(s, "x0", var->pos);  // x0 tem w0 zero extendido
                break;
                case 8:
                    if(var->tipo_base == T_pDOBRO) fp_str(s, "d0", var->pos);
                    else fp_str(s, "x0", var->pos);
                break;
            }
        }
    }
}

void carregar_const(FILE* s, int titulo) {
    Constante* c = &constantes[titulo];
    if(c->tipo == T_FLU) {
        fprintf(s, "  ldr x0, = const_%d\n", titulo);
        fprintf(s, "  ldr s0, [x0]\n");
    } else if(c->tipo == T_DOBRO) {
        fprintf(s, "  ldr x0, = const_%d\n", titulo);
        fprintf(s, "  ldr d0, [x0]\n");
    } else if(c->tipo == T_LONGO) {
        fprintf(s, "  ldr x0, = const_%d\n", titulo);
        fprintf(s, "  ldr x0, [x0]\n");
    } else if(c->tipo == T_INT) {
        fprintf(s, "  ldr x0, = const_%d\n", titulo);
        fprintf(s, "  ldr w0, [x0]\n");
    }
}

int add_const(TipoToken tipo, const char* lex, double d_val, long l_val) {
    for(int i = 0; i < const_cnt; i++) {
        if(tipo == T_FLU && constantes[i].tipo == T_FLU && fabs(constantes[i].d_val - d_val) < 1e-9)
            return constantes[i].titulo;
        if(tipo == T_DOBRO && constantes[i].tipo == T_DOBRO && fabs(constantes[i].d_val - d_val) < 1e-9)
            return constantes[i].titulo;
        if(tipo == T_INT && constantes[i].tipo == T_INT && constantes[i].l_val == l_val)
            return constantes[i].titulo;
        if(tipo == T_LONGO && constantes[i].tipo == T_LONGO && constantes[i].l_val == l_val)
            return constantes[i].titulo;
    }
    if(const_cnt >= MAX_CONST) {
        MAX_CONST += 2;
        Constante *temp = realloc(constantes, MAX_CONST * sizeof(Constante));
        if(!temp) {
            printf("[add_const]: Erro ao alocar memória, constantes %d\n", const_cnt);
            exit(1);
        }
        constantes = temp;
    }
    Constante* c = &constantes[const_cnt];
    c->tipo = tipo;
    strcpy(c->lex, lex);
    c->d_val = d_val;
    c->l_val = l_val;
    c->titulo = const_cnt;
    const_cnt++;
    return c->titulo;
}

int add_tex(const char* valor) {
    if(debug_o) printf("[add_tex]: Tentando adicionar texto: \"%s\" a lista\n", valor);
    
    for(int i = 0; i < tex_cnt; i++) {
        if(strcmp(texs[i].valor, valor) == 0) {
            if (debug_o) printf("[add_tex]: Texto já existe, id %d\n", i);
            return i;
        }
    }
    if(tex_cnt >= MAX_TEX) {
        MAX_TEX += 2;
        Tex *temp = realloc(texs, MAX_TEX * sizeof(Tex));
        if(temp == NULL) {
            printf("[gerar_texs]: Erro ao realocar memória, textos: %d\n", tex_cnt);
            free(texs);
            exit(1);
        }
        texs = temp;
    }
    Tex* tex = &texs[tex_cnt];
    tex->valor = malloc(MAX_TOK * sizeof(char));
    strcpy(tex->valor, valor);
    sprintf(tex->nome, ".tex_%d", tex_cnt);
    tex_cnt++;
    if(debug_o) printf("[add_tex]: Novo texto adicionado com id %d\n", tex_cnt - 1);
    return tex_cnt - 1;
}