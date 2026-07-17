# Dependencias do GravaTelaFacil

## Build

Para compilar o projeto:

- Visual Studio 2022 ou Build Tools.
- Workload `Desktop development with C++`.
- Toolset MSVC x64/v143.
- Windows SDK.

Verificacao:

```powershell
.\scripts\check-environment.ps1
```

Preparacao assistida:

```powershell
.\scripts\prepare-build-tools.ps1
```

## Runtime

O aplicativo usa `ffmpeg.exe` como backend de captura, audio e encoding `.mp4`.

O binario precisa suportar:

- `gdigrab` para captura de tela.
- Algum encoder H.264, como `libx264`, `h264_nvenc`, `h264_qsv` ou `h264_amf`.
- `aac` para audio em `.mp4`.

O som do PC e o microfone sao capturados pelo proprio aplicativo com WASAPI nativo do Windows, entao o FFmpeg nao precisa ter os dispositivos `wasapi` ou `dshow`.

Para o instalador final, o binario deve existir em:

```text
third_party\ffmpeg\ffmpeg.exe
```

Durante a execucao, o aplicativo procura FFmpeg nesta ordem:

1. `tools\ffmpeg.exe` ao lado do executavel instalado.
2. `ffmpeg.exe` disponivel no `PATH`.

## Instalador

Para gerar o instalador:

- Inno Setup 6.

Comando:

```powershell
.\scripts\package-installer.ps1
```

O script falha de proposito se `third_party\ffmpeg\ffmpeg.exe` nao existir, porque o instalador precisa levar todas as dependencias necessarias.
