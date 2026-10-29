# NFSMW 2005 — mapa de subsistemas e dependencias (29/10/2026)

## Escopo e proveniencia

Esta etapa continua a branch de pesquisa Ghidra MCP, mantendo o codigo do
**ASI v0.0.61** inalterado. Base: evidencias estaticas coletadas pelo dono
em 18/10/2026 e ja documentadas neste repositorio, e logs reais
documentados das sessoes 11–17/10/2026. **Nao houve nova execucao do jogo,
novo dump de memoria ou acesso ao Ghidra local durante esta etapa.**

Executavel alvo em disco: speed.exe 6.029.312 bytes;
MD5 C0516B485065FABDD69579816B5DF763 (Black Edition do proprietario).
A metadata Ghidra informa PE32/x86 com base 0x00400000 e 26.067 funcoes
reconhecidas no momento da coleta. A imagem **importada no Ghidra ainda
nao foi comparada byte a byte ao EXE validado em disco**.

Fontes primarias ja preservadas como *notas*, nao pseudocodigo integral:

- [Host Racer e goal](GHIDRA_EVIDENCE_0043EF70_2026-10-18.md)
- [Troca de goals](GHIDRA_EVIDENCE_00422480_0043D400_2026-10-18.md)
- [Lote de 60 enderecos](GHIDRA_BULK_60_FUNCTIONS_2026-10-18.md)
- [Fronteiras nativas incertas](GHIDRA_NATIVE_BOUNDARIES_2026-10-18.md)
- [Perda de Rico a 7 m](NEAR_PASS_LOSS_V57.md)
- [Sessao v59 / projeto v60](PLAYABLE_OUTRUN_RECOVERY_V60.md)
- [Auditoria da entrega v61](ASI_RELEASE_AUDIT_V61.md)

## Legenda de evidencia

- **Observado em Ghidra:** listagem/callgraph/descompilacao fornecida pelo
  proprietario; nomes e assinaturas permanecem interpretacoes.
- **Observado no jogo:** logs reais da instalacao do proprietario.
- **Hipotese:** comportamento ainda nao confirmado pelo alvo em runtime.
- **Nao autorizado:** nenhum endereco novo ganha permissao de executar
  hooks, construtores, destruidores ou alterar tokens/registradores.

## Mapa de dependencias, nao um mapa de todas as funcoes

~~~text
                     ESTADO DO MUNDO
           GameFlow + GRaceStatus + transicoes
                            |
                            v
    REGISTROS NATIVOS + IDENTIDADE DO VEICULO
         PVehicle / IVehicle / ISimable handle
               |                    |
               v                    v
       CONSTRUCAO/RETIRADA       STREAMING
               |                 E CACHE
               +----------+---------+
                          v
                     IA DO RACER
         alocacao host --> inicializacao host
                                |
                         construcao goal
                                |
                   troca/atualizacao goal
                          |
                          v
                NAVEGACAO + FISICA
                          |
                          v
              ENCONTROS / SPRINT / OUTRUN
                          |
                          v
                  TELEMETRIA / SAVE FRR
~~~

O diagrama e a organizacao da investigacao do mod; ele **nao** prova que
cada seta corresponde a uma unica chamada nativa do jogo.

| Subsistema | Evidencia | Estado atual | Falta demonstrar |
| --- | --- | --- | --- |
| Contexto mundo/corrida | MWSDK + GRaceStatus e gates no GameBridge | integracao atual do mod | estados de transicao ao perder owner |
| Registro / identidade | IVehicle/PVehicle registrados; ISimable::_mHandle observado divergente | bug reproduzido em logs | escritor do handle, ordem e causa |
| Fabricacao fisica | PVehicle::Construct e amostras de Rico | 1 rival teste nativo | lifetime e cleanup em todos os modos |
| IA Racer | cadeia 0x43F040, 0x43EF70, 0x43D330, 0x422480 | chamada estatica conhecida | host e goal vivos apos transicoes |
| Atualizacao IA | 0x42F140 e 0x42AA80 com xrefs em vtables | relacoes estaticas | acionamento real em Free Roam |
| Pista/streaming | WRoadNav; comportamento de cache em reconstrucao publica | investigacao e guards | residencia da secao e politica de retiro |
| Corrida real | Sprint fisico de 120 m e Outrun 300 m | modos experimentais com limites | IA em rota de curvas, sincronizacao e ownership |
| Garagem/economia | leitura de carreira + estruturas publicas | pesquisa somente leitura | transacao segura com rollback e autosave |

## 1. Chamada e alocacao do host Racer

Evidencia Ghidra fornecida:

~~~text
0x0043F040 -> 0x0043EF70 -> 0x0043D330 -> 0x00422480
          \-> 0x005D29D0     (alocacao, segundo decompilador)

0x0043D400 -> 0x0043D330
          \-> 0x005D29D0     (outro objeto, alocacao)
~~~

- 0x0043F040 solicita 0x7EC bytes, aparentemente para o host.
- 0x0043D400 solicita 0x7CC bytes, aparentemente para o goal.
- 0x0043EF70 instala vtable principal 0x00892AD0.
- 0x0043D330 instala vtable 0x00892720 e referencia AIGoalRacer.
- 0x00422480 processa troca de goal; +0xB8 e +0xC4 sao campos
  interpretados na descompilacao, **nao layout publico validado**.
- A correspondencia desses objetos ao Rico em runtime nao foi
  demonstrada diretamente por essa coleta.
- 0x0043D388 foi confirmado como **endereco interior de 0x0043D330**.
  NUNCA tratar como entrada de construtor ou usar em hook.

### Atualizacao e chamadas indiretas

- 0x0042F140 tem DATA xref em vtable 0x00892AD0+0x28;
  a rotina referencia 0x004293D0 e 0x0042AA80.
- 0x0042AA80 tem DATA xref em vtable 0x00892720+0x28;
  inclui uma chamada virtual em objeto apontado a partir de +0xB8.
- Vtable xref **nao** prova que o tick foi disparado durante a
  corrida, nem informa quem liberou host e goal.
- O bloco 0x004293D0 inclui estado contextual de corrida/perseguicao,
  mas nao existe ligacao comprovada com o writer do _mHandle.

## 2. Contrato de identidade: diagnostico principal de Rico

**Observado no jogo:**
- Em 11/10, ultimo snapshot saudavel viu Rico a ~7,04 m,
  com 56,08 m/s; logo depois a leitura perdeu validade em Free Roam,
  embora os registros nativos ainda exibissem o carro.
- Na sessao v59 documentada de 17/10, o Golf GTI fisico dirigiu,
  foi observado por varios minutos e passou de 2,6 km radiais do
  ponto de construcao. A antiga tese do limite de 1.024 m foi refutada.
- Um Outrun terminou com motivo rival_native_presence_lost:
  simable_handle nao coincidiu com o valor guardado enquanto
  os registros ainda identificavam o objeto. **Causa ainda desconhecida.**

Essas observacoes nao provam se houve recycling de objeto, handle
reassign, cleanup, mudança de goal, streaming ou outra causa. Nao
regravar o valor antigo, nao adotar automaticamente o novo handle,
nao chamar Kill/despawn a partir de ponteiro divergente.

### Proximo experimento controlado

Coletar, *no mesmo tick do gameplay verificado*, somente informacoes
autorizadas pelo codigo e SDKs conhecidos:

1. contexto mundo/sessao/geracao e frame;
2. identidade do PVehicle/IVehicle nas duas listas nativas;
3. simable esperado versus simable observado, motivo de rejeicao;
4. ativo/carregando/destruido, AI host/goal somente se ABI e
   leitura segura ja estiverem comprovados;
5. antes/depois de aproximacao, inicio de Outrun, curva,
   perseguicao e transicao de mundo.

A primeira leitura inconsistente **revoga** a observacao: registrar
ausencia/erro, sem desreferenciar ou mutar o objeto suspeito.
Nao reconstruir artificialmente uma continuidade de tick.

Criterio para investigar o writer: localizar em Ghidra referencias e
**stores** para o campo ISimable::_mHandle, estabelecer offsets,
caller e boundary num build correspondente, e correlacionar com
o instante da falha in-game. Xref de endereco/constante isolado nao
basta; chamadas indiretas podem requerer rastreamento de vtables.

## 3. Fronteiras sem autorizacao de chamada

A coleta de 18/10 encontrou bytes nos enderecos abaixo, mas o
Ghidra **nao** devolveu funcoes analisadas completas para eles:

- 0x00689820 — candidato de construcao fisica;
- 0x006851D0 — candidato de retirada;
- 0x00688120 — getter simples sugerido;
- 0x006693A0 / 0x006693C0 — wrappers de ativacao/desativacao;
- 0x006876E0 — candidato de setter de classe.

O primeiro bloco de 32 bytes e xrefs de vtable sao **pistas**.
Nao remover o VersionGuard, assinatura de bytes, lease de ownership,
checagens de contexto, observacao de registradores, ou fallback
fail-closed ja existentes.

## 4. Ferramenta nova: grafo estrutural derivado dos ZIPs locais

O catalogo Ghidra existente lista todas as entradas de funcao
reconhecidas, mas **nao** estabelece automaticamente ligacoes.
A ferramenta nova deriva **somente arestas diretas explicitamente
listadas** em get_function_callers e get_function_callees do
ZIP local de 60 funcoes. Nao cria ligações a partir de pseudocodigo,
xref DATA, nomes inferidos ou endereco interior.

Na raiz do repositorio, no Windows:

~~~powershell
py -3 tools\ghidra_subsystem_map.py --bulk "research\local\ghidra_bulk_SEU_ARQUIVO.zip"
~~~

Opcionalmente, para cruzar nomes do atlas completo:

~~~powershell
py -3 tools\ghidra_subsystem_map.py --bulk "research\local\ghidra_bulk_SEU_ARQUIVO.zip" --atlas "research\local\ghidra_game_atlas_SEU_ARQUIVO.zip"
~~~

O resultado e um ZIP **novo** em research/local/ contendo:

- manifest.json: proveniencia SHA-256 dos inputs, limites e contadores;
- nodes.json: enderecos observados, nomes opcionais e hipoteses rotuladas;
- edges.json: chamadas diretas explicitamente listadas, com a origem
  de cada observacao e indicador de concordancia entre endpoints;
- LEIA-ME.txt: limites de uso.

A concordancia de dois endpoints **nao** constitui verificacao
independente, porque ambos vieram do mesmo projeto Ghidra. Arquivos
de pseudocodigo do jogo nao sao copiados no output. A ferramenta
nao abre a porta MCP, nao interage com o game e nao modifica o ASI.
O ZIP bruto do Ghidra e pseudocodigo permanecem fora do Git.

Se os ZIPs antigos do proprietario nao estiverem mais disponiveis,
gerar um novo usando as ferramentas da branch, sem alterar o
executavel: [guia MCP](AI_GHIDRA_MCP_WORKFLOW.md).

## 5. Ordem de trabalho para a proxima implementacao real

1. **P0 — Handle/lifetime:** obter rastros do writer e correlacionar
   owner, registries, states, tentativas de cleanup e frames.
2. **P1 — AI host/goal:** observar se a IA existe, e se o goal/tick
   se mantem associado depois de eventos Free Roam, sem chamadas
   experimentais nao verificadas.
3. **P2 — Navegacao/streaming:** medir disponibilidade de estrada e
   secao de mundo por candidato; evitar falsa certeza por
   LoadingComplete, visibilidade LOD ou registry count.
4. **P3 — Movimento e corrida:** validar uma rota curva e comportamento
   nativo, antes de ampliar Sprint e criar varios rivais.
5. **P4 — Persistencia e carreira:** habilitar escrita de garagem
   apenas com transacoes, rollback, snapshot e integridade do save.

### Gates para promover qualquer nova API nativa

**Evidencia minima:** identidade do EXE e do programa importado,
limites reais da funcao, ABI/calling convention, tipo correto do
this, dono do objeto, thread, contexto, limpeza, falhas/rollback
e reproducao no computador do proprietario. CI e testes offline
validam logica do mod, mas nao substituem observacao no jogo.

**Resultado desta etapa:** ferramenta read-only + mapa auditavel.
**Nao concluido:** correção de desaparecimento de Rico, multi-rivais
automaticos ou driver de IA de rota real. Nenhum desses sera
declarado funcional sem novos logs/testes in-game.
