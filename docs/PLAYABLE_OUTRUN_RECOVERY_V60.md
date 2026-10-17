# v0.0.60-dev — tentativa de aproximação à versão jogável, sem inventar resultados

## O que os logs reais de 17/10/2026 comprovam

- V59 corrigiu a preparação da estrada: o Golf GTI foi criado em **03:30:08**, ativado em **03:30:09**, rodou fisicamente pelo Free Roam e permaneceu acompanhado até **03:33:41**.
- Houve três Outruns, com 3 interrupções: primeiro `route_unmatched` após curvas reais, segundo `world_identity_changed` (transição/perseguição) e terceiro `rival_native_presence_lost`.
- No último incidente, `ownerReadReason=simable_handle`; **os dois registros nativos ainda continham o carro**, mas o _mHandle da interface ISimable já não coincidia com o token capturado na construção. Proibir chamadas mutantes num owner não certificado continua obrigatório.
- Nos mesmos testes Rico percorreu >**2.6 km de distância radial** em relação ao ponto de criação e ainda era amostrado. Assim, a antiga hipótese de corte determinístico ao alcançar 1.024 m foi **REFUTADA pelos novos dados**. Não se deve programar uma correção baseada em teto 1024.
- O arquivo `live-rico*.state` indica `battles=10, wins=0, losses=0, draws=0, aborts=10` — *tentativas*, não prova de dez duelos concluídos.
- O `NativeExceptions.log` do teste v59 não inclui exceção gravada além do inventário; não prova ausência de erros internos.

## O que funciona melhor nesta versão

**Outrun mais parecido com Underground 2**: a condição de vitória/derrota é a distância FÍSICA horizontal/espacial entre os carros durante três segundos ou valor que constar no INI, não a distância acumulada em uma trilha que pode enroscar numa curva/hairpin. Só muda líder em uma **ultrapassagem próxima e geometricamente verificada**. O fato de um carro escolher outra rua deixa de cancelar a disputa automaticamente após 3 s; o HUD esconde instruções da trilha quando não há certeza do percurso. Outrun não cria corrida nativa no gerenciador do jogo, mas dá um resultado competitivo por distância real. Teleportes, transições, falta de telemetria, 300s de duração e cruzamentos continuam protegidos.

**Rico com telemetria temporariamente ausente**: apenas se o log assinalar `simable_handle`, o contexto for o mesmo, o sample anterior tiver menos de dois segundos e AMBAS as listas nativas ainda comprovarem o mesmo owner registrado, uma corrida Outrun (nunca Sprint nem ForceStop) pode ficar congelada até **1,25 s**. Não há leitura ou modificação de ponteiro suspeito, nenhum efeito no controlador nativo e não é feito novo F8. Se a identidade ORIGINAL reaparecer, trilha/tempo de hold reiniciam sem creditar movimentos que não foram amostrados. Se não reaparecer, a corrida continua sendo **interrompida**, sem vitória falsa nem criação duplicada. Este recurso NÃO cura invalidade permanente de `_mHandle` ou despawn do motor.

**Controles e estatísticas mais claros**: Ctrl+Shift+G alterna Sprint experimental/Outrun também antes de F8 quando o protótipo está habilitado, sem alterar INI e saves. HUD identifica pontuação com V, D e AB e indica a distância física e como vencer. G pede desafio; F8 cria Rico.

## O que ainda bloqueia uma versão final de verdade

1. **Ciclo de vida nativo**: simable_handle mudou com o carro ainda nas listas — é preciso documentar o efeito do engine, aprender uma API de pin/keepalive comprovada ou criar uma abordagem que não depende de um token permanente; jamais reatribuir handle por esperança.
2. **IA durante Sprint**: v58/59 ainda exigem alinhamento manual, ForceStop experimental e uma reta limitada de 120 m. Não existe ainda uma rota nativa válida com checkpoints curvos, autoesteering/autorregulação e restauração comprovada do input.
3. **Produção**: mod conserva F8 manual e Golf GTI de teste, não seis rivais nativos robustos, nem dinheiro/pink-slip/garagem gravados no jogo. Bancos de carros/progressão são arquivos declarativos, não prova de modificação do game real.
4. **Testes finais**: somente o jogo do usuário valida callbacks, lifetime, comportamento de trânsito/colisão e coexistência com WidescreenFix, ExtraOptions, XtendedInput etc.

## Referências examinadas

- [Código de MW Native Free Roam Racer, MIT](https://github.com/Zakkey250/MW-NativeFreeRoamRacer) — uma implementação independente de concorrência por 300 m baseada em carro físico, líder observado e trilhas de orientação; usada como **referência arquitetural**, sem copiar binários ou fazer o mod depender do ASI alheio. Recomenda ASIs separados: rodar ambos simultaneamente pode causar conflito de ownership/slots.
- [NFSPluginSDK](https://github.com/berkayylmao/NFSPluginSDK) — layouts de `ISimable`, `PVehicle`, `IVehicle`, `AIVehicle`; SDK autoidentifica parte do jogo, não prova que o token `_mHandle` permaneça fixo quando o engine remove ou recicla um simable.

## Teste in-game v60

Instale apenas o ZIP ASI-only após backup do anterior e perfil. Entre no Free Roam, com o mod ativado, selecione o modo se quiser usando Ctrl+Shift+G, crie Rico com F8, siga e pressione G atrás dele. Tente se afastar mais de 300 m numa reta ou em curvas, sem usar teleport nem sair da sessão. Resultado V/D exige distância física e tempo de espera. Registre se os dois carros estiveram fisicamente presentes em todo o período. Se o rival sumir e não reaparecer, a versão ainda não resolveu a origem do problema. Para Sprint, use uma cópia isolada do jogo: controle de parada física permanece em ensaio.
