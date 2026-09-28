/*
 * data.h - Tabela de canais e grupos da Soundcraft Ui24R do usuario.
 *
 * Gerado a partir de Canais.xlsx. Se a numeracao de canais da mesa
 * mudar, edite so este arquivo (os grupos e a tela "Todos os Canais"
 * sao montados automaticamente a partir dele).
 *
 * Os nomes abaixo sao de proposito sem acentuacao (ex.: "Violao", nao
 * "Violão") - a interface usa as fontes prontas do LVGL, que so tem os
 * caracteres ASCII normais, entao um acento apareceria como um
 * quadrado vazio na tela.
 */
#pragma once
#include <stdint.h>
#include <lvgl.h>
#include "icons.h"

/* --------------------------------------------------------------------
 * Grupos (os 4 botoes grandes da tela principal).
 * A ordem aqui e a ordem de exibicao (grade 2x2: esquerda->direita,
 * cima->baixo).
 * ------------------------------------------------------------------ */
enum GroupId {
  GRP_VOCAL = 0,
  GRP_INSTRUMENTOS,
  GRP_BATERIA,
  GRP_SEMFIO,
  GRP_COUNT
};

struct GroupInfo {
  const char *nome;            /* nome exibido (tela principal e cabecalho) */
  lv_color_t cor;               /* cor de identificacao do grupo */
  const lv_img_dsc_t *icone;    /* icone do bloco (tamanho 96px) */
};

static const GroupInfo GRUPOS[GRP_COUNT] = {
  { "Vocal",        lv_color_hex(0xFF453A), ICONES[IC_MIC][0] },
  { "Instrumentos", lv_color_hex(0x30D158), ICONES[IC_GUITAR][0] },
  { "Bateria",      lv_color_hex(0xFF9F0A), ICONES[IC_KICK][0] },
  { "Sem Fio",      lv_color_hex(0x5E5CE6), ICONES[IC_WIRELESS][0] },
};

/* --------------------------------------------------------------------
 * Canais. "grupo" usa GroupId, ou -1 quando o canal nao pertence a
 * nenhum dos 4 grupos (Midia, Talkback, VS L, VS R) - esses so
 * aparecem na tela "Todos os Canais".
 * ------------------------------------------------------------------ */
struct ChannelInfo {
  uint8_t canal;                /* numero do canal na mesa (1-22) */
  int8_t grupo;                 /* GroupId ou -1 */
  const lv_img_dsc_t *icone40;  /* icone pequeno (grade compacta "Todos os Canais") */
  const lv_img_dsc_t *icone72;  /* icone medio (lista de canais do grupo) */
  const char *nome;             /* nome do canal */
};

static const ChannelInfo CANAIS[] = {
  {  1, GRP_VOCAL,        ICONES[IC_MIC][2],      ICONES[IC_MIC][1],      "Microfone Amarelo" },
  {  2, GRP_VOCAL,        ICONES[IC_MIC][2],      ICONES[IC_MIC][1],      "Microfone Verde" },
  {  3, GRP_VOCAL,        ICONES[IC_MIC][2],      ICONES[IC_MIC][1],      "Microfone Azul" },
  {  4, GRP_VOCAL,        ICONES[IC_MIC][2],      ICONES[IC_MIC][1],      "Microfone Preto" },
  {  5, GRP_SEMFIO,       ICONES[IC_WIRELESS][2], ICONES[IC_WIRELESS][1], "Microfone Sem Fio Branco" },
  {  6, GRP_SEMFIO,       ICONES[IC_WIRELESS][2], ICONES[IC_WIRELESS][1], "Microfone Sem Fio Azul" },
  {  7, GRP_INSTRUMENTOS, ICONES[IC_GUITAR][2],   ICONES[IC_GUITAR][1],   "Guitarra 1" },
  {  8, GRP_INSTRUMENTOS, ICONES[IC_GUITAR][2],   ICONES[IC_GUITAR][1],   "Guitarra 2" },
  {  9, GRP_INSTRUMENTOS, ICONES[IC_ACOUSTIC][2], ICONES[IC_ACOUSTIC][1], "Violao" },
  { 10, GRP_INSTRUMENTOS, ICONES[IC_KEYS][2],     ICONES[IC_KEYS][1],     "Teclado" },
  { 11, GRP_INSTRUMENTOS, ICONES[IC_BASS][2],     ICONES[IC_BASS][1],     "Baixo" },
  { 12, -1,                ICONES[IC_MEDIA][2],    ICONES[IC_MEDIA][1],    "Midia" },
  { 13, -1,                ICONES[IC_TALKBACK][2], ICONES[IC_TALKBACK][1], "Talkback" },
  { 14, GRP_BATERIA,      ICONES[IC_SNARE][2],    ICONES[IC_SNARE][1],    "Caixa" },
  { 15, GRP_BATERIA,      ICONES[IC_TOM][2],      ICONES[IC_TOM][1],      "Tom 1" },
  { 16, GRP_BATERIA,      ICONES[IC_TOM][2],      ICONES[IC_TOM][1],      "Tom 2" },
  { 17, GRP_BATERIA,      ICONES[IC_FLOORTOM][2], ICONES[IC_FLOORTOM][1], "Surdo" },
  { 18, GRP_BATERIA,      ICONES[IC_KICK][2],     ICONES[IC_KICK][1],     "Bumbo" },
  { 19, GRP_BATERIA,      ICONES[IC_HIHAT][2],    ICONES[IC_HIHAT][1],    "Chimbal" },
  { 20, GRP_BATERIA,      ICONES[IC_CYMBAL][2],   ICONES[IC_CYMBAL][1],   "Over" },
  { 21, -1,                ICONES[IC_SPEAKER][2],  ICONES[IC_SPEAKER][1],  "VS L" },
  { 22, -1,                ICONES[IC_SPEAKER][2],  ICONES[IC_SPEAKER][1],  "VS R" },
};
static const int NUM_CANAIS = sizeof(CANAIS) / sizeof(CANAIS[0]);

/* conta quantos canais existem num grupo - usado na tela principal
 * (subtitulo "N canais" de cada bloco) */
static inline int grupo_contar_canais(int grupo_id) {
  int n = 0;
  for (int i = 0; i < NUM_CANAIS; i++) {
    if (CANAIS[i].grupo == grupo_id) n++;
  }
  return n;
}
