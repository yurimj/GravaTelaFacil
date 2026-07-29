$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$requiredFiles = @(
  "premissas.md",
  "checklist.md",
  "status.md",
  "AGENTS.md",
  "GravaTelaFacil.sln",
  "src\GravaTelaFacil.vcxproj",
  "src\GravaTelaFacil.rc",
  "src\resource.h",
  "src\main.cpp",
  "assets\GravaTelaFacil.png",
  "assets\GravaTelaFacil.ico",
  "assets\toolbar\record.png",
  "assets\toolbar\record.ico",
  "assets\toolbar\stop.png",
  "assets\toolbar\stop.ico",
  "assets\toolbar\pause.png",
  "assets\toolbar\pause.ico",
  "assets\toolbar\open.png",
  "assets\toolbar\open.ico",
  "assets\toolbar\size.png",
  "assets\toolbar\size.ico",
  "assets\toolbar\sound.png",
  "assets\toolbar\sound.ico",
  "scripts\build-release.ps1",
  "scripts\package-installer.ps1",
  "scripts\check-environment.ps1",
  "scripts\prepare-build-tools.ps1",
  "scripts\fetch-dependencies.ps1",
  "installer\GravaTelaFacil.iss",
  "docs\dependencias.md",
  "docs\plano-validacao.md"
)

$requiredTokens = @{
  "src\main.cpp" = @(
    "Gravar",
    "Pausar",
    "Retomar",
    "Tamanho",
    "Som",
    "Abrir",
    "ChooseOutputDirectory",
    "SetProcessThreadsSuspended",
    "SetWindowDisplayAffinity",
    "IDI_APP_ICON",
    "IDI_TOOLBAR_RECORD",
    "IDI_TOOLBAR_STOP",
    "aresample=async=1:first_pts=0",
    "-shortest",
    "Selecao livre",
    "9x16 (smartphone)",
    "16x9",
    "FOLDERID_Videos",
    "GTFacil",
    "ffmpeg.exe",
    "gdigrab",
    "wasapi",
    "MMDeviceEnumerator",
    "GetDefaultAudioEndpoint",
    "h264_nvenc",
    "h264_qsv",
    "h264_amf",
    "libx264",
    "amix=inputs=2",
    "GTFacil_%04u-%02u-%02u_%02u-%02u-%02u.mp4",
    "--self-test-ui",
    "--self-test-logic",
    "--self-test-runtime",
    "--self-test-settings",
    "--self-test-open-folder",
    "--self-test-record",
    "--self-test-record-mic",
    "--self-test-record-mix",
    "--self-test-record-no-audio"
  )
  "installer\GravaTelaFacil.iss" = @(
    "GravaTelaFacil",
    "third_party\ffmpeg\ffmpeg.exe",
    "Videos\GTFacil",
    "WizardStyle=modern",
    "SetupIconFile=..\assets\GravaTelaFacil.ico",
    "Source: ""..\third_party\ffmpeg\ffmpeg.exe"""
  )
  "src\GravaTelaFacil.vcxproj" = @(
    "ResourceCompile Include=""GravaTelaFacil.rc"""
  )
  "src\GravaTelaFacil.rc" = @(
    "IDI_APP_ICON ICON ""..\\assets\\GravaTelaFacil.ico""",
    "IDI_TOOLBAR_RECORD ICON ""..\\assets\\toolbar\\record.ico""",
    "IDI_TOOLBAR_STOP ICON ""..\\assets\\toolbar\\stop.ico""",
    "IDI_TOOLBAR_PAUSE ICON ""..\\assets\\toolbar\\pause.ico""",
    "IDI_TOOLBAR_OPEN ICON ""..\\assets\\toolbar\\open.ico"""
  )
  "scripts\package-installer.ps1" = @(
    "ffmpeg.exe nao encontrado",
    "third_party\ffmpeg\ffmpeg.exe"
  )
  "scripts\prepare-build-tools.ps1" = @(
    "Microsoft.VisualStudio.Workload.NativeDesktop",
    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
  )
  "scripts\check-environment.ps1" = @(
    "gdigrab",
    "libx264",
    "aac"
  )
  "scripts\fetch-dependencies.ps1" = @(
    "ffmpeg-release-essentials.zip",
    "innosetup-6.7.3.exe",
    "curl.exe"
  )
}

$failed = $false

foreach ($file in $requiredFiles) {
  $path = Join-Path $root $file
  if (Test-Path $path) {
    Write-Host "[OK] Arquivo existe: $file"
  } else {
    Write-Host "[FALHA] Arquivo ausente: $file" -ForegroundColor Red
    $failed = $true
  }
}

foreach ($entry in $requiredTokens.GetEnumerator()) {
  $path = Join-Path $root $entry.Key
  if (-not (Test-Path $path)) {
    continue
  }
  $content = Get-Content -Raw -Path $path
  foreach ($token in $entry.Value) {
    if ($content.Contains($token)) {
      Write-Host "[OK] Token encontrado em $($entry.Key): $token"
    } else {
      Write-Host "[FALHA] Token ausente em $($entry.Key): $token" -ForegroundColor Red
      $failed = $true
    }
  }
}

if ($failed) {
  exit 1
}

Write-Host "[OK] Validacao estatica de fonte concluida."
