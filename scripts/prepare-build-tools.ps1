$ErrorActionPreference = "Stop"

$vsInstaller = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\setup.exe"

if (-not (Test-Path $vsInstaller)) {
  throw "Visual Studio Installer nao encontrado. Instale Visual Studio 2022 Build Tools ou Visual Studio 2022."
}

Write-Host "Este script prepara o workload C++ necessario para compilar o GravaTelaFacil."
Write-Host "Ele usa o Visual Studio Installer em modo passivo e pode demorar."

& $vsInstaller modify `
  --installPath "C:\Program Files\Microsoft Visual Studio\2022\Enterprise" `
  --add Microsoft.VisualStudio.Workload.NativeDesktop `
  --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
  --add Microsoft.VisualStudio.Component.Windows10SDK.22000 `
  --includeRecommended `
  --passive `
  --norestart

if ($LASTEXITCODE -ne 0) {
  throw "Falha ao preparar Build Tools. Codigo de saida: $LASTEXITCODE"
}

Write-Host "Instalacao/reparo solicitado. Rode scripts\check-environment.ps1 para confirmar."
