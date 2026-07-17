# GravaTelaFacil

GravaTelaFacil e um gravador de tela em C++ para Windows, guiado por `premissas.md`, `checklist.md` e `status.md`.

## Build

Requisitos:

- Visual Studio 2022 ou Build Tools com workload C++.
- Windows SDK.

Verificacao do ambiente:

```powershell
.\scripts\check-environment.ps1
```

Se o workload C++ estiver ausente, ha um script auxiliar:

```powershell
.\scripts\prepare-build-tools.ps1
```

Comando:

```powershell
.\scripts\build-release.ps1
```

O executavel sera copiado para:

```text
build\Release\GravaTelaFacil.exe
```

## Gravacao

A versao atual usa FFmpeg como backend de gravacao. Para gravar, coloque `ffmpeg.exe` em:

```text
build\Release\tools\ffmpeg.exe
```

ou deixe `ffmpeg.exe` disponivel no `PATH`.

O instalador final deve empacotar `ffmpeg.exe` em `tools\ffmpeg.exe` para cumprir a premissa de nao exigir instalacao manual de dependencias.

Mais detalhes em:

```text
docs\dependencias.md
```

Para baixar FFmpeg e instalar Inno Setup automaticamente:

```powershell
.\scripts\fetch-dependencies.ps1
```

## Instalador

Requisito:

- Inno Setup 6.

Comando:

```powershell
.\scripts\package-installer.ps1
```

Saida esperada:

```text
dist\GravaTelaFacil-Setup.exe
```

No GitHub, o instalador deve ser publicado como asset de uma Release, nao commitado no repositorio.

## Validacao

Validacao estatica de arquivos e requisitos principais:

```powershell
.\scripts\validate-source.ps1
```

Plano de validacao manual:

```text
docs\plano-validacao.md
```
