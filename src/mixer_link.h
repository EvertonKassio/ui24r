#pragma once

enum StatusMesa {
  STATUS_MODO_TESTE = 0,  /* integracao desligada (WIFI_HABILITADO 0) */
  STATUS_CONECTANDO,      /* Wi-Fi e/ou WebSocket da mesa ainda subindo */
  STATUS_CONECTADO,       /* WebSocket com a mesa aberto: comandos e estado reais */
  STATUS_DESCONECTADO,
};

void mixer_link_init();
void mixer_link_loop();
StatusMesa mixer_link_status();

/* Envia o mute de um canal (numero 1-22 de data.h) para a mesa.
 * ativo=true -> som passa; false -> mutado. Sem conexao, nao faz nada
 * (o estado local continua mudando normalmente na tela). */
void mixer_link_enviar_mute(int numero_canal, bool ativo);
