# Street Duel — largada lado a lado SEM cutscene (primeira etapa)

## Pedido e comportamento exigido

Quando o jogador desafiar Rico (G enquanto a buzina nativa não estiver mapeada), o resultado pretendido **não é um Outrun de perseguição**:

1. Identificar os dois carros e a mesma sessão Free Roam, pista segura e sem perseguição.
2. Rico reduz a velocidade, segue até uma área de formação verificada; o jogo reduz e retém o carro do jogador através de **controle nativo reversível** — não congelar o mundo nem falsificar HUD.
3. Os dois carros se alinham fisicamente, no sentido da pista, sem cutscene, câmera trocada ou teleporte invisível.
4. Manter os dois parados, bloquear temporariamente o input do jogador com posse/restauração comprovadas, exibir contagem **3, 2, 1, GO!**.
5. Em GO!, liberar ambos no mesmo frame de gameplay verificado, devolver controle ao jogador, usar meta/rota válidas e registrar chegada/vencedor. Rico segue existindo após a prova.
6. Qualquer perda de física/owner, pursuit, pista, mundo, foco ou autorização cancela e exige devolução dos controles.

## O que este PR implementa

- Preset de domínio `streetDuelStagingTuning()` com **nenhuma cutscene**, **nenhum alinhamento por teleporte** e **nenhuma negociação**.
- A máquina existente `StagingStateMachine` mantém o modo antigo; no modo estrito, exige sinais separados de (a) posse do input do jogador, (b) comando de freio do rival, (c) restauração comprovada, (d) carros alinhados e parados para contagem e (e) ACK de restauração no release. Tempo inválido/irreal é rejeitado.
- Timeout de aproximação nunca vira falsamente `ApproachComplete`; alinhamento incompleto nunca chama alinhamento escondido; carro que começa a mover durante contagem aborta.
- `tests/street_duel_staging_tests.cpp` exercita impossibilidade de iniciar sem posse nativa, cancelamento, alinhamento, contagem sem release, restauração, impossibilidade de cinema/teleporte e compatibilidade do modo legado.
- Não habilita o preset estrito por INI nem o acopla a `G`: isso bloquearia o Outrun atual sem oferecer largada funcional.

## Faltando para esta corrida se tornar jogável (necessita prova Win32 in-game)

- Descobrir e verificar a **ação HORN** real; não tratar `UseHornToChallenge=1` como hook nativo existente. Enquanto não mapeada, G continua apenas fallback.
- Escolher o ponto de partida com **vias realmente conectadas, width/ground/clearance, footprints e streaming** e manter referências de ambos os carros válidas.
- Encontrar API/ABI **comprovada por bytes, calling convention e snapshots in-game** que adquire e devolve controle do jogador **sem interferir no XInput/WidescreenFix**.
- Verificar como ordenar à AI Racer reduzir, ir ao lugar de formação, segurar freio e retomar rota, com **estado anterior/restauração**, e sem usar `Kill`, teleporte arbitrário ou escrita de campos supostos.
- Só depois conectar `tickNativeEncounter` ou novo adapter ao preset, e fazer contagem D3D sem assumir que HUD == mundo pronto.
- Validar ponto de chegada/checkpoints e decisão simultânea na mesma thread de gameplay. Nunca reutilizar `route_unmatched` como se fosse uma chegada de corrida Sprint.
- Testar início/conclusão, perda de owner a 151m, corte de foco, troca de carro, pausa, policiais e incompatibilidades com outros .asi numa cópia separada do jogo.

## Estado esperado

**Fundação de código e testes, NÃO corrida funcional no jogo.** Na versão atual, G ainda inicia Outrun. Os campos de autorização no domínio são apenas contratos que um adapter nativo precisa provar, não permissões que o código pode forjar. Não alterar WidescreenFix/FixHUD, os INIs do usuário, salvamentos, metric cache nem a versão distribuída até CI e teste real.

### Arquivos técnicos

- `src/domain/StagingStateMachine.h/.cpp`
- `tests/street_duel_staging_tests.cpp`
- `src/domain/RaceFormationReadiness.h`
- `src/game/NativeEncounter.cpp` (integração futura)
- `src/game/NativeVehicleFactory.cpp` (identidade/owner)
