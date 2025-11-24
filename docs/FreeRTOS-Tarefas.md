# FreeRTOS – Tarefas do Sistema  
Projeto Final – Sistemas Embarcados – 2025.2  
Mesa Labirinto Controlada por Joystick (ESP32)

Este documento descreve todas as tarefas utilizadas no sistema embarcado, suas responsabilidades, prioridades e períodos de execução. As tarefas foram definidas conforme as exigências das Fases 1 e 2 do projeto.

---

# 📌 1. TaskJoystick – Leitura do Joystick
**Responsabilidade:**  
- Ler continuamente os valores analógicos dos eixos X e Y do joystick.  
- Aplicar filtragem (média móvel / suavização) se necessário.  
- Armazenar os dados em variáveis compartilhadas protegidas por mutex.

**Entrada:**  
- ADC do ESP32 (ou valores simulados no modo desenvolvimento sem hardware).

**Saída:**  
- Valores X e Y normalizados para a TaskServo.

**Período:** 50 ms  
**Prioridade:** 2 (média)

---

# 📌 2. TaskServo – Controle dos Servomotores
**Responsabilidade:**  
- Ler valores do joystick provenientes da TaskJoystick.  
- Mapear valores analógicos para pulsos PWM (1000–2000 µs).  
- Controlar dois servomotores (eixo X e eixo Y).  
- Garantir movimento suave e proporcional da mesa.

**Entrada:**  
- Valores X e Y filtrados.

**Saída:**  
- Sinais PWM (ou logs simulados no modo de teste sem hardware).

**Período:** 70 ms  
**Prioridade:** 2 (média)

---

# 📌 3. TaskDebug – Monitoramento e Logs (Serial)
**Responsabilidade:**  
- Exibir no Serial Monitor informações de depuração:  
  - Leituras do joystick  
  - PWM enviado aos servos  
  - Estado geral do sistema  
- Usada para validação e acompanhamento do comportamento do sistema.

**Entrada:**  
- Variáveis globais (joystick, PWM, flags do sistema).

**Saída:**  
- Logs via comunicação serial.

**Período:** 500 ms  
**Prioridade:** 1 (baixa)

---

# 📌 4. TaskMPU – Leitura do MPU6050 (Fase 2)
**Responsabilidade:**  
- Comunicar com o sensor MPU6050 via I²C.  
- Ler aceleração e giroscópio.  
- Calcular os ângulos **pitch** e **roll** da mesa.  
- Enviar dados para a TaskDebug ou TaskSerialSend.

**Entrada:**  
- Dados brutos do MPU6050.

**Saída:**  
- Ângulos calculados (pitch e roll).

**Período:** 50–100 ms  
**Prioridade:** 3 (alta)

---

# 📌 5. TaskSerialSend – Envio de Dados ao Computador (Opcional)
**Responsabilidade:**  
- Receber dados da TaskMPU.  
- Formatar os valores em **JSON** ou **CSV**.  
- Enviar via UART para o computador, onde serão processados pelo script que alimenta o InfluxDB/Grafana.

**Entrada:**  
- Pitch e Roll.

**Saída:**  
- Pacotes JSON/CSV via comunicação serial.

**Período:** 200 ms  
**Prioridade:** 1–2 (baixa a média)

---

# 📦 Resumo das Tarefas

| Task            | Função                              | Período     | Prioridade |
|-----------------|--------------------------------------|-------------|------------|
| TaskJoystick    | Leitura do joystick                 | 50 ms       | 2 |
| TaskServo       | Controle dos servomotores           | 70 ms       | 2 |
| TaskDebug       | Logs e monitoramento serial         | 500 ms      | 1 |
| TaskMPU (Fase 2)| Leitura MPU6050 + Pitch/Roll        | 50–100 ms   | 3 |
| TaskSerialSend  | Envio ao PC (JSON/CSV) (Opcional)   | 200 ms      | 1–2 |

---

# 📚 Observações
- Todas as variáveis compartilhadas entre tasks devem ser protegidas com **mutex** ou enviadas via **queues**.
- As prioridades foram escolhidas para garantir fluidez na leitura dos sensores e suavidade nos servos.
- A TaskMPU recebe prioridade maior por executar cálculos e leituras I²C.

---

**Documento pronto para uso no repositório.**

