#!/bin/sh

mv build/bin/*.exe build

mkdir build/lib/aspell-0.60
mkdir build/lib/enchant
mkdir build/share/enchant

cp /local/bin/libaspell-15.dll build
cp /local/bin/libenchant-1.dll build
cp /local/bin/libgnurx-0.dll build
cp /local/bin/libpcre-0.dll build
cp /local/bin/libxml2-2.dll build

strip --strip-debug build/*.dll
strip --strip-debug build/*.exe
strip --strip-debug build/lib/bluefish*/*.dll

cp /local/lib/enchant/libenchant_aspell.dll build/lib/enchant
cp /local/lib/aspell-0.60/*.cmap build/lib/aspell-0.60
cp /local/lib/aspell-0.60/*.cset build/lib/aspell-0.60
cp /local/lib/aspell-0.60/*.amf build/lib/aspell-0.60
cp /local/lib/aspell-0.60/*.kbd build/lib/aspell-0.60

cp /local/share/enchant/enchant.ordering ./build/share/enchant
