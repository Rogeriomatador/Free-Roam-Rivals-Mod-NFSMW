# v0.0.50-dev — priorizar possível visibilidade e diagnosticar construtor nulo

## Evidência: dois testes reais da v49 em 11/10/2026

**Primeiro teste (14:58–15:01):** F8 às 15:00:06. O GTA foi construído a 42,7661 m às 15:00:07, confirmado por dois frames, IA Racer preparada, estrada pronta e carro **fisicamente ativado** às 15:00:08.955. O motor registrou aceleração de cerca de 0,68 a 60,1 m/s (até 216 km/h), a 626m do jogador 36 segundos depois; o deslocamento total passou de 1,3 km. Após 15:00:45.706 o snapshot ficou indisponível. Retirada preventiva foi adiada por falta de prova de câmera **e oito oclusões**. O log não pode comprovar que a GPU renderizou o GTI nem que ele estava na frente da câmera quando começou a dirigir.

**Segundo teste (15:01–15:04):** F8 às 15:03:22, candidata a 60,4122 m. Às 15:03:23.876 `PVehicle::Construct` retornou NULL, `phase=constructor_null`, `exception=0`; nenhum carro foi ativado. A factory travou novas criações corretamente para evitar corrupção ou alocação parcial. O número de veículos ativos registrado pouco antes estava próximo a 20; isso **não prova** que o limite de capacidade foi a causa.

Os arquivos de primeira chance não mostram uma falha fatal identificada. A calibração foi restaurada e comprovada pelos dois testes. Os presets de ambos tinham `NativeRivalPrototypeNearPlayer=1`.

## Alterações

1. Com `NativeRivalPrototypeNearPlayer=1` **somente**, o mod avalia cada candidato de rua de 20–120m com o snapshot da câmera do jogador, sem escrever nela. Candidatos cujo volume de segurança cruza o frustum da câmera primária entram antes dos candidatos fora dele. Empates usam distância crescente. Se a amostra de câmera não for válida, a ordem antiga por distância é preservada. Busca, campo de colisão, limites de rua, segurança e decisão de spawn original permanecem inalterados.
2. O log mostra `primaryCameraVerifiedCandidates`, `potentiallyOnscreenCandidates`, `preferredOnscreenCandidate` e, na tentativa efetiva, `cameraFrustumPotentiallyVisible`. Um volume potencialmente visível no frustum **não garante que o renderer tenha desenhado o carro**, nem que o usuário esteja olhando para ele depois da ativação: a IA ainda pode acelerar e sair da câmera rapidamente.
3. Quando o construtor retorna NULL, o log grava `baselineLive`, `baselinePhysical`, `nativeCapacityScalar`, chave do GTI e coordenadas tentadas. As leituras já eram disponíveis antes da chamada; nenhum teste novo de ponteiro é executado depois de um NULL. O mod não tenta instanciar novamente no mesmo processo, pois a alocação parcial ainda é desconhecida.
4. Os estados da factory, seguro F8, retirada preventiva v48 e dados do jogador não mudam. Nenhum limite de capacidade arbitrário é inventado nem há escrita em dinheiro/garagem/jogo.

## Teste

Fechar o jogo completamente, atualizar a ASI pelo ZIP v50 e abrir Free Roam dirigindo GTI. Pressionar F8 em trecho de rua estável e verificar `NativePrototype search` / `cameraFrustumPotentiallyVisible` e a fase `active` no log. Com a câmera voltada para a rua, o protótipo tende a escolher um alvo no seu campo de visão **se o candidato existir e for seguro**. Se o carro sumir, procurar `RICO ... M` no HUD; a posição é medida, não um marcador de imagem/render. Se `constructor_null` acontecer, enviar o novo bloco de diagnóstico e reiniciar para testar de novo. Preservar INIs auxiliares, cache e saves.

## Limitações

Este patch ajuda o teste visual e identifica melhor falhas, mas não muda a IA de Racer, não limita a velocidade, não fixa o carro perto do jogador nem prova carregamento de modelos/renderer. A solução permanente exige pesquisa adicional de spawn/streaming e um objetivo de AI nativo verificado.
