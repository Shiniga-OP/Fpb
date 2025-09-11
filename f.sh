ARQ=$1
clang++ /storage/emulated/0/pacotes/fpb/cff.cpp -o cff
cp -r /storage/emulated/0/pacotes/fpb/biblis ./
cp -r /storage/emulated/0/pacotes/fpb/tmp ./
cp /storage/emulated/0/pacotes/fpb/$ARQ.fpb ./
./cff $ARQ -asm
cp $ARQ.asm /storage/emulated/0/pacotes/fpb/
cp f.sh /storage/emulated/0/pacotes/fpb/
time ./$ARQ
