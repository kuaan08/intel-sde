# Estudo: por que o Xenia exige AVX

## Conclusão

Anunciar AVX via `CPUID` pode remover a primeira mensagem de erro, mas não torna o Xenia funcional. O código público do Xenia Canary confirma que existem pelo menos três gates independentes: uma verificação estática de CPU antes da inicialização, uma segunda verificação no backend x64 e emissão de código host JIT por Xbyak, com uso de AVX e, quando disponível, AVX2/FMA e outras extensões.

Por isso, o SDE AVX Lab teria de traduzir o código x64 gerado pelo próprio Xenia, além de lidar com as instruções presentes no executável e com páginas de código JIT.

## Evidência no código do Xenia

No arquivo `src/xenia/base/main_init_win.cc`, o Xenia cria `Xbyak::util::Cpu` antes da inicialização e encerra o processo se `cpu.has(Xbyak::util::Cpu::tAVX)` for falso. A mensagem exibida é:

```text
Your CPU does not support AVX, which is required by Xenia.
```

Fonte: [main_init_win.cc](https://github.com/xenia-canary/xenia-canary/blob/master/src/xenia/base/main_init_win.cc)

No arquivo `src/xenia/cpu/backend/x64/x64_backend.cc`, `X64Backend::Initialize` verifica AVX novamente. Quando a verificação falha, registra:

```text
This CPU does not support AVX. The emulator will now crash.
```

Fonte: [x64_backend.cc](https://github.com/xenia-canary/xenia-canary/blob/master/src/xenia/cpu/backend/x64/x64_backend.cc)

No arquivo `src/xenia/cpu/backend/x64/x64_emitter.cc`, o construtor de `X64Emitter` também encerra com erro se AVX não estiver disponível. Esse componente é o emissor JIT do backend x64, portanto a exigência não está limitada ao código de startup.

Fonte: [x64_emitter.cc](https://github.com/xenia-canary/xenia-canary/blob/master/src/xenia/cpu/backend/x64/x64_emitter.cc)

No arquivo `src/xenia/base/platform_amd64.cc`, o Xenia detecta recursos adicionais e cria flags de emissão para AVX2, FMA, BMI, MOVBE, GFNI e AVX-512. A flag `x64_extension_mask` documenta que AVX1 é o piso e que AVX2/FMA são habilitados quando detectados.

Fonte: [platform_amd64.cc](https://github.com/xenia-canary/xenia-canary/blob/master/src/xenia/base/platform_amd64.cc)

## O que o binário analisado mostrou

O release `9eedf60` analisado contém `CPUID` e `XGETBV`, mensagens explícitas de recusa de AVX e milhares de instruções VEX. O inventário inclui operações escalares, packed-single, conversões, comparações, instruções inteiras AVX2, shuffles, packs, blends, `VEXTRACTF128`, `VINSERTF128` e operações de memória.

O perfil estatístico está em [XENIA-CANARY-AVX-INSTRUCTION-PROFILE-2026-11-15.md](XENIA-CANARY-AVX-INSTRUCTION-PROFILE-2026-11-15.md).

## Por que não basta alterar CPUID

Se somente o bit AVX for anunciado, o Xenia passa o primeiro gate, mas o Xbyak ainda produzirá instruções AVX para o código host JIT. Em um Celeron N5100 sem AVX, uma dessas instruções pode causar `STATUS_ILLEGAL_INSTRUCTION` assim que o bloco for executado.

Se também anunciarmos AVX2 ou FMA sem implementá-los, o Xenia poderá selecionar caminhos ainda mais difíceis. A virtualização correta precisa ser condicionada à cobertura efetiva do backend, incluindo `CPUID`, `XGETBV`, estado XMM/YMM, páginas executáveis, threads, exceções e código JIT.

## Estratégia correta para o SDE AVX Lab

O primeiro alvo não deve ser “enganar” o Xenia, e sim construir o dispatcher que recebe os blocos x64 executados pelo processo, identifica VEX/EVEX, traduz as famílias cobertas para SSE/scalar e mantém o estado guest por thread.

A ordem de cobertura para o Xenia deve começar por `VMOV*`, `VADDSS`, `VSUBSS`, `VMULSS`, `VDIVSS`, conversões, comparações e `VZEROUPPER`. Depois devem vir as famílias inteiras AVX2 mais frequentes, como `VPADDD`, `VPAND`, `VPOR`, `VPXOR`, shifts, packs, unpack e `VPSHUFB`. Só depois faz sentido habilitar AVX no processo e medir qual caminho adicional do JIT é alcançado.

O requisito oficial do Xenia também confirma que AVX ou AVX2 é requisito mínimo e que o programa verifica CPU e GPU no startup: [Xenia Canary Quickstart](https://github.com/xenia-canary/xenia-canary/wiki/Quickstart). O FAQ oficial informa que a mensagem de CPU sem AVX significa que o processador não atende aos requisitos: [Xenia Canary FAQ](https://github.com/xenia-canary/xenia-canary/wiki/FAQ).

## Estado atual

O SDE AVX Lab agora possui um primeiro slice de `VMOVSS`, `VADDSS`, `VSUBSS`, `VMULSS` e `VDIVSS`, mas ainda não possui o dispatcher Windows geral necessário para interceptar o JIT do Xenia. Portanto, AVX continua corretamente fechado por padrão.
