#include <stdio.h>

typedef struct {
    float pesos[8];
    float bias;
    float taxa_aprendizado;
} Perceptron;

static Perceptron p;

int degrau(float x) {
    return x > 0 ? 1 : 0;
}

int prever(float entrada[]) {
    float soma = p.bias;
    for(int i = 0; i < 8; i++) soma += p.pesos[i] * entrada[i];
    return degrau(soma); // 1 = derramar, 0 = manter
}

void treinar(float entrada[], int saidaEsperada) {
    int saida = prever(entrada);
    int erro = saidaEsperada - saida;
    // ajusta pesos
    for(int i = 0; i < 8; i++) p.pesos[i] += p.taxa_aprendizado * erro * entrada[i];
    // ajusta o bias
    p.bias += p.taxa_aprendizado * erro;
}
/*
* dados do registrador pra IA:
* [0] ultimo_uso(normalizado)
* [1] eh_preservado(0 ou 1)
* [2] eh_sujo(0 ou 1) 
* [3] eh_flutuante(0 ou 1)
* [4] eh_constante(0 ou 1)
* [5] proximo_uso_estimado(0-1)
* [6] custo_derramamento(0-1)
* [7] prioridade_tipo(0-1)
*/

// cada array tem 8 características
float dados[][8] = {
    // registradores ruins para derramar(deve derramar = 1)
    {0.9, 0, 1, 0, 0, 0.1, 0.2, 0.3}, // ultimo uso alto, não preservado, sujo
    {0.8, 0, 0, 0, 0, 0.2, 0.1, 0.4}, // ultimo uso alto, não preservado
    {0.7, 0, 1, 1, 0, 0.3, 0.4, 0.2}, // sujo, flutuante, baixa prioridade
    {0.6, 0, 0, 0, 1, 0.4, 0.3, 0.1}, // constante, próximo uso baixo
    
    // registradores bons para manter(deve manter = 0)
    {0.1, 1, 0, 1, 1, 0.9, 0.8, 0.7}, // preservado, constante, alto próximo uso
    {0.2, 1, 0, 0, 0, 0.8, 0.7, 0.6}, // preservado, limpo
    {0.3, 0, 0, 1, 0, 0.7, 0.6, 0.8}, // flutuante, alto próximo uso
    {0.4, 1, 0, 0, 1, 0.6, 0.5, 0.9} // preservado, constante, alta prioridade
};
int saidas_esperadas[] = {1, 1, 1, 1, 0, 0, 0, 0}; 

int main() {
    // inicializa a IA:
    for(int i = 0; i < 8; i++) p.pesos[i] = 0;
    p.bias = 0;
    p.taxa_aprendizado = 0.1f;
    // treinamento:
    for(int epoca = 0; epoca < 500; epoca++) {
        for(int i = 0; i < 8; i++) treinar(dados[i], saidas_esperadas[i]);
    }
    // teste de treino:
    printf("\n=== Teste do perceptron ===\n");
    for(int i = 0; i < 8; i++) {
        int decisao = prever(dados[i]);
        printf("exemplo %d: Esperado %d -> Decidiu %d [%s]\n", 
        i, saidas_esperadas[i], decisao, 
        (decisao == saidas_esperadas[i]) ? "✓" : "✗");
    }
    // testa com um novo caso:
    float novo_reg[] = {0.85, 0, 1, 0, 0, 0.15, 0.25, 0.35};
    printf("\nnovo registro: Decisão = %d(deve ser 1)\n", prever(novo_reg));
    
    return 0;
}