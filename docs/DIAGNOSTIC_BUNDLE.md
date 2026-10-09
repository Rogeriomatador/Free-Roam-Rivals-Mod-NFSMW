# Diagnóstico amplo — v0.0.38-dev

Este pacote observa somente áreas para as quais já existem probes e contratos
verificados no projeto. Não é um varredor que entende todo o jogo nem uma
prova automática de que o rival funciona. Pesquisa e testes offline não
substituem executar o NFSMW no PC do usuário.

## Um interruptor e uma captura

1. Instale a v38 e preserve os INIs editados.
2. Na seção existente `[Diagnostics]` de
   `scripts/FreeRoamRivals/FreeRoamRivals.ini`, adicione:

   ```ini
   DiagnosticBundleEnabled=1
   ```

3. Reinicie o jogo. Entre no mundo, preferencialmente no Free Roam sem
   perseguição, dirija um pouco e pressione **F9 uma vez**.
4. Envie `scripts/FreeRoamRivals/FreeRoamRivals.log`, ou use o coletor abaixo.

O preset ativa render/input, estrada, loop, colisão, câmera, movimento e
observação de racers pós-corrida. Mantém a frequência de amostragem configurada.
Os probes de colisão continuam no callback validado, com revalidação de
contexto; a opção não autoriza novas chamadas desconhecidas.

**F9 não cria um carro.** O preset não liga `NativeRivalPrototypeEnabled`,
`ExperimentalSpawnEnabled`, IA, economia ou garagem. As opções F8 existentes
continuam separadas: se já estavam ativadas, continuam ativadas. Para um teste
só de diagnóstico, use `NativeRivalPrototypeEnabled=0`.

O log inicial inclui módulos carregados a partir do primeiro callback válido,
e não durante inicialização sob o loader lock. F9 atualiza o inventário,
compara as 11 funções em memória/disco e registra o contexto do mundo,
calibração e modelo do jogador. Módulos têm nome/base/tamanho; isso não prova
qual plugin escreveu uma instrução. A enumeração é limitada a 256 módulos;
falhas ou truncamento são registrados, sem inventar completude.

Se o contexto for Free Roam válido e a perseguição estiver comprovadamente
limpa, F9 também reúne a lista espacial e quatro grupos de estrada, cada um
com o limite de captura já usado pela fábrica. Não é a cidade inteira.
Contexto inseguro ou estado de perseguição desconhecido deixa essas consultas
extras bloqueadas. Repita F9 somente depois de 10 segundos.

A auditoria F9 não instala hook de capacidade, não chama construtor/goal/reset/
ativação/retirada e não altera o resultado de compatibilidade do protótipo.
Antes de construir por F8, a fábrica faz sua própria verificação de novo.
Depois que a fábrica já estiver preparada, a auditoria de código informa
indisponibilidade: seu próprio hook altera a entrada de capacidade e não pode
ser confundido com um conflito externo. Reiniciar permite uma auditoria limpa.

## ZIP de evidências (opcional, Python 3.9 ou posterior)

Feche o jogo para obter uma cópia consistente do log. Na pasta do jogo:

```bat
py -3 tools\collect_diagnostic_bundle.py "."
```

Ou abra `tools\collect_diagnostics.cmd`; ele usa `py`/`python` se disponível.
Sem Python, basta enviar o log F9, sem instalar nada para este primeiro teste.

O coletor cria um ZIP novo, sem substituir arquivos existentes. Inclui:

- relatório JSON, motivos de bloqueio e sugestões derivadas do trecho coletado;
- até os últimos 8 MiB do log, com truncamento e hash do trecho explicitados;
- os quatro INIs conhecidos do FRR, até 256 KiB por arquivo;
- inventário/hash de speed.exe e DLL/ASI nas pastas raiz/scripts/subpastas
  imediatas de scripts: máximo 256 plugins e orçamento de hash de 64 MiB; leitura de hash limitada ao tamanho observado,
  com mudança/truncamento de arquivo explicitados.

EXE, DLL, ASI e saves não entram no ZIP. O coletor não envia nada pela internet,
não inicia o jogo e não edita INIs. Arquivo no disco não prova módulo carregado;
criação solicitada não prova chamada; ativação não prova imagem ou direção.
O relatório não atribui automaticamente a culpa a um plugin.

## Próximos passos já definidos

| Evidência | Próximo trabalho |
| --- | --- |
| Assinatura divergente | Analisar primeira diferença/janela e destinos, cruzar módulos/hash com fontes; não aceitar hash novo sem prova |
| 11 assinaturas corretas | Testar F8 com GTI; confirmar chamada real e identidade única antes de discutir IA |
| Construtor chamado, carregamento parado | Analisar recursos/modelo, tempo e objeto próprio; sem deletar ponteiros não provados |
| Ativação registrada, carro invisível | Comparar pose/câmera/volume e observação visual; streaming ainda não provado |
| Carro visível, deslocamento ausente | Examinar goal/ação/contexto; não forçar controle por função desconhecida |
| Rival desaparece | Correlacionar listas/identidade/remoção nativa, perseguição e transições |
| Perseguição/cooldown | Confirmar bloqueios em teste controlado; não alterar IA/remover durante estado inseguro |

Pesquisa e fontes: [DIAGNOSTIC_RESEARCH_2026-10-09.md](DIAGNOSTIC_RESEARCH_2026-10-09.md).
