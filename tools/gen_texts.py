#!/usr/bin/env python3
"""Gera rotulos de texto como bitmaps alpha 8 bits (lv_img_dsc_t) para o
LVGL 8.3. Evita depender do conversor de fontes do LVGL (precisa de
Node.js/lv_font_conv, indisponivel offline) e garante que acentos do
portugues (a, i, ~, ^, ç ...) sejam sempre renderizados corretamente,
pois cada rotulo vira uma imagem, nao um glifo de fonte em tempo real.
"""
import sys
from PIL import Image, ImageDraw, ImageFont

SS = 3  # supersampling para anti-aliasing suave
FONTES = {
    'regular': '/usr/share/fonts/truetype/google-fonts/Poppins-Regular.ttf',
    'medium': '/usr/share/fonts/truetype/google-fonts/Poppins-Medium.ttf',
    'bold': '/usr/share/fonts/truetype/google-fonts/Poppins-Bold.ttf',
}

# (id, texto, tamanho_px, peso)
MANIFESTO = [
    # -- cabecalho / marca --------------------------------------------
    ('app_title', 'Painel de Canais', 21, 'bold'),

    # -- status de conexao (trocados em tempo de execucao) ------------
    ('status_teste', 'Modo teste \u2014 sem conex\u00e3o com a mesa', 16, 'regular'),
    ('status_conectando', 'Conectando \u00e0 mesa...', 16, 'regular'),
    ('status_conectado', 'Conectado \u00e0 mesa', 16, 'regular'),
    ('status_desconectado', 'Desconectado da mesa', 16, 'regular'),

    # -- blocos de grupo (tela principal) ------------------------------
    ('grp_title_vocal', 'Vocal', 29, 'bold'),
    ('grp_title_instrumentos', 'Instrumentos', 26, 'bold'),
    ('grp_title_bateria', 'Bateria', 29, 'bold'),
    ('grp_title_semfio', 'Sem Fio', 29, 'bold'),

    ('grp_sub_vocal', '4 canais', 15, 'medium'),
    ('grp_sub_instrumentos', '5 canais', 15, 'medium'),
    ('grp_sub_bateria', '7 canais', 15, 'medium'),
    ('grp_sub_semfio', '2 canais', 15, 'medium'),

    ('grp_hint', 'toque: mute   \u2022   segure: abrir canais', 13, 'regular'),
    ('grp_all_btn', 'Todos os canais individualmente', 15, 'medium'),

    # -- titulos das telas (grupo e "todos os canais") -----------------
    ('title_vocal', 'Vocal', 24, 'bold'),
    ('title_instrumentos', 'Instrumentos', 24, 'bold'),
    ('title_bateria', 'Bateria', 24, 'bold'),
    ('title_semfio', 'Sem Fio', 24, 'bold'),
    ('title_todos', 'Todos os Canais', 24, 'bold'),

    # -- rotulos dos 22 canais (planilha Canais.xlsx) ------------------
    ('ch_01', 'Microfone Amarelo', 18, 'medium'),
    ('ch_02', 'Microfone Verde', 18, 'medium'),
    ('ch_03', 'Microfone Azul', 18, 'medium'),
    ('ch_04', 'Microfone Preto', 18, 'medium'),
    ('ch_05', 'Microfone Sem Fio Branco', 18, 'medium'),
    ('ch_06', 'Microfone Sem Fio Azul', 18, 'medium'),
    ('ch_07', 'Guitarra 1', 18, 'medium'),
    ('ch_08', 'Guitarra 2', 18, 'medium'),
    ('ch_09', 'Viol\u00e3o', 18, 'medium'),
    ('ch_10', 'Teclado', 18, 'medium'),
    ('ch_11', 'Baixo', 18, 'medium'),
    ('ch_12', 'M\u00eddia', 18, 'medium'),
    ('ch_13', 'Talkback', 18, 'medium'),
    ('ch_14', 'Caixa', 18, 'medium'),
    ('ch_15', 'Tom 1', 18, 'medium'),
    ('ch_16', 'Tom 2', 18, 'medium'),
    ('ch_17', 'Surdo', 18, 'medium'),
    ('ch_18', 'Bumbo', 18, 'medium'),
    ('ch_19', 'Chimbal', 18, 'medium'),
    ('ch_20', 'Over', 18, 'medium'),
    ('ch_21', 'VS L', 18, 'medium'),
    ('ch_22', 'VS R', 18, 'medium'),
]


def render(text, size_px, peso):
    font = ImageFont.truetype(FONTES[peso], size_px * SS)
    tmp = Image.new('L', (8, 8), 0)
    d = ImageDraw.Draw(tmp)
    bbox = d.textbbox((0, 0), text, font=font)
    pad = 2 * SS
    w = (bbox[2] - bbox[0]) + pad * 2
    h = (bbox[3] - bbox[1]) + pad * 2
    im = Image.new('L', (w, h), 0)
    d = Image.new('L', (w, h), 0)
    dr = ImageDraw.Draw(im)
    dr.text((pad - bbox[0], pad - bbox[1]), text, font=font, fill=255)
    fw, fh = max(1, w // SS), max(1, h // SS)
    return im.resize((fw, fh), Image.LANCZOS)


def gerar(saida):
    linhas = ['/* Arquivo gerado por tools/gen_texts.py - nao edite a mao. */',
              '#include "texts.h"', '']
    entradas = []
    largura_total = 0
    for tid, texto, tam, peso in MANIFESTO:
        img = render(texto, tam, peso)
        w, h = img.size
        largura_total += w * h
        dados = img.tobytes()
        sim = f'txt_{tid}_map'
        linhas.append(f'static const uint8_t {sim}[] = {{')
        for i in range(0, len(dados), 24):
            linhas.append('  ' + ','.join(f'0x{b:02x}' for b in dados[i:i + 24]) + ',')
        linhas.append('};')
        dsc = f'txt_{tid}'
        linhas.append(f'const lv_img_dsc_t {dsc} = {{')
        linhas.append('  .header.cf = LV_IMG_CF_ALPHA_8BIT,')
        linhas.append('  .header.always_zero = 0,')
        linhas.append('  .header.reserved = 0,')
        linhas.append(f'  .header.w = {w},')
        linhas.append(f'  .header.h = {h},')
        linhas.append(f'  .data_size = {len(dados)},')
        linhas.append(f'  .data = {sim},')
        linhas.append('};')
        linhas.append('')
        entradas.append((tid, w, h))
    open(f'{saida}/src/texts.c', 'w').write('\n'.join(linhas) + '\n')

    h_lines = ['/* Arquivo gerado por tools/gen_texts.py - nao edite a mao. */',
               '#pragma once', '#include <lvgl.h>', '',
               '#ifdef __cplusplus', 'extern "C" {', '#endif', '']
    for tid, w, h in entradas:
        h_lines.append(f'extern const lv_img_dsc_t txt_{tid}; /* {w}x{h}px */')
    h_lines += ['', '#ifdef __cplusplus', '}', '#endif']
    open(f'{saida}/src/texts.h', 'w').write('\n'.join(h_lines) + '\n')

    print(f'{len(entradas)} textos gerados, {largura_total/1024:.1f} KB de pixels brutos')
    for tid, w, h in entradas:
        if w > 430:
            print(f'  AVISO: "{tid}" tem {w}px de largura, pode nao caber na tela (480px)')


if __name__ == '__main__':
    gerar(sys.argv[1])
