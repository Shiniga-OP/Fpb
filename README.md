# FPB (Fácil Programação Baixo nivel)

## estado:
em desenvolvimento.

## sintaxe
este é um código que testa maior parte dos recursos e margem de erros:

## descrição:
FPB é uma linguagem de programação de alto nivel, com compatibilidade direta com Assembly ARM64.

```Fpb
/*
comentarios
multi linhas
suportados
*/
#incluir "biblis/impressao.asm";
#incluir "biblis/texs.asm";
#incluir "biblis/mem.asm";
#incluir "biblis/sistema.asm";

// macro:
#def TAM 14;

// variaveis globais:
#global inicio();
#global int varGlobal = 10;

// estrutura de dados:
#espaco Pessoa {
    car nome;
    int idade;
}

/* pré definição */
vazio testeAlteracoes(int s, int numero, car letra, bool flag);
vazio testeOperacoes();
vazio testeComparacoes();
vazio testeMemoria();
vazio testeLoops();
vazio testeMatrizes();
vazio testeEspaco();

int textam(car* texto);
int texcar(car* texto, car alvo);
int texcmp(car* x, car* y);

vazio subscar(car[] array, car alvo, car novo);
vazio memcp(car[] array, car* p, int tam);

longo obter_tempo_milis();

int somar(int a, int b) {
   escrever("valor a: ", a, "\n");
   escrever("valor b: ", b, "\n");
   retorne a + b;
}

vazio inicio() {
    escrever("Testando tipos básicos:\n");
    car letra = 'A';
    escrever("\ncaractere: ", letra);
    int numero = 42;
    escrever("\ninteiro: ", numero);
    flu pi = 3.14;
    escrever("\nflutuante: ", pi);
    bool marca = verdade;
    escrever("\nbooleano: ", marca);
    longo numLongo = obter_tempo_milis();
    escrever("\nlongo: ", numLongo);
    byte byteBranco = 0xFF;
    escrever("\nbyte: ", byteBranco, " (0xFF)");

    escrever("\n\nTestando função soma:\n");
    int s = somar(5, 7);
    escrever("\nsoma com retorno 5 + 7 = esperando 12, veio: ", s);
    final int varFinal = 1;
    escrever("\n\nVariavel final inteira:\n");
    escrever(varFinal, "\n");
    testeAlteracoes(s, numero, letra, marca, numLongo);
}

vazio testeAlteracoes(int s, int numero, car letra, bool marca, longo numLongo) {
    s = 5 + 7;
    escrever("\nsoma comum 5 + 7 = esperando 12, veio: ", s);

    escrever("\n\nTestando atribuições:\n");
    numero = 100;
    letra = 'Z';
    marca = falso;
    numLongo = 1763400788119L;
    escrever("\nnovo inteiro: ", numero);
    escrever("\nnovo caractere: ", letra);
    escrever("\nnovo booleano: ", marca);
    escrever("\nnovo longo: ", numLongo, "\n");
    escrever("\nTeste de positivo e negativo:\n\n");
    int x = 1;
    escrever("inteiro positivo: ", x, "\n");
    x = -1;
    escrever("inteiro negativo: ", x, "\n");
    flu y = 1.1;
    escrever("flutuante positivo: ", y, "\n");
    y = -1.1;
    escrever("flutuante negativo: ", y, "\n");
    escrever("\nTeste de conversão:\n\n");
    escrever("(car)65 = ", (car)65);
    // teste operações:
    testeOperacoes();
    testeComparacoes();
    testeLoops();
    testeMemoria();
    testeMatrizes();
    testeEspaco();
}

vazio testeOperacoes() {
    escrever("\n\nTeste de operações matematicas:\n\n");
    // testando ordem de precedencia:
    escrever("operação 5 + 5 * 5, esperado: 30, veio: ", 5 + 5 * 5, "\n");
    escrever("operação (5 + 5) * 5, esperado: 50, veio: ", (5 + 5) * 5, "\n");

    escrever(5 + 5, "\n"); // expressão direta
    escrever(somar(5, 5), "\n"); // retorno passado direto
    
    escrever("10 % 3 = ?, esperado: 1, recebido: ", 10 % 3, "\n");
    escrever("\n\nTeste de operações entre tipos:\n\n");
    int y = 3;
    flu x = 0.5f; // pode ter sufixo também
    escrever("x: ", x, " * y: ", y, ", resultado: ", x * y, "\n");
    escrever("10 << 2, resultado: ", 10 << 2, "\n");
    escrever("10 >> 2, resultado: ", 10 >> 2, "\n");
    escrever("124 & 15, resultado: ", 124 & 15, "\n");
    escrever("124 & 0xF, resultado: ", 124 & 0xF, "\n");
    escrever("1 | 2, resultado: ", 1 | 2, "\n");
}

vazio testeComparacoes() {
    escrever("\nTeste comparações:\n\n");
    int x = 4;

    se(x > 5) {
        escrever("x é maior que 5\n");
    } senao se(x >= 5) {
        escrever("x é maior ou igual a 5\n");
    } senao escrever("x não é maior nem igual a 5\n");
    
    int y = 5;
    
    se(y >= 4 && x > 4) escrever("y >= 4 && x > 4 é verdadeiro\n");
    senao escrever("y >= 4 && x > 4 é falso\n");
    
    se(y == x || x > 3) {
        escrever("y == x || x > 3 é verdadeiro\n");
    } senao {
        escrever("y == x || x > 3 é falso\n");
    }
    escrever("\nComparação com textos:\n\n");
    car* t1 = "texto 1";
    car* t2 = "texto 2";
    escrever(t1, "\n", t2, "\n");
    se(texcmp(t1, t2) == 1) {
        escrever("texto 1 é igual a texto 2\n");
    }
    se(texcmp(t1, t2) == 0) {
        escrever("texto 1 não é igual a texto 2\n");
    }
    se(texcmp(t1, t1) == 1) {
        escrever("o primeiro texto é texto 1");
    } senao {
        escrever("o primeiro texto não é texto 1");
    }
}

vazio testeMemoria() {
    escrever("\nTeste de array:\n");
    car[] array = "texto";
    escrever("\nvalor do array: ", array);
    array[0] = 'X';
    escrever("\narray mudado no indice 0: ", array);
    
    escrever("\n\nTeste de ponteiro:\n\n");
    int x1 = 10;
    escrever("int x1 = 10; int* p1 = @x1;\n");
    int* p1 = @x1;
    escrever("p1 = p1 + 5; x1 = ?\n");
    p1 = p1 + 5;
    escrever("x1 = ", x1, "\n");
    
    car* ponteiro = "exemplo de ponteiro";
    
    escrever("\nponteiro texto, valor: ", ponteiro, "\ntamamho em bytes: ", textam(ponteiro));
    
    int i = texcar(ponteiro, 't');

    se(i >= 0) escrever("\no ponteiro tem t no indice: ", i);
    senao escrever("\no ponteiro não tem t\n");
    
    escrever("\nTeste de manipulação da memoria:\n");
    
    car* p = "XxXmplo maior";
    
    car[TAM] array2 = "exemplo"; // array com tamanho do macro
    escrever("\nArray padrão: ", array2);
    
    memcp(array2, p, textam(p));
    escrever("\nArray copiado da memoria: ", array2);
    
    subscar(array2, 'X', 'e');
    
    escrever("\nArray usando subscar(array2, 'X', 'e'): ", array2);
    
    escrever("\nTeste de acesso a itens array:\n");
    car ca = array2[0];
    escrever("item do indice 0 do array: ", ca, "\n");
    
    int[] num = { 0, 1, 2, 5 };
    escrever("\nArray de inteiros: \n\n");
    por(int j = 0; j < 4; j++) {
        escrever("no indice: ", j, " valor: ", num[j], "\n");
    }
    flu[] flutuante = { 0.2, 1.5, 5.1, 5.1 };
    escrever("\nArray de flutuantes: \n\n");
    por(int j = 0; j < 4; j++) {
        escrever("no indice: ", j, " valor: ", flutuante[j], "\n");
    }
}

vazio testeLoops() {
    escrever("\n\nTeste de loops:");
    escrever("\nEnquanto:");
    int i = 0;
    enq(i < 10) {
        escrever("\nvalor de i: ", i);
        i++;
    }
    int x = 0;
    escrever("\n\nTeste de parada do loop (deve parar em 5)\n");
    enq(x < 10) {
        escrever(x, "\n");
        se(x == 5) {
            escrever("parando\n");
            pare;
        }
        x++;
    }
    escrever("\nLoop Por:\n");
    por(int i = 0; i < 10; i++) {
        escrever("indice: ", i, "\n");
    }
}

vazio testeMatrizes() {
    escrever("\n\nTeste de matrizes:\n\n");
    int[][] m2 = { {0, 1, 4}, {4, 1, 0} };
    escrever("matriz 2D int m2[0][1]: ", m2[0][1], "\n");
    flu[][] m2f = { {3f, 5f, 7f}, {2f, 9f, 10f} };
    escrever("matriz 2D flu m2f[0][1]: ", m2f[0][1], "\n");
}

vazio testeEspaco() {
    escrever("\nTeste de estrutura de dados (#espaco):\n\n");
    Pessoa p;
    p.nome = 'A';
    p.idade = 1991;
    
    Pessoa p2;
    p2.nome = 'B';
    p2.idade = 1992;
    
    escrever("Pessoa 1:\nNome: ", p.nome, "\nIdade: ", p.idade);
    escrever("\nPessoa 2:\nNome: ", p2.nome, "\nIdade: ", p2.idade, "\n");
}
```
## info extra:
ponteiros são automaticamente dereferenciados. Logo, qualquer operação com ponteiros é uma operação do valor.
para capturar o *endereço* do ponteiro, se usa o operador '@' ANTES do identificador:
```Fpb
int* p = @variavel;
```
isso cria um ponteiro diretamente com o endereço.

mas caso você queira alterar o endereço do ponteiron *diretamente* e não apenas apontar para uma varíavel local, você pode usar por exemplo:
```Fpb
int* p;
@p = 0x000000;
```

funções globais NÃO SÃO APAGADAS, mesmo se não forem usadas no mesmo código com a otimização nivel 1.
## documentação (beta)
você pode encontrar mais explicações em doc.txt

## como compilar
para compilar, você deve usar:
```Bash
fpb ola
# ou
fpb ola -s /caminho/arquivo
```
não precisa da extensão, o compilador gera o binário com o nome específico automaticamente, mas agora pode passar a extensão se quiser.

# configuração extra
caso você queira o código assembly intermediário, utilize:
```Bash
fpb ola -asm
# ou
fpb ola -s ola -asm
```
assim o arquivo .asm será gerado sem ser apagado.

para ver a versão:
```Bash
fpb -v
```

as configurações:
```Bash
fpb -c
```

em caso de erro, você poderá ver algo como:
```Bash
ola.fpb [ERRO] linha: 1 coluna: 1, próximo de ""
```

caso a biblioteca de impressao.asm não estiver no mesmo ambiente do compilador, ele soltará um aviso, mas compilará normalmente.
# otimizações:
1. reutilização de constantes.
2. reutilização de texs.
3. primeiros 8 parametros sendo passados por registradores.
4. usando a marcação -O1, será ativado a eliminação de funções não usadas e labels, + reorganização de sintaxe. (beta)
5. a marcação -O2 otimiza ainda mais o código juntando textos chamados para serem imprimidos em sequencia, incluindo a eliminação de código morto da O1. (beta)
# requisitos:
para o compilador funcionar, você precisa ter **ld** e **as** instalados na sua máquina para o binário ser gerado.

o compilador do gera binários válidos para a arquitetura ARM64 Linux (Android).

# extra:
parâmatros sempre são mutáveis, nunca finais.

as bibliotecas incluidas com **#incluir** não são linkadas, o assembly é colado ao final do arquivo intermediário ASM antes de ser compilado.

o compilador é auto suficiente, sem a necessidade de libc.so para binários **gerados pelo compilador**, o compilador em si, por ser escrito em C, ainda precisa de libc.so pra funcionar.

(f.sh é o shell de compilação que uso pra testar o compilador mais rápido).

use *fpb -ajuda* para visualizar todos comandos.
