ARQ=$1
clang /storage/emulated/0/pacotes/fpb/fpb.c -o fpb
cp -r /storage/emulated/0/pacotes/fpb/biblis ./
cp -r /storage/emulated/0/pacotes/fpb/tmp ./
cp /storage/emulated/0/pacotes/fpb/$ARQ.fpb ./
./fpb $ARQ -asm
rm fpb
cp $ARQ.asm /storage/emulated/0/pacotes/fpb/
cp f.sh /storage/emulated/0/pacotes/fpb/
time ./$ARQ
rm $ARQ
