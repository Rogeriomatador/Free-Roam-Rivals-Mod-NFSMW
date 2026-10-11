# v0.0.51-dev — Rico só nasce à frente do jogador

## Pedido resolvido neste patch

Com `NativeRivalPrototypeNearPlayer=1` (modo de teste PLAY atualmente pré-configurado), a tecla **F8 só aceita uma rua à frente do Golf GTI**. Não é mais suficiente estar perto ou potencialmente na câmera: o mod verifica o **vetor para a frente do corpo físico do carro do jogador**, obtido pela interface IVehicle/rigid body que o código já usa em `VehicleSpatialProbe`. Isso funciona mesmo com o jogador parado e não usa o vetor de velocidade, que poderia apontar para trás ao dar ré.

- Candidato de rua precisa estar **20 a 115 m à frente** e dentro do alcance total existente de **20 a 120 m**. O corredor lateral permitido é limitado a 10–30 m (a depender da distância); não escolhe ruas paralelas muito afastadas nem estradas elevadas mais de 7,5 m em relação ao jogador.
- Entre candidatos dianteiros, prefere os volumes possivelmente no campo da câmera do jogador, depois uma direção de estrada semelhante à do jogador e então uma posição de aproximadamente **45 m à frente**, relativamente centralizada.
- O vetor vem do `VehicleSpatialProbe` apenas se a identidade IVehicle coincidir com a do jogador e a posição for coerente com o `PlayerMotionProbe`. Uma lista de veículos inválida, identidade ausente, física ou direção desconhecida **bloqueiam** a criação; nunca se troca por um ponto atrás.
- A fila de fontes de estradas, limite de busca, colisões, cinco testes do terreno, restrições de perseguição, identidade nativa, duas confirmações, preparação da IA e limitação de apenas um veículo são preservados.
- A preferência pela câmera **não prova** que o renderizador desenhou o modelo ou que ele não será rapidamente perdido pela IA. O modo de testes continua sendo para o Golf GTI do jogador, porque não verificamos ainda o carregamento independente de recursos.

## Log e feedback

`NativePrototype search` registra quantos candidatos estavam na distância correta (`distanceWindowCandidates`) e quantos eram realmente dianteiros (`frontConeCandidates`). A tentativa loga `playerForwardMeters`, `playerLateralMeters`, `alignedWithPlayer` e `cameraFrustumPotentiallyVisible`.

Se não houver pista segura à frente, o HUD mostra **SEM PISTA LIVRE A FRENTE** e pede para apontar o carro a uma rua segura. Se o vetor de orientação/identidade física não puder ser validado, exibe **DIRECAO DO CARRO INDISPONIVEL** em vez de nascer atrás.

## Como testar

1. Feche NFSMW e instale o ZIP upgrade desta versão na pasta com `speed.exe`. Preserve o INI principal personalizado em backup; cache de calibração, Rivals.ini, VehiclePools.ini, UndergroundBlacklist.ini e os saves não fazem parte do ZIP de atualização.
2. Selecione Golf GTI e entre no Free Roam em um trecho aberto com espaço **à sua frente**. Você pode deixar o carro parado, pois o vetor é o apontamento físico, não a velocidade.
3. Pressione F8 uma vez. O mod procura, no lote de estradas capturadas, um alvo de aproximadamente 20–115 m *à frente* com corredor lateral e terreno válidos. Se encontrar, tenta criar o Rico usando o mesmo construtor nativo guardado da v50.
4. Se a criação for bem-sucedida, observe rapidamente à frente; a IA Racer pode começar a acelerar sozinha. Se não aparecer, envie log e vídeo/screenshot: o log comprova direção e corpo ativo, mas não renderização. Se `constructor_null` ou outra falha nativa ocorrer, reinicie o jogo antes de outro teste; nunca pressione F8 para forçar duplicação.

## Limitações

Não há teleporte, autospawn, IA que espere o jogador, garantia de visualização, nem carregamento de carros independentes. A v51 resolve **a escolha do lado do jogador** no modo de teste, não o ciclo completo de streaming ou o comportamento da IA. Quando não existe alvo real de rua à frente, recusa com segurança.
