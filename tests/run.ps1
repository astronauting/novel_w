$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    New-Item -ItemType Directory -Force obj/tests | Out-Null
    $sources = @('tests/core_tests.cpp', 'src/ChapterIndex.cpp', 'src/FileIo.cpp')
    if (Get-Command cl.exe -ErrorAction SilentlyContinue) {
        & cl.exe /nologo /std:c++17 /utf-8 /EHsc /DUNICODE /D_UNICODE /Isrc @sources /Fe:obj/tests/core_tests.exe /Fo:obj/tests/ /link user32.lib
    } else {
        $condaBin = Join-Path $env:USERPROFILE '.conda/envs/novelw-build/Library/bin'
        if (Test-Path "$condaBin/g++.exe") { $env:PATH = "$condaBin;$env:PATH" }
        if (-not (Get-Command g++.exe -ErrorAction SilentlyContinue)) {
            throw 'Use a VS x64 Native Tools prompt or add MinGW-w64 to PATH.'
        }
        $extra = @()
        $sysrootLib = Join-Path $env:USERPROFILE '.conda/envs/novelw-build/Library/x86_64-w64-mingw32/sysroot/usr/lib'
        if (Test-Path "$condaBin/g++.exe") { $extra += "-B$sysrootLib/" }
        & g++.exe @extra -std=c++17 -municode -static -DUNICODE -D_UNICODE -Isrc @sources -o obj/tests/core_tests.exe -luser32
    }
    if ($LASTEXITCODE -ne 0) { throw "Test compilation failed: $LASTEXITCODE" }
    & ./obj/tests/core_tests.exe (Join-Path $root 'obj/tests')
    if ($LASTEXITCODE -ne 0) { throw "Core tests failed: $LASTEXITCODE" }
} finally {
    Pop-Location
}
