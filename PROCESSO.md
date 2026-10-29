# PROCESSO.md — Lab 3 · Falhas e watchdog

**Aluno:** Gabriel Silva Domingos · gabriel.domingos@sou.inteli.edu.br
**Turma / grupo:** T26 · Grupo 2

## 1. Contextualizar
- **Objetivo:** Criar um sistema de medição de distância capaz de detectar falhas de hardware e se recuperar autonomamente.
- **Comportamento esperado:** Leituras fora do intervalo útil (2 a 400 cm) isoladas são tratadas como suspeitas. Três leituras suspeitas consecutivas alteram o estado para FALHA no LCD e log. Se o LCD perder a alimentação e voltar, o sistema o reinicializa sem travar a execução. Um travamento da função loop() por mais de 2 segundos provoca o reset automático da placa pelo watchdog.
- **Critério de aceitação:** Transição para o estado de falha do sensor em até 2 s; reconexão do LCD com restauração de texto em até 2 s sem reset do Arduino; acionamento do WDT para reiniciar o chip em ~2 s caso o loop() trave.
- **Delegação à IA × feito sozinho:** Para IA eu delego a parte de explicação de alguns conceitos teóricos, enquanto a parte técnica do hardware e dos testes eu cuido.

## 2. Consultar
- **Ferramenta:** Gemini
- **Pergunta (exata):** Por que pulseIn() pode deixar o loop() lento e o que muda ao adicionar o terceiro parâmetro de timeout?
- **Resposta resumida:** A função pulseIn() por padrão aguarda até 1 segundo (1.000.000 µs) pelo início e fim do pulso. Se o sensor falhar ou o fio soltar, ela bloqueia a execução do loop() por 1 segundo inteiro a cada chamada. Ao definir o terceiro parâmetro (timeout em microsegundos, como 30.000 µs), limita-se o tempo máximo de espera a 30 ms. Se o eco não vier nesse tempo, a função desiste e retorna 0 imediatamente, evitando travamentos na rotina.

## 3. Confrontar
- **Afirmação conferida:** O tempo padrão de espera (timeout) da função pulseIn() quando não especificado é de 1 segundo (1.000.000 µs), retornando 0 caso estoure o tempo.
- **Fonte primária:** Documentação oficial do Arduino
- **Trecho / cálculo / resultado:** De acordo com a documentação oficial da função pulseIn(), a sintaxe é pulseIn(pin, value, timeout) e o parâmetro timeout tem valor padrão de 1 segundo ("Gives up and returns 0 if no pulse started within the timeout. Works on pulses from 10 microseconds to 3 minutes in length.").
- **Veredito:** Confirmada
- **ADR:** Mantido o uso de pulseIn(PINO_ECHO, HIGH, TIMEOUT_ECO) com TIMEOUT_ECO = 30000 (30 ms). Essa decisão garante que o sensor espere tempo suficiente para medir distâncias de até ~500 cm sem estourar o tempo útil, ao mesmo tempo em que impede que uma falha física prenda o loop() por 1 segundo.

## 4. Construir
- **Parte A · volta do loop, sensor funcionando:** 27 ms
- **Parte A · volta do loop, TRIG solto:** 715 ms
- **Parte A · LCD religado:** O LCD volta com sua forma padrão, sem nenhum texto e apenas com os quadrados brancos.
- **Parte B · 1º W SENSOR → E SENSOR:** 500 ms
- **Parte C · boots antes/depois do teste t:** #15 -> #16
- **Parte C · wdt_reset() dentro do while:** Ao colocar wdt_reset() dentro do laço while(true), a placa para de responder ao Monitor Serial e trava a execução do programa, mas NÃO reinicia, pois o watchdog continua sendo alimentado continuadamente ("enganado").
- **Repositório:** https://github.com/zZGabsZz/Falhas-e-watchdog-no-Arduino
- **Parte B · falha do sensor detectada e recuperada:** sim
- **Parte B · LCD recuperado sem reset:** sim
- **Parte C · reset pelo watchdog:** sim

## 5. Comprovar (sem IA)
1. **Por que char buf[17]:** Porque o texto em C termina com o caractere nulo '\0', que também ocupa um byte. — Em linguagem C, strings são sequências de caracteres terminadas pelo caractere nulo '\0'. Para armazenar 16 caracteres visíveis no display, precisamos de 16 bytes mais 1 byte reservado exclusivamente para o '\0', totalizando 17 bytes.
2. **Maior distância com timeout de 30 000 µs:** 517.24 cm — cálculo: 30000 µs / 58 µs/cm ~ 517,24cm
3. **1ª suspeita → falha confirmada:** 500 ms — cálculo: t1 = 0; t2 = 250 ms; t3 = 500 ms (terceira leitura ruim confirma a falha)
4. **wdt_reset() dentro do while:** Porque o laço continua dizendo “estou vivo” enquanto o resto do sistema está parado: o watchdog é enganado. — Ao chamar wdt_reset() dentro do laço infinito, o temporizador de hardware do watchdog é reiniciado constantemente. Com isso, o circuito integrado entende que o sistema está operando normalmente e nunca atinge o tempo limite (timeout) necessário para resetar a placa, bloqueando a rotina principal sem sofrer reinicialização.
5. **Parte A + WDTO_500MS com o TRIG solto:** A placa entra em um ciclo de reinícios: cada espera de 1 s pelo eco passa do limite de 0,5 s do watchdog. — correção: Adicionar um timeout menor ao pulseIn() (por exemplo, pulseIn(PINO_ECHO, HIGH, 30000) para esperar no máximo 30 ms) ou aumentar o tempo do watchdog para um valor superior ao timeout da função (ex.: WDTO_2S), garantindo que o loop() termine e execute o wdt_reset() bem antes do limite do watchdog.

Declaro que respondi a etapa Comprovar sem IA: sim
