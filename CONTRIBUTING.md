# Contribuindo com o GravaTelaFacil

Obrigado por contribuir.

## Fluxo recomendado

1. Abra uma issue ou descreva claramente o problema no pull request.
2. Crie uma branch a partir de `master`.
3. Faca alteracoes pequenas e focadas.
4. Rode as validacoes locais antes de enviar:

```powershell
.\scripts\validate-source.ps1
.\scripts\build-release.ps1
```

## Pull requests

Inclua no PR:

- Resumo do que foi alterado.
- Como foi testado.
- Impacto em instalador, audio, video ou interface, se houver.

Nao envie binarios, downloads locais, builds, instaladores ou arquivos gerados. O instalador deve ser publicado em Releases.
