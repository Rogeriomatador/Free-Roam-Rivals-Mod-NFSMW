# Captura v42: G reconhecido, interrupção imediata e conflito F7

Arquivos originais fornecidos pelo usuário em 10/10/2026:
- `FreeRoamRivals(20261010-185142).log`, SHA-256 `c61a9191049730e743e61493e6acf8ff2081bf2dab515a36c9414e8ee4164a40`
- `NativeExceptions(3).log`, SHA-256 `acb2f7c93fe5e5c240c17c911f6378fb1fcfb21cf34bafa45a9e827f2ed4c331`
- `live-rico-5c67ad4b679a57d8.state`, SHA-256 `692e74434279a8447c2833485f6467b67161edc0e0e736ef8ff249001e70851b`

O log registra 19 pressões de G. Às 15:42:48.940, uma inicia `LiveEncounter`; às 15:42:49.199, aparece `result=Aborted modHistorySaved=1`. O estado salvo confirma battles=1, aborts=1, wins/losses/draws=0. Portanto o G funcionou como entrada do mod, mas a disputa foi interrompida em aproximadamente 0,259 segundo. O log v42 não registra qual proteção causou a interrupção.

Há retomadas de observação ativa após transições (incluindo 15:42:16 → 15:42:19 e 15:44:56 → 15:44:58). A partir de 15:45:26 aparecem novos bloqueios de contexto, sem retomada ativa na captura. Não há pedido de cleanup nem retirada externa confirmada. A ausência visual não prova destruição, unloading ou ponteiro válido. O observador de exceções não contém FIRST_CHANCE selecionadas; isso não prova ausência de todo erro.

## Erro estático encontrado e corrigido na v43

O detour declarava `void (__cdecl*)(float)` e consumia esse argumento como segundos. O executável exato suportado faz:

- 0x663D3A: `FILD DWORD PTR [esp+4]`: leitura **inteira**, não FLD float.
- 0x663D3F: multiplicação por float em 0x890980, bytes `00 00 80 37`: 1/65536.
- 0x663D45: multiplicação por float em 0x890D60, bytes `6F 12 83 3A`: 0.001f.
- 0x663D4B–0x663D4E: passagem do valor convertido para 0x661280.

A v43 mantém o argumento int32 original inalterado para a função/trampolim do jogo e entrega `ticks / 65536 * 0.001f` somente aos callbacks FRR. Não escreve relógios do jogo ou modifica física. Mesmo tamanho/palavra cdecl é preservado, inclusive em um wrapper que apenas encaminha os bits.

Exemplo de regressão: 16384000 ticks = 250 ms = 0,25 s. Reinterpretar a mesma palavra como float gera um valor minúsculo; um deslocamento de 13m excede a tolerância de continuidade calculada com esse tempo errado. A conversão nativa aceita o mesmo movimento a 52m/s. Isso prova o defeito de tempo e sua correção matemática. Sem o argumento bruto da amostra v42, não é possível afirmar que foi a única causa daquele abort.

## Controles e feedback v43

F7 não é mais consultado por padrão. `Input.RetireRivalKey=0` desativa retirada manual, tanto em INIs antigos sem a chave como no preset entregue. O jogador relatou conflito de F7 com outro recurso do jogo; não foi identificado qual mod/recurso. Retirada opcional pode ser explicitamente configurada com outra virtual key, modificadores Ctrl+Shift e tempo mínimo; nenhum atalho alternativo é declarado universalmente livre de conflito.

G continua sendo entrada alternativa de desafio do mod, não buzina. Ainda não existe fonte de buzina verificada nesta integração. Ao recusar G, o mod mostra orientação de aproximação/velocidade/direção/intervalo e registra a razão. Interrupções registram motivo e amostra terminal quando disponível. Não flexibiliza identidade, perseguição, ground, distância ou código nativo para tentar esconder falhas.

A pontuação congela se ambos os carros mantêm a posição com velocidade não nula reportada, até movimento consistente voltar; não é uma leitura de flag de pausa verificada. Transições inseguras continuam interrompendo a disputa sem desreferenciar ponteiros antigos. Não há novo respawn, captura de carro alheio ou remendo especulativo de streaming.
