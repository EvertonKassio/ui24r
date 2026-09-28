/*
 * ui.cpp - Telas do painel de controle da mesa.
 *
 * Estrutura:
 *  - tela principal: status + 4 blocos de grupo (2x2)
 *      clique curto  -> muta/desmuta o grupo inteiro
 *      toque longo   -> abre a tela daquele grupo
 *      botao (canto sup. direito) -> abre a tela "Todos os Canais"
 *  - tela de grupo / "Todos os Canais": grade dos botoes de canal
 *      clique curto  -> muta/desmuta aquele canal
 *      botao (canto sup. direito) -> volta para a tela principal
 *
 * Todo texto usa lv_label com as fontes prontas do LVGL (Montserrat),
 * que so cobrem ASCII - por isso os nomes em data.h sao sem acento.
 */
#include "ui.h"
#include "data.h"
#include "state.h"
#include "board_config.h"

#include <stdio.h>
#include <stdlib.h>

/* cor usada para canais que nao pertencem a nenhum dos 4 grupos
 * (Midia, Talkback, VS L, VS R) nas telas onde aparecem */
static const lv_color_t COR_NEUTRA = LV_COLOR_MAKE(0x8E, 0x8E, 0x93);
static const lv_color_t COR_FUNDO_TELA = LV_COLOR_MAKE(0x0A, 0x0A, 0x0C);
static const lv_color_t COR_FUNDO_BOTAO = LV_COLOR_MAKE(0x18, 0x18, 0x1C);
static const lv_color_t COR_TEXTO_CLARO = LV_COLOR_MAKE(0xEF, 0xEF, 0xF2);
static const lv_color_t COR_TEXTO_ESCURO = LV_COLOR_MAKE(0x10, 0x10, 0x12);
static const lv_color_t COR_VERMELHO = LV_COLOR_MAKE(0xFF, 0x00, 0x00);

/* ------------------------------------------------------------- refs */
struct RefGrupo {
  lv_obj_t *tile, *icon, *title, *sub;
  int grupo;
  bool long_press_ocorreu;
};
static RefGrupo refs_grupo[GRP_COUNT];

struct RefCanal {
  lv_obj_t *btn, *icon, *label, *tag_mute;
  int idx_canal;
};
static RefCanal refs_canal[NUM_CANAIS * 2];
static int num_refs_canal = 0;

static lv_obj_t *scr_principal;
static lv_obj_t *scr_grupo[GRP_COUNT];
static lv_obj_t *scr_todos;
static lv_obj_t *lbl_status;
static StatusMesa status_mostrado = (StatusMesa)-1;

static lv_color_t cor_do_canal(int idx_canal) {
  int g = CANAIS[idx_canal].grupo;
  return (g < 0) ? COR_NEUTRA : GRUPOS[g].cor;
}

/* ------------------------------------------------------- visual sync */
static void aplicar_visual(lv_obj_t *fundo, lv_obj_t *icon, lv_obj_t *titulo_ou_null,
                            lv_color_t cor, bool ativo) {
  if (ativo) {
    lv_obj_set_style_bg_color(fundo, cor, 0);
    lv_obj_set_style_bg_opa(fundo, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(fundo, cor, 0);
    lv_obj_set_style_border_opa(fundo, LV_OPA_COVER, 0);
    lv_obj_set_style_img_recolor(icon, COR_TEXTO_ESCURO, 0);
    lv_obj_set_style_img_recolor_opa(icon, LV_OPA_COVER, 0);
    if (titulo_ou_null) {
      lv_obj_set_style_text_color(titulo_ou_null, COR_TEXTO_ESCURO, 0);
    }
  } else {
    lv_obj_set_style_bg_color(fundo, COR_FUNDO_BOTAO, 0);
    lv_obj_set_style_bg_opa(fundo, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(fundo, cor, 0);
    lv_obj_set_style_border_opa(fundo, LV_OPA_COVER, 0);
    lv_obj_set_style_img_recolor(icon, COR_TEXTO_CLARO, 0);
    lv_obj_set_style_img_recolor_opa(icon, LV_OPA_COVER, 0);
    if (titulo_ou_null) {
      lv_obj_set_style_text_color(titulo_ou_null, COR_TEXTO_CLARO, 0);
    }
  }
}

static void atualizar_visual_grupo(int g) {
  RefGrupo &r = refs_grupo[g];
  bool ativo = grupo_get_ativo(g);
  aplicar_visual(r.tile, r.icon, r.title, GRUPOS[g].cor, ativo);
}

static void atualizar_visual_canal(int idx) {
  bool ativo = canal_get_ativo(idx);
  lv_color_t cor = cor_do_canal(idx);
  for (int i = 0; i < num_refs_canal; i++) {
    if (refs_canal[i].idx_canal == idx) {
      aplicar_visual(refs_canal[i].btn, refs_canal[i].icon, refs_canal[i].label, cor, ativo);
      if (refs_canal[i].tag_mute) {
        if (ativo) lv_obj_add_flag(refs_canal[i].tag_mute, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_clear_flag(refs_canal[i].tag_mute, LV_OBJ_FLAG_HIDDEN);
      }
    }
  }
}

void ui_refresh_state() {
  for (int g = 0; g < GRP_COUNT; g++) atualizar_visual_grupo(g);
  for (int i = 0; i < NUM_CANAIS; i++) atualizar_visual_canal(i);
}

/* ------------------------------------------------------------ eventos */
static void cb_grupo_pressed(lv_event_t *e) {
  int g = (int)(intptr_t)lv_event_get_user_data(e);
  refs_grupo[g].long_press_ocorreu = false;
}

static void cb_grupo_longpress(lv_event_t *e) {
  int g = (int)(intptr_t)lv_event_get_user_data(e);
  refs_grupo[g].long_press_ocorreu = true;
  lv_scr_load(scr_grupo[g]);
}

static void cb_grupo_clicked(lv_event_t *e) {
  int g = (int)(intptr_t)lv_event_get_user_data(e);
  if (refs_grupo[g].long_press_ocorreu) {
    refs_grupo[g].long_press_ocorreu = false;
    return;
  }
  bool novo_estado = !grupo_get_ativo(g);
  grupo_alternar(g);
  mixer_link_set_group_mute(g, !novo_estado);
  atualizar_visual_grupo(g);
}

static void cb_canal_clicked(lv_event_t *e) {
  int idx = (int)(intptr_t)lv_event_get_user_data(e);
  canal_alternar(idx);
  atualizar_visual_canal(idx);
}

static void cb_abrir_todos(lv_event_t *e) {
  (void)e;
  lv_scr_load(scr_todos);
}

static void cb_voltar_principal(lv_event_t *e) {
  (void)e;
  lv_scr_load(scr_principal);
}

/* ----------------------------------------------------------- widgets */
static lv_obj_t *criar_tela_base() {
  lv_obj_t *scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, COR_FUNDO_TELA, 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(scr, 0, 0);
  lv_obj_set_style_pad_all(scr, 14, 0);
  lv_obj_set_style_pad_row(scr, 10, 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  return scr;
}

/* topo com titulo "principal" (status de conexao) + botao de canto */
static lv_obj_t *criar_topbar_principal(lv_obj_t *parent) {
  lv_obj_t *bar = lv_obj_create(parent);
  lv_obj_remove_style_all(bar);
  lv_obj_set_size(bar, LV_PCT(100), 56);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *titulo = lv_label_create(bar);
  lv_label_set_text(titulo, "Painel de Canais");
  lv_obj_set_style_text_font(titulo, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(titulo, COR_TEXTO_CLARO, 0);
  lv_obj_align(titulo, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_t *wifi = lv_img_create(bar);
  lv_img_set_src(wifi, ICONES[IC_WIFI][2]);
  lv_obj_set_style_img_recolor(wifi, COR_TEXTO_CLARO, 0);
  lv_obj_set_style_img_recolor_opa(wifi, LV_OPA_80, 0);
  lv_obj_align(wifi, LV_ALIGN_TOP_LEFT, 0, 15);

  lbl_status = lv_label_create(bar);
  lv_label_set_text(lbl_status, "Modo teste - sem conexao com a mesa");
  lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl_status, COR_TEXTO_CLARO, 0);
  lv_obj_set_style_text_opa(lbl_status, LV_OPA_80, 0);
  lv_obj_align(lbl_status, LV_ALIGN_TOP_LEFT, 45, 30);

  lv_obj_t *btn = lv_obj_create(bar);
  lv_obj_set_size(btn, 48, 48);
  lv_obj_set_style_radius(btn, 24, 0);
  lv_obj_set_style_bg_color(btn, COR_FUNDO_BOTAO, 0);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(btn, 2, 0);
  lv_obj_set_style_border_color(btn, lv_color_hex(0x3A3A3E), 0);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_add_event_cb(btn, cb_abrir_todos, LV_EVENT_CLICKED, NULL);

  lv_obj_t *icone_btn = lv_img_create(btn);
  lv_img_set_src(icone_btn, ICONES[IC_GRID_TODOS][2]);
  lv_obj_set_style_img_recolor(icone_btn, COR_TEXTO_CLARO, 0);
  lv_obj_set_style_img_recolor_opa(icone_btn, LV_OPA_COVER, 0);
  lv_obj_center(icone_btn);

  return bar;
}

/* topo das subtelas: titulo do grupo/"todos" + botao "voltar" */
static lv_obj_t *criar_topbar_subtela(lv_obj_t *parent, const char *titulo_txt) {
  lv_obj_t *bar = lv_obj_create(parent);
  lv_obj_remove_style_all(bar);
  lv_obj_set_size(bar, LV_PCT(100), 56);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *titulo = lv_label_create(bar);
  lv_label_set_text(titulo, titulo_txt);
  lv_obj_set_style_text_font(titulo, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(titulo, COR_TEXTO_CLARO, 0);
  lv_obj_align(titulo, LV_ALIGN_LEFT_MID, 0, 0);

  lv_obj_t *btn = lv_obj_create(bar);
  lv_obj_set_size(btn, 48, 48);
  lv_obj_set_style_radius(btn, 24, 0);
  lv_obj_set_style_bg_color(btn, COR_FUNDO_BOTAO, 0);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(btn, 2, 0);
  lv_obj_set_style_border_color(btn, lv_color_hex(0x3A3A3E), 0);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(btn, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(btn, cb_voltar_principal, LV_EVENT_CLICKED, NULL);

  lv_obj_t *icone_btn = lv_img_create(btn);
  lv_img_set_src(icone_btn, ICONES[IC_HOME][2]);
  lv_obj_set_style_img_recolor(icone_btn, COR_TEXTO_CLARO, 0);
  lv_obj_set_style_img_recolor_opa(icone_btn, LV_OPA_COVER, 0);
  lv_obj_center(icone_btn);

  return bar;
}

static lv_obj_t *criar_bloco_grupo(lv_obj_t *parent, int g) {
  const GroupInfo &info = GRUPOS[g];
  lv_obj_t *tile = lv_obj_create(parent);
  lv_obj_set_style_radius(tile, 22, 0);
  lv_obj_set_style_border_width(tile, 3, 0);
  lv_obj_set_style_pad_all(tile, 8, 0);
  lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *icon = lv_img_create(tile);
  lv_img_set_src(icon, info.icone);
  lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 6);

  lv_obj_t *title = lv_label_create(tile);
  lv_label_set_text(title, info.nome);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_obj_set_width(title, LV_PCT(100));
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 106);

  char buf_sub[16];
  snprintf(buf_sub, sizeof(buf_sub), "%d canais", grupo_contar_canais(g));
  lv_obj_t *sub = lv_label_create(tile);
  lv_label_set_text(sub, buf_sub);
  lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_opa(sub, LV_OPA_70, 0);
  lv_obj_set_width(sub, LV_PCT(100));
  lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 142);

  refs_grupo[g] = { tile, icon, title, sub, g, false };
  lv_obj_add_event_cb(tile, cb_grupo_pressed, LV_EVENT_PRESSED, (void *)(intptr_t)g);
  lv_obj_add_event_cb(tile, cb_grupo_longpress, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)g);
  lv_obj_add_event_cb(tile, cb_grupo_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)g);

  atualizar_visual_grupo(g);
  return tile;
}

static void construir_scr_principal() {
  scr_principal = criar_tela_base();
  criar_topbar_principal(scr_principal);

  lv_obj_t *grid = lv_obj_create(scr_principal);
  lv_obj_remove_style_all(grid);
  lv_obj_set_width(grid, LV_PCT(100));
  lv_obj_set_flex_grow(grid, 1);
  lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
  static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  lv_obj_set_style_grid_column_dsc_array(grid, col_dsc, 0);
  lv_obj_set_style_grid_row_dsc_array(grid, row_dsc, 0);
  lv_obj_set_style_pad_column(grid, 14, 0);
  lv_obj_set_style_pad_row(grid, 14, 0);
  lv_obj_set_layout(grid, LV_LAYOUT_GRID);

  for (int g = 0; g < GRP_COUNT; g++) {
    lv_obj_t *tile = criar_bloco_grupo(grid, g);
    lv_obj_set_grid_cell(tile, LV_GRID_ALIGN_STRETCH, g % 2, 1, LV_GRID_ALIGN_STRETCH, g / 2, 1);
  }

  lv_obj_t *hint = lv_label_create(scr_principal);
  lv_label_set_text(hint, "toque: mute   -   segure: abrir canais");
  lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(hint, COR_TEXTO_CLARO, 0);
  lv_obj_set_style_text_opa(hint, LV_OPA_50, 0);
}

static lv_obj_t *criar_botao_canal_completo(lv_obj_t *parent, int idx) {
  const ChannelInfo &c = CANAIS[idx];
  lv_obj_t *btn = lv_obj_create(parent);
  lv_obj_set_style_radius(btn, 18, 0);
  lv_obj_set_style_border_width(btn, 3, 0);
  lv_obj_set_style_pad_all(btn, 6, 0);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *numero = lv_label_create(btn);
  char buf[8];
  snprintf(buf, sizeof(buf), "%02d", c.canal);
  lv_label_set_text(numero, buf);
  lv_obj_set_style_text_font(numero, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(numero, COR_TEXTO_CLARO, 0);
  lv_obj_set_style_text_opa(numero, LV_OPA_60, 0);
  lv_obj_align(numero, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_t *tag_mute = lv_label_create(btn);
  lv_label_set_text(tag_mute, "MUTE");
  lv_obj_set_style_text_font(tag_mute, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(tag_mute, COR_VERMELHO, 0);
  lv_obj_align(tag_mute, LV_ALIGN_TOP_RIGHT, 0, 0);

  lv_obj_t *icon = lv_img_create(btn);
  lv_img_set_src(icon, c.icone72);
  lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 20);

  /* nome do canal: centralizado, quebra em 2 linhas se for comprido
   * (ex.: "Microfone Sem Fio Branco") em vez de estourar o botao */
  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, c.nome);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
  lv_obj_set_width(label, LV_PCT(100));
  lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);

  refs_canal[num_refs_canal++] = { btn, icon, label, tag_mute, idx };
  lv_obj_add_event_cb(btn, cb_canal_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)idx);
  atualizar_visual_canal(idx); /* aplica cor/borda e a etiqueta MUTE conforme o estado atual */

  return btn;
}

/* Versao compacta (so icone + numero, sem o nome por extenso) - usada
 * so na tela "Todos os Canais", que com 22 canais nao tem espaco para
 * o cartao completo sem precisar de rolagem (e a ideia e nunca rolar,
 * ver pedido do usuario). O nome por extenso de cada canal continua
 * disponivel na tela do grupo dele. */
static lv_obj_t *criar_botao_canal_compacto(lv_obj_t *parent, int idx) {
  const ChannelInfo &c = CANAIS[idx];
  lv_obj_t *btn = lv_obj_create(parent);
  lv_obj_set_style_radius(btn, 14, 0);
  lv_obj_set_style_border_width(btn, 2, 0);
  lv_obj_set_style_pad_all(btn, 3, 0);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *icon = lv_img_create(btn);
  lv_img_set_src(icon, c.icone40);
  lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 4);

  lv_obj_t *numero = lv_label_create(btn);
  char buf[8];
  snprintf(buf, sizeof(buf), "%02d", c.canal);
  lv_label_set_text(numero, buf);
  lv_obj_set_style_text_font(numero, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(numero, COR_TEXTO_CLARO, 0);
  lv_obj_align(numero, LV_ALIGN_BOTTOM_MID, 0, 0);

  refs_canal[num_refs_canal++] = { btn, icon, NULL, NULL, idx };
  lv_obj_add_event_cb(btn, cb_canal_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)idx);
  atualizar_visual_canal(idx);

  return btn;
}

/* Grade SEM rolagem: numero de linhas/colunas fixo, calculado para a
 * quantidade de canais daquela tela, preenchendo sempre o espaco
 * disponivel (nunca precisa arrastar para ver o resto). */
static lv_obj_t *criar_grade_fixa(lv_obj_t *parent, int cols, int rows) {
  lv_obj_t *cont = lv_obj_create(parent);
  lv_obj_remove_style_all(cont);
  lv_obj_set_width(cont, LV_PCT(100));
  lv_obj_set_flex_grow(cont, 1);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

  lv_coord_t *col_dsc = (lv_coord_t *)malloc((cols + 1) * sizeof(lv_coord_t));
  lv_coord_t *row_dsc = (lv_coord_t *)malloc((rows + 1) * sizeof(lv_coord_t));
  for (int i = 0; i < cols; i++) col_dsc[i] = LV_GRID_FR(1);
  col_dsc[cols] = LV_GRID_TEMPLATE_LAST;
  for (int i = 0; i < rows; i++) row_dsc[i] = LV_GRID_FR(1);
  row_dsc[rows] = LV_GRID_TEMPLATE_LAST;
  /* ficam alocados ate o fim do programa (as telas nunca sao destruidas) */
  lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
  lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);
  lv_obj_set_style_pad_column(cont, 10, 0);
  lv_obj_set_style_pad_row(cont, 10, 0);
  lv_obj_set_layout(cont, LV_LAYOUT_GRID);
  return cont;
}

static void posicionar_na_grade(lv_obj_t *item, int indice, int cols) {
  lv_obj_set_grid_cell(item, LV_GRID_ALIGN_STRETCH, indice % cols, 1,
                        LV_GRID_ALIGN_STRETCH, indice / cols, 1);
}

static void construir_scr_grupo(int g) {
  /* colunas x linhas escolhidas para o numero de canais de cada grupo
   * (Vocal=4, Instrumentos=5, Bateria=7, Sem Fio=2), sempre cabendo
   * tudo numa tela so, sem rolagem */
  static const int COLS[GRP_COUNT] = {2, 3, 4, 1};
  static const int ROWS[GRP_COUNT] = {2, 2, 2, 2};

  scr_grupo[g] = criar_tela_base();
  criar_topbar_subtela(scr_grupo[g], GRUPOS[g].nome);
  lv_obj_t *grade = criar_grade_fixa(scr_grupo[g], COLS[g], ROWS[g]);

  int pos = 0;
  for (int i = 0; i < NUM_CANAIS; i++) {
    if (CANAIS[i].grupo != g) continue;
    lv_obj_t *btn = criar_botao_canal_completo(grade, i);
    posicionar_na_grade(btn, pos++, COLS[g]);
  }
}

static void construir_scr_todos() {
  const int COLS = 6, ROWS = 4; /* 24 vagas para os 22 canais */
  scr_todos = criar_tela_base();
  criar_topbar_subtela(scr_todos, "Todos os Canais");
  lv_obj_t *grade = criar_grade_fixa(scr_todos, COLS, ROWS);

  for (int i = 0; i < NUM_CANAIS; i++) {
    lv_obj_t *btn = criar_botao_canal_compacto(grade, i);
    posicionar_na_grade(btn, i, COLS);
  }
}

/* ------------------------------------------------------------- API */
void ui_init() {
  construir_scr_principal();
  for (int g = 0; g < GRP_COUNT; g++) construir_scr_grupo(g);
  construir_scr_todos();
  lv_scr_load(scr_principal);
}

void ui_set_status(StatusMesa status) {
  if (status == status_mostrado) return;
  status_mostrado = status;
  const char *txt = "Modo teste - sem conexao com a mesa";
  switch (status) {
    case STATUS_MODO_TESTE:    txt = "Modo teste - sem conexao com a mesa"; break;
    case STATUS_CONECTANDO:    txt = "Conectando a mesa..."; break;
    case STATUS_CONECTADO:     txt = "Conectado a mesa"; break;
    case STATUS_DESCONECTADO:  txt = "Desconectado da mesa"; break;
  }
  lv_label_set_text(lbl_status, txt);
}
