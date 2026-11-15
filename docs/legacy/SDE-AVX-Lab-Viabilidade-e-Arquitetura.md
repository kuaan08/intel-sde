# SDE AVX Lab — viabilidade, arquitetura e prova de conceito

## Situação comprovada do projeto

O handoff foi auditado integralmente: 30 entradas no ZIP principal e cinco arquivos-fonte repetidos no ZIP interno do launcher. Os hashes das cópias internas e externas coincidem. O pacote contém documentação de estado e histórico, cinco scripts BAT, fontes C# e PowerShell do launcher, e uma pasta `logs` sem resultados de execução. O documento de pesquisa compartilhado do projeto também foi lido.

A Intel lista SSE4.2 entre as extensões do Celeron N5100 e não lista AVX para esse modelo, consistente com o alvo descrito no handoff. O hardware físico não estava disponível nesta sessão para teste. [22]

O que já existe é um launcher de conveniência para Intel SDE, scripts de execução/diagnóstico e um registro experimental. Não há, no handoff, decoder AVX, tradutor, backend SSE, cache JIT ou teste automatizado de equivalência. A versão compilada do launcher também não está validada no computador de referência. O resultado concreto mantido como fato é que o usuário abriu a janela do Xenia Canary com Intel SDE 9.58 e perfil `-snb`. Não há evidência de jogo carregado, gameplay, estabilidade ou FPS. O erro `Create pind process failed with code 5` em SDE 10.13.1 segue sem causa comprovada; não se deve atribuí-lo automaticamente ao Windows, às permissões ou à Integridade da Memória.

A Intel registrou nas notas da versão 10.5, publicada em 13/01/2026, que removeu suporte a Windows 10 e Windows Server 2019. Portanto, SDE 10.13.1 não é uma referência suportada para o Windows 10 do laboratório; isso torna o código 5 compatível com um cenário de sistema operacional fora do suporte, mas não prova que essa seja a causa específica. A versão 9.58 que iniciou o Xenia continua sendo a referência experimental relatada. Não foi feita engenharia reversa nem alteração de componentes proprietários do SDE/Pin. [20] [21]

## Viabilidade e escolha de arquitetura

Não foi encontrada uma solução de código aberto pronta que, como produto, execute transparentemente qualquer aplicação Windows x64 AVX/AVX2 em CPU sem AVX. A opção mais plausível para um protótipo de runtime nativo é **DynamoRIO como DBI e cache de blocos**, com transformação app-to-app antes de o bloco entrar no code cache. O decoder/IR x86 do próprio DynamoRIO pode ser o frontend inicial; Intel XED pode ser usado em ferramentas ou testes independentes. A recomendação é provisória: a documentação consultada não garante que DynamoRIO consiga executar, numa CPU sem AVX, todo código AVX capturado, nem que seja compatível com qualquer JIT Windows. Esses dois pontos precisam virar gates experimentais antes de expandir o escopo. [3] [4] [5]

DynamoRIO documenta build e execução Windows x64, transformação de blocos e acompanhamento padrão de processos filhos. Seus callbacks e IR permitem substituir instruções ou inserir operações. Isso faz dele a base de instrumentação mais alinhada ao produto pretendido, mas não implementa a equivalência AVX→SSE: a semântica, o estado vetorial, as exceções e o controle de recursos continuam a cargo do SDE AVX Lab. O suporte a processos filhos de outra bitness exige configuração apropriada, e a transparência tem limitações documentadas. [3] [4]

| Componente | Papel e adequação para o produto | Licença e ressalvas |
|---|---|---|
| **DynamoRIO** | Candidato recomendado para runtime DBI/code cache Windows x64. Não é tradutor AVX pronto. | Núcleo BSD-3-Clause. Há extensões e dependências com outras licenças; auditar tudo que for empacotado. [3] [4] [5] |
| **Intel XED** | Decoder/encoder x86/VEX útil para análise, testes e emissão de instruções; não acompanha processos nem executa blocos. Redundante no caminho quente se for usada a IR do DynamoRIO. | Apache-2.0 para XED; conferir MBuild e dependências efetivamente incluídas. [6] [7] |
| **QEMU TCG** | Tradutor guest→host maduro, adequado como referência de IR/cache/invalidação ou como parte de uma VM Windows completa. Não é um shim para processos Windows nativos; a API pública de plugins é principalmente observacional. O suporte a AVX/AVX2 no QEMU não prova uma saída SSE4.2 específica em toda configuração. [8] [9] [10] [11] | QEMU como um todo é GPL-2.0; arquivos e componentes TCG têm licenças variadas. Reutilização requer auditoria por arquivo e compatibilidade com a forma de distribuição. |
| **Intel Pin / SDE** | Referência experimental. Pin tem DBI e APIs para instrumentação/JIT; SDE já emula instruções, mas não é uma implementação aberta nem um backend AVX→SSE a incorporar. [18] [20] | Runtime Pin/SDE sob termos Intel proprietários. Não modificar nem redistribuir fora da licença exata do kit. [19] [20] |
| **QBDI** | Alternativa de DBI para protótipos controlados; APIs permitem callbacks e pular instruções, mas não há tradutor AVX→SSE pronto nem follow-child/JIT geral confirmado. | Apache-2.0 para o núcleo; auditar LLVM e demais dependências distribuídas. [12] [13] |
| **Frida Gum/Stalker** | Alternativa de instrumentação dinâmica x64 por thread. O relocator pode manter instruções sem traduzi-las; não emula AVX por padrão nem resolve estado YMM. | wxWindows Library Licence 3.1 com exceção, além de licenças próprias para dependências como GLib e Capstone. [14] [15] |
| **LLVM ORC** | Possível backend JIT futuro, depois de existir um lifting semântico do x86 AVX para IR. Não é DBI nem lifter de executáveis nativos. | Apache-2.0 com LLVM Exceptions para LLVM; conferir componentes de terceiros. [16] [17] |

A arquitetura-alvo recomendada é composta por:

1. **Runtime DBI:** DynamoRIO transforma blocos dinâmicos, identifica instruções suportadas, acompanha mudanças de código e controla execução no code cache. Antes de adotá-lo, provar que o runtime carrega no Windows 10 x64 de referência e que um bloco que contém AVX pode ser interceptado antes de executar a instrução original numa CPU sem AVX.
2. **Decoder e classificação:** começar com a IR x86 do DynamoRIO; manter Intel XED como opção para um utilitário independente de decode/encode. Cada instrução precisa ser classificada por opcode, largura, forma de operandos, registradores e efeitos. XED é componente de decode/encode, não tradutor. [3] [6]
3. **IR semântica própria:** representar operação, operandos, largura, comportamento de zeroing, acesso à memória, estado MXCSR e requisitos de exceção. Não tratar o opcode como texto nem assumir que uma troca de prefixo seja suficiente.
4. **Backend SSE:** emitir instruções host realmente suportadas. Uma operação AVX de 256 bits pode ser dividida em duas de 128 bits quando a semântica permite; operações com permutações, cruzamento de lanes, máscaras ou gather podem exigir sequências maiores ou fallback escalar. O AVX2 deve ficar fora do perfil anunciado até haver cobertura por instrução e testes.
5. **Estado vetorial por thread:** manter as 16 YMMs lógicas completas. Uma representação inicial em memória/TLS é simples, mas tem custo alto; otimizar armazenamento em XMMs físicos apenas depois de testes de aliasing, chamadas, callbacks e preempção. O programa deve observar estado coerente após cada instrução e em exceções.
6. **Gerência de capacidades:** CPUID, XGETBV, OSXSAVE e estado XSAVE precisam concordar com o subconjunto traduzido. Não anunciar AVX/AVX2 antes de o caminho correspondente cobrir toda instrução que o processo possa executar. A especificação VEX diferencia fonte não destrutiva, largura de vetor e o zero dos bits altos em escrita VEX.128; esses efeitos fazem parte do contrato, não são detalhe opcional. [1]
7. **Código dinâmico e processos:** acompanhar blocos JIT, invalidar o cache quando o código muda, cobrir módulos/DLLs e processos filhos e sincronizar threads. Para aplicações protegidas, antitrapaça ou componentes de kernel, documentar incompatibilidade; não contornar mecanismos de segurança.

QEMU TCG é uma referência técnica valiosa porque traduz blocos guest→host e mantém mecanismos de cache/invalidação; sua emulação de sistema é uma alternativa diferente, com Windows inteiro em VM, não uma camada leve de compatibilidade por aplicação. A documentação do user-mode lista Linux e BSD como sistemas operacionais suportados e descreve tradução de chamadas de sistema; portanto, esse modo não é um executor de PE/WinAPI. LLVM ORC pode ser reavaliado quando existir IR apropriada; incluí-lo agora acrescentaria uma camada JIT sem resolver captura ou lifting do x86. [8] [9] [16] [23]

## Prova de conceito AVX1 → SSE executada

Foi criado `avx1-sse-poc`, um conversor de instrução isolada. O decoder experimental reconhece formas VEX2/VEX3 de registrador-registrador para `VADDPS`, `VSUBPS`, `VMULPS`, `VDIVPS`, `VXORPS`, `VANDPS`, `VORPS`, `VMOVUPS`, `VMOVAPS` e `VZEROUPPER`. Para VEX.256, o backend executa duas operações XMM legacy. Para VEX.128, zera a metade superior do YMM virtual de destino. `VZEROUPPER` zera as metades superiores dos 16 YMMs virtuais. O estado lógico YMM é mantido como 16 vetores de 256 bits em memória.

O escopo deliberadamente não inclui operandos de memória AVX, AVX2, estado CPUID/XGETBV em aplicações, exceções arquiteturais, integração DynamoRIO, threads de aplicações, processos filhos ou compatibilidade com jogos. `VMOVUPS/VMOVAPS` foram testados apenas na forma registrador-registrador. Formas não suportadas são recusadas, não executadas como se fossem traduzidas. Os bytes VEX literais de fixtures assembly foram montados e conferidos com GNU assembler/objdump. O código emitido foi desmontado e não contém mnemônicos AVX; no caso de `VADDPS` 256-bit, o dump de 49 bytes mostra dois pares de `MOVUPS` mais `ADDPS` e `RET`.

**Resultado de execução:** 40 verificações passaram. O harness foi compilado como x86-64 com GCC cross 13.3 e CMake 3.28.3, e executado por QEMU user-mode/Tiny Code Generator 8.2.2 no sandbox Linux ARM64. QEMU foi configurado com `-cpu max,-avx,-avx2,-xsave`. O próprio programa leu CPUID e confirmou `SSE4.2=yes`, `AVX=no`, `AVX2=no`, `OSXSAVE=no` antes de executar. Os resultados de estado foram comparados byte a byte contra uma referência escalar em valores controlados. O build direto e o build CMake cross foram testados.

Esse resultado é evidência de que **este subset controlado** pode ser convertido e executado como código SSE no modelo x86 emulado sem AVX. Não é execução nativa no Celeron N5100, teste no Windows 10, medição de desempenho, equivalência completa de ponto flutuante nem demonstração de que DynamoRIO execute código AVX de terceiros numa CPU sem AVX. O ambiente desta sessão é ARM64; ainda faltam compilação e teste no Windows x64 e no hardware de referência. A especificação Intel e o Intrinsics Guide continuam sendo as bases para ampliar a validação; os testes atuais não cobrem NaNs, subnormais, diferentes modos MXCSR, exceções FP ou faults de memória. [1] [2]

## Próximos gates de engenharia

1. **Prova de runtime DBI:** compilar DynamoRIO/cliente para Windows x64 e testar num executável próprio que contém VEX AVX1. A condição de saída é interceptar e substituir a instrução antes de ocorrer #UD em host sem AVX; não basta decodificar o bloco ou abrir o processo.
2. **Decoder de produção:** substituir ou comparar o parser manual da POC com DynamoRIO/XED; incluir operandos de memória, endereço RIP-relative, limites de página e instruções VEX de 2/3 bytes. O decoder manual atual é apenas didático.
3. **Semântica e validação diferencial:** comparar cada instrução/formato com hardware AVX real e, quando apropriado, SDE permitido pela licença. Acrescentar casos de NaN, infinito, subnormais, precisão, rounding, MXCSR, faults, alinhamento e aliasing; cobrir ambos os lados de vetores de 256 bits.
4. **Estado e capacidades:** implementar estado YMM por thread e interceptação coerente de CPUID/XGETBV/XSAVE antes de expor AVX. A primeira versão do runtime deve anunciar somente o subset efetivamente implementado e falhar de forma diagnóstica em instruções ausentes.
5. **Código JIT/processos:** validar cache/invalidação, geração de código concorrente, DLLs, chamadas/retornos, processos filhos e exceções no Windows. Depois, construir matriz de compatibilidade por aplicação.
6. **Desempenho:** medir custo da tradução e cache separadamente de um jogo/emulador. Não prometer FPS do Xenia ou suporte universal até haver testes repetíveis; o teste de abertura registrado no SDE não equivale a gameplay.

O objetivo de compatibilidade ampla é tecnicamente investigável, mas permanece um objetivo, não uma capacidade atual. A primeira prova útil do runtime deverá ser um programa de teste AVX independente executado com sucesso em Windows x64 sem AVX físico. Só depois faz sentido testar o Xenia, jogos e outros aplicativos.

## Referências

[1]: https://cdrdv2-public.intel.com/843829/325383-sdm-vol-2abcd-dec-24.pdf "Intel 64 and IA-32 Architectures Software Developer Manual, Volume 2"
[2]: https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html "Intel Intrinsics Guide"
[3]: https://dynamorio.org/API_BT.html "DynamoRIO Code Manipulation API"
[4]: https://dynamorio.org/page_deploy.html "DynamoRIO How to Run and Deployment"
[5]: https://dynamorio.org/page_license.html "DynamoRIO License"
[6]: https://intelxed.github.io/ "Intel XED Documentation"
[7]: https://github.com/intelxed/xed/blob/main/LICENSE "Intel XED License"
[8]: https://www.qemu.org/docs/master/devel/tcg.html "QEMU TCG Translator Internals"
[9]: https://www.qemu.org/docs/master/devel/tcg-plugins.html "QEMU TCG Plugins"
[10]: https://github.com/qemu/qemu/blob/master/LICENSE "QEMU License"
[11]: https://www.qemu.org/2022/12/14/qemu-7-2-0/ "QEMU 7.2 Release Notes"
[12]: https://qbdi.readthedocs.io/en/stable/api_description.html "QBDI API Description"
[13]: https://github.com/QBDI/QBDI/blob/dev-next/LICENSE.txt "QBDI License and Dependency Notices"
[14]: https://frida.re/docs/stalker/ "Frida Stalker Documentation"
[15]: https://github.com/frida/frida-gum/blob/main/COPYING "Frida Gum License"
[16]: https://llvm.org/docs/ORCv2.html "LLVM ORC Design and Implementation"
[17]: https://llvm.org/LICENSE.txt "LLVM License"
[18]: https://software.intel.com/sites/landingpage/pintool/docs/99977/Pin/doc/html/index.html "Intel Pin User Guide"
[19]: https://software.intel.com/sites/landingpage/pintool/intel-simplified-software-license.txt "Intel Pin Simplified Software License"
[20]: https://www.intel.com/content/www/us/en/developer/articles/tool/software-development-emulator.html "Intel Software Development Emulator"
[21]: https://www.intel.com/content/www/us/en/developer/articles/release-notes/intel-software-development-emulator-release-notes.html "Intel SDE Release Notes"
[22]: https://www.intel.com/content/www/us/en/products/sku/212329/intel-celeron-processor-n5100-4m-cache-up-to-2-80-ghz/specifications.html "Intel Celeron N5100 Specifications"
[23]: https://www.qemu.org/docs/master/user/main.html "QEMU User Space Emulator"
