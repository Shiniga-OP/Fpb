CODIGO=$1
CODIGO2=$2
OTIMI=$3
if [ -d "$CASA/pacotes/fpb" ]; then
    rm -rf fpb  # remove se existir(arquivo ou pasta)
    cp -rf $CASA/pacotes/fpb .
fi
cp -rf $CASA/pacotes/fpb .
cd fpb
clang++ fpbc.cpp -o fpbc $OTIMI
cp $HOME/fpbc.sh $CASA/pacotes/fpb
cp testes/$CODIGO.fpb .
echo tempo de compilação
time ./fpbc $CODIGO.fpb -s $CODIGO2 -asm
