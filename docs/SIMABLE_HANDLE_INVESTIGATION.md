# Investigação da identidade nativa de Rico — evidência, não correção presumida

**Escopo:** diagnóstico mínimo sobre o projeto existente. Não altera IA, spawn, limites,
pontuação, saves, HUD, dependências pinadas, offsets, endereços ou a guarda MD5 do jogo.

## Por que este experimento

O teste real v59/v60 (17/10/2026) registrou **ownerReadReason=simable_handle**
enquanto **ambos** os registros nativos ainda continham os endereços numéricos
do carro. A referência guardada **ISimable::_mHandle** deixou de coincidir.
Isso **não demonstra** se houve reciclagem de handle, uma transição legítima,
reconstrução de subsistema, invalidação parcial ou outra causa no motor.

A criação/ativação e a movimentação física de um Golf GTI já foram observadas.
Um segundo spawn no lugar do carro ainda registrado seria arriscado.

## Alteração do código

Na rotina **identity** de src/game/NativeVehicleFactory.cpp:

1. Mantém verificação de mesmo mundo, lista IVehicle + lista PVehicle,
   vtable esperada e **mesmo ponteiro ISimable**.
2. Lê **_mHandle** uma única vez após essas verificações e compara com o
   token previamente carimbado após Deactivate.
3. Somente se esses portões foram ultrapassados e os números divergem,
   copia **expectedSimableHandle** e **observedSimableHandle** para o snapshot
   de diagnóstico. Se qualquer porta anterior falha, os campos permanecem
   desmarcados; **não** tenta ler um objeto antigo.
4. O **primeiro** log de perda por episódio registra
   **handleMismatchObserved**, **expectedSimableHandle** e
   **observedSimableHandle**. Campos sem comparação mostram **not_read**.

Esses números **não são** novos handles de propriedade; não devem ser
substituídos no Owned, usados como autorização para Kill, Activate,
ForceStop, ResetVehicleToRoadNav ou para construir um segundo rival.
O protótipo mantém o fail-closed e a curta pausa de Outrun experimental
que já existia. Nenhuma função nova é chamada no jogo.

## Ferramenta local de análise

Dentro da cópia do **repositório**:

~~~powershell
py -3 tools\analyze_native_presence.py "D:\Need For Speed Most Wanted Black Edition\scripts\FreeRoamRivals\FreeRoamRivals.log" -o "FRR_native_presence.json"
~~~

O programa analisa eventos documentados de construção, ativação, perda,
retorno e retirada. Gera JSON novo (nunca sobrescreve) sem iniciar o jogo
ou modificar DLL/ASI/INI/saves. Limite de entrada: 32 MiB. O resultado
distingue:

- **observed_handle_mismatch**: números diferentes, com relato do mesmo
  mundo e das duas listas nativas ainda contendo o owner.
- **legacy_log_no_handle_fields**: log anterior à instrumentação.
- **incomplete_registry_evidence**: falta alguma prova de pertencimento.
- **inconsistent_handle_evidence**: campos ausentes/ilegíveis/iguais.
- **other_unavailability**: outro motivo de indisponibilidade.

Mesmo observed_handle_mismatch prova **somente a discrepância registrada**;
o campo **engine_cause_proven** continua false.

## Teste manual no PC do proprietário

1. Só usar o ASI gerado por CI se a compilação e testes passarem. Fazer backup
   do ASI instalado, INIs e saves. **Não** instalar dois ASIs de FRR juntos.
2. Conferir speed.exe = 6.029.312 bytes, MD5
   **C0516B485065FABDD69579816B5DF763**. Não remover ou afrouxar a guarda.
3. Na cópia de testes, preservar WidescreenFix/FixHUD, X360Stuff, XtendedInput,
   ExtraOptions etc., sem sobrescrever suas configurações.
4. Entrar no Free Roam, aguardar segurança de perseguição, usar **F8 uma vez**
   para criar o GTI e **G** para solicitar Outrun quando Rico estiver presente.
   Não insistir em F8 após perda de identidade ou em uma transição.
5. Se ocorrer perda, encerrar o jogo normalmente e preservar
   FreeRoamRivals.log, NativeExceptions.log, nome/hash do ASI e hora
   aproximada. Rode o analisador e compare a primeira ocorrência.
6. Reportar separadamente: Rico estava fisicamente visível? O carro voltou?
   Havia perseguição, fade ou transição? **Não** inferir despawn apenas do HUD.

## Próximas hipóteses a investigar usando a IA + engenharia reversa

- Verificar em Ghidra o fluxo de escrita de _mHandle no executável exato.
- Comparar quem modifica o campo com eventos de pool/registro, streaming e
  troca de objetivo de IA; não usar endereços provenientes de outras builds
  sem prova no MD5 suportado.
- Testar leituras e hipóteses offline antes de promover qualquer hook ou
  reparo de lifecycle. O contrato lifetimeProven=0 permanece.

## Fontes auxiliares (pistas, não certificação do build do usuário)

- [nfsmw-re — investigação com Ghidra](https://github.com/s-b-repo/nfsmw-re)
- [MWSDK — tabelas e layouts com proveniência](https://github.com/TsyVM/MWSDK)
- [NFSPluginSDK](https://github.com/berkayylmao/NFSPluginSDK)
- [Observações do nosso v60](PLAYABLE_OUTRUN_RECOVERY_V60.md)
- [Mapa de integração do projeto](ENGINE_INTEGRATION_MAP.md)
