cl65 -O -t c128 browser.c userial.c -o browser128.prg
del *.o
del *.o65
del /Q ..\client\c128\*.*
copy browser128.prg ..\client\c128\browser128.prg
copy c128-*.* ..\client\c128\