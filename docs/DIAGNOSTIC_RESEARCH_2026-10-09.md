# Pesquisa e plano de diagnóstico — 09/10/2026

Cobertura ampla dos obstáculos atuais, não pesquisa exaustiva de todo o jogo.
Não houve execução do NFSMW no ambiente de desenvolvimento. Fontes públicas
são pistas; ABI/endereço/layout precisam corresponder ao speed.exe alvo.

## Resultado que orienta a próxima captura

O log da v36 mostrou 125 solicitações, todas bloqueadas na assinatura de
SetGoal. As 11 assinaturas foram recalculadas no arquivo alvo e corresponderam.
Ainda não recebemos nesta etapa a captura v37 com a primeira diferença em
memória. Portanto não há evidência suficiente para identificar um plugin
responsável, aceitar uma assinatura alternativa ou declarar criação real.

A v38 reduz essa dependência: F9 coleta código e módulos antes de existir
um candidato de spawn, enquanto o preset reúne probes já implementados.
Não usa pesquisa online como substituto de uma medição do jogo.

## Fontes consultadas e como poderão ajudar

| Fonte primária | O que foi consultado | Uso e limite |
| --- | --- | --- |
| [Decompilação NFSMW](https://github.com/dbalatoni13/nfsmw/blob/1f2cdd7996791c81a580b3f7b36b44d4f9f6719c/README.md) | README do commit 1f2cdd7996791c81a580b3f7b36b44d4f9f6719c e reconstruções já disponíveis no workspace | Projeto em desenvolvimento abrange várias plataformas. Estudar lógica de IA/estrada/veículo; não transportar layout de outra plataforma |
| [Física dos veículos](https://github.com/Brawltendo/Most-Wanted-Vehicles-Decomp) | README | O autor caracteriza o material como pesquisa incompleta, sem programa completo compilável. Usar para esclarecer geometria/semieixos antes de confrontar o binário; não apresentar como jogo que podemos executar aqui |
| [Notas de vtables IA](https://github.com/s-b-repo/nfsmw-2005-re/blob/main/docs/wave16_ai_vtables.md) | Tabelas e pendências do documento | Distingue tráfego e helicóptero, com slots ainda não confirmados. Evitar tratar nomes preliminares e tabelas de outra interface como contrato do nosso racer |
| [WidescreenFix](https://github.com/ThirteenAG/WidescreenFixesPack/blob/708f7528fb8b54c79d4ac3c6071ea7ccc01ae162/.github/docs/nfsmw.md) | Documentação NFSMW no commit 708f7528fb8b54c79d4ac3c6071ea7ccc01ae162 | Explica correções de câmera/FOV/HUD e opções. Inventário e configuração ajudam a interpretar a cena; não demonstra autoria da alteração em SetGoal |
| [Bartender](https://github.com/rng-guy/NFSMWBartender/tree/6d50d495052527fc7eb558c99cb819dd3a07dcea) | README e Source/dllmain.cpp, Utilities/MemoryTools.hpp, Advanced/CopSpawnOverrides.hpp, Basic/GeneralSettings.hpp | Implementa opções de perseguição e ferramentas de patch. Estudar convivência e transições de perseguição. A revisão desse subconjunto não exclui patches em outros arquivos e não prova presença no PC do usuário |
| [Tool Help da Microsoft](https://learn.microsoft.com/en-us/windows/win32/toolhelp/traversing-the-module-list) | Exemplo oficial de enumeração de módulos | Base para inventário com snapshot e Module32First/Next. Módulo listado não prova autoria de hook; falha de enumeração não vira lista vazia “completa” |

Os SDKs usados pelo mod continuam presos aos commits no CMake:

- nfsmw-2005-sdk: 3b3d05b9194844883aa42d75dd7aae66838092ff;
- MWSDK: 6db158647fe05a3d1cbbfee38f0f5d1e91b2f473;
- NFSPluginSDK: d238bdffc648498840d17133edb256fa0310d355.

Não incorporamos código desses mods nesta alteração. O inventário/hash serve
para decidir qual fonte específica revisar quando o próximo log indicar
endereço/janela/destino. Código de outro mod só será reutilizado com licença
compatível, atribuição e validação do alvo.

## Contratos já verificados no nosso alvo

- Executável de 6029312 bytes, MD5 C0516B485065FABDD69579816B5DF763.
- FNV-1a de 256 bytes das 11 entradas da fábrica; o log v36 difere em SetGoal.
- Callback externo após update original e identidade/thread/contexto guardados.
- Navegação usa escalares próprios, não cópia de WRoadNav/ponteiros de outro carro.
  O SDK e o alvo diferem no tamanho completo/offsets tardios desse objeto.
- CurrentRoad/FutureRoad getters podem atualizar caches; a captura implementada
  lê campos e valida getters sem invocar essas rotas em render.
- SetSpawned limpa internals/goal; a sequência conectada é SetSpawned, SetGoal,
  Activate. A validação no jogo permanece pendente.
- Proteções: contexto/pursuit/cooldown, escala métrica, terreno, espaço livre,
  capacidade, identidade própria e remoção confirmada nas duas listas.

Esses contratos estão descritos em NATIVE_FACTORY_ADAPTER.md,
NATIVE_RIVAL_PROTOTYPE.md e nos testes de ABI/domínio. Não provam que o corpo
foi renderizado, que a IA dirigiu, que streaming preservou o racer ou que os
bloqueios de perseguição funcionam em todas as situações reais.

## Pesquisa e implementação seguinte, condicionadas a evidência

1. **Compatibilidade:** cruzar primeiras diferenças com módulos e fontes do
   plugin identificado. Salto em entrada e patch interno exigem análises
   diferentes; não normalizar bytes para “passar” sem conhecer sua função.
2. **Criação/visual:** primeiro confirmar chamada real, identidade única,
   carregamento, pose, ativação e presença visual. A v38 não contorna o bloqueio.
3. **Direção:** correlacionar posição/velocidade com contexto/goal e situação
   observada; deslocamento sozinho não prova condução pela IA.
4. **Vida pós-corrida/streaming:** acompanhar a mesma identidade nas listas,
   distinguir retirada do motor de retirada do mod, observar distâncias e
   transições. Relato de racers que permanecem é uma pista, não uma garantia
   para um veículo que nossa fábrica criou.
5. **Polícia:** validar início de perseguição, cooldown, prisão/transição,
   perseguição do rival e reengajamento; preservar bloqueios e não alterar
   estado de outra IA quando o dono/contexto não estiver comprovado.
6. **Depois do primeiro rival funcional:** desafios por input, aparência e
   persistência de identidade, integração com progressão. Diagnóstico não será
   tratado como substituto dessas funcionalidades.

## O que pode ser testado aqui e o que exige PC

Os testes automáticos compilam o ASI no Windows e exercitam políticas, ABI,
hooks sintéticos e coletores com arquivos temporários. O coletor possui casos
para limites de log/hash, omissão de binários/saves, proteção de saída existente,
assinaturas divergentes e interpretação prudente de ativação. A enumeração
real de módulos e F9 dentro do jogo ainda precisam do log do usuário.

Instruções executáveis: [DIAGNOSTIC_BUNDLE.md](DIAGNOSTIC_BUNDLE.md).
