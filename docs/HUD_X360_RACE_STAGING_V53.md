# v0.0.53-dev — HUD adaptável, diagnóstico X360Stuff e base da largada sem cutscene

## Relato e evidências reais (11/10/2026, v52)

Arquivos inspecionados: `FreeRoamRivals(20261011-221209).log`, `NativeExceptions(20261011-221209).log`, `FreeRoamRivals(9).ini`, configurações de carros/rivais, calibração e persistência.

- O executável continua sendo **speed.exe 6.029.312 bytes, MD5 C0516B485065FABDD69579816B5DF763**. O WidescreenFix e XtendedInput estão carregados; não há módulo chamado X360Stuff na lista fornecida. **Isso não prova** ausência de alterações de texturas ou wrappers sem esse nome.
- Rico foi instanciado e confirmado em 19:04:39–19:04:40 e ativado; viveu fisicamente por minutos e participou de duas disputas iniciadas às 19:05:28 e 19:07:53. Uma terminou `route_unmatched`, outra `world_identity_changed`. Não houve vitória real no jogo original e o jogo não criou um evento RaceStatus original.
- Em 19:08:18, ocorreu `owned_vehicle_unavailable_wait_without_cleanup` após transição/contexto do mundo. A retirada após 19:09:17 continuou sem identidade carregada suficiente para Kill verificado e segundo spawn foi bloqueado para não corromper listas nativas. **Esta versão não resolve stream-in/out físico**, que requer um ciclo de carregamento de recursos e um ownership seguro e comprovado.
- Em tentativas anteriores, a construção produziu `constructor_removed_preexisting_identity`. Trata-se de falha de segurança importante, não apenas problema visual.
- `UseHornToChallenge=1` **é informativo**: a ação de buzina nativa não foi identificada com ABI no mapeamento disponível. **G continua sendo a tecla funcional**.

## Entregue na v53

1. **Novo radar redesenhado do FRR:** bússola verde estilizada, distância em metros e marcações de orientação; padrão na **lateral direita** para não sobrepor o minimapa no canto inferior esquerdo. Não insere nem corrige o minimapa original.
2. **Interface dimensionada pelo viewport D3D9**: escala considera largura e altura (4:3, widescreen/ultrawide, 1080p/4K), com limites seguros. A HUD do FRR não depende de o Xbox360 Stuff Pack estar instalado para posicionamento adequado.
3. **Configurável no INI principal** em `[LiveRival]`:
   - `RadarPosition=0`: canto inferior esquerdo (comportamento legado);
   - `RadarPosition=1`: canto superior direito;
   - `RadarPosition=2`: meio direito, novo padrão;
   - `RadarPosition=3`: ocultar a bússola adicional.
   - `RadarScale=1.00`: tamanho (0.5–1.5).
   A seta de percurso Outrun fica numa posição relativa à tela, afastada do topo.
4. **Reconhecimento de alinhamento para futuro início de corrida sem cutscene**: quando jogador e Rico estão lado a lado, no mesmo sentido/altura e quase parados, o HUD mostra que a formação foi detectada. **Não trava controle, não freia Rico nem começa contagem** enquanto os três requisitos de segurança não forem provados: bloqueio de comandos do jogador, controle nativo de freios/objetivo de IA e restauração de ambos em qualquer abort. Esta é uma etapa real de instrumentação, não largada pronta.
5. Novos testes de posição para cinco resoluções e regras de alinhamento (distância, velocidade, rua e requisitos de restauração). Mantém G/Outrun existentes.

## X360Stuff — instalação controlada, não garantia de compatibilidade

A documentação oficial do WidescreenFix mostra `FixHUD`, escala e `HudAspectRatioConstraint`. O Xbox 360 Stuff Pack atual envolve múltiplos shaders/texturas/hooks, e alguns conjuntos incluem `4GB Patch` e arquivos `d3d9.dll`. Já existem relatos públicos de conflitos/fechamentos com WidescreenFix. Referências:
- https://github.com/ThirteenAG/WidescreenFixesPack/blob/master/.github/docs/nfsmw.md
- https://www.youtube.com/watch?v=rh4wLkPF3g0
- https://www.reddit.com/r/needforspeed/comments/1ugz4ph/most_wanted_xbox_360_stuff_mod_41/
- https://nfsmods.xyz/mod/1200

**Não altere o speed.exe do teste atual**: muitos patches 4GB alteram o hash/tamanho de PE32 e a proteção estrita do FRR o recusa. Para testar, duplique toda a pasta do jogo antes de instalar o pack; mantenha uma cópia conhecida como boa, execute sem duplicar diferentes dinput8.dll ou WidescreenFix. Se a versão do X360Stuff substituir speed.exe, o mod continuará **fail-closed** até que essa variante seja auditada. Não desative `RequireSupportedExecutable` para forçar chamadas nativas.

Para localizar um crash, envie os logs após reproduzir num diretório de teste, a versão exata do pack, e confira se `speed.exe` continua com MD5 suportado. Instale primeiro um componente por vez e faça comparação: WidescreenFix + FRR; depois visual/recursos X360; depois componentes de runtime específicos.

## Próximas implementações que exigem prova de motor

- **Streaming/persistência física:** observer de perda, representação lógica separada da entidade física e recarga segura somente após ausência registrada em PVehicle/IVehicle, recursos de modelo residentes, estrada ocupável e persistência do mundo. Não reutilizar ponteiros de mundo antigo nem instanciar novamente após `constructor_removed_preexisting_identity`.
- **Largada automática sem cutscene:** resolver evento REAL da buzina, adquirir e restaurar controle do jogador, travar acelerador/direção de ambos com funções verificadas, frear e alinhar sem teleporte visível, apresentação 3-2-1 e soltar simultaneamente, abortar limpando todos os sinais. A estrutura `StagingStateMachine` já existe mas seus sinalizadores `suppressPlayerControl`/`requestHiddenAlignment` **não são mutações implementadas**.
- **Rota GPS e marcador verde original:** exige API e lifespan de FNG minimap, evento nativo ou overlay de caminho verificado; bússola relativa é diferente de um ícone nativo.
