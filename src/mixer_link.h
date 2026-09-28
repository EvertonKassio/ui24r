#pragma once
#include <stdint.h>

enum StatusMesa {
  STATUS_MODO_TESTE = 0,  /* Wi-Fi da mesa desligado de proposito (ver README) */
  STATUS_CONECTANDO,
  STATUS_CONECTADO,       /* Wi-Fi da mesa OK - protocolo da mesa ainda nao */
  STATUS_DESCONECTADO,
};

void mixer_link_init();
void mixer_link_loop();
StatusMesa mixer_link_status();

/* Controla um dos grupos de mute da Ui24R (1 bit por grupo). */
void mixer_link_set_group_mute(int grupo_id, bool mutado);

/* Controla o forceunmute de um canal individual (1-22). */
void mixer_link_set_channel_unmute(uint8_t numero_canal, bool ativo);
