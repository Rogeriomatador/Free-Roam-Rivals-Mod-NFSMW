# v0.0.42-dev — rival nativo e desafio outrun

A captura real da v41 confirmou o GTI dirigindo. A v42 integra desafio G, painel e histórico próprio; esses componentes ainda exigem teste no jogo. Veja [LIVE_RIVAL_V42.md](LIVE_RIVAL_V42.md) para instalação, evidências e limites. O rival segue sua IA nativa de passeio; não há corrida oficial criada ou IA seguindo a rota do jogador.

## Teste no PC

1. Instale o ZIP **v0.0.42-dev** na pasta do jogo, substituindo
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
6. Para testar retirada, segure **F7 por 1,5 segundo**. A remoção espera que o rival
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

## Compatibilidade bloqueada no PC e auditoria da v37

O log da v36 de 08/10/2026, 16:14:39–16:17, contém 125 solicitações de
construção, todas recusadas no teste de assinatura de SetGoal (0x422480):
esperado 0x849130427be78946, memória 0x06790c5598f847e8. Solicitar construção
não significa invocar o construtor. Nenhum GTI foi alocado por esse caminho.

Os 11 hashes foram recalculados no arquivo speed.exe disponível, com tamanho
6029312 e MD5 C0516B485065FABDD69579816B5DF763; todos correspondem aos valores
esperados. Isso exclui erro desses valores em relação a esse arquivo, mas o
log anterior não contém bytes suficientes para atribuir a diferença em
memória a um plugin ou a outro mecanismo. A posição exata da diferença e a
identidade de um eventual hook ainda são desconhecidas.

A v37 não troca nem ignora assinaturas. Antes de instalar o hook de capacidade
ou chamar o construtor, compara TODAS as 11 janelas de 256 bytes com seus
hashes esperados e com a leitura do executável em disco. A leitura usa a
correspondência RVA/offset da seção .text do alvo exato, somente após o guard
de tamanho/MD5 e com limites de janela. Cada função registra legibilidade,
hash de memória, hash de arquivo e resultado. Leitura indisponível bloqueia.

Para divergências, o log registra os primeiros 32 bytes da memória/arquivo,
o primeiro offset diferente e uma janela de 32 bytes ao redor. Se houver
formato E9 ou FF25 na entrada, mostra o destino legível e o nome do módulo
que contém esse endereço, quando resolvível. Um formato de salto sozinho não
prova hook de terceiros, e sua ausência não exclui patch interno à função.
Esses bytes são diagnósticos locais do teste, não fixtures do jogo no repo.

Após auditar as 11 funções, qualquer divergência interrompe novas tentativas
nessa sessão, sem executar o construtor ou instalar o hook da fábrica. Isso
substitui os 125 avisos repetidos por uma auditoria completa. Reinicie após
resolver o conflito. `native construction request` identifica a solicitação;
`NativeFactory constructor invoke` distingue a chamada efetiva, que só ocorre
após compatibilidade e os demais requisitos passarem.

Para obter a evidência, instale a v37 preservando o INI próximo da v36, entre
no Free Roam com GTI, calibre dirigindo e aperte F8 uma vez. Envie o log
completo. Não é necessário remover plugins para essa primeira auditoria.
Um teste separado com menos plugins só deverá ocorrer após salvar sua
configuração e identificar as diferenças; não sabemos ainda qual é o responsável.

## Diagnóstico amplo independente do spawn na v38

`DiagnosticBundleEnabled=1` em Diagnostics ativa o preset de observação e F9.
F9 permite obter código/módulos/contexto sem aguardar um candidato de F8.
Não autoriza criação nem relaxa assinaturas. Veja DIAGNOSTIC_BUNDLE.md para
coleta, limites, relatório e próximos passos. O primeiro rival real continua pendente.

## v0.0.39 — confirmação após o construtor

GTI visível relatado pelo usuário na v0.0.38, mas ativação não alcançada. A v0.0.39 confirma identidade em duas atualizações concluídas, com limite de dois segundos, antes de preparar o corredor. Não repete o construtor. Falhas agora registram etapa/identidade/exceção. Veja `TARGET_CAPTURE_V38_VISIBLE_GTI.md`. Mantenha Bartender desativado neste teste. Direção e retirada ainda exigem validação no jogo.


## v0.0.41 — confirmação limitada às identidades necessárias

A captura real v0.0.40 de 16:32 passa nas 11 assinaturas e retorna um GTI (handle 34), mas para em `preexisting_registry_identity_lost` antes de Deactivate, preparação Racer, navegação e Activate. A comparação anterior exigia que toda a frota anterior permanecesse registrada durante outras atualizações do jogo. O log não identifica qual veículo saiu nem demonstra sua causa; não prova falha da IA Racer.

A v0.0.41 verifica a preservação de toda a frota imediatamente após o construtor, na mesma execução síncrona, mantendo o bloqueio de expulsão por capacidade. Uma perda nesse momento registra os primeiros endereços ausentes e bloqueia a adoção. Entre frames, a comparação da frota anterior é apenas telemetria: não congela veículos alheios. A confirmação exige jogador e GTI em ambos os registros atuais, contexto/perfil/road/race iguais, e verifica vtable, simable, handle, modelo e exclusão de veículo do jogador em cada um dos dois frames. Ambas as perseguições precisam estar verificadamente livres, inclusive cooldown do jogador. Nenhum ponteiro ausente é desreferenciado. As etapas posteriores conservam seus próprios bloqueios.

A confirmação acontece a cada callback de gameplay concluído, sem a espera de 250 ms usada na busca. Dois frames distintos continuam obrigatórios; o limite de dois segundos e a proibição de repetir o construtor permanecem. F8 durante confirmação continua apenas enfileirando retirada para depois da identidade confirmada.

Veja `TARGET_CAPTURE_V40_CONSTRUCTION_CONFIRMATION.md` para evidências, pesquisa e teste. A correção remove um bloqueio demonstrado no log; **não comprova direção autônoma**. Instale somente o novo ASI preservando seu INI configurado. Fora de perseguição, calibre dirigindo seu Golf GTI, pressione F9 e F8 uma vez. Observe a preparação/ativação e envie o log mesmo se ainda ficar parado. O ZIP mantém o protótipo desligado por padrão.

## Atualização v42

F8 não retira um rival existente. F7 segurado solicita retirada com os bloqueios originais de ocultação/distância/perseguição. Um resultado outrun nunca solicita retirada. Observação temporariamente indisponível pausa a integração e interrompe pontuação; só ausência confirmada nos dois registros é tratada como retirada externa. Isso não prova que o motor nunca descarregará o carro.
