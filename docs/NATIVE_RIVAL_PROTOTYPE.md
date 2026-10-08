# v0.0.36-dev — primeiro protótipo nativo de um rival

Esta versão conecta código real de criação, navegação, ativação e pedido de
remoção ao callback do jogo. **Ainda não foi executada no NFSMW nesta sessão.**
Compilar e passar testes automáticos não confirma que o GTI aparecerá, dirigirá
ou terá remoção correta no seu PC. O teste abaixo busca exatamente essa evidência.

O protótipo fica desativado por padrão. Ele não escreve dinheiro, garagem,
progressão, blacklist ou resultados de corrida. Ainda não oferece desafios,
corridas ou a aparência/identidade persistente de Rico: é um Golf GTI de fábrica.

## Teste no PC

1. Instale o ZIP **v0.0.36-dev** na pasta do jogo, substituindo
   `scripts/FreeRoamRivals.asi`. Preserve seus INIs editados.
2. No arquivo `scripts/FreeRoamRivals/FreeRoamRivals.ini`, acrescente:

   ```ini
   [Experimental]
   NativeRivalPrototypeEnabled=1
   NativeRivalPrototypeNearPlayer=1
   ```

   Use a seção existente, sem duplicá-la. Os antigos `ExperimentalSpawnEnabled`
   e `ExperimentalAIControlEnabled` podem continuar em 0. Este novo interruptor
   é o que autoriza o protótipo. Mantenha `RenderProbeEnabled=1` em Diagnostics;
   o callback de gameplay é instalado também para o protótipo.
3. Entre no Free Roam com um **Volkswagen Golf GTI**. O primeiro teste exige esse
   modelo para reutilizar recursos do modelo já carregado; não troca seu carro.
   Dirija em trecho relativamente reto com velocidade estável, sem perseguição,
   para obter a calibração de distância. O log registra `World metric calibration
   verified`. Não há escala de metros inventada nem temporizador que substitua
   essa evidência.
4. Com `NativeRivalPrototypeNearPlayer=1`, o teste permite aparecer à vista
   em um alvo livre de estrada a **20–120 metros**, dando prioridade aos mais
   próximos de cada grupo. Não garante um carro exatamente ao lado: depende
   de uma navegação válida capturada das IAs e espaço livre. Para a busca
   oculta original a 350–850 m, use `NativeRivalPrototypeNearPlayer=0`.
   Pressione **F8** uma vez. O protótipo busca um candidato por até 10 segundos.
   Se não houver local seguro, a tentativa termina sem criar veículo; pode
   dirigir para outra rua e pressionar F8 novamente. O log informa o bloqueio.
5. Se o log chegar a `NativePrototype stage=active`, houve retorno bem-sucedido
   das chamadas de construção, reset da estrada e ativação. Isso ainda não prova
   movimento ou renderização. As observações seguintes incluem modelo, posição,
   velocidade, deslocamento e estado de perseguição do jogador. Procure o GTI e
   acompanhe seu comportamento; não há marcador/mapa customizado nesta versão.
6. Para testar retirada, pressione **F8** de novo. A remoção espera que o rival
   esteja oculto, a pelo menos 300 metros do jogador, e que perseguição/cooldown
   estejam ausentes por uma janela estável. Afaste-se sem iniciar perseguição.
   `stage=finished` significa que ambas as listas de veículos omitiram suas
   identidades em dois callbacks diferentes depois do pedido nativo de remoção.
7. Envie `scripts/FreeRoamRivals/FreeRoamRivals.log` completo, com o que foi visível
   no jogo. Há uma construção por sessão; reinicie o jogo para um segundo teste
   após construir/remover ou após uma falha nativa.

Se F8 não produzir efeito, confira o log: compatibilidade, callback, GTI,
calibração, população e posição segura são requisitos. Não habilite outras
escritas experimentais para contornar bloqueios. Para retornar à observação,
coloque `NativeRivalPrototypeEnabled=0` e reinicie o jogo.

## Caminho implementado

- Um veículo máximo, somente em callback externo após a atualização original.
- Estado de perseguição fresco: qualquer ponteiro de perseguição, cooldown ou
  estado desconhecido bloqueia criação/alterações/remoção. O estado do rival
  também é verificado antes de alterar sua IA ou solicitar remoção.
- Alvo de estrada capturado de uma IA de tráfego/corredor atualmente registrada.
  Apenas segmento, nó, parâmetro, pista, deslocamento lateral e validade são
  copiados. Nenhum WRoadNav/ponteiro de cookie/spline de outro carro é retido.
- Distância de criação/ativação 350–850 m, gráfico de estrada no mesmo contexto,
  terreno sob centro e quatro cantos, leitura completa de veículos e sem
  sobreposição com margem. Antes da construção reserva-se um volume conservador
  de semieixos 3×2×8 unidades de mundo; o corpo real precisa caber nesses limites
  e passa por nova checagem antes de ativar.
- Ocultação específica do protótipo exige exclusão do frustum da câmera principal
  e oito raios de colisão de mundo bloqueados antes dos cantos do volume. Isso
  **não promove** `spawnVisibilityVerified` nem comprova espelhos, sombras,
  todas as vistas, streaming de renderização ou um sistema de despawn definitivo.
- A fábrica recusa capacidade física esgotada sem executar a rota nativa que
  mata outro veículo. Cache/customização/desempenho externos não são emprestados.
- O objeto criado é desativado imediatamente, aguarda carregamento, recebe
  DriverClass::Racer e reset nativo de estrada usando sua própria navegação.
- A sequência final é **SetSpawned → SetGoal(Racer) → Activate**. Foi confirmado
  no executável que SetSpawned chama ResetInternals e limpa a meta anterior.
- Remoção solicita Deactivate/Kill nativos; não usa delete/free. Após Kill,
  confirmação consulta apenas associação numérica às listas, sem ler o objeto
  retirado. Transições de mundo bloqueiam acessos por ponteiros antigos. O
  protótipo não tenta forçar limpeza através de um contexto perdido.

## Correção do diagnóstico anterior

GetCurrentRoad (`0x442A70`), GetFutureRoad (`0x442A90`) e os getters de posição
futura/seek-ahead chamam UpdateRoads: não são leitura pura. O diagnóstico agora
lê os campos embutidos, sem chamá-los no render. As entradas nativas mostram
CurrentRoad em IVehicleAI+0xF4 e FutureRoad em IVehicleAI+0x3DC. O layout WRoadNav/USpline/Matrix4 do SDK não corresponde ao layout completo
nativo: os offsets de pista e o tamanho de WRoadNav diferem. A alteração usa o
endereço interno verificado para FutureRoad, os campos iniciais confirmados e
leituras explícitas de fim de estrada/pista/offset lateral em +0x2C0/+0x2C1/+0x2C4.
As verificações compiladas detectam essa diferença; não alocamos ou copiamos
objetos nativos pelo tamanho declarado no SDK.

## O que continua pendente

A primeira execução poderá revelar falha de criação, carregamento, goal/ação,
pose, renderização, IA imóvel, remoção ou incompatibilidade com outros plugins.
Falhas nativas interrompem novas construções. Um objeto parcial cuja identidade
não possa ser provada não é adivinhado/deletado; reiniciar o jogo pode ser
necessário. Nenhum desses caminhos foi apresentado como testado no jogo.

A perseguição/arresto/reengajamento e a conservação do carro em streaming precisam
ser validados. O protótipo não promete manter um rival vivo para sempre nem impedir
a lógica nativa de destruí-lo. `movementObserved` registra deslocamento medido;
sozinho não prova que a IA causou esse movimento. O primeiro objetivo de teste é
verificar se o GTI aparece, dirige por conta própria e pode ser retirado sem
atingir outros carros. O sistema completo de rivais continua em desenvolvimento.

## Busca corrigida na v0.0.35-dev

A busca alterna grupos de até 16 entradas da lista de veículos e avalia no máximo
quatro candidatos por atualização, com cursor separado para cada grupo. A v32
repetia os primeiros alvos e os primeiros quatro candidatos elegíveis; um local
válido mais adiante podia nunca ser examinado. Não há relaxamento de distância,
ocultação, colisão, terreno ou perseguição. O log `NativePrototype search` mostra
o grupo, os alvos capturados, os elegíveis por distância e a janela avaliada.
Mudanças na população/estrada podem mudar os alvos; o teste automático prova a
cobertura com população estável, não a existência de um local seguro no jogo.

## Alvos vazios observados no PC e fonte de navegação da v34

O log 13:04:29–13:07:16 de 08/10/2026 confirmou `nativePrototype=1`,
calibração e comandos F8, mas todas as buscas registraram `capturedTargets=0`.
Nenhuma construção ou ativação foi tentada. O fechamento da primeira execução
aconteceu antes da entrada no mundo; o log não determina sua causa.

A v34 inclui a navegação própria de direção da IA (DriveToNav), além dos caches
CurrentRoad/FutureRoad. No executável alvo, o getter 0x431C50 lê o ponteiro em
IVehicleAI+0x24; a leitura verifica o slot 18 e os quatro bytes da entrada, sem
chamá-lo. Apenas os seis escalares são copiados, com as mesmas validações.
Os caches atual/futuro dependem de UpdateRoads e podem estar inválidos; este
log anterior não identifica qual validação rejeitou cada carro. A nova fonte
é uma correção de cobertura, não prova de que os alvos aparecerão no PC.

`NativePrototype search` agora mostra `captureStatus`, entradas da lista,
entradas examinadas e contadores de rejeição por veículo, driver, contrato de
IA, ponteiro/memória de navegação, escalares, mudança, geometria e exceção.
O protótipo também evita leituras adicionais de mundo/fábrica enquanto estiver
Idle/Finished/Disabled; o F8 inicia uma nova janela estável. Isso reduz o trabalho
antes da solicitação e não confirma a causa nem a resolução do fechamento.

## Terreno corrigido na v35

O log da v34 de 14:24:35–14:27:45 confirmou a captura de 5–10 alvos de
estrada, com até seis na faixa de distância. Não houve construção: as
rejeições registradas foram terreno sob a footprint e espaço ocupado.

O protótipo exigia normal.y >= 0.5, mas usava o fallback diagnóstico
que lança um raio de baixo para cima. O executável 0x78574F–0x7857B7
calcula normal dot (origem − impacto) e nega os componentes quando negativo
(as instruções de troca de sinal estão em 0x78579D/0x7857A7). Logo, uma
face plana vista de baixo devolve normal para baixo e falha nessa condição.
O log não continha a normal; a incompatibilidade foi confirmada estaticamente,
não por medição do resultado da chamada no PC.

A v35 usa somente no protótipo um raio local de Y+2 até Y−4 unidades de
mundo. Mantém normal para cima, delta de altura <=1.5, inclinação <=0.35,
centro e quatro cantos, lista completa, distância, perseguição e ocultação.
O probe diagnóstico antigo permanece disponível. Uma regressão analítica
valida direção/normal e mantém rejeições de inclinação, altura, barreira e
chão ausente; não executa o motor do jogo. Rejeições passam a registrar
normal, altura, inclinação, tipo e conclusão da chamada.

Novos apertos de F8 durante Seeking não cancelam a busca nem reiniciam
a janela estável. Aguarde até 10 segundos. F8 após construção/ativação
ainda solicita retirada segura. Criação e movimento continuam pendentes.

## Teste visível perto do jogador na v36

O log de 08/10/2026, 15:44:39–15:45:38, registra alvos capturados e
rejeições por `primary_camera_and_eight_world_rays`, além de posições com
altura/cantos inadequados. A ocultação é verificada depois do chão e espaço
livre: alguns candidatos chegaram a essa etapa. Nenhuma construção foi
registrada. Isso não prova renderização, IA ou estabilidade do rival.

`NativeRivalPrototypeNearPlayer=1` é uma exceção explícita de visibilidade e
distância somente no teste manual F8 habilitado. Tanto construção quanto
ativação usam a mesma política de 20–120 m. A visibilidade é marcada como
possivelmente presente, sem fabricar prova de ocultação ou streaming.
Distâncias não finitas e fora do intervalo são rejeitadas, mesmo com prova
de streaming. A faixa normal continua 350–850 m no protótipo, com ocultação.

O teste mantém GTI do jogador, contexto, calibração, janela estável,
perseguição/cooldown, população, capacidade, terreno sob todo o volume e
lista completa de veículos sem sobreposição. Não copia a posição do jogador,
não desloca arbitrariamente uma semente de estrada e não remove o tráfego.
Pode terminar sem candidato quando as posições próximas estiverem ocupadas.
A remoção continua exigindo ocultação e distância de 300 m também neste modo.

O log da construção registra modo, distância medida e posição. Reinicie o
jogo para carregar o INI alterado. Procure um trecho reto/plano com tráfego,
calibre dirigindo e pressione F8 uma vez. O carro pode aparecer à frente,
atrás ou numa rua próxima; sem marcador, ainda é necessário confirmar sua
presença visualmente. Testes automáticos verificam política e regressões,
não executam o jogo. A primeira criação/ativação/movimentação real permanece
pendente até o novo log e observação no PC.
