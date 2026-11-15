# SDE AVX Lab — POC AVX1 → SSE

**Status:** prova de conceito experimental, não é runtime de compatibilidade de aplicações.

O protótipo recebe uma única instrução VEX, decodifica uma forma estreita de AVX1 e gera bytes x86-64 que usam somente instruções SSE legacy. Um estado de teste armazena 16 registradores YMM lógicos como dois blocos de 128 bits em memória; o código gerado usa XMM0/XMM1 temporários, carrega operandos, executa a operação SSE e grava o resultado.

## Subconjunto implementado

- `VADDPS`, `VSUBPS`, `VMULPS`, `VDIVPS`, `VXORPS`, `VANDPS` e `VORPS` nas formas VEX.128 e VEX.256, somente registrador-registrador.
- `VMOVUPS` e `VMOVAPS` nas formas registrador-registrador VEX.128/VEX.256. Formas de load/store com memória ainda não são aceitas.
- `VZEROUPPER`, zerando os 128 bits superiores dos 16 registradores YMM virtuais.
- Prefixos VEX2/VEX3, incluindo identificadores de registrador vetorial 8–15 nas formas aceitas.

Formas não aceitas são rejeitadas explicitamente. Não há suporte a AVX2, memória, instruções escalares, saltos, CPUID virtualizado, XGETBV/XSAVE, exceções/flags arquiteturais, threads, DLLs, código JIT de aplicação, processo-filho ou integração DynamoRIO.

## Tradução e estado

O backend atual emite `MOVUPS`, operações packed-single legacy (`ADDPS`, `SUBPS`, `MULPS`, `DIVPS`), operações lógicas legacy e `RET`. Uma operação VEX.256 é dividida em duas operações XMM independentes. Para uma escrita VEX.128, os 128 bits altos do YMM de destino são zerados, conforme a semântica AVX; para `VZEROUPPER`, os altos de todos os 16 YMMs lógicos são zerados. O tradutor não informa CPUID ao software e não intercepta processos.

## Teste no ambiente ARM64

O sandbox de desenvolvimento é Linux ARM64, portanto não executa instruções x86 nativamente. O script cruza o build para x86-64 e roda o binário com QEMU user-mode/Tiny Code Generator, configurado para expor SSE4.2 e ocultar AVX/AVX2/XSAVE. O próprio harness verifica CPUID antes dos casos. O teste de QEMU é uma validação funcional do subset sob emulação x86 sem AVX, não uma medição de desempenho nem um teste no Celeron N5100/Windows 10.

Dependências no Ubuntu: `g++-x86-64-linux-gnu`, `binutils-x86-64-linux-gnu` e `qemu-user`.

```bash
./tests/run-tests.sh
```

O script grava `build/generated_addps256_sse.bin` e sua desmontagem em `build/generated_addps256_sse.disasm`, além da desmontagem do harness. A geração de bytes executáveis usa a sequência W^X: páginas são graváveis durante a cópia e tornam-se executáveis em seguida. Windows x64 usa RCX como primeiro argumento; SysV AMD64 usa RDI.

## Validade e próximos gates

Os testes comparam o estado dos vetores com uma referência escalar para valores controlados e conferem aliasing de registradores, largura 128/256, registradores altos, cópias e `VZEROUPPER`. Também verificam que formas não suportadas são recusadas e que o código gerado/harness não contém mnemônicos AVX.

Isso **não** estabelece equivalência completa da ISA: MXCSR, NaNs, subnormais, exceções FP, arredondamento alternativo, falhas de acesso à memória e todas as formas de instrução ainda precisam de testes dedicados. Tampouco demonstra que DynamoRIO possa carregar AVX em host sem AVX. Antes de expor qualquer capacidade AVX a uma aplicação, seria necessário integrar a transformação em um runtime DBI, emular coerentemente a detecção/estado de CPU, instrumentar todo código executado (inclusive JIT e processos filhos), rejeitar instruções fora da cobertura e passar testes diferenciais e de exceções.

## Arquitetura pesquisada (decisão provisória)

DynamoRIO é o candidato principal de runtime DBI/code-cache para protótipo Windows x64, usando sua IR x86 ou o decoder próprio. Intel XED pode apoiar ferramentas e testes offline, mas não fornece um runtime. Intel SDE deve permanecer como referência externa permitida pelo acordo do kit, não como componente a modificar ou redistribuir. QEMU TCG permanece uma referência de arquitetura de tradução; sua emulação de sistema não equivale a um shim para processos Windows nativos. A recomendação é provisória até provar execução de blocos AVX em host sem AVX sob o framework escolhido.
