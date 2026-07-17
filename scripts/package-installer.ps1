$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$isccCandidates = @(
  "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
  "C:\Program Files\Inno Setup 6\ISCC.exe"
)

$iscc = $isccCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) {
  throw "Inno Setup 6 nao encontrado. Instale o Inno Setup para gerar o instalador."
}

if (-not (Test-Path "$root\build\Release\GravaTelaFacil.exe")) {
  & "$PSScriptRoot\build-release.ps1"
}

if (-not (Test-Path "$root\third_party\ffmpeg\ffmpeg.exe")) {
  throw "ffmpeg.exe nao encontrado em third_party\ffmpeg\ffmpeg.exe. O instalador final precisa empacotar essa dependencia."
}

& $iscc "$root\installer\GravaTelaFacil.iss"
Write-Host "Instalador gerado em dist\GravaTelaFacil-Setup.exe"
