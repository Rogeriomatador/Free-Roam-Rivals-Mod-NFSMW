# v0.0.48-dev — Rico se afasta, streaming e novo encontro protegido

## Evidência real: captura do usuário de 11/10/2026

- `12:23:45`: novo Golf GTI criado em uma pista medida a **30,13 m** do jogador, com v47 e `NativeRivalPrototypeNearPlayer=1`. Portanto o modelo **não nasceu longe**: ele se afastou depois.
- `12:23:46`: o novo Racer ativa e começa a dirigir; `12:24:39` chega a mais de **1.865 m** do jogador, enquanto a medição fica fisicamente válida.
- Depois de avançar muito, o estado da física fica `owned_vehicle_unavailable_wait_without_cleanup`: o carro permanece como objeto próprio, mas seu snapshot deixa de ser útil. `12:26:17` o jogador solicita retirada `Ctrl+Shift+R`; `cleanup_requires_fresh_loaded_identity` impede executar Kill em ponteiro possivelmente descarregado.
- `12:26:44` e `12:26:55`: F8 é recusado porque a identidade anterior **não** saiu com confirmação de dois frames das duas registries. Isto evita criar duplicatas/corrupção no jogo.
- O `world-metric-v1.state` enviado confirma que o cache da v46 funcionou, ~1,0016 unidades/m. O resultado acima **não é um erro de calibração**.

## Mudanças da v48

1. **Prevenção antes do streaming:** somente no modo de protótipo próximo (`NativeRivalPrototypeNearPlayer=1`), quando Rico alcança 350 a 850 m e permanece um veículo ATIVO, carregado, com caixa física válida, mesma geração de mundo, sem perseguição, sem corrida em andamento e após seis quadros estáveis, o mod considera retirar o GTI antes que ele se descarregue. A verificação profunda de câmera e oito raios de oclusão do mundo **continua obrigatória**; apenas com evidência de ocultação confirmada invoca `NativeVehicleFactory::requestRemoval()`, usando identidade atual, procedimento nativo existente e restrições da factory. Frequência de sonda no máximo 1x/s. Nunca teleporta, reativa objeto descarregado nem atinge rivais de terceiros.
2. **F8 aguardado:** se F8 for apertado quando a retirada está em curso, marca um pedido pendente. A nova busca ocorre só depois de duas amostras, em quadros completos distintos, demonstrando que o carro anterior não está nem na IVehicle nem na PVehicle registry, com factory íntegra. Se a retirada não termina, F8 **não** cria outro carro; reiniciar o jogo continua sendo o procedimento para sair de um estado nativo incerto.
3. **Mensagens esclarecedoras:** HUD distingue `RICO FORA DO ALCANCE`, `RICO DESCARREGADO`, `RETIRADA NATIVA PENDENTE` e `NOVO ENCONTRO SOLICITADO`. O registro explica falhas de prova de oclusão e limitações de streaming.
4. **Sem alteração de IA de direção:** o GTI ainda conduz segundo o objetivo nativo, não sabe esperar o jogador. Em pista aberta ou se as oito linhas de oclusão falharem, o mod pode não conseguir retirar o carro antes do streaming — nesse caso mantém o bloqueio seguro, sem promessa de respawn.

## Instalação

O ZIP `FreeRoamRivals-v0.0.48-dev-upgrade.zip` substitui exclusivamente o ASI principal e o INI play. Antes de extrair, faça backup se personalizou o INI; feche o jogo e copie o ZIP para a pasta que contém `speed.exe`. O preset play já inclui `NativeRivalPrototypeNearPlayer=1`, F8, G e Ctrl+Shift+R por 1,5 segundos. O upgrade deixa os arquivos personalizados `Rivals.ini`, `VehiclePools.ini`, `UndergroundBlacklist.ini`, `world-metric-v1.state` e saves intocados.

## Teste real obrigatório

1. Entre no Free Roam com Golf GTI e use F8. Confira `distanceMeters` perto de 20–120 m na criação; o destino de Rico depois de dirigir não está controlado pelo mod.
2. Deixe o GTI ir para longe, sem iniciar desafio. Em uma rua com obstáculos ocultando fisicamente o carro, procure `NativePrototype pre-streaming retirement` e `NativePrototype owned retirement confirmed`. Se a oclusão não for comprovada, verá `pre-streaming retirement deferred`, sem chamadas de Kill.
3. Durante uma retirada comprovada, pressione F8 uma vez; deve registrar `F8 queued` e só criar outro GTI depois de duas ausências nas duas registries, com segurança mantida.
4. Se o rival descarregar antes de poder ser removido, não force reativação via arquivos ou INI; feche o jogo e reinicie. Envie `FreeRoamRivals.log` e `NativeExceptions.log` para identificar o ponto exato de perda.

### Não comprovado
A v48 melhora as tentativas de retirada *antes* do carregamento distante, mas não resolve o streaming geral do mundo do MW05 nem estabelece permanência de um rival nativo em qualquer região. O ciclo completo de remoção e novo spawn precisa de teste no PC do proprietário.
