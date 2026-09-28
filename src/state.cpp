#include "state.h"
#include "mixer_link.h"

bool estado_canal[64]; /* folga acima de NUM_CANAIS de proposito */

void estado_init() {
  for (int i = 0; i < NUM_CANAIS; i++) {
    estado_canal[i] = true; /* comeca tudo ativado/desmutado */
  }
}

bool canal_get_ativo(int indice_canal) {
  return estado_canal[indice_canal];
}

void canal_set_ativo(int indice_canal, bool ativo) {
  estado_canal[indice_canal] = ativo;
  enviar_mute_para_mesa(CANAIS[indice_canal].canal, ativo);
}

void canal_alternar(int indice_canal) {
  canal_set_ativo(indice_canal, !estado_canal[indice_canal]);
}

bool grupo_get_ativo(int grupo_id) {
  for (int i = 0; i < NUM_CANAIS; i++) {
    if (CANAIS[i].grupo == grupo_id && estado_canal[i]) return true;
  }
  return false;
}

void grupo_alternar(int grupo_id) {
  bool novo_estado = !grupo_get_ativo(grupo_id);
  for (int i = 0; i < NUM_CANAIS; i++) {
    if (CANAIS[i].grupo == grupo_id) {
      canal_set_ativo(i, novo_estado);
    }
  }
}

void estado_aplicar_mgmask(uint32_t mgmask) {
  for (int grupo = 0; grupo < GRP_COUNT; grupo++) {
    bool ativo = (mgmask & (1UL << grupo)) == 0;
    for (int i = 0; i < NUM_CANAIS; i++) {
      if (CANAIS[i].grupo == grupo) estado_canal[i] = ativo;
    }
  }
}

void enviar_mute_para_mesa(int numero_canal, bool ativo) {
  if (numero_canal < 1 || numero_canal > 22) return;
  mixer_link_set_channel_unmute((uint8_t)numero_canal, ativo);
}
