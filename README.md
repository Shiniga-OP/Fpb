# sobre CFF (Compilador Foca-do em FPB):

compilador de FPB escrito em C++ focado em legibilidade e facilidade.

## estado:
em desenvolvimento.

# sobre FPB (compilador):

## AVISO:
o gerenciamento de memória dos arrays locais finalmente foi consertado.

## atualização mais recente:
correção temporaria do uso de arrays.

## sobre
essa foi uma tentativa de compilador que fiz para minha linguagem de alto nível, a implementação usa assembly aarch64 para traduzir código .fpb e assim usar **as** para compilar para .o, usando **ld** para linkagem e execução do binário compilado.

## sintaxe
este é um código que testa maior parte dos recursos e margem de erros:

```Fpb
/*
comentarios
multi linhas
suportados
*/
#incluir "biblis/impressao.asm";
#incluir "biblis/texs.asm";
#incluir "biblis/mem.asm";

/* pré definição */
vazio testeAlteracoes(int s, int numero, car letra, bool flag);
vazio testeOperacoes();
vazio testeComparacoes();
vazio testeMemoria();
vazio testeLoops();
int textam(car* texto);
int texcar(car* texto, car alvo);
vazio subscar(car[] array, car alvo, car novo);
vazio memcp(car[] array, car* p, int tam);

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
    bool flag = 1;
    escrever("\nbooleano: ", flag);

    escrever("\n\nTestando função soma:\n");
    int s = somar(5, 7);
    escrever("\nsoma com retorno 5 + 7 = esperando 12, veio: ", s);
    testeAlteracoes(s, numero, letra, flag);
}

vazio testeAlteracoes(int s, int numero, car letra, bool flag) {
    s = 5 + 7;
    escrever("\nsoma comum 5 + 7 = esperando 12, veio: ", s);

    escrever("\n\nTestando atribuições:\n");
    numero = 100;
    letra = 'Z';
    flag = 0;
    escrever("\nnovo inteiro: ", numero);
    escrever("\nnovo caractere: ", letra);
    escrever("\nnovo booleano: ", flag, "\n");
    // teste operações:
    testeOperacoes();
    testeComparacoes();
    testeLoops();
    testeMemoria();
}

vazio testeOperacoes() {
    escrever("\n\nTeste de operações matematicas:\n");
    // testando ordem de precedencia:
    escrever("operação 5 + 5 * 5, esperado: 30, veio: ", 5 + 5 * 5, "\n");
    escrever("operação (5 + 5) * 5, esperado: 50, veio: ", (5 + 5) * 5, "\n");

    escrever(5 + 5, "\n"); // expressão direta
    escrever(somar(5, 5), "\n"); // retorno passado direto
    
    escrever("10 % 3 = ?, esperado: 1, recebido: ", 10 % 3, "\n");
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
    
    se(y >= 4 && x > 4) {
        escrever("y >= 4 && x > 4 é verdadeiro");
    } senao {
        escrever("y >= 4 && x > 4 é falso");
    }
}

vazio testeMemoria() {
    escrever("\nTeste de array:\n");
    car[] array = "texto";
    escrever("\nvalor do array: ", array);
    array[0] = 'X';
    escrever("\narray mudado no indice 0: ", array);

    escrever("\n\nTeste de ponteiro:\n");
    car* ponteiro = "exemplo de ponteiro";
    
    escrever("\nponteiro texto, valor: ", ponteiro, "\ntamamho em bytes: ", textam(ponteiro));
    
    int i = texcar(ponteiro, 't');

    se(i >= 0) escrever("\no ponteiro tem t no indice: ", i);
    senao escrever("\no ponteiro não tem t\n");
    
    escrever("\nTeste de manipulação da memoria:\n");
    car[] array = "exemplo";
    escrever("\nArray padrão: ", array);
    
    car* p = "XxXmplo maior";
    
    memcp(array, p, textam(p));
    escrever("\nArray copiado da memoria: ", array);
    
    subscar(array, 'X', 'e');
    
    escrever("\nArray usando subscar(array, 'X', 'e'): ", array);
    
    escrever("\nTeste de acesso a itens array:\n");
    car ca = array[0];
    escrever("item do indice 0 do array: ", ca, "\n");
    
    int[] num = { 0, 1, 2, 5 };
    escrever("array de inteiros: \n\n");
    por(int i = 0; i < 4; i = i + 1) {
        escrever("no indice: ", i, " valor: ", num[i], "\n");
    }
}

vazio testeLoops() {
    escrever("\n\nTeste de loops");
    escrever("\nEnquanto:");
    int i = 0;
    enq(i < 10) {
        escrever("\nvalor de i: ", i);
        i = i + 1;
    }
    escrever("\n\nPor:\n");
    por(int i = 0; i < 10; i = i + 1) {
        escrever("indice: ", i, "\n");
    }
}
```
## como compilar
para compilar, você deve usar:
```Bash
fpb ola
# ou
fpb ola -s /caminho/arquivo
```
não precisa da extensão, o compilador gera o binário com o nome específico automaticamente, por isso não a extensão.

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

m caso de erro, você poderá ver algo como:
```Bash
ola.fpb [ERRO] linha: 1 coluna: 1, próximo de ""
```

caso a biblioteca de impressao.asm não estiver no mesmo ambiente do compilador, ele soltará um aviso, mas compilará normalmente.
# otimizações:
1. reutilização de constantes.
2. reutilização de texs.
3. primeiros 8 parametros sendo passados por registradores.
# requisitos:
para o compilador funcionar, você precisa ter **ld** e **as** instalados na sua máquina para o binário ser gerado.

o compilador do gera binários válidos para a arquitetura ARM64 Linux (Android).

# extra:
as bibliotecas incluidas com **#incluir** não são linkadas, o assembly é colado ao final do arquivo intermediário ASM antes de ser compilado.

o compilador é auto suficiente, sem a necessidade de libc.so para binários **gerados pelo compilador**, o compilador em si, por ser escrito em C, ainda precisa de libc.so pra funcionar.

(f.sh é o shell de compilação que uso pra testar o compilador mais rápido).

use *fpb -ajuda* para visualizar todos comandos.
