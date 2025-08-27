## sobre
essa foi uma tentativa de compilador que fiz para minha linguagem de alto nível, a implementação usa assembly aarch64 para traduzir código .fpb e assim usar **as** para compilar para .o, usando **ld** para linkagem e execução do binário compilado.

## problemas
por mais que a linguagem funcione por simplicidade, você pode notar alguns problemas de funções, ou operações matemáticas, isso se deve porque eu fui meio burro e não pesquisei sobre como implementar bibliotecas em binários. então as bibliotecas para imprimir são escritas em assembly e coladas no topo do arquivo do código .fpb no processo de tradução, eu sei, foi uma solução de gambiarra.

## sintaxe
a sintaxe é simples, é difícil achar algo estável, mas este é o teste que usei para sintaxe:

```Fpb
dobro calcular(dobro a, dobro b, flu pi) {
    dobro res = a * b + pi;
    retornar res;
}

int soma(int a, int b) {
escrever("\nvalor a: ", a);
escrever("\nvalor b: ", b);
   int res = a + b;
   retornar res;
}

vazio inicio() {
    car letra = 'A';
    int numero = 42;
    flu pi = 3.1415;
    bool flag = 1;
    dobro grande = 123456789.123456;

    escrever("testando tipos básicos:\n");
    escrever("\nchar: ", letra);
    escrever("\nint: ", numero);
    escrever("\nfloat: ", pi);
    escrever("\nbool: ", flag);
    escrever("\ndouble: ", grande);
    
    escrever("\n\nTestando função soma:");
    int s = soma(5, 7);
    escrever("soma com retorno 5 + 7 = esperando 12, veio: ", s);
    s = 5 + 7;
    escrever("\nsoma comum 5 + 7 = esperando 12, veio: ", s);
    
    escrever("\nTestando atribuições:");
    numero = 100;
    letra = 'Z';
    flag = 0;
    escrever("\nnovo int: ", numero);
    escrever("\nnovo char: ", letra);
    escrever("\nnovo bool: ", flag);
    
    fim();
}
```
## como compilar
para compilar, você deve usar
```Bash
fpb ola
```
não precisa da extensão, o compilador gera o binário com o nome específico automaticamente, por isso não a extensão.

## configuração extra
caso você queira o código assembly intermediário, utilize:
```Bash
fpb ola -asm
```
assim o arquivo .s será gerado sem ser apagado.

## requisitos:
para o compilador funcionar, você precisa ter **ld** e **as** instalados na sua máquina para o binário ser gerado.
