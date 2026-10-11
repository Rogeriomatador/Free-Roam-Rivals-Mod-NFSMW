# v0.0.55-dev — Rico desapareceu a 151m: distinguir perda física e corrigir o diagnóstico

## Captura original de 11/10/2026

O arquivo `FreeRoamRivals(20261011-231439).log` contém duas inicializações: primeiro v0.0.53-dev (19:46, sem entrar no mundo); depois **v0.0.54-dev às 20:05:47**.

- Às **20:10:08.736**, F8 cria GTI aproximadamente 116,3 metros adiante (alvo no frustum potencialmente visível); o motor confirma criação e ativa às 20:10:09.816.
- O mod observou o veículo passando até a apenas 12m do jogador, depois dirigindo livremente. Às 20:12:29.279 o jogador iniciou um desafio que o código nomeia explicitamente `Outrun`, **não uma corrida Sprint original**. Às 20:12:43.050 o jogador chegou a ultrapassar o Rico, mas as trajetórias divergiram, dando `route_unmatched` às 20:12:51.075.
- Às **20:13:50.430** o último `NativePrototype observation` confiável mediu Rico a **151,178m**, velocidade física **32,8627m/s (~118,3km/h)**, posição (-81.49, 94.54, 1662.76) e jogo no modo Free Roam normal. Às **20:13:51.199**, menos de 1s depois, `NativePrototype blocked=owned_vehicle_unavailable_wait_without_cleanup stage=active`. Às 20:13:51.423 ainda havia cenário e player válidos, mas `racers=0` nos contadores públicos do snapshot do jogo.
- A transição de mundo só ocorreu depois, às 20:14:05, portanto **não explica a primeira perda às 20:13:51**. O carro sumiu **perto**, antes do limiar de 350m para a retirada preventiva. Isso NÃO comprova qual subsistema nativo descarregou ou removeu o veículo, nem se os dois registradores ainda retinham os ponteiros.
- O `NativeExceptions(20261011-231439).log` não traz um registro identificado de exceção fatal. O estado `live-rico-*.state` registra **6 batalhas, 6 interrupções, zero vitórias, zero derrotas**.
- `FreeRoamRivals(10).ini`: `[Outrun] Enabled=1`, `[Staging] Enabled=0`. O G inicia uma perseguição baseada na trajetória observada; o controle nativo de parada/alinhamento simultâneo ainda **não existe**.

## Mudanças na v55

1. A primeira perda de Rico no mesmo contexto em que o carro estava ativo gera um log único `NativePrototype sudden native presence loss`, com distância e velocidade da última amostra (apenas se emitida há no máximo 5 segundos), idade da amostra, status do contexto e indicadores `nearLossAtMost250Meters` e `unsafeToForceRespawn=1`.
2. Na mesma callback pós-gameplay já comprovada, `NativeVehicleFactory::auditOwnedPresenceReadOnly()` coleta de modo limitado as listas de **IVehicle** e **PVehicle** e confere numericamente se a identidade própria do rival aparece em cada uma, sem desreferenciar os ponteiros guardados. Os campos `liveRegistryContainsOwner`, `physicalRegistryContainsOwner`, `registryVerified` e contagens revelam se a retirada foi registrada no motor. Contexto não verificável deixa `registryVerified=0`; isso **não equivale** a provar ausência!
3. HUD deixa de mostrar a afirmação enganosa `RICO FORA DO ALCANCE` em uma perda a 151m: passa a mostrar `RICO SUMIU PERTO DE VOCE` quando a distância observada foi até 250m, ou `RICO NAO ESTA DISPONIVEL` se ainda não há evidência suficiente. Mantém a proteção de ownership sem duplicar veículo.
4. Durante perda inesperada, o Outrun é interrompido por `rival_native_presence_lost`, não pelo impreciso `world_identity_changed` se o Free Roam não mudou. O modo Outrun é rotulado claramente `OUTRUN LIVRE` e seu log indica que não existe `sideBySideStart` nem `jointCountdown`.
5. Adicionado CTest para situação real de 151m, amostras antigas, distantes, nulas e relógio revertido. **Nenhuma mudança na IA, streaming do jogo ou no controle nativo dos carros**.

## Próximo passo técnico seguro

A causa real poderá ser melhor diferenciada com os próximos dados: identidade em IVehicle somente, PVehicle somente, ambas ou nenhuma, e quais contadores ainda são coerentes. Uma outra ASI não deve chamar `Kill`, construir novamente ou trocar a identidade com registradores ainda ambíguos: isso pode corromper o processo.

Para produzir uma **corrida tradicional**, há trabalho independente: adquirir e devolver controles do jogador e IA do rival, encontrar uma rua segura, parar/alinhá-los por comandos nativos verificados, iniciar contagem e definir destino/checkpoints aceitos pelo motor. Não basta ativar `[Staging] Enabled=1` no INI: o planejador original ainda não executa essas mutações.

## Teste

1. Atualize **somente** o Free Roam Rivals pela opção ASI-only v55. O WidescreenFix, incluindo `FixHUD=1`, não é modificado.
2. Com GTI, crie Rico com F8, jogue normalmente. Se ele desaparecer, **não pressione F8 repetidamente**.
3. Envie o log recém-criado e, se possível, vídeo dos últimos segundos. Filtre por `NativePrototype sudden native presence loss` e pela presença dos dois registradores.
4. Os desafios com G continuam sendo **Outrun**, sem largada lado a lado. A versão não afirma consertar sua ausência.

