# Premissas do Projeto GravaTelaFacil

## Objetivo

Desenvolver um software profissional em C++ para Windows capaz de gravar a tela em formato `.mp4`, com interface simples, bonita, estilizada e adequada para distribuicao publica pela internet.

## Requisitos obrigatorios

1. O software deve ser desenvolvido em C++.
2. O software deve gravar a tela do computador.
3. Deve existir um botao chamado `Gravar`.
4. Deve existir um botao chamado `Tamanho`.
5. No botao `Tamanho`, o usuario deve poder escolher:
   - `Selecao livre`
   - `9x16 (smartphone)`
   - `16x9`
6. Quando `Selecao livre` estiver ativa, o usuario deve conseguir selecionar livremente a area da tela que sera gravada.
7. A selecao livre deve ser exibida com um retangulo verde, com bordas e pontos de ajuste semelhantes a referencia visual enviada.
8. Ao iniciar a gravacao, o retangulo da selecao livre deve mudar de verde para vermelho, indicando que aquela area esta sendo gravada.
9. Deve existir um botao chamado `Som`.
10. No botao `Som`, o usuario deve poder configurar:
    - Gravar som do PC.
    - Selecionar um microfone disponivel.
    - Nao gravar microfone.
    - Nao gravar nenhum som, quando essa for a escolha do usuario.
11. A lista de microfones deve ser detectada a partir dos dispositivos disponiveis no Windows.
12. A gravacao deve ser salva por padrao em `Videos\GTFacil`.
13. No computador do usuario atual, o caminho padrao esperado e `C:\Users\Administrador\Videos\GTFacil`.
14. Deve existir um botao chamado `Abrir`.
15. O botao `Abrir` deve abrir diretamente o diretorio `Videos\GTFacil`.
16. O formato final do video deve ser `.mp4`.
17. Caso seja necessario escolher codec, encoder ou backend de video, o software deve tentar detectar automaticamente a melhor opcao disponivel para desempenho, compatibilidade e qualidade.
18. O software deve seguir boas praticas de desempenho e seguranca.
19. A interface deve ter aparencia profissional, moderna e confiavel.
20. O executavel deve exibir icone pequeno e grande do aplicativo.
21. O retangulo de selecao deve ter um simbolo central de movimento para facilitar arrastar a area de gravacao.
22. Quando `9x16 (smartphone)` ou `16x9` estiver selecionado, o redimensionamento deve preservar sempre a proporcao escolhida.
23. Deve existir um botao pequeno `...` ao lado do caminho da pasta para alterar a pasta de gravacao.
24. A interface deve mostrar o tempo de gravacao.
25. Deve existir botao para pausar e retomar a mesma gravacao.
26. O MP4 final deve buscar manter som e imagem sincronizados.

## Instalador

1. Deve existir um instalador para distribuicao publica pela internet.
2. O instalador deve instalar tudo que for necessario para o software funcionar corretamente.
3. O instalador deve prever dependencias como DLLs, runtimes, bibliotecas de audio/video, recursos visuais, configuracoes e qualquer outro componente obrigatorio.
4. O usuario final nao deve precisar instalar dependencias manualmente.
5. O instalador deve criar ou garantir a existencia do diretorio padrao `Videos\GTFacil`.
6. O instalador deve ser pensado para uso em computadores Windows de usuarios comuns.

## Premissas tecnicas iniciais

1. O alvo inicial do projeto e Windows.
2. A captura de tela deve priorizar APIs modernas e eficientes do Windows.
3. A captura de audio deve considerar:
   - Audio do sistema.
   - Microfones disponiveis.
   - Combinacao de audio do sistema com microfone, quando selecionado.
4. A gravacao deve evitar travamentos da interface.
5. O processo de gravacao deve rodar de forma segura, com tratamento de erros e mensagens claras para o usuario.
6. O software deve lidar bem com:
   - Monitores com escalas diferentes de DPI.
   - Mais de um monitor.
   - Falta de microfone.
   - Falha ao acessar audio do sistema.
   - Falha ao criar o arquivo de saida.
   - Permissoes insuficientes na pasta de destino.

## Comportamento esperado da interface

1. A tela principal deve conter, no minimo:
   - Botao `Gravar`.
   - Botao `Tamanho`.
   - Botao `Som`.
   - Botao `Abrir`.
2. A interface deve deixar claro quando a gravacao esta parada ou em andamento.
3. Durante a gravacao, o botao `Gravar` pode mudar para uma acao equivalente a parar a gravacao, se essa for a melhor experiencia de uso.
4. A selecao livre deve permitir mover e redimensionar a area escolhida antes de iniciar a gravacao.
5. A selecao livre deve evitar areas invalidas, como largura ou altura igual a zero.
6. As opcoes `9x16 (smartphone)` e `16x9` devem facilitar a criacao de videos em proporcoes fixas.
7. Em modos de proporcao fixa, mover a selecao deve ser permitido, mas redimensionar nao pode deformar o ratio escolhido.
8. O indicador de tempo deve refletir a duracao gravada, desconsiderando o periodo pausado.

## Saida dos arquivos

1. Os arquivos gravados devem ser salvos em `.mp4`.
2. O nome do arquivo deve evitar sobrescrever gravacoes anteriores.
3. Uma sugestao de padrao de nome e:
   - `GTFacil_YYYY-MM-DD_HH-mm-ss.mp4`
4. O diretorio padrao de saida deve ser criado automaticamente caso ainda nao exista.

## Qualidade, desempenho e seguranca

1. A gravacao deve buscar bom equilibrio entre qualidade visual, tamanho do arquivo e desempenho.
2. O software deve evitar consumo excessivo de CPU, memoria e disco.
3. O software deve validar caminhos de arquivo antes de gravar.
4. O software deve tratar excecoes e falhas de APIs externas.
5. O software nao deve coletar, enviar ou armazenar dados pessoais sem necessidade explicita.
6. O software deve evitar execucao de componentes externos inseguros ou nao verificados.

## Pontos a validar durante o desenvolvimento

1. Escolha final da biblioteca ou API de captura de tela.
2. Escolha final da estrategia de encoding para `.mp4`.
3. Escolha final da biblioteca ou API para captura e mixagem de audio.
4. Escolha final do framework de interface grafica em C++.
5. Escolha da ferramenta de instalador.
6. Versoes minimas de Windows suportadas.
7. Necessidade de assinatura digital do instalador para distribuicao publica.
8. Politica de atualizacao do software apos instalado.
