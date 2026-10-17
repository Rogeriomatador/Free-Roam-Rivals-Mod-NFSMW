# Free Roam Rivals NEXT — grande avanço sem exigir teste diário

## A decisão técnica

Nosso projeto v0.0.61 comprovou um Golf GTI físico rodando pelo mundo, mas seu `simable_handle` ficou inválido durante um Outrun, com o objeto ainda registrado nas listas nativas. A abordagem atual usa um protótipo **de um rival**, F8 manual e vários probes experimentais. Isso não é fundamento suficiente para prometer uma população permanente de seis a quinze corredores e eventos completos.

Em vez de continuar 0.0.62, 0.0.63, 0.0.64 de diagnóstico, abrimos uma trilha **opcional, independente, e automática**, usando um motor com funcionalidade já desenvolvida: [MW Native Free Roam Racer v0.1.2](https://github.com/Zakkey250/MW-NativeFreeRoamRacer), escrito por **Zakkey250**, MIT. Ele já implementa:
- Gerenciador de até 15 rivais no tráfego real (padrão 6, sujeito à capacidade);
- IA de corrida `Custom AI` que segue a trajetória observada e tenta ultrapassar; `Stable` nativo alternativo;
- Desafio real de Outrun ao observar proximidade e equiparar velocidade, ultrapassagem e 300 m;
- HUD, marcadores, recompensas, garagem/configuração e integração opcional com Limit Breaker.

O projeto de referência NÃO oferece API pública para transferir a posse dos veículos para outro ASI, nem afirma possuir Sprint de largada parada com checkpoints nativos. Por isso não é seguro instalar os dois controladores de população ao mesmo tempo.

## Marco zero realizado neste ramo

O script `tools/prepare_nextgen.py` constrói automaticamente um candidato **standalone**, sem tocar na instalação do jogador:
1. Requer ZIP oficial v0.1.2 e `SHA256SUMS.txt` correspondentes; valida SHA256 **antes** de extrair.
2. Empacota apenas **três artefatos** NFRR (ASI/INI/TPK), não o nosso ASI, e não inclui o Limit Breaker por padrão.
3. Ativa preset conservador: quatro corredores, Custom AI, inglês, atualizações e telemetria desligadas, extras do Limit Breaker desligados, desempenho de protótipo sem viradas experimentais.
4. Confere licença MIT e escreve avisos em português, lista de SHA-256 de cada arquivo, e critérios de compatibilidade do EXE do próprio upstream.
5. Recusa entrada insegura, nomes repetidos, ausência de artefatos, payloads de tamanho anormal, SHA incorreto, INI com esquema inesperado, destino preexistente e `speed.exe` explicitamente incompatível.
6. A pipeline de CI cria um arquivo **experimental independente**, nunca uma v0.0.62 oficial. Os testes de embalagem e integridade rodam no GitHub sem nenhum teste manual no NFSMW.

## Importante: compatibilidade não presumida

- Nosso `speed.exe`: MD5 `C0516B485065FABDD69579816B5DF763`, tamanho `6.029.312` bytes.
- Executável pedido por NFRR: SHA256 `B248271BF8EAC8C9B283B8C95E3ADD672B713BF529B05F1780E58268493B9D06`.
- **A coincidência de tamanho, nome ou versão 1.3 NÃO prova que os dois hashes pertencem ao mesmo binário.** Até termos o SHA256 real do EXE que será usado, o candidato permanece opcional e não pode ser declarado compatível.
- O candidato **não é uma versão integrada do nosso ASI**, mas uma alternativa de motor de jogo, sujeita a crédito ao autor. Não colocar NFRR+FRR na mesma instalação ativa.

## Próximo salto técnico (sem loops de testes do usuário)

1. **Fundação funcional / offline:** fixar uma base de código MIT do NFRR e avaliar como adaptá-la ao design "Free Roam Rivals", mantendo o reconhecimento do autor e preservando os módulos de pesquisa do nosso projeto. Automatizar compilação do fork, builds e testes (CI), sem exigir usuário no jogo.
2. **Unificação nativa:** alterar apenas UM motor de população, nunca duas fábricas correndo em paralelo. Migrar perfis, lista de rivais, mecânica de Blacklist, cartas de personagem e configurações para a arquitetura escolhida. Manter módulo persistente independente do save original até provar capacidade de escrita nativa.
3. **Sprint de verdade:** projetar o gerenciador de rota/checkpoints com âncoras de navegação e IA que efetivamente percorra o caminho, não só um placar. Atrasar ativação por padrão até provar física da largada, controle conjunto e liberação segura.
4. **Aceite final antes de recomendar a todos:** jogo fecha/reabre, mundo livre com rivais múltiplos, ritmo aceitável, conclusão de duelo, sem criação duplicada, sem controle travado, sem perda de histórico, compatibilidade comprovada do EXE, teste de longa duração, seis interrupções/transições. Até lá: `candidate`, não `final`.

## Compromisso de não abusar de testes manuais

- CI é responsável por build, hashes, licença, integridade de ZIP, compatibilidade estática, histórias persistentes e casos simulados de duelo.
- Não solicitar novo log/print a cada revisão de texto ou ajuste de HUD.
- Só procurar validação do usuário depois de uma etapa completa com funcionalidade nova comprovada em build e uma lista curta de verificações in-game. Não é possível comprovar os efeitos do motor nativo somente com testes C++.
- Nunca alterar `speed.exe`, salvar dinheiro/garagem experimentalmente ou trocar INIs pessoais sem confirmação explícita e plano de rollback.

## Propriedade intelectual

Código, DLL e a ideia original de NFRR continuam de Zakkey250 sob MIT. Nossa configuração, empacotamento e camada de integração são próprios, sem falsificar autoria. `nextgen/LICENSE-NFRR-MIT.txt` acompanha o candidato. Bibliotecas vendorizadas e recursos devem manter suas licenças específicas. Se optarmos pela incorporação efetiva do código, precisamos importar as notas e licenças dos arquivos utilizados.
