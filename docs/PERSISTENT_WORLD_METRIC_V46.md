# v0.0.46-dev — calibração automática persistente

## Problema resolvido
Antes, o jogo guardava em memória a escala entre unidades internas do mapa e metros e a perdia ao sair. Ao iniciar novamente, mesmo no mesmo speed.exe, era preciso dirigir para produzir pelo menos 20 medições físicas consistentes.

## Funcionamento (sem editar INI)
1. O mod primeiro valida o **speed.exe exato** por tamanho + MD5, como nas versões anteriores. Com executável incompatível, nenhum hook ou cache nativo é aceito.
2. Após uma medição física válida pela lógica existente, guarda automaticamente o valor em `scripts/FreeRoamRivals/world-metric-v1.state`, junto com a prova de amostras e fingerprint do executável, versão de formato, bits exatos de ponto flutuante e checksum.
3. Na próxima abertura com o mesmo executável, lê esse arquivo antes de instalar os hooks. Se o arquivo for válido, a escala em metros já começa pronta, sem dirigir novamente. O HUD deve mostrar `CALIBRACAO PRONTA - USE GOLF GTI` no estado de espera.
4. Quando houver novos dados de movimento suficientes, o mod verifica passivamente a escala contra as medições atuais. Uma divergência superior a 5% bloqueia a escala em memória, descarta **apenas o arquivo desse cache** e não tenta corrigir dinamicamente a escala enquanto um carro nativo possa estar ativo. É necessário reiniciar e fazer uma calibração legítima para recuperar.
5. Arquivo ausente, incompatível, incompleto, corrompido ou com escala fora de limites não é usado; a calibração normal é aplicada e uma prova válida pode substituir o arquivo inválido. Falha de gravação no disco não bloqueia a calibração válida da sessão.

## O que NÃO muda
- Ainda é necessário dirigir **uma vez** se não houver um cache que tenha sido criado por esta versão. A v45 e versões anteriores não salvavam a calibração nesse arquivo. Não há valor de metros inventado nem configuração manual obrigatória.
- F8 continua sendo a forma manual de solicitar um único Golf GTI. O protótipo ainda exige que o jogador use Golf GTI para compartilhar recursos do modelo; **o cache não elimina esse requisito**.
- Posição na estrada, ausência de colisão, distância, perseguição, carregamento, câmera, identidade e ABI do carro continuam sendo verificadas antes da criação.
- Não há mudança nos saves vanilla, garagem, dinheiro, veículos da Blacklist ou rotas/velocidade da IA nativa. O arquivo de cache **não faz parte dos ZIPs**, portanto instalar a próxima atualização não deve substituí-lo.

## Plano de teste no PC
1. Fechar o jogo. Extrair `FreeRoamRivals-v0.0.46-dev-upgrade.zip` na pasta contendo `speed.exe`, mantendo backup opcional do INI principal customizado.
2. Na primeira execução, se o cache ainda não existir, escolher Golf GTI e dirigir até aparecer `World metric calibration verified` e `Verified world metric cached` no log. Isso será necessário uma única vez por executável/instalação com cache íntegro.
3. Fechar o jogo, iniciar novamente **sem mover o carro** e confirmar a linha `Verified world metric cache restored` no log, além do HUD `CALIBRACAO PRONTA`.
4. Em Free Roam seguro, com Golf GTI, F8 deve iniciar a busca sem a antiga exigência de dirigir para calcular metros. O spawn ainda depende dos outros bloqueios de segurança. G desafia, Ctrl+Shift+R solicita retirada segura.
5. Para simular corrupção, guardar cópia e alterar o cache; o mod deve ignorá-lo e exigir a calibração normal, sem travar nem substituir saves. Não edite manualmente o valor para tentar pular a validação.

## CI
Uma suíte independente testa bit-exact float, fingerprint errado, tamanho/NaN/prova insuficiente, integridade, ausência do arquivo, persistência atômica e descarte. O GitHub Actions continua compilando o ASI Win32, todos os testes existentes e os dois ZIPs. Comportamento real de spawn e IA ainda requer validação no jogo.
