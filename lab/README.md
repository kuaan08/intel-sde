# SDE AVX Lab — integração com o Intel SDE

O repositório agora mantém duas distribuições separadas do kit Intel SDE:

- `windows-9.58/`: distribuição oficial Windows `sde-external-9.58.0-2025-06-16-win.tar.xz`, baixada do espelho oficial da Intel e validada pelo SHA-256 publicado pela Intel.
- `linux-9.14/`: kit Linux 9.14.0 preservado como referência histórica; ele não é mais a raiz principal do repositório.

Os binários, bibliotecas, fontes Pin, arquivos `.sig` e licenças Intel permanecem sem alteração. Os arquivos em `lab/` são adições do SDE AVX Lab e não modificam nem ampliam a licença do kit.

## Auditoria

A auditoria padrão aponta para Windows 9.58:

```bash
./lab/sde-kit-audit.sh | tee lab/sde-kit-audit.log
```

Para auditar a referência Linux preservada:

```bash
KIT_ROOT="$PWD/linux-9.14" ./lab/sde-kit-audit.sh
```

No sandbox ARM64, a auditoria executa apenas metadados das ferramentas Linux sob QEMU. Ela não tenta executar `.exe` Windows. Em Windows x64, a verificação deve ser repetida nativamente com `windows-9.58\\sde.exe -version`.

## O que a versão Windows contém

A distribuição Windows inclui `sde.exe`, `xed.exe`, `intel64/pin.exe`, `intel64/sde-mix-mt.dll`, DLLs de suporte, documentação HTML, perfis CPUID e as pastas `ia32` e `intel64`. A versão identificada no binário é **9.58.0 external**. Os perfis incluem, entre outros, `snb`, `ivb`, `hsw`, `skl`, `glm`, `tnt`, `adl`, `tgl`, `spr` e `future`.

O SDE oferece knobs de emulação e análise, como seleção de chip, CPUID emulado, `-mix`, detecção AVX/SSE e opções de precisão/velocidade para algumas famílias de instruções. Esses modos são recursos do SDE Intel, não um backend AVX→SSE do SDE AVX Lab. A POC independente permanece necessária para gerar código SSE executável no Celeron N5100.

## Organização Windows recomendada

No computador Windows 10 x64 do laboratório, extraia a distribuição oficial preservando toda a estrutura. Use a variante de 64 bits para aplicações x64:

```bat
windows-9.58\\sde.exe -version
windows-9.58\\sde.exe -snb -- "C:\\caminho\\para\\programa.exe"
```

Para o Xenia, o perfil `-snb` continua sendo a primeira referência porque corresponde ao experimento registrado no handoff. O resultado deve ser anotado separadamente: inicialização, menu, jogo carregado, FPS, erros e logs. Abrir a janela do processo não prova compatibilidade de gameplay.

Antes do teste, mantenha `windows-9.58` intacta, não misture DLLs com outra versão, confira o Visual C++ Redistributable exigido pela documentação e preserve os logs gerados pelo SDE. A versão Windows 9.58 é a referência correta para o teste Windows 10 do laboratório; o kit Linux 9.14 não deve ser usado para esse teste.

## Sobre “versão otimizada”

O SDE não fornece uma versão otimizada que transforme automaticamente todas as instruções AVX em SSE nativo para obter FPS de jogo. O que existe são builds por plataforma/bitness, perfis de chip emulados, knobs de emulação e ferramentas de análise. A escolha recomendada para o computador de referência é o `sde.exe`/`intel64` da distribuição Windows e o perfil de CPU mínimo necessário, começando por `-snb`. FMA, AVX2 ou perfis mais novos só devem ser usados quando o programa exigir e depois de medir o custo.

## Engenharia reversa

Não será feita engenharia reversa, descompilação, desmontagem ou patching dos binários Intel. A licença incluída no kit proíbe essas ações. O trabalho permitido e útil para o projeto é inspeção de documentação, hashes, metadados, perfis CPUID, testes externos e desenvolvimento de um tradutor independente. A integração com o SDE ocorre como referência/oráculo experimental, nunca como código proprietário modificado.

## Próxima etapa

No Windows x64 nativo, executar primeiro `sde.exe -version`, guardar `-help`/`-help-long` e repetir o teste `-snb` do handoff. Em seguida, usar `-mix` e o relatório AVX/SSE para descobrir quais instruções o aplicativo realmente usa. O cliente Pin/SDE de observação será compilado separadamente e só depois será comparado à POC AVX1→SSE do projeto principal.
