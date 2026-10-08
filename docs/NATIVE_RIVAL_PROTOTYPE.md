# v0.0.33-dev — primeiro protótipo nativo de um rival

Esta versão conecta código real de criação, navegação, ativação e pedido de
remoção ao callback do jogo. **Ainda não foi executada no NFSMW nesta sessão.**
Compilar e passar testes automáticos não confirma que o GTI aparecerá, dirigirá
ou terá remoção correta no seu PC. O teste abaixo busca exatamente essa evidência.

O protótipo fica desativado por padrão. Ele não escreve dinheiro, garagem,
progressão, blacklist ou resultados de corrida. Ainda não oferece desafios,
corridas ou a aparência/identidade persistente de Rico: é um Golf GTI de fábrica.

## Teste no PC

1. Instale o ZIP **v0.0.33-dev** na pasta do jogo, substituindo
   `scripts/FreeRoamRivals.asi`. Preserve seus INIs editados.
2. No arquivo `scripts/FreeRoamRivals/FreeRoamRivals.ini`, acrescente:

   ```ini
   [Experimental]
   NativeRivalPrototypeEnabled=1
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
4. Pressione **F8** uma vez. O protótipo busca um candidato por até 10 segundos.
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

## Busca corrigida na v0.0.33-dev

A busca alterna grupos de até 16 entradas da lista de veículos e avalia no máximo
quatro candidatos por atualização, com cursor separado para cada grupo. A v32
repetia os primeiros alvos e os primeiros quatro candidatos elegíveis; um local
válido mais adiante podia nunca ser examinado. Não há relaxamento de distância,
ocultação, colisão, terreno ou perseguição. O log `NativePrototype search` mostra
o grupo, os alvos capturados, os elegíveis por distância e a janela avaliada.
Mudanças na população/estrada podem mudar os alvos; o teste automático prova a
cobertura com população estável, não a existência de um local seguro no jogo.
