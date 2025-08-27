#!/bin/bash

if [ -e tmp ]; then
files=$(ls tmp)
for f in $files
do
	ff=$(cat tmp/$f)
	mv -f $ff.bak $ff
done
rm -rf tmp
fi

cp  ./Obj/*.elf project.elf
cp  ./Lst/*.map project.map
cp  ./Obj/*.ihex project.hex

if [ -e parameter.bincfg ]; then
cp -f ./parameter.bincfg parameter.cfg
fi
if [ -e parameter_ui.setcfg ]; then
cp -f ./parameter_ui.setcfg parameter_ui.cfg
fi


[ -e ../sdk/chip/txw81x/loader_compress.bin ] && cp -f ../sdk/chip/txw81x/loader_compress.bin loader_compress.bin


cp parameter.bincfg parameter.cfg

BinScript.exe BinScript.BinScript
makecode.exe

#crc.exe crc.ini
#BinScript.exe BinScript_Bin2Hex.BinScript

