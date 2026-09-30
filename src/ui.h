#pragma once
#include "mixer_link.h"

/* Monta todas as telas e mostra a tela principal. Chame uma vez, depois
 * de display_init() e estado_init(). */
void ui_init();

/* Atualiza o texto/indicador de status de conexao mostrado na tela
 * principal. Chame periodicamente do loop() com o status atual. */
void ui_set_status(StatusMesa status);
