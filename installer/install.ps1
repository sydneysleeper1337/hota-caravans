$ErrorActionPreference = 'Stop'
$gameRoot = Split-Path -Parent $PSScriptRoot
$payloadRoot = Join-Path $PSScriptRoot 'files'

$targets = @(
    @{ Source = 'h3hota.exe'; Destination = 'h3hota Caravan.exe'; Sha256 = 'B5F2F793AF0986050FB41DF7209C25D861AE0F837AF52BB3BD6864BA4DE84F41' },
    @{ Source = 'h3hota HD.exe'; Destination = 'h3hota Caravan HD.exe'; Sha256 = '5AAAB925F06CCCF23BB09814767590A95B84A557EB33D244800520BE4F1F18DE' }
)
$oldName = [Text.Encoding]::ASCII.GetBytes('VERSION.dll')
$newName = [Text.Encoding]::ASCII.GetBytes('Caravan.dll')

foreach ($target in $targets) {
    $sourcePath = Join-Path $gameRoot $target.Source
    $destinationPath = Join-Path $gameRoot $target.Destination
    if (-not (Test-Path -LiteralPath $sourcePath)) {
        throw "Файл $($target.Source) не найден. Поместите всю папку Caravan_HotA_1.8.0 непосредственно в папку игры."
    }
    $actualHash = (Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash
    if ($actualHash -ne $target.Sha256) {
        throw "$($target.Source) не соответствует HotA 1.8.0, для которой собран мод. Исходный файл не изменён."
    }
    $bytes = [IO.File]::ReadAllBytes($sourcePath)
    $matches = New-Object Collections.Generic.List[int]
    for ($i = 0; $i -le $bytes.Length - $oldName.Length; $i++) {
        $same = $true
        for ($j = 0; $j -lt $oldName.Length; $j++) {
            if ($bytes[$i + $j] -ne $oldName[$j]) { $same = $false; break }
        }
        if ($same) { $matches.Add($i) }
    }
    if ($matches.Count -ne 1) { throw "Неожиданная таблица импорта в $($target.Source). Установка остановлена." }
    [Array]::Copy($newName, 0, $bytes, $matches[0], $newName.Length)
    [IO.File]::WriteAllBytes($destinationPath, $bytes)
}

Copy-Item -LiteralPath (Join-Path $payloadRoot 'Caravan.dll') -Destination (Join-Path $gameRoot 'Caravan.dll') -Force
$configTarget = Join-Path $gameRoot 'Caravan.ini'
if (-not (Test-Path -LiteralPath $configTarget)) {
    Copy-Item -LiteralPath (Join-Path $payloadRoot 'Caravan.ini') -Destination $configTarget
}
$versionProxyTarget = Join-Path $gameRoot 'version_real.dll'
if (-not (Test-Path -LiteralPath $versionProxyTarget)) {
    Copy-Item -LiteralPath "$env:WINDIR\SysWOW64\version.dll" -Destination $versionProxyTarget
}

Write-Host ''
Write-Host 'Караваны установлены.' -ForegroundColor Green
Write-Host 'Запускайте: h3hota Caravan HD.exe'
