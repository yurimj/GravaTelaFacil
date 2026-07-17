$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$downloads = Join-Path $root ".downloads"
$ffmpegDir = Join-Path $root "third_party\ffmpeg"
$ffmpegZip = Join-Path $downloads "ffmpeg-release-essentials.zip"
$innoInstaller = Join-Path $downloads "innosetup-6.7.3.exe"

New-Item -ItemType Directory -Force -Path $downloads | Out-Null
New-Item -ItemType Directory -Force -Path $ffmpegDir | Out-Null

function Download-File {
  param(
    [Parameter(Mandatory = $true)][string]$Uri,
    [Parameter(Mandatory = $true)][string]$OutFile
  )

  if (Test-Path $OutFile) {
    Remove-Item -LiteralPath $OutFile -Force
  }

  $curl = Get-Command curl.exe -ErrorAction SilentlyContinue
  if ($curl) {
    & $curl.Source -L --fail --retry 3 --connect-timeout 30 --output $OutFile $Uri
    if ($LASTEXITCODE -ne 0) {
      throw "Falha ao baixar $Uri com curl.exe. Codigo: $LASTEXITCODE"
    }
  } else {
    Invoke-WebRequest -Uri $Uri -OutFile $OutFile
  }

  if (-not (Test-Path $OutFile) -or (Get-Item $OutFile).Length -eq 0) {
    throw "Download vazio ou ausente: $OutFile"
  }
}

if (-not (Test-Path (Join-Path $ffmpegDir "ffmpeg.exe"))) {
  Write-Host "Baixando FFmpeg release essentials..."
  Download-File -Uri "https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip" -OutFile $ffmpegZip

  $extractDir = Join-Path $downloads "ffmpeg"
  if (Test-Path $extractDir) {
    Remove-Item -LiteralPath $extractDir -Recurse -Force
  }
  New-Item -ItemType Directory -Force -Path $extractDir | Out-Null
  Expand-Archive -Path $ffmpegZip -DestinationPath $extractDir -Force

  $ffmpegExe = Get-ChildItem $extractDir -Recurse -Filter ffmpeg.exe | Select-Object -First 1
  if (-not $ffmpegExe) {
    throw "ffmpeg.exe nao encontrado no pacote baixado."
  }
  Copy-Item $ffmpegExe.FullName (Join-Path $ffmpegDir "ffmpeg.exe") -Force
  Write-Host "FFmpeg copiado para third_party\ffmpeg\ffmpeg.exe"
}

$isccCandidates = @(
  "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
  "C:\Program Files\Inno Setup 6\ISCC.exe"
)
$iscc = $isccCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $iscc) {
  Write-Host "Baixando Inno Setup 6.7.3..."
  Download-File -Uri "https://github.com/jrsoftware/issrc/releases/download/is-6_7_3/innosetup-6.7.3.exe" -OutFile $innoInstaller
  Write-Host "Instalando Inno Setup em modo silencioso..."
  $process = Start-Process -FilePath $innoInstaller -ArgumentList "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP-" -PassThru -Wait
  if ($process.ExitCode -ne 0) {
    throw "Falha ao instalar Inno Setup. Codigo: $($process.ExitCode)"
  }
}

Write-Host "Dependencias preparadas."
