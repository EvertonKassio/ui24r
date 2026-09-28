/* Arquivo gerado por tools/gen_icons.py - nao edite a mao. */
#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

enum IconId {
  IC_MIC,
  IC_WIRELESS,
  IC_GUITAR,
  IC_ACOUSTIC,
  IC_BASS,
  IC_KEYS,
  IC_MEDIA,
  IC_TALKBACK,
  IC_SNARE,
  IC_TOM,
  IC_FLOORTOM,
  IC_KICK,
  IC_HIHAT,
  IC_CYMBAL,
  IC_SPEAKER,
  IC_GRID_GRUPOS,
  IC_GRID_TODOS,
  IC_HOME,
  IC_WIFI,
  IC_COUNT
};

/* tamanhos disponiveis (indice 0..2) */
static const int ICON_TAM[3] = {96, 72, 40};

extern const lv_img_dsc_t * const ICONES[IC_COUNT][3];

#ifdef __cplusplus
}
#endif
