# Engenharia reversa do Intel SDE e plano de otimização do SDE AVX Lab

## Decisão sobre a versão Windows

A distribuição `windows-9.58/` é um pacote proprietário oficial da Intel. A licença incluída em `windows-9.58/Licenses/LICENSE.pdf` determina que o software pode ser usado e redistribuído somente dentro dos termos da licença e declara:

> No reverse engineering, decompilation, or disassembly of this Software is permitted.

Por isso, o SDE AVX Lab não fará engenharia reversa, descompilação, desmontagem, extração de pseudocódigo, patching ou cópia de implementação dos binários `sde.exe`, `xed.exe`, `pin.exe` ou DLLs do pacote Windows. Nenhum código derivado desses binários será colocado no repositório.

A versão Windows permanece intacta e pode ser usada, conforme a licença, para executar testes autorizados, observar logs documentados, comparar resultados e medir comportamento externo. Isso não transforma o SDE em parte do tradutor independente.

## O que pode ser usado legitimamente

- documentação HTML e README distribuídos com o pacote;
- opções públicas de execução, como perfis de chip, `-mix` e relatórios AVX/SSE;
- hashes, versões, metadados e perfis CPUID;
- resultados observáveis de testes próprios, sem copiar implementação interna;
- XED ou outro decodificador com licença compatível, integrado ao código independente;
- testes de equivalência usando referências matemáticas e execução nativa autorizada;
- código original desenvolvido no diretório `avx1-sse-poc`.

## Plano de otimização independente

### 1. Medir antes de otimizar

No Windows x64, executar a aplicação-alvo com o SDE 9.58 e guardar:

```bat
windows-9.58\sde.exe -version
windows-9.58\sde.exe -snb -mix -mix_format json -- "C:\caminho\programa.exe"
```

Registrar o perfil usado, duração, saída `sde-mix`, instruções AVX observadas, número de threads, falhas e estado funcional. O relatório serve somente como telemetria externa; não é código do SDE.

### 2. Completar o decoder próprio

Implementar no projeto independente:

- prefixos VEX de dois e três bytes;
- `VEX.L` para largura 128/256;
- `vvvv`, `R`, `X`, `B`, opcode, `modrm`, `sib` e displacement;
- operações de registrador, memória e broadcast relevantes;
- rejeição explícita de AVX2/AVX-512 ainda não implementados;
- testes com fixtures de bytes montados independentemente.

O decoder deve produzir uma IR própria. Nenhum opcode deve ser reconhecido apenas por strings ou heurísticas frágeis.

### 3. Backend AVX1→SSE4.2

Priorizar o subconjunto já definido pelo projeto:

- `VADDPS`, `VSUBPS`, `VMULPS`, `VDIVPS`;
- `VMOVUPS`, `VMOVAPS`;
- `VXORPS`, `VANDPS`, `VORPS`;
- `VZEROUPPER`.

Para operações 256-bit, gerar duas operações SSE de 128-bit, preservando a ordem das metades e o estado arquitetural. Para instruções VEX.128, garantir que a metade superior correspondente seja zerada conforme a semântica VEX.

### 4. Estado e transições

Manter uma representação explícita de XMM/YMM no runtime próprio. O backend deve tratar:

- zero da metade superior em instruções VEX.128;
- `VZEROUPPER` como operação de estado, não como NOP universal;
- MXCSR e arredondamento;
- loads/stores não alinhados;
- aliasing entre memória e registradores;
- chamadas auxiliares somente quando a sequência SSE não for suficiente.

### 5. Cache e desempenho

Depois da equivalência funcional:

- cachear blocos traduzidos por endereço, bytes e permissões da página;
- invalidar blocos quando código JIT for alterado;
- usar fast path para blocos sem instruções AVX;
- separar blocos cold e hot;
- especializar formas register-register antes de formas com memória;
- eliminar `VZEROUPPER` redundante somente após análise de dependências;
- reduzir chamadas ao runtime e manter o estado vetorial em registradores host quando seguro;
- medir taxa de acerto do cache, custo de decode e custo por instrução.

Nenhuma otimização pode ser aceita sem comparação bit a bit ou tolerância formalmente justificada para o subconjunto testado.

### 6. Validação

A matriz mínima deve comparar:

1. referência escalar independente;
2. execução SSE nativa;
3. execução da sequência traduzida;
4. execução AVX em máquina compatível, quando disponível;
5. estados de registradores e flags relevantes;
6. casos normais, NaN, infinito, zero com sinal, overflow, underflow e alinhamentos variados.

O SDE Windows pode ajudar como referência externa de execução, mas não é considerado implementação de referência suficiente para justificar diferenças de precisão.

## Critério de conclusão

O projeto poderá declarar uma otimização concluída somente quando houver:

- fonte original no repositório;
- testes reproduzíveis;
- disassembly do código gerado comprovando ausência de AVX no backend SSE;
- equivalência funcional no subconjunto coberto;
- benchmark separado para decode, tradução, cache e execução;
- documentação da CPU host e das limitações.

O objetivo continua sendo uma implementação independente de tradução dinâmica, não uma cópia do Intel SDE.
