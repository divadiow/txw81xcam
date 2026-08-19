$ErrorActionPreference='Stop'
$p=0..10|%{"diag_input/v2first.{0:D2}"-f$_}
$p+=1..4|%{"diag_input/v2.raw.b64.{0:D2}"-f$_}
$s=($p|%{(gc $_ -Raw).Trim()})-join''
$b=[Convert]::FromBase64String($s)
$h=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($b)).ToLowerInvariant()
if($s.Length-ne 9100-or$b.Length-ne 6824-or$h-ne'5b2fd2ab3a1bbb05c6bcbe9ff26a04538639695354ff230b5238aadebbfa01ee'){throw "image mismatch: $($s.Length) $($b.Length) $h"}
[IO.File]::WriteAllBytes('project/txw81x_fpv.bin',$b)
cp project/parameter.bincfg project/parameter.cfg -Force
@'
reset(ibuf);
reset(obuf);
read(bin, txw81x_fpv.bin);
def16_at(v_param_len, 180);
read(bin, parameter.cfg);
memmode(ibuf, 32, lit);
remap(210, 0, v_param_len);
setbuf16(0, v_param_len);
outrange(0, v_param_len, 10);
write(bin, param.bin);
'@|sc project/BinScript.BinScript -Encoding ascii
pushd project
& .\BinScript.exe .\BinScript.BinScript
if($LASTEXITCODE){throw "BinScript rc=$LASTEXITCODE"}
& .\makecode.exe
if($LASTEXITCODE){throw "makecode rc=$LASTEXITCODE"}
popd
$a=gi project/APP.bin
if($a.Length-lt 8192-or$a.Length-gt 1048576){throw "APP.bin size $($a.Length)"}
mkdir essential -Force|Out-Null
cp project/APP.bin essential/TXW817_810_DIAG_FLASH.bin
