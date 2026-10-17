# v0.0.61-dev — confiabilidade de distribuição e auditoria de binário

## Escopo

Esta atualização responde à auditoria do ZIP **v0.0.60-dev**. O foco é empacotamento e reprodutibilidade de depuração, não alterar a IA nativa de Rico, nem prometer que os problemas de gameplay foram resolvidos.

### 1. CRT VC++ x86

O projeto usa /MT (MSVC_RUNTIME_LIBRARY = MultiThreaded) para o binário ASI de Release; a dependência dos DLLs de runtime `MSVCP140.dll`, `MSVCP140_2.dll`, `VCRUNTIME140.dll` e `ucrtbase.dll` não deve existir **como import direto deste ASI**. O CI avalia o executável PE32 real via `tools/audit_asi_pe.py` e falha se encontrar qualquer DLL dinâmica do CRT conhecida.

Isto NÃO garante que o jogo, ASI loader ou outros plugins nunca precisem instalar VC++ Redistributable (x86). Tampouco elimina incompatibilidades de ABI/layout de dados nativos. `/MT` aumenta o binário, mas remove uma classe de falha silenciosa de distribuição.

### 2. Símbolos separados

A configuração **Release** do MSVC emite `FreeRoamRivals.pdb` (/Zi + /DEBUG:FULL) e `FreeRoamRivals.map` (/MAP) com otimizações /OPT:REF/ICF.

- O ASI-only permanece composto exclusivamente de `scripts/FreeRoamRivals.asi` e `INSTALAR-ASI-SOMENTE.txt`.
- Os símbolos estão disponíveis separadamente como um *GitHub Actions artifact*, e `FreeRoamRivals-v0.0.61-dev-symbols.zip` é um download separado na release. **Não instalar arquivos PDB/MAP na pasta do jogo**.
- Para simbolizar endereços do `NativeExceptions.log`, use os símbolos **exatos da mesma versão/hash do ASI** e leve em conta a base ASLR do `FreeRoamRivals.asi` mostrada no inventário de módulos. A presença de PDB não transforma exceção first-chance em crash fatal.
- Sempre preserve o ZIP de símbolos específico de uma build; recompilar código idêntico pode alterar a identidade PDB.

### 3. File Version no Windows

O arquivo `.asi` contém recurso Win32 VERSIONINFO derivado da mesma versão do CMake (0.0.61-dev); o CI lê `FileVersion` e `ProductVersion` do PE via as propriedades do arquivo no Windows para garantir consistência.

### 4. Instalação e integridade

O `INSTALAR-ASI-SOMENTE.txt` está em português e contém SHA-256 do ASI calculado com `Get-FileHash` a partir dos bytes realmente compilados. O CI verifica que esse hash foi incorporado, e garante o conjunto exato de dois arquivos. Usuários mantêm INI, histórico, métricas, saves e WidescreenFix/FixHUD.

### 5. Imports e proteção de hooks

- `src/core/ExceptionTrace.cpp` registra um `AddVectoredExceptionHandler`, mas antes fixa a DLL até o fim do processo com `GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN)`. Por design o handler sobrevive até o fim do processo, sem permitir descarregar seu código; `RemoveVectoredExceptionHandler` é usado somente em `stopExceptionTraceForTest`, depois de encerrar threads de teste. Handler somente observa exceptions selecionadas e retorna `EXCEPTION_CONTINUE_SEARCH`.
- Os imports `SuspendThread`, `SetThreadContext`, `ResumeThread` são rotinas da biblioteca `MinHook` fixada pelo SDK: `extern/minhook/src/hook.c` executa `Freeze`/`Unfreeze` ao instalar e desinstalar hooks. Isso requer cuidado durante instalação, mas não comprova deadlock real em nossos testes. Não se deve substituir patch/hook nativo por raw writes não validados.
- A questão da mudança definitiva do `simable_handle` no gameplay continua **sem solução comprovada**.

### 6. Dívida de histórico do Git

Mensagens repetidas de commits anteriores podem ser resultado de edições sequenciais e PRs antigas. A v61 não reescreve commits já publicados (mudança não reversível para clones) nem declara redução de tempo CI não medida. Mantemos PR de revisão, build, testes e squash commit para rastreabilidade. Futuro: agrupar edições por arquivo em commits locais e limitar pushes redundantes.

### 7. Limites da v61

O resultado prova compilação Win32, que o release contém o ASI, que imports e símbolos/versão passam nas inspeções e que os testes automatizados existentes continuam executando. **Não prova funcionamento em NFSMW Black Edition com cada combinação de WidescreenFix, ExtraOptions e XtendedInput.** Não há cura nova garantida de desaparecimento de Rico nem Sprint de checkpoints nativos. Backup e teste manual in-game são indispensáveis.
