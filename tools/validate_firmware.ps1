# Native Windows companion to validate.sh; run after ESP-IDF export.ps1.
$ErrorActionPreference = 'Stop'
$repoPath = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$pythonPath = Join-Path $env:IDF_PYTHON_ENV_PATH 'Scripts/python.exe'
$idfScript = Join-Path $env:IDF_PATH 'tools/idf.py'
if (!(Test-Path -LiteralPath $pythonPath) -or !(Test-Path -LiteralPath $idfScript)) {
    throw 'Activate ESP-IDF 5.5.3 with export.ps1 first.'
}
# This process uses native Python/CMake/Ninja, not the MSYS toolchain.
Remove-Item Env:MSYSTEM -ErrorAction SilentlyContinue
$validationPath = Join-Path $repoPath ('build/validation-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $validationPath -Force | Out-Null
$env:SDKCONFIG_DEFAULTS = Join-Path $repoPath 'sdkconfig.defaults'
Set-Location -LiteralPath $repoPath
function Invoke-CheckedPython {
    & $pythonPath @args
    if ($LASTEXITCODE -ne 0) { throw "Validation command failed ($LASTEXITCODE); retained $validationPath" }
}
Invoke-CheckedPython $idfScript -B $validationPath -D "SDKCONFIG=$validationPath/sdkconfig" build
Invoke-CheckedPython $idfScript -B $validationPath merge-bin -o "$validationPath/FoloToy-AI-Passport-full.bin"
Invoke-CheckedPython tools/verify_firmware.py $validationPath
Invoke-CheckedPython tools/verify_pokedex_layout.py $validationPath
Invoke-CheckedPython tools/archive_firmware.py create $validationPath --archive-root "$repoPath/build/firmware"
Copy-Item -LiteralPath "$validationPath/FoloToy-AI-Passport-full.bin" -Destination "$repoPath/build/FoloToy-AI-Passport-full.bin"
# Guard the sole recursive cleanup target before removing a successful scratch build.
$resolvedPath = (Resolve-Path -LiteralPath $validationPath).Path
$expectedParent = (Resolve-Path -LiteralPath "$repoPath/build").Path
if ((Split-Path $resolvedPath -Parent) -ne $expectedParent -or
    (Split-Path $resolvedPath -Leaf) -notmatch '^validation-[0-9a-f]{32}$') {
    throw 'Unexpected validation cleanup path'
}
Remove-Item -LiteralPath $resolvedPath -Recurse -Force
Write-Output 'Firmware build: PASS'
