# v0.0.47-dev — Inventário seguro de recursos do Most Wanted

## Pesquisa de projetos públicos

Esta versão usa como referências documentadas, **não** como dados extraídos do jogo do usuário:
- [NFS Modder's Corner — Need for Speed Most Wanted](https://nfsmodderscorner.blogspot.com/p/need-for-speed-most-wanted.html): estrutura padrão CARS/<XNAME>/GEOMETRY.BIN e TEXTURES.BIN, GLOBALB.LZC e bancos de atributos.
- [NFSTools/GlobalLib](https://github.com/NFSTools/GlobalLib): leitores abertos de GLOBALA.BUN, GLOBALB.LZC e tipos/presets de veículos. Não incorpora a biblioteca e não modifica arquivos originais.
- [TsyVM/MWSDK](https://github.com/TsyVM/MWSDK): decodificação de TRACKS/L2RA.BUN e STREAML2RA.BUN, CARP/waypoints e reconhecimento de estruturas nativas. O mod já usa um commit fixado desse SDK; arquivos de mundo decodificados offline não provam que regiões estão **residentes** no runtime.
- [NFSPluginSDK MW05](https://github.com/berkayylmao/NFSPluginSDK): atributos Gen::pvehicle e interfaces do motor; SDK parcial e dependente de build.

## Mudança efetiva no mod

Ao pressionar F9 no Free Roam, a v47 executa um inventário limitado a no máximo 128 nomes seguros de modelos de seu catálogo. As verificações de disco usam **somente** o diretório real que contém speed.exe (sem caminhos externos recebidos por INI):
- GLOBAL/GLOBALA.BUN e GLOBAL/GLOBALB.LZC;
- TRACKS/L2RA.BUN e TRACKS/STREAML2RA.BUN;
- CARS/<NOME_INTERNO_MAIÚSCULO>/GEOMETRY.BIN e TEXTURES.BIN, arquivo comum não vazio;
- para cada modelo, em **Free Roam seguro**, consulta a coleção de atributos pvehicle pelo NFSPluginSDK e registra sua chave.

Dados são colocados no próprio scripts/FreeRoamRivals/FreeRoamRivals.log, em linhas com prefixo `DiagnosticBundle assets:`, `DiagnosticBundle assetModel:`, `DiagnosticBundle attributeModel:` e `DiagnosticBundle activeModel:`. Após conferir que não há perseguição, o F9 cruza as chaves dos atributos registrados com as **caixas físicas de IVehicle ativos** do coletor espacial, mas só apresenta contagens se a lista foi lida completamente e sem falhas; separa `activeVehicleBoxes` do número `nonPlayer` e registra que uma nova construção ainda é `unverified`. Nenhum arquivo do jogo é alterado. Uma pasta ou textura ausente no endereço esperado pode indicar empacotamento diferente ou mods; não significa que o modelo seja impossível. A presença no disco ou no cadastro de atributos **não prova** que o motor terminou de carregar o modelo em RAM/GPU ou que o spawn é seguro.

## Por que não removemos a exigência de Golf GTI nesta etapa

Em v46, F8 cria um único carro nativo em um ponto de estrada, com a segurança de estado de jogo e perseguição. A exigência de Golf GTI é um limite intencional do protótipo para compartilhar modelo que já está em uso. Um arquivo GEOMETRY.BIN não garante que recursos, IA, textura e colisão desse modelo estejam residentes na região alvo. Não existe ainda uma API de 'load this rival car now' que tenhamos comprovado ser segura no binário MD5 fixado.

**Próxima prova necessária:** cruzar atributos e nomes internos com a residência real de pacotes/modelos, o setor de mundo ativado, a factory/caches, a preparação de estrada e o car-group, sem alterar temporariamente ponteiros de carros de terceiros. Somente depois habilitar o spawn de modelos independentes, inicialmente em modo experimental e com um único objeto.

## Como testar

1. Feche o jogo e atualize apenas ASI e INI principal com o ZIP upgrade; preserve seu backup de INI customizado e todos os saves/cache.
2. Abra o Free Roam (sem perseguição ou transições), aperte F9 uma vez. A auditoria não depende de usar Golf GTI nem de haver rival criado.
3. Consulte scripts/FreeRoamRivals/FreeRoamRivals.log e procure `DiagnosticBundle assets:`, `DiagnosticBundle assetModel:` e `DiagnosticBundle attributeModel:`. Esses números permitem identificar caminhos de arquivos encontrados e nomes internos validados.
4. F8/G/retirada e a calibração persistente permanecem como na v46. Não foi habilitada criação automática nem mudança de modelo.

## Validação automatizada

Novos testes verificam nomes de pastas restritos, ausência de leitura fora de CARS, detecção de pares completos, arquivos vazios, estrutura de TRACKS/GLOBAL, nenhuma modificação de arquivos, contagem de IVehicles ativos por modelo e falha fechada se a frota observada estiver incompleta. Windows CI roda todos os testes já existentes, compilação e verificação dos ZIPs. O jogo no computador do autor precisa fornecer a prova real de recursos de runtime.
