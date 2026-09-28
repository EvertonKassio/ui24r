/*
 * Painel de controle da Soundcraft Ui24R no display ESP32-4848S040C(E).
 * Veja README.md para instrucoes de montagem, pinagem e o que falta
 * fazer para ligar isto de verdade na mesa.
 */
#include <Arduino.h>
#include <lvgl.h>

#include "display.h"
#include "state.h"
#include "ui.h"
#include "mixer_link.h"

void setup() {
  Serial.begin(115200);

  display_init();
  estado_init();
  ui_init();
  mixer_link_init();
}

void loop() {
  static uint32_t ultimo_ms = 0;
  uint32_t agora = millis();
  lv_tick_inc(agora - ultimo_ms);
  ultimo_ms = agora;

  lv_timer_handler();
  mixer_link_loop();
  ui_set_status(mixer_link_status());
  delay(2);
}
