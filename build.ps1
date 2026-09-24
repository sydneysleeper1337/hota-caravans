param(
    [string]$Compiler = ''
)

$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$outputRoot = Join-Path $projectRoot 'dist'
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null

if (-not $Compiler) {
    foreach ($candidate in @('i686-w64-mingw32-g++.exe', 'g++.exe')) {
        $command = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($command) {
            $Compiler = $command.Source
            break
        }
    }
}

if (-not $Compiler -or -not (Test-Path -LiteralPath $Compiler)) {
    throw 'A MinGW-w64 C++ compiler was not found. Pass its full path with -Compiler.'
}

$arguments = @(
    '-m32',
    '-std=c++17',
    '-O2',
    '-Wall',
    '-Wextra',
    '-DCARAVAN_PHYSICS',
    '-shared',
    (Join-Path $projectRoot 'src\Caravan.cpp'),
    (Join-Path $projectRoot 'src\Caravan.def'),
    '-static-libgcc',
    '-static-libstdc++',
    "-Wl,-Map,$(Join-Path $outputRoot 'Caravan.map')",
    '-o',
    (Join-Path $outputRoot 'Caravan.dll'),
    '-lcomctl32',
    '-lgdi32'
)

& $Compiler @arguments
if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE"
}

Write-Host 'Built dist\Caravan.dll.' -ForegroundColor Green

