cd /d %~dp0

set dirname=%1
set dirname2=%dirname:~0,-4%


del /q/f APP.bin


copy %dirname% program.bin
copy %dirname% APP.bin


if exist .\script\xz.exe (
BinScript.exe real_code.BinScript
.\script\xz.exe -kfz -0 realcode.bin
.\script\compress.exe APP.bin loader_compress.bin compress.bin APP_compress.bin
copy APP_compress.bin %dirname2%_compress.bin
)

set dirname=%dirname:~0,-4%
set dirname=%dirname%_%date:~0,4%%date:~5,2%%date:~8,2%%time:~0,2%%time:~3,2%%time:~6,2%
if not exist bakup md bakup
if not exist bakup\%dirname% md bakup\%dirname%
copy Lst\* bakup\%dirname%\
copy project.map bakup\%dirname%\
