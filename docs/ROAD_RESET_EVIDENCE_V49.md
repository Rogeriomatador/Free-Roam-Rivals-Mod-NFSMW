# v0.0.49-dev — evidência real de falha ResetToRoad, sem prometer re-spawn impossível

## Análise do arquivo enviado no dia 11/10/2026

Foi estudada a sessão **v0.0.48-dev** iniciada 14:38:29 no arquivo `FreeRoamRivals(20261011-174203).log`: a calibração v46 foi recuperada e cruzada com nova movimentação (14:38:30 e 14:38:55). Em 14:40:08 o usuário pressionou F8 e o carro foi criado a **118,889 m**. A factory confirmou a identidade em dois quadros completos e preparou a IA Racer.

Às 14:40:09 a chamada nativa de redefinição na estrada `ResetToRoad` retornou êxito, mas a **verificação de posição após a chamada** reprovou o carro, com `roadResetDetail=post_reset_position_mismatch`. A factory entrou em falha (`disabled=true`) por segurança, sem executar Activate. Logo depois, o estado foi `Retiring`, mas a distância abaixo de 300m ou oclusão não comprovada bloquearam Kill; ao se afastar até o setor descarregar, o snapshot deixou de ser confiável. F8 entre 14:41:00 e 14:41:37 foi colocado em fila, **mas o estado de falha proíbe recriação durante esta partida**, mesmo que o antigo carro possa ser retirado. Nenhuma retirada conclusiva foi observada.

### Correção

1. Quando `ResetToRoad` retorna sucesso mas o GTI termina mais de 1 unidade horizontal do alvo, o log registra as coordenadas esperadas e observadas, delta X/Y/Z, distância horizontal e os dados da semente (segmento, nó, pista, tempo e deslocamento). **A rejeição permanece ativa**: nunca tratamos esse retorno nativo como prova suficiente para colocar um carro na rua.
2. F8 durante a retirada só entra na fila se a factory permitir uma nova construção segura. Após falha da factory, o mod informa `F8 INDISPONIVEL` / reinicie o jogo após sair dele, e não produz repetidos falsos `F8 queued`.
3. O HUD da retirada dá prioridade à falha de criação, evitando sugerir que afastar-se irá liberar F8 na mesma sessão. A retirada do objeto antigo continua condicionada à identidade e oclusão.

### O que continua pendente

- Identificar por que o ResetToRoad escolhe um ponto diferente para uma seed válida. A v49 **coleta a diferença geométrica real para investigação**, mas não corrige a função nativa nem permite criar outro rival depois de um reset falho. Isso evita ativar um carro em estrada errada, acidente ou posição inválida.
- Independentemente do problema de ResetToRoad, preservar rivais entre distritos exige provar e integrar streaming e ciclo de vida autônomo do motor; a implementação de v48 é apenas uma retirada antecipada opcional e não uma solução completa.

### Teste

Feche completamente o jogo, instale o ZIP upgrade v49 na pasta com `speed.exe`, faça backup apenas se seu INI principal foi personalizado; abra Free Roam e pressione F8 em lugar seguro. Se falhar, envie o log gerado, procurando `NativeFactory road-reset position mismatch`. Não force o F8 e não apague saves/cache: uma falha no motor exige reiniciar para outro teste; os perfis/arquivos do jogo não são afetados.
