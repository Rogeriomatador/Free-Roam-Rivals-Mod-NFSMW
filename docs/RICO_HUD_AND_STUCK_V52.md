# v0.0.52-dev — Rico: navegação visual, trajetória observada e detecção de imobilização

## Fatos do teste v51 (11 de outubro de 2026)

Log real `FreeRoamRivals(20261011-194058).log`: Rico surgiu **30,16m** do jogador, 29,76m à frente e 4,79m lateral, com direção de estrada alinhada, e sua IA Racer foi ativada. Depois dirigiu mais de 1.510m, mas permaneceu nas coordenadas próximas de (-957, 122, 1847) de aproximadamente **16:36 até 16:38**, com oscilações de poucos centímetros e velocidades perto de zero. Este é o melhor exemplo concreto de imobilização; não está provado se a causa é obstáculo, tráfego, spline de estrada ou objetivo de IA. O mod não pode destravá-lo com segurança apenas por detectar a imobilização.

O mesmo teste criou e interrompeu desafios `route_unmatched` (outras ruas). O sistema Outrun não cria um evento original do jogo e não tem checkpoints ou um destino Sprint: a sua rota é a trilha real do líder medida pelo mod.

## O que foi implementado

1. **Indicador verde do Rico**: a sobreposição gráfica já existente agora mostra uma pequena bússola rotativa, com a seta para a posição do veículo observado e a legenda RICO, próxima à região do minimapa. A direção é extraída da posição e do vetor físico atual **do seu carro**, com identidade conferida na lista nativa e posição cruzada com GameBridge. Funciona quando o jogador está parado. Nunca mostra dados antigos após perda de identidade, menus/transições ou leitura inválida. É uma **sobreposição separada**, NÃO um ícone registrado no `Hud_Minimap` original, e a seta **não segue ruas**. A orientação é relativa ao carro e a distância permanece no painel superior.
2. **Indicação da trilha em Outrun**: durante o desafio, enquanto Rico estiver liderando e o jogador ainda estiver na mesma trajetória conferida, uma seta amarela `TRILHA RICO` indica um ponto **realmente percorrido pelo líder** cerca de 25m adiante na trilha observada (ou a última posição medida com margem suficiente). Se o jogador for o líder, o percurso não for verificável, a trajetória for interrompida ou a amostra não for física confiável, a indicação some. Não inventa curvas, percurso futuro, GPS ou checkpoints.
3. **Rico possivelmente preso**: quando um Rico ativo e carregado passa **12 segundos** com velocidade baixa e posição dentro de um raio de 5m no mesmo nível da rua, o HUD mostra `RICO QUASE PARADO` e um aviso PARADO perto da bússola. A mudança é registrada uma vez por transição. O sinal é um diagnóstico (também pode ser trânsito, pausa e espera), **não** autorização para reset, aceleração, teleporte ou escrita na IA. Nova movimentação >5m ou velocidade >3m/s limpa o aviso; mudança de mundo/perda de dados também o limpa.
4. **Segurança**: permanece a construção nativa guardada, GTI, F8, G e retirada controlada. Nada escreve no HUD, GPS ou minimapa FNG do jogo; todos os desenhos são D3D9 no overlay do próprio mod, com restaurador completo do estado D3D.

## Pesquisa sobre o minimapa original

A engenharia reversa pública documenta `Hud_Minimap` e sua rotina de atualização `0x59db50`; a estrutura de múltiplos ícones também está relacionada aos handlers do veículo/cop. Fontes:
- https://github.com/s-b-repo/nfsmw-2005-re/blob/main/docs/ARCHITECTURE.md
- https://github.com/s-b-repo/nfsmw-2005-re/blob/main/docs/MODDING_GUIDE.md

Esses levantamentos **não** fornecem ainda uma API suportada, lifetime de objetos ou contrato para registrar um novo rival como participante de uma corrida original. O status da v52 é deliberadamente um **indicador verde separado, próximo do minimapa**, não um pin nativo. Próximo passo: comparar objetos/hashes e funções da lista de blips/racers no radar durante corrida original em um executável compatível, sem criar falsamente eventos originais nem sobrescrever slots de outros rivais.

## Como testar

- Feche NFSMW, instale o ZIP upgrade no diretório `speed.exe` e preserve seu INI personalizado em backup. Os salvamentos, `world-metric-v1.state`, `live-rico-*.state`, `Rivals.ini` e `VehiclePools.ini` não são substituídos.
- Abra Free Roam com Golf GTI, pressione F8 em pista livre diante do carro. Com Rico ativo, confira a **seta verde RICO** perto do minimapa; tente virar o carro parado — ela deve mudar de direção relativa, ou desaparecer se a orientação deixar de ser confiável.
- Aproxime-se de Rico por trás na mesma direção e velocidade e use G. **Durante um Outrun**, seguindo Rico numa rota verificada, surge a seta amarela **TRILHA RICO** indicando a trilha percorrida (não uma curva garantida à frente). Em ruas diferentes, a seta não deve inventar uma rota.
- Se Rico parar por mais de 12s, deve surgir aviso de `RICO QUASE PARADO` e log `Rico stationary observation`; não há movimento forçado. Envie log e screenshot ou vídeo da situação para identificar a geometria/IA que o prende.

## Limitações e próximos passos

O motor pode acelerar longe sem esperar o jogador, continuar preso e desaparecer no streaming. Isso **não** foi solucionado na v52. Recuperação de IA precisa de ABI validada, objetivo de navegação verificável e teste real; ícones **dentro** do minimapa e caminhos de Sprint nativos exigem uma integração distinta com o motor. O protótipo continua sendo um único Golf GTI, sem mexer na Blacklist original.
