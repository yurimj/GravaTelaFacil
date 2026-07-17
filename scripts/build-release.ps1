$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$msbuildCandidates = @(
  "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
  "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
  "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
  "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
)

$msbuild = $msbuildCandidates | Where-Object {
  if (-not (Test-Path $_)) { return $false }
  $vsRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $_)))
  Test-Path (Join-Path $vsRoot "MSBuild\Microsoft\VC\v170\Microsoft.Cpp.Default.props")
} | Select-Object -First 1
if (-not $msbuild) {
  throw "MSBuild nao encontrado. Instale Visual Studio 2022 ou Build Tools com workload C++."
}

& $msbuild "$root\GravaTelaFacil.sln" /p:Configuration=Release /p:Platform=x64 /m
if ($LASTEXITCODE -ne 0) {
  throw "Falha ao compilar GravaTelaFacil."
}
New-Item -ItemType Directory -Force -Path "$root\build\Release" | Out-Null
Copy-Item "$root\x64\Release\GravaTelaFacil.exe" "$root\build\Release\GravaTelaFacil.exe" -Force
Write-Host "Build gerado em build\Release\GravaTelaFacil.exe"
