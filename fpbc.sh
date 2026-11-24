#!/data/data/com.termux/files/usr/bin/bash
set -e
CODIGO=$1
SAIDA=$2
OTIMI=$3
DESTINO=/data/data/com.termux/files/usr/bin/fpb

rm -rf fpb
cp -rf $CASA/pacotes/fpb .
cd fpb
clang++ fpbc.cpp -o fpbc $OTIMI
cp $HOME/fpbc.sh $CASA/pacotes/fpb
cp testes/$CODIGO.fpb .

mkdir -p $DESTINO
mv -f fpbc $DESTINO/
cp -rf biblis $DESTINO/
export FPB_RAIZ=$DESTINO/biblis
echo tempo de compilação
time $DESTINO/fpbc $CODIGO.fpb -s $SAIDA -asm
