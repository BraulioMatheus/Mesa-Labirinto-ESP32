// =========================
// main.c — Fase 1 comentado
// =========================

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/adc.h"                // Leitura ADC do joystick
#include "driver/gpio.h"               // LED de inicialização
#include "driver/mcpwm_prelude.h"      // Controle dos servos via PWM

#include "esp_log.h"                   // Logs no terminal

static const char *TAG = "MESA_FASE1"; // Tag para filtrar mensagens no Serial


// =========================
// Definições de hardware
// =========================

/* Canais ADC do joystick */
#define JOY_X_CHANNEL ADC1_CHANNEL_3   // Joystick VRX → GPIO4
#define JOY_Y_CHANNEL ADC1_CHANNEL_4   // Joystick VRY → GPIO5

/* Pinos dos servos */
#define SERVO_X_PIN 14                 // Servo eixo X
#define SERVO_Y_PIN 15                 // Servo eixo Y

/* LED para status inicial */
#define LED_INIT_PIN 2                 // LED verde indicando inicialização


// =========================
// Parâmetros de servo
// =========================

// Pulso mínimo e máximo do servo (SG90)
#define SERVO_MIN_PULSE_US 500
#define SERVO_MAX_PULSE_US 2400

// Frequência do servo: 50 Hz (período = 20 ms)
#define SERVO_FREQ_HZ      50


// =========================
// Parâmetro do ADC
// =========================

// Valor central do ADC de 12 bits (0–4095)
#define ADC_CENTER 2048.0f


// =========================
// Estrutura do joystick
// =========================

typedef struct {
    float x_norm;   // valor normalizado -1..1 para servo X
    float y_norm;   // valor normalizado -1..1 para servo Y
} joystick_t;

static joystick_t joystick = {0}; // inicia com valores 0


// =========================
// Variáveis do MCPWM
// =========================

static mcpwm_timer_handle_t timer = NULL;
static mcpwm_oper_handle_t oper  = NULL;

static mcpwm_cmpr_handle_t cmpr_x = NULL; // comparador servo X
static mcpwm_cmpr_handle_t cmpr_y = NULL; // comparador servo Y

static mcpwm_gen_handle_t gen_x = NULL;   // saída PWM do servo X
static mcpwm_gen_handle_t gen_y = NULL;   // saída PWM do servo Y


// =========================
// Funções auxiliares
// =========================

/*
 * Normaliza leitura ADC (0..4095) para faixa -1.0 .. +1.0
 */
static float normalize_adc(int raw) {
    float v = ((float)raw - ADC_CENTER) / ADC_CENTER;
    if (v > 1.0f) v = 1.0f;
    if (v < -1.0f) v = -1.0f;
    return v;
}

/*
 * Converte normalizado (-1..1) para pulso de servo (500–2400 µs)
 */
static int norm_to_pulse_us(float n) {
    float t = (n + 1.0f) / 2.0f;  // converte para 0..1
    return (int)(SERVO_MIN_PULSE_US +
                 t * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US));
}


// =========================
// Inicialização do MCPWM
// =========================
// API correta para ESP-IDF 5.x
// =========================

static void init_mcpwm_servos(void) {

    // Timer com resolução de 1 MHz → precisão de 1 µs
    mcpwm_timer_config_t timer_cfg = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000,
        .period_ticks = 1000000 / SERVO_FREQ_HZ,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_cfg, &timer));

    // Operator do MCPWM
    mcpwm_operator_config_t oper_cfg = { .group_id = 0 };
    ESP_ERROR_CHECK(mcpwm_new_operator(&oper_cfg, &oper));

    // Liga o operador ao timer
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    // Criar comparadores dos servos
    mcpwm_comparator_config_t cmp_cfg = {
        .flags.update_cmp_on_tez = true  // atualiza no início do ciclo
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &cmp_cfg, &cmpr_x));
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &cmp_cfg, &cmpr_y));

    // Configura gerador PWM do servo X
    mcpwm_generator_config_t gen_cfg_x = {
        .gen_gpio_num = SERVO_X_PIN
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &gen_cfg_x, &gen_x));

    // Configura gerador PWM do servo Y
    mcpwm_generator_config_t gen_cfg_y = {
        .gen_gpio_num = SERVO_Y_PIN
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &gen_cfg_y, &gen_y));


    // Ação: Sobe o pulso quando o ciclo inicia
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event(
        gen_x,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                     MCPWM_TIMER_EVENT_EMPTY,
                                     MCPWM_GEN_ACTION_HIGH),
        MCPWM_GEN_TIMER_EVENT_ACTION_END()
    ));
    // Desce quando atinge o comparador
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(
        gen_x,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                       cmpr_x,
                                       MCPWM_GEN_ACTION_LOW),
        MCPWM_GEN_COMPARE_EVENT_ACTION_END()
    ));

    // Mesma configuração para o servo Y
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event(
        gen_y,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                     MCPWM_TIMER_EVENT_EMPTY,
                                     MCPWM_GEN_ACTION_HIGH),
        MCPWM_GEN_TIMER_EVENT_ACTION_END()
    ));
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(
        gen_y,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                       cmpr_y,
                                       MCPWM_GEN_ACTION_LOW),
        MCPWM_GEN_COMPARE_EVENT_ACTION_END()
    ));

    // Habilita e inicia o timer
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));
}


// =========================
//        TAREFAS
// =========================

/*
 * TASK 1 — Leitura do joystick
 * Período: 10 ms
 */
static void task_joystick(void *pv) {

    // Configura ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(JOY_X_CHANNEL, ADC_ATTEN_DB_12);
    adc1_config_channel_atten(JOY_Y_CHANNEL, ADC_ATTEN_DB_12);

    while (1) {

        // Leitura bruta dos ADCs
        int raw_x = adc1_get_raw(JOY_X_CHANNEL);
        int raw_y = adc1_get_raw(JOY_Y_CHANNEL);

        // Normalização -1..1
        joystick.x_norm = normalize_adc(raw_x);
        joystick.y_norm = normalize_adc(raw_y);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


/*
 * TASK 2 — Controle dos servos
 * Período: 20 ms (compatível com servo)
 */
static void task_servo(void *pv) {

    while (1) {

        // Converte posição do joystick em pulso PWM
        int pulse_x = norm_to_pulse_us(joystick.x_norm);
        int pulse_y = norm_to_pulse_us(joystick.y_norm);

        // Atualiza registradores PWM (MCPWM)
        mcpwm_comparator_set_compare_value(cmpr_x, pulse_x);
        mcpwm_comparator_set_compare_value(cmpr_y, pulse_y);

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}


/*
 * TASK 3 — Status e debug
 * Pisca LED na inicialização e mostra valores no Serial
 */
static void task_status(void *pv) {

    gpio_reset_pin(LED_INIT_PIN);
    gpio_set_direction(LED_INIT_PIN, GPIO_MODE_OUTPUT);

    // Piscar LED 4 vezes ao ligar
    for (int i = 0; i < 4; i++) {
        gpio_set_level(LED_INIT_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(150));
        gpio_set_level(LED_INIT_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(150));
    }

    // Deixa LED ligado fixo
    gpio_set_level(LED_INIT_PIN, 1);

    // Log contínuo
    while (1) {
        ESP_LOGI(TAG, "{\"joy_x\": %.3f, \"joy_y\": %.3f}",
                 joystick.x_norm, joystick.y_norm);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


// =========================
//   Função principal
// =========================

void app_main(void) {

    ESP_LOGI(TAG, "Inicializando MCPWM para servos...");
    init_mcpwm_servos();  // inicializa PWM

    // Criação das tasks FreeRTOS
    xTaskCreate(task_joystick, "task_joystick", 2048, NULL, 6, NULL);
    xTaskCreate(task_servo,    "task_servo",    2048, NULL, 6, NULL);
    xTaskCreate(task_status,   "task_status",   2048, NULL, 4, NULL);

    ESP_LOGI(TAG, "Sistema iniciado.");
}

