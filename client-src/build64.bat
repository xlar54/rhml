cl65  -O -t c64 browser.c userial.c -o browser64.prg
del *.o
del *.o65
del /Q ..\client\c64\*.*
copy browser64.prg ..\client\c64\browser64.prg
copy c64-*.* ..\client\c64\