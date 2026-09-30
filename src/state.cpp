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

void enviar_mute_para_mesa(int numero_canal, bool ativo) {
  mixer_link_enviar_mute(numero_canal, ativo);
}

static EstadoMudouCb cb_mudou = NULL;

void estado_set_callback(EstadoMudouCb cb) {
  cb_mudou = cb;
}

void canal_set_ativo_remoto(int indice_canal, bool ativo) {
  if (estado_canal[indice_canal] == ativo) return;
  estado_canal[indice_canal] = ativo;
  if (cb_mudou) cb_mudou(indice_canal);
}
