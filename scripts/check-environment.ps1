$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"

Write-Host "Verificando ambiente do GravaTelaFacil..."

if (-not (Test-Path $vswhere)) {
  Write-Host "[FALHA] vswhere.exe nao encontrado. Instale Visual Studio 2022 ou Build Tools." -ForegroundColor Red
  exit 1
}

$instances = & $vswhere -all -products * -format json | ConvertFrom-Json
$cppReady = $false

foreach ($instance in $instances) {
  $path = $instance.installationPath
  $props = Join-Path $path "MSBuild\Microsoft\VC\v170\Microsoft.Cpp.Default.props"
  $toolsets = Join-Path $path "MSBuild\Microsoft\VC\v170\Platforms\x64\PlatformToolsets\v143"
  $cl = Get-ChildItem (Join-Path $path "VC\Tools\MSVC") -Recurse -Filter cl.exe -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match "\\bin\\Hostx64\\x64\\cl.exe$" } |
    Select-Object -First 1

  Write-Host ""
  Write-Host "Visual Studio: $($instance.displayName)"
  Write-Host "Caminho: $path"
  Write-Host "MSBuild C++ props: $(if (Test-Path $props) { 'OK' } else { 'FALTANDO' })"
  Write-Host "Toolset v143 x64: $(if (Test-Path $toolsets) { 'OK' } else { 'FALTANDO' })"
  Write-Host "cl.exe Hostx64/x64: $(if ($cl) { 'OK' } else { 'FALTANDO' })"

  if ((Test-Path $props) -and (Test-Path $toolsets) -and $cl) {
    $cppReady = $true
  }
}

$ffmpegProject = Join-Path $root "third_party\ffmpeg\ffmpeg.exe"
$ffmpegReady = $false
if (Test-Path $ffmpegProject) {
  $devices = & $ffmpegProject -hide_banner -devices 2>&1 | Out-String
  $encoders = & $ffmpegProject -hide_banner -encoders 2>&1 | Out-String
  $hasGdiGrab = $devices.Contains("gdigrab")
  $hasH264 = $encoders.Contains("libx264") -or $encoders.Contains("h264_nvenc") -or $encoders.Contains("h264_qsv") -or $encoders.Contains("h264_amf")
  $hasAac = $encoders.Contains("aac")
  $ffmpegReady = $hasGdiGrab -and $hasH264 -and $hasAac
}
$innoCandidates = @(
  "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
  "C:\Program Files\Inno Setup 6\ISCC.exe"
)
$inno = $innoCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1

Write-Host ""
if (Test-Path $ffmpegProject) {
  Write-Host "FFmpeg para instalador: $(if ($ffmpegReady) { 'OK' } else { 'INCOMPLETO: requer gdigrab, H.264 e AAC' })"
} else {
  Write-Host "FFmpeg para instalador: FALTANDO em third_party\ffmpeg\ffmpeg.exe"
}
Write-Host "Inno Setup 6: $(if ($inno) { 'OK' } else { 'FALTANDO' })"

if (-not $cppReady) {
  Write-Host ""
  Write-Host "[FALHA] O workload C++ x64/v143 nao esta completo." -ForegroundColor Red
  Write-Host "Abra o Visual Studio Installer e instale/repare: Desktop development with C++." -ForegroundColor Yellow
  exit 1
}

Write-Host ""
Write-Host "[OK] Ambiente C++ pronto para compilar." -ForegroundColor Green
