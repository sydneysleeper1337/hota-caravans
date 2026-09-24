$ErrorActionPreference = 'Stop'
$gameRoot = Split-Path -Parent $PSScriptRoot
$names = @('h3hota Caravan.exe', 'h3hota Caravan HD.exe', 'Caravan.dll', 'Caravan.log')
foreach ($name in $names) {
    $path = Join-Path $gameRoot $name
    if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Force }
}
Write-Host 'Мод удалён. Оригинальные EXE и Caravan.ini сохранены.' -ForegroundColor Green
