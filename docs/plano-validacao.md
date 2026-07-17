# Plano de Validacao do GravaTelaFacil

Este plano deve ser executado depois que o ambiente C++ estiver pronto e o aplicativo compilar.

## 1. Ambiente

1. Rodar `scripts\check-environment.ps1`.
2. Confirmar que o workload C++ x64/v143 esta completo.
3. Confirmar que `third_party\ffmpeg\ffmpeg.exe` existe.
4. Confirmar que Inno Setup 6 esta instalado quando for gerar instalador.

## 2. Build

1. Rodar `scripts\build-release.ps1`.
2. Confirmar que `build\Release\GravaTelaFacil.exe` foi gerado.
3. Executar `build\Release\GravaTelaFacil.exe`.

## 3. Interface

1. Confirmar que a tela principal abre com os botoes:
   - `Gravar`
   - `Tamanho`
   - `Som`
   - `Abrir`
2. Confirmar que a interface tem aparencia limpa, profissional e sem textos sobrepostos.
3. Confirmar que o estado muda visualmente durante a gravacao.

## 4. Tamanho e selecao

1. Selecionar `Tamanho > Selecao livre`.
2. Confirmar que o retangulo aparece verde.
3. Mover o retangulo.
4. Redimensionar usando bordas e cantos.
5. Clicar em `Gravar` e confirmar que o retangulo fica vermelho.
6. Selecionar `9x16 (smartphone)` e confirmar proporcao vertical.
7. Selecionar `16x9` e confirmar proporcao horizontal.

## 5. Audio

1. Abrir `Som`.
2. Confirmar que os microfones disponiveis aparecem.
3. Selecionar um microfone e gravar um teste.
4. Marcar som do PC e gravar um teste com audio do sistema.
5. Gravar com som do PC + microfone.
6. Gravar com `Nao gravar Microfone`.
7. Gravar com `Nao gravar nenhum som`.

## 6. Saida

1. Confirmar que `Videos\GTFacil` e criado automaticamente.
2. Confirmar que os arquivos `.mp4` usam o padrao `GTFacil_YYYY-MM-DD_HH-mm-ss.mp4`.
3. Confirmar que gravacoes anteriores nao sao sobrescritas.
4. Clicar em `Abrir` e confirmar que a pasta `Videos\GTFacil` abre.

## 7. Video

1. Abrir cada `.mp4` gerado.
2. Confirmar que a area gravada corresponde a area escolhida.
3. Confirmar que o video tem boa qualidade visual.
4. Confirmar que a interface nao travou durante a gravacao.
5. Verificar uso de CPU, memoria e disco durante uma gravacao de pelo menos 2 minutos.

## 8. Instalador

1. Rodar `scripts\package-installer.ps1`.
2. Confirmar que `dist\GravaTelaFacil-Setup.exe` foi gerado.
3. Instalar em um ambiente limpo de Windows.
4. Confirmar que o app abre sem dependencias manuais.
5. Confirmar que `ffmpeg.exe` foi instalado em `tools\ffmpeg.exe`.
6. Confirmar que `Videos\GTFacil` foi criado ou e criado no primeiro uso.
7. Gerar uma gravacao `.mp4` no ambiente limpo.

