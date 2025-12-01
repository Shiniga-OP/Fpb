COM=$1
ARQ=$2
M1=$3
M2=$4
clang /storage/emulated/0/pacotes/fpb/$COM.c -o $COM
cp -r /storage/emulated/0/pacotes/fpb/biblis ./
cp -r /storage/emulated/0/pacotes/fpb/tmp ./
cp -r /storage/emulated/0/pacotes/fpb/util ./
cp /storage/emulated/0/pacotes/fpb/$ARQ.fpb ./
./$COM  $ARQ $M1 $M2
cp $ARQ.asm /storage/emulated/0/pacotes/fpb/
cp f.sh /storage/emulated/0/pacotes/fpb/
time ./$ARQ
cp fpb /data/data/com.termux/files/usr/bin
cp -rf biblis /data/data/com.termux/files/usr/bin
