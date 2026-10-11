# v0.0.54-dev — atualização SOMENTE do Free Roam Rivals

## Motivo

O usuário confirmou em 11/10/2026 que o HUD original do Most Wanted voltou ao lugar correto após mudar `FixHUD=0` para `FixHUD=1` **no WidescreenFix que JÁ estava instalado**. Esse ajuste resolveu um problema de HUD nativo externo ao Free Roam Rivals e deve ficar na instalação local. O FRR **NÃO** fornece o WidescreenFix e **NÃO** modifica seu arquivo de configuração.

O pacote do mod pode ser instalado/atualizado independentemente de qualquer mod de aparência, incluindo X360Stuff, XtendedInput e WidescreenFix. Essa separação **não é garantia** de compatibilidade de todas as versões desses mods entre si: não forçamos o carregamento de terceiros, nem invalidamos proteções de speed.exe.

## Qual arquivo escolher?

- **`FreeRoamRivals-v0.0.54-dev-asi-only.zip` — recomendado para uma instalação já funcionando:** contém **somente `scripts/FreeRoamRivals.asi`** e um pequeno texto de instruções. Atualiza o código nativo sem sobrescrever nenhum INI, saves, cache de calibração, texturas, WidescreenFix ou X360Stuff. Se o seu `FreeRoamRivals.ini` já habilita o protótipo, continuará assim. Se não habilita, a atualização da ASI não altera essa opção.
- `FreeRoamRivals-v0.0.54-dev-upgrade.zip` — atualiza a ASI **e o INI principal** do nosso mod para o preset PLAY. Inclui documentos. Você deve salvar seu INI principal personalizado antes, pois esse ZIP o substituirá.
- `FreeRoamRivals-v0.0.54-dev.zip` — pacote **completo** do próprio FRR para instalação nova; inclui INIs padrão de rivais e ferramentas de diagnóstico, mas nenhum plugin, dll, arquivo de textura ou executável do Most Wanted.

## Instalação segura da atualização ASI-only

1. Feche o jogo e faça backup de `scripts/FreeRoamRivals.asi`.
2. Extraia o ZIP na pasta raiz do NFSMW que contém `speed.exe`, permitindo substituição de **apenas** `scripts/FreeRoamRivals.asi`.
3. Não altere `scripts/NFSMostWanted.WidescreenFix.ini`; mantenha seu `FixHUD=1` já testado. Também não substitua o arquivo `NFSMostWanted.WidescreenFix.asi`, `X360Stuff.asi.OFF`, `dinput8.dll`, `speed.exe` ou outras configurações.
4. Inicie o jogo e confira a versão `v0.0.54-dev` no começo do log `scripts/FreeRoamRivals/FreeRoamRivals.log` (o nome local pode incluir data). Use o Golf GTI com F8; G continua o desafio nativo experimental; o radar do FRR continua configurável por `RadarPosition=0..3` e `RadarScale=0.5..1.5` no seu INI atual.
5. Caso ocorra problema novo, feche o jogo, restaure a ASI anterior e envie os logs. Não tente resolver desabilitando `RequireSupportedExecutable`.

## Restrições verificadas automaticamente na publicação

Os três ZIPs são inspecionados para **rejeitar** qualquer distribuição de `WidescreenFix`, `X360Stuff`, `XtendedInput`, `dinput8.dll`, `speed.exe`, arquivos de configuração de outros plugins ou artefatos suspeitos de shader/textura. O pacote ASI-only deve conter apenas `scripts/FreeRoamRivals.asi` e `INSTALAR-ASI-SOMENTE.txt`; seu conteúdo é um subconjunto do nosso mod.

## O que esta atualização não resolve

O ajuste visual `FixHUD=1` aconteceu fora do código do FRR; a v54 formaliza um **pacote de atualização isolada** e o protege por auditoria automatizada. Não corrige o stream-out dos rivais, não ativa a buzina nativa, não monta uma largada 3-2-1 com bloqueio de direção e não altera o comportamento original da IA. Esses desenvolvimentos exigem integração nativa e validação dentro do jogo.
