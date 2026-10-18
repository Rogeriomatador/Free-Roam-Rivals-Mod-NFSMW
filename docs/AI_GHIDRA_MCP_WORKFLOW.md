# IA + Ghidra + MCP no FreeRoamRivals

## Objetivo

Permitir que IA (ChatGPT ou Claude) investigue o NFSMW real com Ghidra MCP,
cruze as observações com MWSDK, NFSPluginSDK e nfsmw-re, e só depois
proponha mudanças no mod. **Nenhuma leitura de outro build autoriza hook
ou chamada nativa no executável de nosso usuário.**

## SDKs que já temos (não reinstalar nem duplicar)

O CMakeLists.txt já fixa:
- MWSDK: 6db158647fe05a3d1cbbfee38f0f5d1e91b2f473
- NFSPluginSDK: d238bdffc648498840d17133edb256fa0310d355
- nfsmw-2005-sdk: 3b3d05b9194844883aa42d75dd7aae66838092ff

Integração nova: tools/ghidra_research_bridge.py + testes automatizados,
somente na estação de desenvolvimento. O ASI não importa essa ferramenta.

## Referências

- [MWSDK](https://github.com/TsyVM/MWSDK)
- [nfsmw-re](https://github.com/s-b-repo/nfsmw-re): docs/renames.csv,
  notes/project_ai_racer.md, notes/project_streamer_anchors.md
- [Ghidra](https://github.com/NationalSecurityAgency/ghidra)
- [Ghidra MCP](https://github.com/bethington/ghidra-mcp/releases): escolher
  release compatível com Ghidra/Java, instalar plugin Ghidra e ponte MCP.
- [NFSPluginSDK](https://github.com/berkayylmao/NFSPluginSDK)

As notas nfsmw-re foram coletadas em outro jogo/build. São hipóteses.

## Instalação e consultas no Windows

1. Faça backup da instalação, do ASI anterior, INIs e saves.
2. Instale Ghidra e Java compatíveis, depois a extensão e o bridge
   Python da MESMA release do Ghidra MCP. Instale a extensão em
   Ghidra > File > Install Extensions, habilite o plugin, reinicie.
   Consulte instruções oficiais do release instalado, pois endpoints variam.
3. Importe seu próprio speed.exe PE32 no Ghidra, conclua análise, abra o
   programa speed.exe e habilite Ghidra MCP apenas em 127.0.0.1:8089.
4. Para clientes IA compatíveis com MCP, configure o executável local
   bridge-mcp-ghidra da release instalada como servidor MCP. Use apenas
   operações de leitura até validar ABI/ownership. Este ChatGPT não está
   conectado à porta 8089 do seu computador; os relatórios abaixo são
   a alternativa de compartilhamento sem dar acesso remoto ao jogo.
5. Na pasta do repositório do nosso FRR, em PowerShell:

~~~powershell
git clone https://github.com/s-b-repo/nfsmw-re.git ..\nfsmw-re
py -3 tools\ghidra_research_bridge.py lookup --csv "..\nfsmw-re\docs\renames.csv" --text "Racer"
py -3 tools\ghidra_research_bridge.py verify --exe "D:\Games\Need for Speed Most Wanted\speed.exe"
py -3 tools\ghidra_research_bridge.py snapshot --exe "D:\Games\Need for Speed Most Wanted\speed.exe" --address 0x0043EF70
~~~

Substitua D:\Games\... pelo caminho real. A ferramenta verifica
size=6029312 e MD5=C0516B485065FABDD69579816B5DF763 ANTES de
consultar Ghidra. Ela só faz GET aos endpoints locais de leitura:
mcp/health, get_metadata, get_xrefs_to e get_xrefs_from, com program=speed.exe.

Para gerar também pseudocódigo local, use --decompile (opcional).
Para consultar todas as chamadas da **função inteira** (e não somente
os xrefs da instrução de entrada), use --callgraph. A opção faz apenas
GET /get_function_callers e GET /get_function_callees; se sua versão
GhidraMCP não fornecer os endpoints, interrompa e envie o erro. Não
substitua automaticamente por uma chamada mutante.

Exemplo baseado na captura do proprietário: a função 0x0043EF70 tem
uma chamada em seu corpo a 0x0043D330, apesar do retorno "No references
found" do get_xrefs_from sobre a instrução inicial. Para a próxima
coleta, depois de atualizar a branch de pesquisa:

~~~powershell
cd "E:\\Downloads\\FRR-Ghidra-Novo"
git pull --ff-only
py -3 .\\tools\\ghidra_research_bridge.py snapshot --exe "D:\\Need For Speed Most Wanted Black Edition\\speed.exe" --address 0x0043F040 --callgraph --decompile
~~~

Documentação da primeira evidência:
[GHIDRA_EVIDENCE_0043EF70_2026-10-18.md](GHIDRA_EVIDENCE_0043EF70_2026-10-18.md).

O arquivo de saída fica em research/local/, ignorado pelo Git.
Não publicar ou redistribuir executáveis/pseudocódigo do jogo.
Relatórios não são enviados a servidor nenhum por esse script,
não provocam spawn nem instalação de hooks.

**Limitação crucial:** o MD5 comprova somente o EXE local consultado;
get_metadata identifica speed.exe no Ghidra, mas não comprova
byte a byte qual imagem foi carregada no Ghidra. Confirme manualmente
caminho e MD5 do EXE importado antes de tirar conclusões.

O HTTP utilizado foi documentado pela versão Ghidra MCP 5.7.1
de nfsmw-re. Se a release instalada alterar endpoints, manter
fail-closed e adaptar após ler a documentação da release (não contornar
os checks de identidade).

## Primeiros alvos de investigação (pistas, não chamadas autorizadas)

| VA | Hipótese | Próxima evidência |
| --- | --- | --- |
| 0x0043EF70 | Criação de host Racer | Xrefs, parâmetros, ownership, vtable e ABI |
| 0x0043D388 | Possível criação de AIGoalRacer | Confirmar limite de função (conflito entre pesquisas) |
| 0x00422480 | Mudança de objetivo AI | Xrefs e condições de Free Roam |
| 0x0042AA80 | Update Racer | Chamadores e dependência de corrida ativa |
| 0x00689820 | Construção física | Pool, cleanup e lifetime |

**Divergência importante:** nfsmw-re chama 0x43D388 de entrada
CreateAIGoalRacerInstance; nosso docs/ENGINE_INTEGRATION_MAP.md
e tools/dump_exe_functions.py tratam esse VA como interior de função,
não entrada segura. Não usar/hookar esse VA até confirmar xrefs,
prólogo e limites na versão MD5 suportada.

No último teste do Rico, ambas as listas ainda registravam seu carro,
mas ISimable::_mHandle deixou de corresponder ao valor guardado. O
primeiro objetivo concreto do Ghidra é localizar quem escreve o handle
e em qual sequência do lifecycle. **Não reescrever handles, chamar Kill
ou permitir spawn duplicado com base em hipótese.**

## Critérios de promoção da descoberta para código do mod

Exigir, para cada nova chamada/hook/offset: hash alvo, bytes e versão
do executável, xrefs, ABI, estrutura, identidade do objeto, contexto
do mundo, limpeza, rollback, testes de unidade e logs in-game.
Sem isso, marcar apenas research_only. Não tocar em perfis/saves,
WidescreenFix, ExtraOptions e XtendedInput.
