# Status do Projeto GravaTelaFacil

Ultima atualizacao: 2026-07-17

## Resumo atual

O projeto saiu da fase apenas documental e recebeu a primeira implementacao C++/Win32.

Ja existe estrutura de solucao Visual Studio, app C++ Win32, interface principal, overlay de selecao de area, configuracao inicial de audio, pasta padrao, backend de gravacao via FFmpeg, deteccao inicial de encoder H.264 e script inicial de instalador Inno Setup.

A validacao local avancou: o workload C++ x64/v143 foi preparado, o build `Release|x64` passou com 0 erros e 0 avisos, o executavel iniciou, e a pasta `C:\Users\Administrador\Videos\GTFacil` foi criada/confirmada.

A validacao estatica de fonte passou com `scripts\validate-source.ps1`. Um teste direto com FFmpeg do Shotcut gerou `C:\Users\Administrador\Videos\GTFacil\GTFacil_backend_test.mp4`, provando a captura de tela/MP4 sem audio fora da UI.

O instalador foi gerado em `dist\GravaTelaFacil-Setup.exe` e testado em instalacao silenciosa temporaria. O app instalado iniciou e `tools\ffmpeg.exe` foi instalado junto.

O requisito de som do PC foi destravado sem depender de FFmpeg WASAPI: o app captura audio do sistema com WASAPI loopback nativo, grava WAV temporario e muxa no MP4 final. O autoteste do app instalado gerou `C:\Users\Administrador\Videos\GTFacil\GTFacil_self_test.mp4` com video H.264 e audio AAC estéreo.

Depois de teste manual do usuario, foi corrigida uma regressao importante: o overlay era capturado pelo FFmpeg e gerava video escuro/com efeito espelho. O overlay agora usa color key transparente, cursores corretos por borda/canto e `SetWindowDisplayAffinity` para excluir overlay/janela de controle da captura. A gravacao real pela UI gerou `GTFacil_2026-07-17_10-39-30.mp4`; o frame `artifacts\fixed-ui-record-frame.png` confirmou video limpo.

Nova rodada de ajustes de UX implementou icone pequeno/grande, alca central de movimento no frame, preservacao obrigatoria de ratio em `9x16` e `16x9`, botao `...` para trocar pasta de gravacao, indicador de tempo, pausa/retomada e mux final com reamostragem assíncrona para reduzir risco de dessincronismo entre som e imagem. O teste real pela UI com pausar/retomar gerou `GTFacil_2026-07-17_10-59-52.mp4` com video H.264 e audio AAC.

## Estado geral

| Area | Status | Observacao |
| --- | --- | --- |
| Premissas | Concluido | Arquivo `premissas.md` criado. |
| Checklist | Concluido | Arquivo `checklist.md` criado para rastrear requisitos. |
| Estrutura do projeto | Validado | Solucao Visual Studio, projeto C++ e scripts criados; build Release aprovado. |
| Interface | Validado | UI Win32 com botoes obrigatorios, botao `Pausar`, botao `...`, indicador de tempo, icones pequeno/grande, janela principal topmost acima do overlay, overlay transparente e cursor correto por borda/canto. |
| Captura de tela | Validado | Autotestes validaram proporcoes, limites de area, mover/redimensionar overlay, ratio fixo e MP4 com video H.264. |
| Audio | Validado | Som do PC e microfone via WASAPI nativo validados por autotestes; sem audio tambem validado. |
| Encoding `.mp4` | Validado | H.264/AAC via FFmpeg, com deteccao de `h264_nvenc`, `h264_qsv`, `h264_amf` e fallback `libx264`; streams dos MP4s foram inspecionados. |
| Pasta de saida | Validado | App iniciou e confirmou `C:\Users\Administrador\Videos\GTFacil`. |
| Instalador | Validado | `dist\GravaTelaFacil-Setup.exe` gerado, instalado em pasta temporaria e validado com autoteste do app instalado. |
| Testes/validacao | Validado | Validacao estatica passou, build Release passou, app iniciou, instalador instalou e passou em `--self-test-ui`, `--self-test-logic`, `--self-test-runtime`, `--self-test-open-folder`, `--self-test-record`, `--self-test-record-mic`, `--self-test-record-mix` e `--self-test-record-no-audio`; teste real com pausar/retomar gerou MP4 valido. |

## Decisoes finais desta versao

1. Interface final validada por captura visual local; a janela principal fica acima do overlay para manter `Gravar`, `Tamanho`, `Som` e `Abrir` acessiveis.
2. Botao `Abrir` validado por `--self-test-open-folder`, que executa o mesmo caminho `ShellExecuteW`.
3. Proporcoes `9x16 (smartphone)` e `16x9` validadas por `--self-test-logic`.
4. Instalador validado em instalacao silenciosa temporaria com FFmpeg empacotado e pasta `Videos\GTFacil` criada.
5. Assinatura digital foi avaliada: nao e obrigatoria para o funcionamento, mas e recomendada antes de distribuicao publica ampla para reduzir alertas do Windows SmartScreen.
6. Pausa/retomada nesta versao usa suspensao/retomada das threads do processo FFmpeg e descarte de amostras WASAPI durante pausa, mantendo uma unica gravacao final.
7. Sincronismo A/V usa `aresample=async=1:first_pts=0`, `amix=duration=shortest` quando ha duas fontes de audio e `-shortest` no mux final.

## Recomendacao tecnica inicial

Decisoes tomadas no primeiro ciclo:

- C++ com projeto Visual Studio/MSBuild.
- Interface Win32 nativa para reduzir dependencias.
- Overlay Win32 topmost para selecao livre.
- Captura/encoding `.mp4` via FFmpeg.
- Deteccao de encoder H.264 via `ffmpeg -encoders`, priorizando hardware quando disponivel.
- Audio do sistema e microfone via WASAPI nativo, com mux final por FFmpeg.
- Microfones enumerados via MMDevice API.
- Instalador com Inno Setup.

Essas escolhas privilegiam entrega rapida, instalador previsivel e menor numero de bibliotecas de UI. Uma evolucao futura pode trocar a captura FFmpeg por Windows Graphics Capture/Media Foundation se desempenho, licenca ou controle fino exigirem.

## Proximo passo sugerido

Para verificar o ambiente, rodar:

```powershell
.\scripts\check-environment.ps1
```

O build ja passou. Para repetir:

```powershell
.\scripts\build-release.ps1
```

Para regenerar o instalador:

```powershell
.\scripts\package-installer.ps1
```

## Criterio de conclusao do projeto

O projeto so deve ser considerado concluido quando:

1. Todos os itens obrigatorios do `premissas.md` estiverem implementados.
2. O `checklist.md` estiver com os requisitos obrigatorios marcados como `Validado`.
3. Houver pelo menos uma gravacao `.mp4` gerada com sucesso.
4. A pasta `Videos\GTFacil` for criada automaticamente e aberta pelo botao `Abrir`.
5. As opcoes de tamanho e som funcionarem conforme descrito.
6. Existir um instalador funcional contendo todas as dependencias necessarias.
7. As principais falhas esperadas tiverem tratamento compreensivel para o usuario.

## Riscos conhecidos

| Risco | Impacto | Mitigacao |
| --- | --- | --- |
| Escolha inadequada de biblioteca de video/audio | Alto | Fazer uma prova tecnica antes de fechar arquitetura. |
| Instalador incompleto | Alto | Testar em ambiente limpo de Windows. |
| Captura com baixo desempenho | Alto | Priorizar APIs modernas e encoding eficiente. |
| Problemas com DPI e multiplos monitores | Medio/alto | Validar cedo com cenarios reais. |
| Dependencias grandes ou complexas | Medio | Avaliar impacto no instalador e licenciamento. |
| Codec indisponivel no computador do usuario | Alto | Detectar melhor backend e empacotar o necessario quando permitido. |

## Historico

| Data | Evento |
| --- | --- |
| 2026-07-17 | Criado `premissas.md` com os requisitos iniciais do software. |
| 2026-07-17 | Criados `checklist.md` e `status.md` para acompanhamento do projeto. |
| 2026-07-17 | Criada primeira implementacao C++/Win32 com UI, overlay, audio, pasta de saida, backend FFmpeg e instalador Inno Setup inicial. |
| 2026-07-17 | Build local tentou rodar, mas foi bloqueado por ausencia do toolset C++ x64/v143 completo no Visual Studio instalado. |
| 2026-07-17 | Adicionados `scripts\check-environment.ps1`, `scripts\validate-source.ps1` e `docs\plano-validacao.md`; validacao estatica passou. |
| 2026-07-17 | Melhorada a gravacao: deteccao de encoder H.264, timestamp sem dependencia de `std::format` e parada graciosa do FFmpeg via stdin. |
| 2026-07-17 | Workload C++ x64/v143 preparado; build Release passou com 0 erros e 0 avisos. |
| 2026-07-17 | Executavel iniciou e confirmou a pasta `C:\Users\Administrador\Videos\GTFacil`. |
| 2026-07-17 | Teste direto de captura com FFmpeg gerou `GTFacil_backend_test.mp4`. |
| 2026-07-17 | Inno Setup 6 instalado; empacotamento bloqueado ate incluir FFmpeg completo em `third_party\ffmpeg\ffmpeg.exe`. |
| 2026-07-17 | FFmpeg BtbN baixado e empacotado; possui captura de tela, microfone, H.264 e AAC, mas nao WASAPI. |
| 2026-07-17 | Instalador `dist\GravaTelaFacil-Setup.exe` gerado sem warnings e validado com instalacao silenciosa temporaria. |
| 2026-07-17 | App instalado iniciou com sucesso a partir da pasta temporaria de teste. |
| 2026-07-17 | Implementada captura de som do PC via WASAPI loopback nativo, sem depender de FFmpeg WASAPI. |
| 2026-07-17 | Autoteste `--self-test-record` do app instalado gerou MP4 com video H.264 e audio AAC estéreo. |
| 2026-07-17 | Adicionado autoteste `--self-test-logic` para validar limites de area, proporcoes 9:16/16:9 e capacidades do FFmpeg. |
| 2026-07-17 | Instalador regenerado e validado com `--self-test-logic` e `--self-test-record` a partir de instalacao temporaria. |
| 2026-07-17 | Adicionado e validado `--self-test-record-no-audio`; MP4 gerado sem stream de audio. |
| 2026-07-17 | Instalador temporario validado com os tres autotestes: logica, gravacao com audio do PC e gravacao sem audio. |
| 2026-07-17 | Adicionado `--self-test-ui`; instalador temporario validado com UI, logica, gravacao com audio e gravacao sem audio. |
| 2026-07-17 | `--self-test-ui` passou a simular mover e redimensionar o overlay, validando selecao livre sem clique manual. |
| 2026-07-17 | Microfone migrou para captura WASAPI nativa; `--self-test-record-mic` gerou MP4 com video H.264 e audio AAC. |
| 2026-07-17 | Instalador temporario validado com UI, logica, audio do PC, microfone e sem audio. |
| 2026-07-17 | Adicionado `--self-test-record-mix`; app local e instalado validaram som do PC + microfone juntos, com MP4 H.264/AAC gerado via `amix`. |
| 2026-07-17 | Corrigida camada visual para manter janela principal acima do overlay; captura `artifacts\ui-main-after-topmost.png` validou botoes acessiveis. |
| 2026-07-17 | Adicionado `--self-test-runtime` para pasta padrao, DPI per-monitor, tela virtual, FFmpeg e menus essenciais. |
| 2026-07-17 | Adicionado `--self-test-open-folder`; instalador final validado com UI, logica, runtime, abrir pasta, audio do PC, microfone, audio misto e sem audio. |
| 2026-07-17 | Corrigido bug reportado pelo usuario: video preto/efeito espelho causado pelo overlay capturado; adicionada transparencia por color key, exclusao de captura e cursores corretos nas bordas/cantos. |
| 2026-07-17 | Gravacao real pela UI validada em `GTFacil_2026-07-17_10-39-30.mp4`; frame `artifacts\fixed-ui-record-frame.png` confirmou video limpo. |
| 2026-07-17 | Implementados icones pequeno/grande, alca central de movimento, ratio travado em 9:16/16:9, botao `...` para pasta, indicador de tempo e botao Pausar/Retomar. |
| 2026-07-17 | Mux final ajustado com `aresample=async=1:first_pts=0`, `amix=duration=shortest` e `-shortest`; teste real com pausa/retomada gerou `GTFacil_2026-07-17_10-59-52.mp4`. |
