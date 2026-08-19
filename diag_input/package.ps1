$ErrorActionPreference = 'Stop'

$chunkPaths = @(
    'diag_input/v2.raw.b64.00.fullcopy',
    'diag_input/v2.raw.b64.01',
    'diag_input/v2.raw.b64.02',
    'diag_input/v2.raw.b64.03',
    'diag_input/v2.raw.b64.04'
)

$base64 = (($chunkPaths | ForEach-Object { (Get-Content $_ -Raw).Trim() }) -join '')
$linked = [Convert]::FromBase64String($base64)
$linkedHash = [Convert]::ToHexString(
    [Security.Cryptography.SHA256]::HashData($linked)
).ToLowerInvariant()

if ($base64.Length -ne 9100 -or $linked.Length -ne 6824 -or
    $linkedHash -ne '5b2fd2ab3a1bbb05c6bcbe9ff26a04538639695354ff230b5238aadebbfa01ee') {
    throw "Linked image mismatch: base64=$($base64.Length), bytes=$($linked.Length), sha256=$linkedHash"
}

$linkedText = [Text.Encoding]::ASCII.GetString($linked)
foreach ($marker in @('TXWDIAG READY', 'AT+DIAG=EXEC,', 'TXWDIAG ROMGO')) {
    if (-not $linkedText.Contains($marker)) {
        throw "Linked image is missing marker: $marker"
    }
}

[IO.File]::WriteAllBytes('project/txw81x_fpv.bin', $linked)
Copy-Item 'project/parameter.bincfg' 'project/parameter.cfg' -Force

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
'@ | Set-Content 'project/BinScript.BinScript' -Encoding ascii

$iniPath = 'project/makecode.ini'
$ini = Get-Content $iniPath -Raw
$ini = [regex]::Replace($ini, '(?mi)^\s*CodeAddrOffset\s*=.*$', 'CodeAddrOffset=1000')
$ini = [regex]::Replace($ini, '(?mi)^\s*Post_Script\s*=.*$', 'Post_Script=')
[IO.File]::WriteAllText((Resolve-Path $iniPath), $ini, [Text.Encoding]::ASCII)

Remove-Item 'project/APP.bin', 'project/program.bin' -Force -ErrorAction SilentlyContinue
Get-ChildItem 'project' -File -Filter 'txw81x_fpv_v*.bin' -ErrorAction SilentlyContinue |
    Remove-Item -Force -ErrorAction SilentlyContinue

Push-Location 'project'
try {
    & .\BinScript.exe .\BinScript.BinScript *> binscript.log
    $binScriptRc = $LASTEXITCODE
    Get-Content binscript.log
    if ($binScriptRc -ne 0) {
        throw "BinScript failed with exit code $binScriptRc"
    }

    $param = Get-Item 'param.bin'
    Write-Host "Generated param.bin: $($param.Length) bytes"

    & .\makecode.exe *> makecode.log
    $makeCodeRc = $LASTEXITCODE
    Get-Content makecode.log
    if ($makeCodeRc -ne 0) {
        Get-ChildItem -File | Sort-Object Name | Format-Table Name, Length -AutoSize
        throw "makecode failed with exit code $makeCodeRc"
    }
}
finally {
    Pop-Location
}

$candidates = @(
    Get-ChildItem 'project' -File -Filter 'txw81x_fpv_v*.bin' |
        Where-Object { $_.Name -notlike '*_compress.bin' } |
        Sort-Object LastWriteTimeUtc -Descending
)

if ($candidates.Count -eq 0) {
    Get-ChildItem 'project' -File | Sort-Object Name | Format-Table Name, Length -AutoSize
    throw 'makecode succeeded but produced no packaged firmware file'
}

$packaged = $candidates[0]
if ($packaged.Length -lt 8192 -or $packaged.Length -gt 1048576) {
    throw "Packaged firmware has implausible size: $($packaged.Length) bytes"
}

New-Item -ItemType Directory -Path 'essential' -Force | Out-Null
Copy-Item $packaged.FullName 'essential/TXW817_810_DIAG_FLASH.bin' -Force
Write-Host "Packaged firmware: $($packaged.Name), $($packaged.Length) bytes"
