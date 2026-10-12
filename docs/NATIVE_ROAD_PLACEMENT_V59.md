# v0.0.59-dev — correção de criação do Rico (posição após ResetToRoad)

## Evidência: duas sessões reais v58

- No caso de 23:19:11, alvo `(-263.915, 89.5939, 2450.13)`; centro físico real `(-260.566, 90.3918, 2452.15)`. Deslocamento horizontal **3.91031 world units**.
- No caso de 23:21:15, alvo `(572.771, 117.715, 2185.26)`; centro físico real `(570.895, 118.619, 2185.22)`. Deslocamento horizontal **1.87706 world units**.
- A chamada nativa `IVehicleAI::ResetToRoad` (0x422690) retornou `AL=1` e o objeto permaneceu inativo, com rígido físico coerente. A antiga comparação `distance(WRoadNav::fPosition, rigidBody::GetPosition) > 1` tratava qualquer ajuste lateral/físico como falha fatal e forçava reinício de jogo após retirada.
- Ambas ocorreram no EXE MD5 `C0516B485065FABDD69579816B5DF763`. Isso **não** prova que todo deslocamento seja seguro, nem que a origem de ambos centros coincida.

## Alteração técnica

`src/domain/NativeRoadPlacement.h` separa a posição navegação/ancla do centro físico real e faz **verificação do deslocamento nas coordenadas da estrada e dimensões do veículo** (alinhamento de direção, distância longitudinal/lateral limitada pelo footprint, altura e dados finitos). Nos casos de até quatro unidades no plano da estrada, pode passar por esta primeira etapa se os dados forem coerentes; deslocamento excessivo, carro invertido, posição inválida, desnível e footprint anômalo continuam barrados.

**ATENÇÃO:** Aceitar a projeção inicial NÃO autoriza ativação. A etapa seguinte já existente `NativeRivalPrototype::candidateSafe(actualBox)` analisa a posição FÍSICA final: distância ao jogador, ocupação de outros carros na lista completa, cinco consultas de solo para o footprint integral, detecção de perseguição, contexto de mundo e, fora do near-player test, visibilidade/oclusão. A ativação só ocorre depois dessas verificações. Não há teleporte artificial e não muda offsets/ABI nativos.

## Repetir após falha recuperável, sem duplicar

Uma projeção rejeitada mas **fisicamente limitada, sem exceção, com retorno nativo de sucesso e identidade verificada** ainda bloqueia toda atividade até a limpeza nativa. Se o jogo remover o carro e **ambas as listas nativas** confirmarem ausência em **dois frames**, `observeRemoval` libera o bloqueio apenas dessa classe de falha. No máximo duas recuperações por processo; novos F8 exigem pressionar manualmente. Falhas de construtor, identificador/vtable, ABI, SEH, ativação e ponteiro permanecem bloqueadas até reinício. Sem inventar retirada ou reaproveitar ponteiros mortos.

## Limitações e teste

- Uma projeção limitada **não comprova** correto trajeto de IA ou estabilidade da entidade. V58 apresentava sumiço em testes anteriores e streaming continua tema separado.
- A primeira prova necessária no jogo é a sequência `stage=active` e deslocamento real do Golf GTI; unit tests só simulam geometria, nunca chamadas do speed.exe.
- Faça backup do .asi e dos saves; aplique ASI-only. F8 em Free Roam seguro, espere Rico; sem perseguição nem câmera de corrida. Se reaparecer o problema, envie só o trecho `NativeFactory ...` e eventos de `NativePrototype`, com o `NativeExceptions.log` quando houver erro. **Não** modifique o save do jogo original.
- Sprint de 120m permanece opt-in via Ctrl+Shift+G ou INI; ainda exige formação manual e ForceStop experimental. Nada na v59 promete Sprint nativo/AI auto-alignment.

## Pesquisa técnica, grau de confiança

Base verificável: `NativeVehicleFactory::roadResetCall()` e `NativeRivalPrototype::candidateSafe()`, dados de telemetria reais v58 e `NFSPluginSDK` vinculado ao projeto. Interpretação de diferença entre posição nav e centro rígido é uma hipótese geometricamente fundamentada, **não** uma prova da intenção interna de `ResetToRoad`. Os valores de tolerância decorrem de footprint físico e permanecem sujeitos a ajuste após teste no jogo, nunca simplesmente aumento do antigo 1.
