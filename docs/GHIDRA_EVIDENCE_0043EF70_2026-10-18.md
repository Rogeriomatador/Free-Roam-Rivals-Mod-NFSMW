# Evidência Ghidra real: 0x0043EF70 — 18/10/2026

## Proveniência

Captura **somente leitura** enviada pelo proprietário do jogo através de
tools/ghidra_research_bridge.py. Dois relatórios locais: consulta de xrefs
e consulta de xrefs + pseudocódigo, ambas do mesmo endereço.

- executável em disco verificado pelo bridge: 6.029.312 bytes;
  MD5 C0516B485065FABDD69579816B5DF763;
- Ghidra get_metadata: speed.exe, PE32 x86, base 00400000;
  caminho informado D:/Need For Speed Most Wanted Black Edition/speed.exe;
- análise Ghidra: **26.067 funções** e 72.577 símbolos identificados;
- **limitação:** não há prova automatizada de igualdade byte a byte entre
  o arquivo cujo MD5 foi conferido e a imagem previamente importada no Ghidra.
  Caminhos coincidem, mas não inferir confirmação de ABI só a partir disso.

**Nenhum pseudocódigo do jogo foi commitado.** Evidência transcrita somente
como observações de chamadas/tabelas; os relatórios integrais permanecem
locais e ignorados por Git.

## Achados diretamente observados

- A ferramenta consulta 0x0043EF70, que o Ghidra chama
  **FUN_0043ef70**, decompilada como rotina com convenção sugerida
  **__thiscall** e dois parâmetros provisórios. Assinatura do decompilador
  NÃO é prova independente da ABI.
- Xref **de entrada**: chamada incondicional em **0x0043F07D**, dentro
  de **FUN_0043f040**.
- Corpo descompilado faz uma chamada **FUN_0043D330** e uma chamada
  **FUN_005D7930**, entre outras operações de inicialização.
- Escreve ponteiros para tabelas virtuais, incluindo
  **0x00892AD0**, **0x00892AC8**, **0x008929F0**, **0x008929E8**,
  **0x00892988**, **0x0089297C**, **0x00892974** e **0x00892964**.
- A rotina retorna o primeiro ponteiro/objeto recebido; isso é
  consistente com inicialização de um objeto orientado a interfaces.
- O relatório de **get_xrefs_from(0x0043EF70)** diz "No references found".
  Esse endpoint consulta referências que partem **da instrução de
  entrada exata**, não todas as instruções da função. O próprio corpo
  mostra chamadas: **não interpretar** esse campo como "função sem chamadas".

## Cruzamento com referência externa (hipótese, não prova in-game)

As notas públicas nfsmw-re chamam 0x0043EF70
**CreateAIVehicleRacerInstance** e associam o vtable 0x00892AD0
à classe **AIVehicleRacer**. Nosso resultado do jogo é **compatível**
com essa identificação, mas ainda não prova factory completa,
alocador, lifecycle, car ownership nem autonomia de IA no Free Roam.

A função apontada como **FUN_0043D330** neste Ghidra deve ser
investigada separadamente; o significado de **0x0043D388** continua
conflitante nas referências históricas. Não assumir que esse segundo
VA é entrada segura de função. Não criar hook ou chamar qualquer uma
dessas funções com base apenas nas notas.

## Próximas capturas Ghidra MCP necessárias

1. **0x0043F040** (origem da chamada), com pseudocódigo e grafo
   de chamadores/chamadas — descobrir alocação e contexto de construção.
2. **0x0043D330** (função chamada), com xrefs/pseudocódigo
   para verificar inicialização do subcomponente e limite real de função.
3. Só depois examinar **0x0043D388**, por disassembly/limite de função
   verificado, para resolver a divergência.
4. Em paralelo, localizar writers de **ISimable::_mHandle** para o problema
   do Rico: **não** inferir causa do desaparecimento da IA host sozinha.

O bridge passa a aceitar **--callgraph** opcional, usando as leituras
get_function_callers/get_function_callees. É separado de --decompile,
para preservar a coleta anterior e manter a compatibilidade observável.
O primeiro resultado dos endpoints novos depende do plugin real no PC.

**Proibido** promover novos patches de veículo, goals, handles ou
streaming até confirmar chamada, ABI, ownership, rollback e funcionamento
na sessão de jogo. A alteração deste PR NÃO modifica o ASI.
