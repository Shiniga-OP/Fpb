## sobre
essa foi uma tentativa de compilador que fiz para minha linguagem de alto nível, a implementação usa assembly aarch64 para traduzir código .fpb e assim usar **as** para compilar para .o, usando **ld** para linkagem e execução do binário compilado.

## problemas
por mais que a linguagem funcione por simplicidade, você pode notar alguns problemas de funções, ou operações matemáticas, isso se deve porque eu fui meio burro e não pesquisei sobre como implementar bibliotecas em binários. então as bibliotecas para imprimir são escritas em assembly e coladas no topo do arquivo do código .fpb no processo de tradução, eu sei, foi uma solução de gambiarra.

## sintaxe
a sintaxe é simples, você pode usar o básico como:

vazio inicio() {
 imprimir("codigo");
 funcao();
 fim();
}

int somar(int a, int b) {
 retornar a + b;
}
