clang /storage/emulated/0/pacotes/fpb/fpb.c -o fpb
cp -r /storage/emulated/0/pacotes/fpb/biblis ./
cp -r /storage/emulated/0/pacotes/fpb/tmp ./
cp /storage/emulated/0/pacotes/fpb/ola.fpb ./
./fpb ola -asm
rm fpb
cp ola.asm /storage/emulated/0/pacotes/fpb/
cp f.sh /storage/emulated/0/pacotes/fpb/
./ola
rm ola
