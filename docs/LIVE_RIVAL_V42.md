# v0.0.42-dev: integração jogável e limites

## Evidência real de partida

A captura de 10/10 `FreeRoamRivals(20261010-063114).log` (SHA-256 `02cc81f3a316ac0679f56238d92226f8337cf3b491ed5b5ea2afd2d977cc676c`) foi preservada integralmente na branch `checkpoint/v0.0.41-driving-2026-10-10`. As onze assinaturas passaram; o construtor retornou handle 614; preparação Racer, estrada e ativação concluíram. O GTI acelerou de 5,517 a 46,8413 m/s e percorreu 837,735 metros até a última observação ativa. O jogador confirmou movimento visível. Às 03:30:36,015 há **pedido** de retirada por F8, seguido de adiamentos e transição de mundo. Não há confirmação de retirada. Não é possível afirmar a causa do desaparecimento relatado.

## Instalação e uso

1. Substitua `scripts/FreeRoamRivals.asi` pelo binário v42. Para habilitar a nova disputa, copie `FreeRoamRivals-play.ini` para `scripts/FreeRoamRivals/FreeRoamRivals.ini`. Esse preset deriva do último INI fornecido pelo usuário e mantém suas opções diagnósticas. Os outros três INIs podem ser preservados.
2. Entre no Free Roam com seu Golf GTI, sem perseguição, e dirija para calibrar a distância. Pressione F8: criação de um GTI em uma estrada livre a 20–120 m. Não existe criação garantida exatamente ao lado do jogador.
3. Acompanhe o rival por trás, na mesma direção, a até 60 m, ambos acima de aproximadamente 11 km/h e com diferença de velocidade abaixo de aproximadamente 30 km/h. O painel oferece G quando a aproximação e as duas perseguições estão verificadamente seguras.
4. Pressione G. A disputa termina ao sustentar 300 m de vantagem por três segundos; no limite de cinco minutos, vence quem lidera a trajetória observada (empate dentro de 5 m). A vantagem acompanha a trilha do líder, com verificação de ultrapassagem próxima; não usa apenas distância em linha reta. Uma trajetória incompatível por três segundos interrompe a disputa. O painel mostra resultado, tempo, vantagem, progresso da sustentação e histórico.
5. Um resultado não solicita retirada do rival. Após 15 segundos de intervalo, uma nova aproximação pode permitir outro desafio. Perseguição durante uma disputa já aceita não altera sua pontuação nem provoca remoção pelo mod; iniciar outra exige as duas perseguições livres. Isso não garante o comportamento da polícia ou a sobrevivência do carro no motor.
6. F8 novamente **não retira** o carro. Segurar F7 por 1,5 segundo solicita retirada, interrompendo uma disputa em andamento. A remoção continua aguardando ocultação, distância mínima de 300 m, identidades verificadas e perseguições livres. Não existe respawn na mesma sessão após construção, erro ou retirada.
7. Para verificar os componentes novos, envie `FreeRoamRivals.log` e `NativeExceptions.log`, descrevendo painel, desafio, ultrapassagem, resultado e continuidade do rival. Cada reinício substitui o arquivo de exceções: preserve a captura antes de reiniciar.

Não sobreponha um `FreeRoamRivals.asi` antigo em outra pasta de plugins. Os testes anteriores usaram isolamento de Bartender/X360Stuff; não há prova de que todo conjunto de mods seja compatível.

## O que a versão integra

| Componente | Comportamento real | Limite |
|---|---|---|
| Criação/IA | Mesmas chamadas nativas da v41, com guardas originais | Um GTI de fábrica; jogador também precisa usar GTI |
| Desafio | G conectado ao rival vivo e verificado | Buzina não identificada; sem evento oficial criado |
| Corrida | Pontuação outrun do mod sobre trilha limitada do líder | IA nativa de passeio não sabe do desafio nem segue a rota escolhida pelo jogador |
| Painel | Alfabeto bitmap próprio no EndScene existente, estado D3D9 restaurado | Renderização/posição visual precisam de teste no jogo; HUD desativável |
| Memória | Vitórias, derrotas, empates e interrupções por hash de perfil | Sem dinheiro, garagem, customização ou alteração do save de carreira |
| Polícia | Bloqueia novas disputas em estado não livre; disputa aceita continua | Não reprograma policiais, heat, perseguição ou comportamento do rival |
| Persistência do carro | F8 não retira; resultado não retira; observação ausente não aciona cleanup | Streaming/destruição/retirada pelo motor ainda não têm solução comprovada |

Nome Rico é a identificação do painel; não implica pintura, peças, personalidade ou uma garagem persistente implementadas. Blacklist própria, eventos de destino, apostas, pink slips, cinematics, encontros e múltiplos modelos continuam como domínios/configurações de projeto, sem integração completa no jogo.

## Dados e falhas

Histórico em `scripts/FreeRoamRivals/saves/live-rico-<hash-do-perfil>.state`, separado do save original. Formato limitado a 512 bytes e um milhão de resultados. Validação estrita do perfil e totais; arquivos corrompidos, divergentes ou mais novos são preservados. Gravação Windows em temporário, flush e substituição atômica. Falhas são registradas e o histórico da sessão permanece em memória; não há transação econômica.

Mudança de jogador/estrada/race status, fade/NIS/carregamento, teleporte, posição inválida, amostra inconsistente ou intervalo de atualização acima de um segundo interrompem a disputa. Delta zero não avança o tempo. Não há nova leitura de ponteiros antigos durante mudança de mundo. Nenhum endereço novo ou assinatura relaxada foi adicionado para a disputa.

`[General] Enabled=0` evita instalação dos hooks de runtime. `[LiveRival] HUDEnabled=0` oculta o painel; `PersistHistory=0` mantém os resultados somente durante a sessão. `[Outrun] Enabled=0` mantém apenas passeio/observação do rival. Valores numéricos não finitos ou malformados retornam ao padrão seguro.

## Pesquisa de implementação

- [TsyVM/MWSDK](https://github.com/TsyVM/MWSDK/tree/6db158647fe05a3d1cbbfee38f0f5d1e91b2f473): interfaces nativas de veículo/IA, navegação e perseguição já utilizadas pelo projeto. Fonte fixada, tipos confrontados com o executável alvo; a existência de um método no SDK não prova que uma chamada nova é segura.
- [dbalatoni13/nfsmw](https://github.com/dbalatoni13/nfsmw/tree/1f2cdd7996791c81a580b3f7b36b44d4f9f6719c): organização de goals/navegação/população usada para orientar investigação. Reconstrução pública não substitui teste no executável suportado.
- [TsyVM/MWEncyclopedia](https://github.com/TsyVM/MWEncyclopedia): referências de ações/IA como pistas. Não são prova de ABI e não foram transformadas em escrita especulativa.
- [Zakkey250/MW-NativeFreeRoamRacer](https://github.com/Zakkey250/MW-NativeFreeRoamRacer): comparação do fluxo de free roam; licença permite estudo, mas restringe redistribuição/modificação. Nenhum código desse mod foi importado ou redistribuído. A pontuação, painel e armazenamento desta atualização são implementações próprias.
- [Microsoft: IDirect3DDevice9::CreateStateBlock](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-createstateblock) e [IDirect3DStateBlock9::Apply](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3dstateblock9-apply): captura/restauração do estado gráfico; nenhum recurso GPU permanente mantido entre resets.

Não há execução do NFSMW no ambiente de desenvolvimento. Testes de domínio, arquivos, ABI e hooks não provam renderização, estabilidade de perseguições ou duração do carro no jogo. Esta versão permanece **dev**, e não é apresentada como final.
