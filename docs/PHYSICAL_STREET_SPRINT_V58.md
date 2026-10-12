# v0.0.58-dev — corrida curta com chegada real no Free Roam

## Implementação prática (diferente do Outrun)

A v58 integra `StreetSprint`, competição física de dois carros. **Há UMA linha de chegada compartilhada**, 120 m à frente de uma largada alinhada (comprimento configurável de 60–500 m). A corrida é ganha quando a posição física de um carro atravessa primeiro o mesmo plano. O programa mede a interpolação da fração do frame para desempate quando ambos cruzam juntos, e registra vitória, derrota ou empate no histórico existente por perfil. No HUD há progresso do jogador e do Rico, distância da prova e uma marca no radar indicando a meta. O carro continua sendo físico — nenhum checkpoint é declarado sem amostra de mundo.

A largura máxima do corredor é 16 m para cada lado, com elevação verificada, limite de tempo de 45 s e teste de continuidade para evitar teleporte/avanço artificial. Se alguém sair do corredor, passar por salto impossível, entrar em transição/perseguição, sumir ou descontinuar telemetria, a prova é **interrompida**, sem declarar vencedor.

**Não é um evento de corrida do jogo original**: ainda não usa gestor de corridas nativo, playlist de GPS, checkpoints curvos ou modificações nas telas originais de resultados. Só permite reta curta segura, não corridas longas por ruas curvas.

## Como jogar sem sobrescrever o INI existente

1. Atualize apenas `scripts/FreeRoamRivals.asi` (ASI-only).
2. Na cópia de teste, dentro do Free Roam e com Rico já criado usando F8, pressione **Ctrl+Shift+G**. O mod registra `Challenge mode changed ... short_physical_sprint_opt_in=1`. É uma escolha explícita de gameplay, não altera o INI ou save.
3. Encontre Rico e alinhe seu carro manualmente ao lado do Golf GTI, na mesma direção, devagar (os dois abaixo de ~29 km/h), a poucos metros. Use **G** para solicitar parada nativa de ambos.
4. Se `ForceStopOn` funcionar e os dois ficarem realmente parados, o HUD conta **3, 2, 1**, `ForceStopOff` libera os carros e começa a corrida com meta de 120m. Carros permanecem no mapa, sem cutscene.
5. A disputa registra o primeiro a chegar ao plano de chegada ou interrupção. G cancela a preparação ou a prova. **Ctrl+Shift+G** volta ao Outrun tradicional, sem editar arquivos. Não há prêmio em dinheiro ou alteração no save original do jogo.

**Limite fundamental:** Rico ainda não desacelera sozinho em resposta ao desafio a qualquer distância, não encontra espaço de largada, não se alinha sozinho e não fica totalmente sob controle do mod. A versão executa a corrida de verdade DEPOIS de uma parada/alinhamento físico aceitos; não promete largada se Rico passar rápido ou longe. `ForceStop` do NFSPluginSDK não foi comprovado no jogo do usuário — usar cópia de teste e backup de saves. Se algo der errado e o controle do carro não for restituído, encerre o jogo e não force F8 até nova investigação. O controle do jogador não é tomado por `InputPlayer::BlockInput`.

## Preferências

`[Staging] NativeDuelPilotEnabled=0` mantém Outrun após iniciar. `SprintLengthMeters=120` regula distância para o modo Sprint. Como alternativa a editar o INI, Ctrl+Shift+G alterna a modalidade apenas nesta sessão. O comando de buzina ainda não foi verificado no mapeamento nativo e G continua sendo a tecla de ação.

## Segurança / isolamento

- O ASI-only tem só `scripts/FreeRoamRivals.asi` e guia de instalação, sem reescrever INIs, saves, dinput8, TexWizard, X360Stuff ou NFSMostWanted.WidescreenFix (`FixHUD=1`).
- A prova não cria outro veículo nem tenta recuperar Rico usando ponteiros antigos. O incidente de desaparecimento perto de 7 m e ~1009 m da origem permanece em investigação (issue #76).
- `tests/street_sprint_tests.cpp` testa vitórias, derrotas, empate, mudança de trajetória, teleporte, pausa e desaparecimento do rival no domínio. CI Win32 compila ASI e executa CTest, mas não substitui um teste físico no jogo.

## Próximos passos

Comprovada a parada/liberação de ambos in-game, implementar abordagem/posicionamento de AI (sem teleporte), posse/restauração real de controles do jogador, competição por trechos de rua conectados com checkpoints nativos, melhorias do ciclo de vida do Rico e modo buzina.
