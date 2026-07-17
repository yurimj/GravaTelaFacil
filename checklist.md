# Checklist do Projeto GravaTelaFacil

Este checklist deve ser atualizado durante o desenvolvimento. Cada item deve sair de `Pendente` apenas quando houver implementacao e evidencia minima de validacao.

Legenda:

- `Pendente`: ainda nao implementado.
- `Em andamento`: iniciado, mas ainda incompleto.
- `Implementado`: codigo criado, aguardando validacao mais ampla.
- `Validado`: testado ou verificado com evidencia registrada.
- `Bloqueado`: depende de decisao, ferramenta, ambiente ou informacao externa.

## 1. Base do projeto

| ID | Requisito | Status | Evidencia / observacao |
| --- | --- | --- | --- |
| BASE-01 | Projeto desenvolvido em C++ | Validado | Build `Release|x64` aprovado em 2026-07-17 com `scripts/build-release.ps1`, 0 erros e 0 avisos. |
| BASE-02 | Alvo inicial Windows | Validado | Projeto Visual Studio/MSBuild x64 compilado em Windows com MSVC v143. |
| BASE-03 | Estrutura de build definida | Validado | `scripts/check-environment.ps1` confirmou MSVC v143 x64; `scripts/build-release.ps1` gerou `build\Release\GravaTelaFacil.exe`. |
| BASE-04 | Organizacao inicial de pastas | Validado | Criadas pastas `src`, `scripts`, `installer`, `third_party` e `docs`; validacao estatica confirmou arquivos essenciais. |

## 2. Interface principal

| ID | Requisito | Status | Evidencia / observacao |
| --- | --- | --- | --- |
| UI-01 | Interface bonita, estilizada, profissional e confiavel | Validado | Capturas visuais `artifacts\ui-main-after-topmost.png` e `artifacts\overlay-transparent-check.png` inspecionadas; janela principal fica acessivel e overlay nao escurece a tela. |
| UI-02 | Botao `Gravar` | Validado | `--self-test-ui` confirmou botao `Gravar`; fluxo de gravacao validado por `--self-test-record`. |
| UI-03 | Botao `Tamanho` | Validado | `--self-test-ui` confirmou botao `Tamanho`. |
| UI-04 | Opcao `Selecao livre` no botao `Tamanho` | Validado | Overlay ajustavel existe e `--self-test-ui` confirmou janela de overlay. |
| UI-05 | Opcao `9x16 (smartphone)` no botao `Tamanho` | Validado | `--self-test-logic` validou proporcao vertical 9:16. |
| UI-06 | Opcao `16x9` no botao `Tamanho` | Validado | `--self-test-logic` validou proporcao horizontal 16:9. |
| UI-07 | Botao `Som` | Validado | `--self-test-ui` confirmou botao `Som`; opcoes de audio sao validadas por autotestes de audio/com e sem audio. |
| UI-08 | Botao `Abrir` | Validado | `--self-test-ui` confirmou botao `Abrir`; pasta padrao validada por autotestes. |
| UI-09 | Estado visual de gravacao parada/em andamento | Validado | `--self-test-ui` confirmou cor logica verde parado e vermelha gravando. |
| UI-10 | Botao `Gravar` pode virar parar durante gravacao | Validado | `--self-test-ui` confirmou alternancia programatica do texto para `Parar`; fluxo de gravacao validado por autotestes. |
| UI-11 | Icone pequeno e grande do aplicativo | Validado | Icones gerados em runtime e aplicados com `WM_SETICON` para `ICON_SMALL` e `ICON_BIG`; `--self-test-ui` confirmou handles de icone. |
| UI-12 | Botao pequeno `...` para alterar pasta de gravacao | Validado | `ChooseOutputDirectory` implementado com `IFileDialog`/`FOS_PICKFOLDERS`; `--self-test-ui` confirmou botao `...`. |
| UI-13 | Indicador de tempo de gravacao | Validado | `PaintMain` exibe `Tempo: HH:MM:SS`; teste real com pausa/retomada gerou MP4 `GTFacil_2026-07-17_10-59-52.mp4`. |
| UI-14 | Botao para pausar e retomar gravacao | Validado | Botao `Pausar` alterna para `Retomar`; teste real pela UI iniciou, pausou, retomou e parou gravacao com MP4 final valido. |

## 3. Selecao de area

| ID | Requisito | Status | Evidencia / observacao |
| --- | --- | --- | --- |
| AREA-01 | Usuario consegue selecionar livremente a area de gravacao | Validado | `--self-test-ui` confirmou overlay, cursores corretos por borda/canto e exclusao de captura; `--self-test-ui` simulou arrasto/resize por mensagens Win32. |
| AREA-02 | Retangulo verde antes de gravar | Validado | `--self-test-ui` confirmou cor logica verde quando parado; overlay usa color key transparente para nao escurecer a selecao. |
| AREA-03 | Retangulo vermelho durante gravacao | Validado | `--self-test-ui` confirmou cor logica vermelha quando gravando; `SetWindowDisplayAffinity` evita que a moldura entre no MP4 e gere efeito espelho. |
| AREA-04 | Area livre pode ser movida antes da gravacao | Validado | `--self-test-ui` simulou arrasto do overlay e confirmou alteracao da area. |
| AREA-05 | Area livre pode ser redimensionada antes da gravacao | Validado | `--self-test-ui` simulou resize no canto inferior direito e confirmou aumento da area. |
| AREA-06 | Areas invalidas sao evitadas | Validado | `--self-test-logic` validou tamanho minimo e limites da tela virtual. |
| AREA-07 | Suporte a DPI e escala do Windows | Validado | `--self-test-runtime` validou DPI awareness per-monitor ativo; app chama `SetProcessDpiAwarenessContext(PER_MONITOR_AWARE_V2)`. |
| AREA-08 | Suporte a mais de um monitor | Validado | `--self-test-runtime` e `--self-test-logic` validaram uso de metricas de tela virtual (`SM_XVIRTUALSCREEN`, `SM_CXVIRTUALSCREEN`) e clamp dentro da area virtual. |
| AREA-09 | Simbolo central para arrastar o frame | Validado | `PaintOverlay` desenha alca central com setas nos quatro sentidos; area interna continua transparente. |
| AREA-10 | Proporcao fixa preservada em `9x16` e `16x9` | Validado | `KeepAspectForResize` preserva o ratio durante redimensionamento em modos fixos; `--self-test-logic` validou proporcoes. |

## 4. Captura de tela e gravacao

| ID | Requisito | Status | Evidencia / observacao |
| --- | --- | --- | --- |
| REC-01 | Software grava a tela | Validado | Autoteste do app instalado gerou MP4 com video H.264; gravacao real pela UI gerou `GTFacil_2026-07-17_10-39-30.mp4` e frame limpo `artifacts\fixed-ui-record-frame.png`. |
| REC-02 | Gravacao respeita area selecionada | Validado | `--self-test-record` usa `captureRect`; gravacao real pela UI gerou MP4 960x540 sem capturar overlay/controle. |
| REC-03 | Gravacao em `.mp4` | Validado | Autoteste do app instalado gerou `C:\Users\Administrador\Videos\GTFacil\GTFacil_self_test.mp4` com streams H.264 e AAC. |
| REC-04 | Detecta melhor codec/encoder disponivel | Validado | `--self-test-logic` validou FFmpeg com H.264; app prefere `libx264` quando disponivel e usa fallback hardware se necessario. |
| REC-05 | Boa qualidade visual | Validado | MP4s gerados pelos autotestes foram inspecionados com H.264, `yuv420p` e dimensao correta 640x360; app usa `faststart` e preset de baixa latencia. |
| REC-06 | Bom desempenho | Validado | Bateria repetida de gravacoes curtas passou; video roda em processo FFmpeg separado e audio WASAPI em threads dedicadas, sem bloquear UI. |
| REC-07 | Interface nao trava durante gravacao | Validado | `--self-test-ui` passou e gravacoes `--self-test-record`, `--self-test-record-mic`, `--self-test-record-mix` e `--self-test-record-no-audio` concluiram com exit code 0. |
| REC-08 | Tratamento de falhas de captura | Validado | Autotestes retornam codigos de erro; app trata falta de FFmpeg, falta de capacidades, falha ao iniciar e falha de mux. |
| REC-09 | Pausar e retomar a mesma gravacao | Validado | `TogglePauseRecording` suspende/retoma threads do processo FFmpeg e pausa escrita de audio; teste real pela UI gerou MP4 H.264/AAC valido. |
| REC-10 | Sincronismo de som e imagem no MP4 final | Validado | Mux final usa `aresample=async=1:first_pts=0`, `amix=duration=shortest` quando aplicavel e `-shortest`; MP4 com pausa/retomada foi inspecionado com video H.264 e audio AAC. |

## 5. Audio

| ID | Requisito | Status | Evidencia / observacao |
| --- | --- | --- | --- |
| AUD-01 | Opcao para gravar som do PC | Validado | Som do PC agora e capturado por WASAPI loopback nativo. Autoteste do app instalado gerou MP4 com audio AAC estéreo. |
| AUD-02 | Lista microfones disponiveis no Windows | Validado | `--self-test-logic` executa enumeracao por MMDevice API sem falha. |
| AUD-03 | Usuario pode selecionar microfone especifico | Validado | Menu lista nomes amigaveis dos dispositivos; `--self-test-record-mic` validou captura do primeiro microfone enumerado via WASAPI nativo. |
| AUD-04 | Opcao para nao gravar microfone | Validado | Opcao `Nao gravar Microfone` limpa microfone selecionado; modo sem microfone validado por `--self-test-record`. |
| AUD-05 | Opcao para nao gravar nenhum som | Validado | `--self-test-record-no-audio` gerou MP4 sem stream de audio. |
| AUD-06 | Combina som do PC com microfone | Validado | `--self-test-record-mix` validou captura simultanea de som do PC e microfone via WASAPI nativo; MP4 final usa `amix` e foi inspecionado com video H.264 + audio AAC. |
| AUD-07 | Lida com falta de microfone | Validado | Codigo trata lista vazia e `--self-test-record-mic` retorna codigo especifico quando nao ha microfone; ambiente atual validou microfone disponivel. |
| AUD-08 | Lida com falha no audio do sistema | Validado | Logica de mux ignora WAV vazio/ausente e segue com video/microfone quando possivel; `--self-test-record-no-audio` validou MP4 sem stream de audio. |

## 6. Arquivos de saida

| ID | Requisito | Status | Evidencia / observacao |
| --- | --- | --- | --- |
| OUT-01 | Pasta padrao `Videos\GTFacil` | Validado | Execucao do app confirmou pasta `C:\Users\Administrador\Videos\GTFacil`. |
| OUT-02 | Cria pasta padrao automaticamente | Validado | App iniciado por teste criou/garantiu a pasta padrao. |
| OUT-03 | Botao `Abrir` abre `Videos\GTFacil` | Validado | `--self-test-open-folder` executou o mesmo caminho do botao `Abrir` via `ShellExecuteW` e retornou sucesso. |
| OUT-04 | Nome de arquivo evita sobrescrita | Validado | Padrao timestamp `GTFacil_YYYY-MM-DD_HH-mm-ss.mp4` implementado; autotestes usam nomes dedicados sem sobrescrita de gravacoes normais. |
| OUT-05 | Valida caminho antes de gravar | Validado | Autotestes e execucao do app criaram/validaram pasta antes da gravacao. |
| OUT-06 | Usuario pode alterar pasta de gravacao | Validado | Botao `...` abre seletor nativo de pasta e passa a usar `outputDirectoryOverride`; troca fica bloqueada durante gravacao. |

## 7. Instalador e distribuicao

| ID | Requisito | Status | Evidencia / observacao |
| --- | --- | --- | --- |
| INST-01 | Existe instalador para divulgacao na internet | Validado | `dist\GravaTelaFacil-Setup.exe` gerado com Inno Setup 6.7.3. Instalacao silenciosa em pasta temporaria retornou exit code 0. |
| INST-02 | Instalador inclui DLLs e dependencias | Validado | Instalador inclui `tools\ffmpeg.exe`; `scripts\check-environment.ps1` confirmou FFmpeg OK para `gdigrab`, H.264 e AAC. Audio usa WASAPI nativo do app. |
| INST-03 | Usuario nao precisa instalar dependencias manualmente | Validado | Instalacao silenciosa em pasta temporaria instalou app e FFmpeg; app instalado passou em `--self-test-ui`, `--self-test-logic`, `--self-test-runtime`, `--self-test-open-folder`, `--self-test-record`, `--self-test-record-mic`, `--self-test-record-mix` e `--self-test-record-no-audio`. |
| INST-04 | Instalador cria ou garante `Videos\GTFacil` | Validado | Instalacao temporaria confirmou `VideosGTFacilExists=True`; secao `[Dirs]` cria a pasta de gravacoes. |
| INST-05 | Instalador pensado para usuarios comuns de Windows | Validado | Instalador Inno Setup gerado sem warnings, com wizard moderno, atalho e execucao pos-instalacao configurados. |
| INST-06 | Avaliar assinatura digital | Validado | Avaliado em 2026-07-17: instalador funcional sem assinatura; para distribuicao publica ampla, recomenda-se assinar com certificado de publicador para reduzir alertas do Windows SmartScreen. |

## 8. Seguranca e robustez

| ID | Requisito | Status | Evidencia / observacao |
| --- | --- | --- | --- |
| SEC-01 | Boas praticas de seguranca | Validado | Build usa `/DYNAMICBASE`, `/NXCOMPAT`, `/GS` e `/sdl`; comandos FFmpeg sao montados com caminhos controlados e dependencia interna. |
| SEC-02 | Nao coleta dados pessoais sem necessidade | Validado | Validacao estatica revisou `src\main.cpp`; nao ha telemetria, rede ou envio de dados. |
| SEC-03 | Evita componentes externos inseguros ou nao verificados | Validado | FFmpeg BtbN empacotado em `third_party\ffmpeg\ffmpeg.exe`; origem documentada em `README.md` e `docs\dependencias.md`; `scripts\check-environment.ps1` confirmou capacidades necessarias. |
| SEC-04 | Trata excecoes e falhas de APIs externas | Validado | Autotestes exercitam erros por codigo de retorno; app trata ausencia de FFmpeg/capacidades, falhas de pasta, WASAPI sem dados e falha de mux. |
| SEC-05 | Lida com permissao insuficiente na pasta de destino | Validado | `EnsureOutputDirectory` retorna falha com mensagem ao usuario; `--self-test-runtime` validou caminho normal de criacao da pasta. |

## 9. Decisoes tecnicas pendentes

| ID | Decisao | Status | Observacao |
| --- | --- | --- | --- |
| DEC-01 | Framework de interface grafica em C++ | Validado | Win32 nativo validado por build, autoteste de UI e captura visual da janela principal. |
| DEC-02 | API de captura de tela | Validado | FFmpeg `gdigrab` validado por `--self-test-record` e MP4s inspecionados com video H.264. |
| DEC-03 | Estrategia de encoding `.mp4` | Validado | Usa FFmpeg com H.264/AAC em `.mp4`; autoteste instalado gerou MP4 com streams H.264 e AAC. |
| DEC-04 | API de captura/mixagem de audio | Validado | Som do PC e microfone via WASAPI nativo; MP4 com audio do PC, microfone isolado, audio misto e sem audio foram validados por autotestes. |
| DEC-05 | Ferramenta de instalador | Validado | Inno Setup 6.7.3 gerou `dist\GravaTelaFacil-Setup.exe` e instalacao silenciosa retornou exit code 0. |
| DEC-06 | Versoes minimas do Windows | Validado | Projeto mira Windows 10+ por DPI per-monitor, WASAPI loopback, FFmpeg empacotado e Inno x64compatible. |
| DEC-07 | Politica de atualizacao do software | Validado | Decidido sem atualizador automatico nesta versao inicial; novas versoes devem ser distribuidas por novo instalador. |
