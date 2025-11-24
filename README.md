# Mesa Labirinto Controlada por Joystick  
### ESP32 + FreeRTOS + MPU6050 + Grafana  
Projeto Final — Sistemas Embarcados — 2025.2

Este repositório contém o desenvolvimento completo do projeto final da disciplina **Sistemas Embarcados**, implementado com **ESP32**, joystick analógico, dois servomotores, sensor inercial **MPU6050**, FreeRTOS e visualização em tempo real no **Grafana** via InfluxDB.

---

# 📌 Objetivo Geral

Desenvolver um sistema embarcado capaz de:

- Controlar a inclinação de uma mesa com labirinto usando **dois servomotores**.
- Utilizar um **joystick analógico** como interface de controle.
- Medir a orientação da mesa (pitch e roll) com o **MPU6050**.
- Enviar dados ao computador via **Serial (JSON/CSV)**.
- Exibir no **Grafana** o gêmeo digital da mesa (digital twin).

---

# 🧩 Objetivo da Fase 1

- Ler o joystick (X/Y) via ADC  
- Converter valores em PWM para dois servos  
- Controlar a mesa em 2 eixos  
- Implementar **3 tarefas FreeRTOS obrigatórias**  
- Fazer o LED piscar na inicialização  
- Exibir logs via Serial (JSON simples)  
- Simulação totalmente funcional via **Wokwi**

---

# 🧵 Tarefas FreeRTOS (descrição completa)

As tarefas implementadas nesta fase são:

| Tarefa | Função | Prioridade | Período |
|--------|--------|------------|----------|
| **TaskJoystick** | Lê ADC X/Y, normaliza | 6 | 10 ms |
| **TaskServo** | Converte joystick → PWM e move os servos | 6 | 20 ms |
| **TaskStatus** | LED + logs via Serial | 4 | 200 ms |

Arquivo completo:  
📄 `docs/FreeRTOS-Tarefas.md`

---

# 🧠 Arquitetura do Sistema

```mermaid
flowchart LR

A[Joystick] -->|X,Y| B[ESP32]

B -->|PWM| C[Servo Motor X]
B -->|PWM| D[Servo Motor Y]

E[MPU6050] -->|I2C| B

B -->|UART JSON/CSV| F[Computador]
F -->|Inserção de Dados| G[InfluxDB]
G -->|Visualização| H[Grafana]
