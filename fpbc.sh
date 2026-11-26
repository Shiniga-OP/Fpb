#!/data/data/com.termux/files/usr/bin/bash
set -e
CODIGO=$1
SAIDA=$2
TRAZER=$3
DESTINO=/data/data/com.termux/files/usr/bin/fpb

rm -rf fpb
tar -C "$CASA/pacotes" --exclude='fpb/.git' -cf - fpb | tar -xf -
cd fpb

if [ "$TRAZER" != "1" ]; then
  clang++ fpbc.cpp -o fpbc
fi

cp $HOME/fpbc.sh $CASA/pacotes/fpb
cp testes/$CODIGO.fpb .

mkdir -p $DESTINO
if [ -f fpbc ]; then
  mv -f fpbc $DESTINO/
fi
cp -rf biblis $DESTINO/
export FPB_RAIZ=$DESTINO/biblis
echo tempo de compilação
time $DESTINO/fpbc $CODIGO.fpb -s $SAIDA -asm
