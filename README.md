## sobre
essa foi uma tentativa de compilador que fiz para minha linguagem de alto nível, a implementação usa assembly aarch64 para traduzir código .fpb e assim usar **as** para compilar para .o, usando **ld** para linkagem e execução do binário compilado.

## sintaxe
a sintaxe é simples, é difícil achar algo estável, mas este é o teste que usei para sintaxe:

```Fpb
/*
comentarios
multi linhas
suportados
*/
#incluir "biblis/teste.asm";

/* incluí uma biblioteca asm */

/* pré definição */
vazio testeAlteracoes(int s, int numero, car letra, bool flag);
vazio testeOperacoes();
vazio teste();

int somar(int a, int b) {
   escrever("\nvalor a: ", a);
   escrever("\nvalor b: ", b);
   retornar a + b;
}

vazio inicio() {
    escrever("testando tipos básicos:\n");
    car letra = 'A';
    escrever("\ncaractere: ", letra);
    int numero = 42;
    escrever("\ninteiro: ", numero);
    flu pi = 3.14;
    escrever("\nflutuante: ", pi);
    bool flag = 1;
    escrever("\nbooleano: ", flag);
    
    escrever("\nTestando função soma:");
    int s = somar(5, 7);
    escrever("\nsoma com retorno 5 + 7 = esperando 12, veio: ", s);
    testeAlteracoes(s, numero, letra, flag);
    fim();
}

vazio testeAlteracoes(int s, int numero, car letra, bool flag) {
    s = 5 + 7;
    escrever("\nsoma comum 5 + 7 = esperando 12, veio: ", s);
    
    escrever("\nTestando atribuições:");
    numero = 100;
    letra = 'Z';
    flag = 0;
    escrever("\nnovo inteiro: ", numero);
    escrever("\nnovo caractere: ", letra);
    escrever("\nnovo booleano: ", flag, "\n");
    // teste operações:
    testeOperacoes();
    escrever("\nTeste de biblioteca:\n");
    // teste da biblioteca
    teste();
}

vazio testeOperacoes() {
    escrever("\nTeste de operações matematicas:\n");
    // testando ordem de precedencia:
    int x = 5 + 5 * 5;
    escrever("operação 5 + 5 * 5, esperado: 30, veio: ", x, "\n");
    x = (5 + 5) * 5;
    escrever("operação (5 + 5) * 5, esperado: 50, veio: ", x, "\n");
}
```
## como compilar
para compilar, você deve usar:
```Bash
fpb ola
```
não precisa da extensão, o compilador gera o binário com o nome específico automaticamente, por isso não a extensão.

## configuração extra
caso você queira o código assembly intermediário, utilize:
```Bash
fpb ola -asm
```
assim o arquivo .asm será gerado sem ser apagado.

para ver a versão:
```Bash
fpb -v
```

em caso de erro, você poderá ver algo como:
```Bash
ola.fpb [ERRO] linha: 1 coluna: 1, próximo de ""
```

caso a biblioteca de impressao.asm não estiver no mesmo ambiente do compilador, ele soltará um aviso, mas compilará normalmente.
## requisitos:
para o compilador funcionar, você precisa ter **ld** e **as** instalados na sua máquina para o binário ser gerado.

o compilador do gera binários válidos para a arquitetura ARM64 Linux (Android).

## extra:
as bibliotecas incluidas com **#incluir** não são linkadas, o assembly é colado ao final do arquivo intermediário ASM antes de ser compilado.

o compilador é auto suficiente, sem a necessidade de libc.so para binários **gerados pelo compilador**, o compilador em si, por ser escrito em C, ainda precisa de libc.so pra funcionar.
