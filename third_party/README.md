# Dependencias de terceiros

## FFmpeg

A implementacao atual usa `ffmpeg.exe` como backend de captura/encoding para gerar `.mp4`.

Para que o instalador inclua essa dependencia automaticamente, coloque o binario em:

```text
third_party\ffmpeg\ffmpeg.exe
```

O aplicativo tambem procura `ffmpeg.exe` em:

```text
tools\ffmpeg.exe
```

ao lado do executavel instalado, e depois no `PATH`.

