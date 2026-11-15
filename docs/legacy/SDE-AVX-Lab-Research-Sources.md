# SDE AVX Lab — Research Sources Pack

**Purpose:** importar este documento como fonte de pesquisa no Manus para o projeto SDE AVX Lab.

**Escopo:** pesquisa e desenvolvimento de um tradutor binário dinâmico independente capaz de executar, em CPUs sem AVX físico, um subconjunto de instruções AVX1 e futuramente AVX2, traduzindo-as para instruções compatíveis com o hardware hospedeiro (inicialmente SSE4.2).

**Plataforma de referência:** Windows 10 x64, Intel Celeron N5100, 8 GB RAM, Intel UHD Graphics. O fabricante lista SSE4.2 como extensão de instruções do N5100; não lista AVX. Fonte: [Intel — Celeron N5100 specifications](https://www.intel.com/content/www/us/en/products/sku/212329/intel-celeron-processor-n5100-4m-cache-up-to-2-80-ghz/specifications.html).

> **Instrução ao Manus:** use os links abaixo como fontes primárias. Acesse os READMEs, documentação, código, exemplos, issues relevantes e arquivos de licença. Não assuma que uma biblioteca pode ser incorporada só porque seu código está publicamente disponível. Registre as licenças por componente. Diferencie fatos verificados, hipóteses e propostas. Não tente modificar binários proprietários do Intel SDE/Pin; investigue uma implementação independente e licenciada.

---

## A. Fontes prioritárias — instrumentação e tradução dinâmica

### A1. DynamoRIO — Dynamic Instrumentation Tool Platform
- **Repositório:** https://github.com/DynamoRIO/dynamorio
- **Documentação oficial:** https://dynamorio.org/
- **O que investigar:** transformação de código em tempo de execução; instrumentação de aplicações x86/AMD64; manipulação de instruções; geração de ferramentas customizadas; Windows x64; cache e gerenciamento de blocos de código.
- **Por que importa:** é uma base aberta para estudar interceptação e transformação dinâmica de instruções em aplicações já compiladas.
- **Licença:** o repositório informa que o código-fonte é primariamente BSD; verificar `License.txt` e as licenças dos componentes usados antes de redistribuir.

### A2. QEMU — emulador e infraestrutura TCG
- **Repositório:** https://github.com/qemu/qemu
- **Documentação de desenvolvimento TCG:** https://www.qemu.org/docs/master/devel/tcg.html
- **Documentação geral de emulação:** https://www.qemu.org/docs/master/about/emulation.html
- **Licenças:** https://github.com/qemu/qemu/blob/master/LICENSE
- **O que investigar:** Tiny Code Generator (TCG); Translation Blocks; intermediate representation; geração de código de host; encadeamento e invalidação de blocos; tratamento de exceções; gerenciamento de estado de CPU.
- **Por que importa:** oferece uma referência madura de tradução dinâmica de instruções e geração de código. Avaliar se conceitos ou componentes específicos são reutilizáveis sem obrigar o projeto a executar um sistema operacional convidado completo.
- **Licença:** QEMU como um todo é GPL-2.0; TCG é composto por arquivos com licenças que podem variar (incluindo BSD/MIT e outras). Auditar arquivo por arquivo antes de copiar código.

### A3. Intel Pin — Dynamic Binary Instrumentation
- **Página oficial:** https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-dynamic-binary-instrumentation-tool.html
- **Downloads:** https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-binary-instrumentation-tool-downloads.html
- **O que investigar:** arquitetura de instrumentação dinâmica; Pintools; instrumentação de executáveis sem recompilação; aplicações que geram código dinamicamente; callbacks e contexto de registradores.
- **Por que importa:** o Intel SDE é construído sobre Pin. Estudar a documentação ajuda a compreender a arquitetura geral, mas não autoriza modificar ou redistribuir componentes proprietários.
- **Licença:** ler a licença específica do kit Pin antes de qualquer uso ou distribuição.

---

## B. Decodificação e codificação x86 — AVX/VEX

### B1. Intel XED — X86 Encoder Decoder
- **Repositório:** https://github.com/intelxed/xed
- **Documentação/API:** https://intelxed.github.io/
- **MBuild (sistema de build usado pelo XED):** https://github.com/intelxed/mbuild
- **O que investigar:** decodificação x86/IA-32 e Intel 64; instruções VEX; opcode, operandos, largura vetorial, registradores e modos de endereçamento; encoder para instruções de saída.
- **Por que importa:** componente-chave para reconhecer com precisão instruções AVX e produzir instruções x86 de destino.
- **Licença:** o repositório XED indica Apache-2.0. Confirmar também a licença e os avisos do MBuild e dos artefatos distribuídos.

### B2. Intel Intrinsics Guide
- **Guia online:** https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html
- **O que investigar:** semântica dos intrinsics SSE, AVX e AVX2; tipos vetoriais; requisitos de ISA; intrinsics correspondentes às operações que serão traduzidas.
- **Por que importa:** ajuda a relacionar operações de alto nível às instruções e às extensões necessárias. Não substitui o manual de arquitetura para casos de borda.

### B3. Intel 64 and IA-32 Architectures Software Developer Manuals (SDM)
- **Página oficial:** https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
- **O que investigar:** descrição arquitetural de cada instrução; codificação VEX; semântica de operandos; flags; exceções; MXCSR; transições XMM/YMM; XSAVE/XRSTOR; CPUID e XGETBV.
- **Por que importa:** é a fonte normativa para implementar equivalência funcional. A tradução precisa preservar a semântica, não apenas produzir resultados aproximados em casos comuns.

---

## C. Intel SDE — referência de comportamento, não base para patch proprietário

### C1. Intel Software Development Emulator (SDE)
- **Página oficial e documentação:** https://www.intel.com/content/www/us/en/developer/articles/tool/software-development-emulator.html
- **Downloads:** disponíveis a partir da página oficial.
- **O que investigar:** papel do SDE; opções de perfis de CPU; ferramentas `-mix`, `-debugtrace`, `-footprint` e `-ast`; relação entre SDE, Pin e XED; comportamento de CPUID e recursos emulados.
- **Estado conhecido do laboratório:** Intel SDE 9.58.0 foi instalado em `C:\Sde\sde.exe`. No computador de referência, `C:\Sde\sde.exe -snb -- "...\xenia_canary.exe"` abriu o Xenia Canary. Isso confirma inicialização sob SDE, não compatibilidade de jogo nem FPS jogável.
- **Limite:** não desmontar, descompilar, modificar nem redistribuir binários proprietários do SDE/Pin fora do que a licença aplicável permitir. Use-o como referência experimental e ferramenta de comparação.

### C2. Intel SDE release/current information
- **Fonte:** https://www.intel.com/content/www/us/en/developer/articles/tool/software-development-emulator.html
- **Nota:** a página oficial indicava SDE 10.13.1 em 28/07/2026. O projeto de referência teve erro `Create pind process failed with code 5` nessa versão no Windows 10; a versão 9.58.0 iniciou o Xenia no teste relatado. Não generalizar esse resultado sem reproduzir.

---

## D. Aplicação-alvo e requisitos de compatibilidade

### D1. Xenia Canary
- **Repositório:** https://github.com/xenia-canary/xenia-canary
- **Quickstart/requisitos atuais:** https://github.com/xenia-canary/xenia-canary/wiki/Quickstart
- **FAQ:** https://github.com/xenia-canary/xenia-canary/wiki/FAQ
- **O que investigar:** backend CPU/JIT; verificações de recursos de CPU; caminho de inicialização; dependência de AVX/AVX2; como registrar logs; compatibilidade por jogo.
- **Aviso importante:** o Quickstart atual lista CPU x86-64 com AVX ou AVX2 como requisito mínimo. Remover uma checagem de CPUID não garante que as instruções ausentes sejam executadas corretamente. A abertura da janela do emulador não comprova funcionamento de jogos.

### D2. Xenia original (referência adicional)
- **Repositório:** https://github.com/xenia-project/xenia
- **Quickstart:** https://github.com/xenia-project/xenia/wiki/Quickstart
- **O que investigar:** comparar a arquitetura original com Canary e identificar diferenças no backend x64/JIT, requisitos, verificações e estratégias de compilação.

### D3. Intel Celeron N5100 — especificação oficial
- **Página Intel:** https://www.intel.com/content/www/us/en/products/sku/212329/intel-celeron-processor-n5100-4m-cache-up-to-2-80-ghz/specifications.html
- **O que verificar:** extensões de instruções listadas pelo fabricante, número de núcleos/threads, frequência e recursos de memória.
- **Dado de referência:** Intel lista `Intel® SSE4.2` como Instruction Set Extensions. Usar essa página como autoridade para o hardware-alvo.

---

## E. Pesquisa técnica complementar

### E1. Intel X86 Assembly / arquitetura
- **Intel SDM (fonte principal):** https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
- **Intel Intrinsics Guide:** https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html
- **Uso:** confirmar semântica de instruções, operandos, tamanhos de vetor, comportamento de precisão e condições de exceção.

### E2. Recursos de compilação e teste
- **CMake:** https://cmake.org/documentation/
- **GoogleTest:** https://github.com/google/googletest
- **LLVM ORC JIT (opção a avaliar):** https://llvm.org/docs/ORCv2.html
- **LLVM:** https://github.com/llvm/llvm-project
- **Uso:** estudar build multiplataforma Windows, testes unitários e geração de código JIT. Avaliar cuidadosamente o custo e o suporte a instruções x86 de destino antes de escolher LLVM como backend.

### E3. Validação de instruções e benchmarks
- Criar programas de teste pequenos e isolados para cada instrução AVX implementada.
- Comparar resultados com hardware AVX real ou uma referência independente confiável.
- Medir latência, throughput, instruções geradas, tempo de tradução, tempo de execução e custo de transições.
- Testar NaNs, infinitos, valores subnormais, arredondamento, alinhamento, memória não alinhada e exceções.
- Não usar FPS do Xenia como primeiro teste de correção do tradutor.

---

## F. Ordem recomendada de leitura e investigação

1. **Intel SDM + Intrinsics Guide:** compreender exatamente a semântica das instruções AVX1 e SSE4.2.
2. **Intel XED:** provar que instruções VEX/AVX são corretamente decodificadas e entender como gerar instruções de saída.
3. **DynamoRIO:** criar uma prova de conceito de instrumentação e transformação de instruções em processo real no Windows.
4. **QEMU TCG:** estudar IR, Translation Blocks, geração de código e invalidação de cache.
5. **Intel Pin/SDE:** entender o desenho do mecanismo de referência e usar o SDE 9.58 como comparação experimental permitida pela licença.
6. **Xenia Canary:** somente depois dos testes unitários, testar o comportamento com código JIT e uma aplicação real.

---

## G. Primeira prova de conceito (POC) proposta

**Objetivo inicial:** não implementar um emulador completo. Implementar um conversor offline de instruções AVX1 para sequências SSE equivalentes, com um pequeno executor de teste ou harness controlado.

### Etapa G1 — decodificador
- Usar Intel XED para decodificar instruções x86-64.
- Identificar instruções VEX e registrar opcode, operandos e largura.
- Criar corpus de testes e saídas legíveis.

### Etapa G2 — subconjunto AVX1
Começar por um grupo pequeno e bem especificado, por exemplo:
- `VADDPS`
- `VSUBPS`
- `VMULPS`
- `VXORPS`
- `VANDPS`
- `VORPS`
- `VMOVUPS`
- `VMOVAPS`
- `VZEROUPPER`

Não presumir que toda instrução terá uma substituição SSE simples. Algumas exigem múltiplas operações, temporários, operações escalares ou tratamento especial.

### Etapa G3 — estado arquitetural
Documentar e testar:
- XMM/YMM e comportamento das metades superiores;
- semântica de VEX.128 e VEX.256;
- alinhamento e acesso à memória;
- MXCSR e arredondamento;
- flags e exceções relevantes;
- CPUID/XGETBV apenas no ambiente de execução controlado.

### Etapa G4 — geração e execução
- Gerar código x86-64 que use apenas recursos suportados pelo host.
- Auditar o código gerado para garantir que nenhuma instrução AVX seja emitida.
- Executar em CPU sem AVX e comparar resultados com referência.

### Etapa G5 — instrumentação dinâmica
- Avaliar integração via DynamoRIO ou implementação DBT própria.
- Dar atenção especial a código JIT gerado dinamicamente, saltos, chamadas, exceções, threads e invalidação de blocos.
- Não expor AVX no CPUID ao programa-alvo antes de garantir cobertura e execução correta das instruções correspondentes.

---

## H. Perguntas de pesquisa que o Manus deve responder

1. O DynamoRIO permite interceptar e reescrever todas as instruções AVX em aplicações Windows x64, inclusive código gerado dinamicamente?
2. Qual é a forma correta de manter a coerência entre CPUID, XGETBV, XSAVE/XRSTOR e o estado XMM/YMM?
3. Quais instruções AVX1 podem ser traduzidas diretamente para SSE4.2 e quais exigem sequências mais longas?
4. Quais operações AVX2 podem ser implementadas com SSE4.2, e qual é o custo provável?
5. Como lidar com instruções que possuem semântica de exceção, arredondamento, máscara ou acesso à memória especial?
6. O melhor caminho é construir um cliente DynamoRIO, estender um DBT existente ou escrever um tradutor próprio?
7. Quais componentes podem ser reutilizados legalmente em um produto distribuível?
8. Como instrumentar código JIT do próprio Xenia sem corromper seu mecanismo de geração de código?
9. Quais benchmarks isolados permitem comparar o tradutor com o Intel SDE sem usar jogos como teste inicial?
10. Qual é o overhead mínimo plausível no Celeron N5100 e quais gargalos vêm do DBT, da CPU do Xbox 360 simulada e da GPU?

---

## I. Regras de qualidade do projeto

- Distinguir **verificado**, **inferido** e **proposto** em toda documentação.
- Não alegar suporte completo a AVX1/AVX2 sem testes de cobertura e equivalência.
- Não falsificar CPUID como substituto de emulação real.
- Não remover verificações de compatibilidade esperando que isso resolva a ausência física de AVX.
- Não prometer desempenho de jogos antes de benchmarks reprodutíveis.
- Preservar estado do processador, exceções e semântica de memória.
- Auditar licenças antes de copiar, modificar, vincular ou redistribuir qualquer componente.
- Priorizar um POC pequeno, reproduzível e testável antes de integrar o Xenia.

---

## J. Lista compacta de links para importação

```text
DynamoRIO GitHub: https://github.com/DynamoRIO/dynamorio
DynamoRIO Docs: https://dynamorio.org/
QEMU GitHub: https://github.com/qemu/qemu
QEMU TCG Docs: https://www.qemu.org/docs/master/devel/tcg.html
QEMU Emulation Docs: https://www.qemu.org/docs/master/about/emulation.html
QEMU License: https://github.com/qemu/qemu/blob/master/LICENSE
Intel XED GitHub: https://github.com/intelxed/xed
Intel XED Docs: https://intelxed.github.io/
Intel MBuild: https://github.com/intelxed/mbuild
Intel Pin: https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-dynamic-binary-instrumentation-tool.html
Intel Pin Downloads: https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-binary-instrumentation-tool-downloads.html
Intel SDE: https://www.intel.com/content/www/us/en/developer/articles/tool/software-development-emulator.html
Intel SDM: https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
Intel Intrinsics Guide: https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html
Xenia Canary: https://github.com/xenia-canary/xenia-canary
Xenia Canary Quickstart: https://github.com/xenia-canary/xenia-canary/wiki/Quickstart
Xenia Canary FAQ: https://github.com/xenia-canary/xenia-canary/wiki/FAQ
Xenia original: https://github.com/xenia-project/xenia
Xenia original Quickstart: https://github.com/xenia-project/xenia/wiki/Quickstart
Intel Celeron N5100 Specs: https://www.intel.com/content/www/us/en/products/sku/212329/intel-celeron-processor-n5100-4m-cache-up-to-2-80-ghz/specifications.html
CMake Docs: https://cmake.org/documentation/
GoogleTest: https://github.com/google/googletest
LLVM ORC JIT Docs: https://llvm.org/docs/ORCv2.html
LLVM Project: https://github.com/llvm/llvm-project
```

**Fim do pacote de fontes.**
