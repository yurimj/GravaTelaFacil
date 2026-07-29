# Regras permanentes de entrega

Estas regras se aplicam a toda modificacao solicitada neste projeto.

## Build e atalho do Desktop

Antes de considerar qualquer modificacao concluida:

1. Compile a configuracao `Release|x64` executando `scripts\build-release.ps1`.
2. Confirme que a compilacao terminou sem erros e que o executavel atualizado existe em `build\Release\GravaTelaFacil.exe`.
3. Confirme que o atalho `GravaTelaFacil.lnk` no Desktop do usuario aponta para `build\Release\GravaTelaFacil.exe`, usando `build\Release` como diretorio de trabalho.
4. Se o atalho estiver ausente ou com destino incorreto, crie-o ou corrija-o antes da entrega.
5. Execute as validacoes e os autotestes aplicaveis a modificacao.
6. Nao informe que a tarefa foi concluida se a compilacao falhar ou se o atalho nao abrir a versao atualizada.

Ao entregar a modificacao, informe explicitamente o resultado da compilacao e da verificacao do atalho.
