#!/usr/bin/env python3
"""Gera os icones (mascaras alpha 8 bits) em C para LVGL 8.3.
Cada icone e desenhado numa grade 256x256 com supersampling 4x."""
import math, sys
from PIL import Image, ImageDraw, ImageChops

S, N = 4, 256
TAMANHOS = [96, 72, 40]


class Cv:
    def __init__(self):
        self.im = Image.new('L', (N * S, N * S), 0)
        self.d = ImageDraw.Draw(self.im)

    def _p(self, pts):
        return [(x * S, y * S) for x, y in pts]

    def rect(self, x0, y0, x1, y1, r=0, fill=255):
        self.d.rounded_rectangle([x0 * S, y0 * S, x1 * S, y1 * S], radius=r * S, fill=fill)

    def ell(self, x0, y0, x1, y1, fill=255):
        self.d.ellipse([x0 * S, y0 * S, x1 * S, y1 * S], fill=fill)

    def poly(self, pts, fill=255):
        self.d.polygon(self._p(pts), fill=fill)

    def line(self, p0, p1, w, fill=255):
        self.d.line(self._p([p0, p1]), fill=fill, width=int(w * S))
        r = w / 2
        for (x, y) in (p0, p1):
            self.ell(x - r, y - r, x + r, y + r, fill=fill)

    def arc(self, cx, cy, r, a0, a1, w, fill=255):
        self.d.arc([(cx - r) * S, (cy - r) * S, (cx + r) * S, (cy + r) * S],
                   a0, a1, fill=fill, width=int(w * S))
        for a in (a0, a1):
            x = cx + r * math.cos(math.radians(a))
            y = cy + r * math.sin(math.radians(a))
            self.ell(x - w / 2, y - w / 2, x + w / 2, y + w / 2, fill=fill)

    def rot(self, ang, center=(128, 128)):
        self.im = self.im.rotate(ang, resample=Image.BICUBIC,
                                 center=(center[0] * S, center[1] * S))
        self.d = ImageDraw.Draw(self.im)
        return self


def unir(*cvs):
    im = cvs[0].im
    for c in cvs[1:]:
        im = ImageChops.lighter(im, c.im)
    r = Cv()
    r.im = im
    r.d = ImageDraw.Draw(im)
    return r


# ----------------------------------------------------------------- icones
def ic_mic():
    c = Cv()
    c.rect(88, 20, 168, 140, r=40)
    for y in (56, 80, 104):
        c.rect(102, y, 154, y + 8, fill=0)
    c.arc(128, 110, 66, 0, 180, 14)
    c.line((128, 176), (128, 224), 14)
    c.line((88, 228), (168, 228), 14)
    return c


def ic_wireless():
    c = Cv()
    c.ell(92, 28, 164, 100)
    for y in (52, 66, 80):
        c.rect(102, y, 154, y + 6, fill=0)
    c.rect(104, 100, 152, 114, r=4)
    c.poly([(106, 114), (150, 114), (142, 232), (114, 232)])
    c.rect(120, 140, 136, 168, r=6, fill=0)
    for r in (62, 88):
        c.arc(128, 64, r, -42, 42, 10)
        c.arc(128, 64, r, 138, 222, 10)
    return c


def _cabeca(c, x0, y0, x1, y1, r=8):
    c.rect(x0, y0, x1, y1, r=r)


def ic_guitar():
    c = Cv()
    c.ell(70, 146, 186, 238)
    c.ell(84, 104, 172, 172)
    c.rect(118, 26, 138, 130)
    _cabeca(c, 110, 6, 146, 42)
    c.rect(100, 168, 156, 178, fill=0)
    c.rect(100, 192, 156, 202, fill=0)
    c.rect(104, 214, 152, 220, fill=0)
    return c.rot(-45)


def ic_acoustic():
    c = Cv()
    c.ell(64, 140, 192, 242)
    c.ell(80, 100, 176, 168)
    c.rect(119, 26, 137, 120)
    _cabeca(c, 112, 6, 144, 40)
    c.ell(108, 164, 148, 204, fill=0)
    c.rect(104, 218, 152, 226, r=3, fill=0)
    return c.rot(-45)


def ic_bass():
    c = Cv()
    c.ell(78, 168, 178, 240)
    c.ell(90, 132, 166, 190)
    c.rect(120, 30, 136, 150)
    _cabeca(c, 112, 6, 144, 40, r=6)
    c.rect(106, 176, 150, 186, fill=0)
    c.rect(106, 196, 150, 206, fill=0)
    c.rect(108, 222, 148, 227, fill=0)
    return c.rot(-45)


def ic_keys():
    c = Cv()
    c.rect(12, 64, 244, 192, r=14)
    n, x0, x1 = 7, 24, 232
    w = (x1 - x0) / n
    for i in range(n):
        c.rect(x0 + w * i + 2, 76, x0 + w * (i + 1) - 2, 180, r=4, fill=0)
    for i in (1, 2, 4, 5, 6):
        x = x0 + w * i
        c.rect(x - 9, 76, x + 9, 134, r=3, fill=255)
    return c


def ic_media():
    c = Cv()
    c.ell(48, 172, 112, 220)
    c.ell(148, 148, 212, 196)
    c.rect(98, 60, 114, 196)
    c.rect(198, 40, 214, 172)
    c.poly([(98, 52), (214, 28), (214, 76), (98, 100)])
    return c


def ic_talkback():
    c = Cv()
    c.arc(128, 112, 88, 180, 360, 16)
    c.rect(24, 108, 72, 184, r=18)
    c.rect(184, 108, 232, 184, r=18)
    c.line((208, 184), (196, 222), 10)
    c.line((196, 222), (150, 230), 10)
    c.ell(126, 216, 152, 242)
    return c


def _tambor(c, x0, x1, ytopo, alt, luvas, faixa=None):
    """Tambor em perspectiva: elipse superior, corpo, elipse inferior."""
    ry = 27
    c.ell(x0, ytopo - ry, x1, ytopo + ry)
    c.rect(x0, ytopo, x1, ytopo + alt)
    c.ell(x0, ytopo + alt - ry, x1, ytopo + alt + ry)
    # pele: aro, vao e pele
    c.ell(x0 + 14, ytopo - ry + 8, x1 - 14, ytopo + ry - 8, fill=0)
    c.ell(x0 + 22, ytopo - ry + 13, x1 - 22, ytopo + ry - 13)
    yb = ytopo + ry + 8
    if faixa:
        c.rect(x0, yb + faixa, x1, yb + faixa + 5, fill=0)
    for x in luvas:
        c.rect(x - 4, yb + (faixa or 0) + 12, x + 4, ytopo + alt + ry - 12, fill=0)


def ic_snare():
    c = Cv()
    _tambor(c, 36, 220, 122, 72, [72, 112, 144, 184], faixa=2)
    # baquetas cruzadas com "halo"
    c.line((58, 12), (150, 100), 22, fill=0)
    c.line((198, 12), (106, 100), 22, fill=0)
    c.line((58, 14), (150, 98), 12)
    c.line((198, 14), (106, 98), 12)
    return c


def ic_tom():
    c = Cv()
    _tambor(c, 52, 204, 104, 90, [80, 112, 144, 176], faixa=2)
    c.rect(96, 14, 160, 32, r=8)
    c.line((128, 30), (128, 80), 12)
    return c


def ic_floortom():
    c = Cv()
    c.line((60, 170), (44, 240), 12)
    c.line((196, 170), (212, 240), 12)
    c.line((128, 190), (128, 244), 12)
    _tambor(c, 40, 216, 78, 92, [76, 112, 144, 180], faixa=2)
    return c


def ic_kick():
    c = Cv()
    c.ell(20, 20, 236, 236)
    c.ell(34, 34, 222, 222, fill=0)
    c.ell(44, 44, 212, 212)
    c.ell(84, 84, 172, 172, fill=0)
    c.ell(98, 98, 158, 158)
    for k in range(8):
        a = math.radians(k * 45 + 22.5)
        x, y = 128 + 101 * math.cos(a), 128 + 101 * math.sin(a)
        c.ell(x - 3.5, y - 3.5, x + 3.5, y + 3.5, fill=0)
    return c


def _tripe(c, y0=214):
    c.line((128, y0), (72, 244), 10)
    c.line((128, y0), (184, 244), 10)


def ic_hihat():
    c = Cv()
    c.rect(121, 40, 135, 236, r=6)
    _tripe(c)
    # halos e pratos
    c.ell(24, 112, 232, 156, fill=0)
    c.ell(28, 118, 228, 150)
    c.ell(24, 72, 232, 116, fill=0)
    c.ell(28, 78, 228, 110)
    c.ell(100, 62, 156, 90)
    c.rect(118, 40, 138, 54, r=6)
    return c


def ic_cymbal():
    c = Cv()
    prato = Cv()
    prato.ell(24, 80, 232, 112)
    prato.ell(104, 64, 152, 94)
    prato.rot(12, center=(128, 96))
    haste = Cv()
    haste.rect(121, 104, 135, 236, r=6)
    _tripe(haste)
    haste.rect(116, 92, 140, 108, r=6)
    return unir(prato, haste)


def ic_speaker():
    c = Cv()
    c.poly([(30, 98), (80, 98), (140, 48), (140, 208), (80, 158), (30, 158)])
    for r in (44, 76):
        c.arc(148, 128, r, -45, 45, 12)
    return c


def ic_home():
    c = Cv()
    c.poly([(128, 22), (26, 112), (52, 112), (52, 232), (110, 232),
            (110, 158), (146, 158), (146, 232), (204, 232), (204, 112),
            (230, 112)])
    c.rect(52, 112, 204, 232, fill=255)
    c.poly([(128, 22), (26, 112), (52, 112), (128, 54), (204, 112), (230, 112)])
    c.rect(110, 158, 146, 232, fill=0)
    return c


def ic_wifi():
    c = Cv()
    for r, w in ((150, 16), (104, 16), (58, 16)):
        c.arc(128, 210, r, 205, 335, w)
    c.ell(112, 192, 144, 224)
    return c


def ic_grid_grupos():
    c = Cv()
    for x in (30, 138):
        for y in (30, 138):
            c.rect(x, y, x + 88, y + 88, r=18)
    return c


def ic_grid_todos():
    c = Cv()
    for i in range(4):
        for j in range(4):
            x, y = 25 + i * 54, 25 + j * 54
            c.rect(x, y, x + 44, y + 44, r=8)
    return c


ICONES = [
    ("mic", ic_mic), ("wireless", ic_wireless), ("guitar", ic_guitar),
    ("acoustic", ic_acoustic), ("bass", ic_bass), ("keys", ic_keys),
    ("media", ic_media), ("talkback", ic_talkback), ("snare", ic_snare),
    ("tom", ic_tom), ("floortom", ic_floortom), ("kick", ic_kick),
    ("hihat", ic_hihat), ("cymbal", ic_cymbal), ("speaker", ic_speaker),
    ("grid_grupos", ic_grid_grupos), ("grid_todos", ic_grid_todos),
    ("home", ic_home), ("wifi", ic_wifi),
]


def alpha(cv, tam):
    return cv.im.resize((tam, tam), Image.LANCZOS)


def gerar_c(path_c, path_h):
    linhas = ['/* Arquivo gerado por tools/gen_icons.py - nao edite a mao. */',
              '#include "icons.h"', '']
    tabela = []
    for nome, fn in ICONES:
        cv = fn()
        ptrs = []
        for t in TAMANHOS:
            dados = alpha(cv, t).tobytes()
            sim = f'ico_{nome}_{t}'
            linhas.append(f'static const uint8_t {sim}_map[] = {{')
            for i in range(0, len(dados), 24):
                linhas.append('  ' + ','.join(f'0x{b:02x}' for b in dados[i:i + 24]) + ',')
            linhas.append('};')
            linhas.append(f'static const lv_img_dsc_t {sim} = {{')
            linhas.append('  .header.cf = LV_IMG_CF_ALPHA_8BIT,')
            linhas.append('  .header.always_zero = 0,')
            linhas.append('  .header.reserved = 0,')
            linhas.append(f'  .header.w = {t},')
            linhas.append(f'  .header.h = {t},')
            linhas.append(f'  .data_size = {len(dados)},')
            linhas.append(f'  .data = {sim}_map,')
            linhas.append('};')
            linhas.append('')
            ptrs.append('&' + sim)
        tabela.append('  { ' + ', '.join(ptrs) + ' },')
    linhas.append('const lv_img_dsc_t * const ICONES[IC_COUNT][3] = {')
    linhas += tabela
    linhas.append('};')
    open(path_c, 'w').write('\n'.join(linhas) + '\n')

    ids = ['IC_' + n.upper() for n, _ in ICONES]
    h = ['/* Arquivo gerado por tools/gen_icons.py - nao edite a mao. */',
         '#pragma once', '#include <lvgl.h>', '',
         '#ifdef __cplusplus', 'extern "C" {', '#endif', '',
         'enum IconId {'] + [f'  {i},' for i in ids] + ['  IC_COUNT', '};', '',
         '/* tamanhos disponiveis (indice 0..2) */',
         'static const int ICON_TAM[3] = {%s};' % ', '.join(map(str, TAMANHOS)), '',
         'extern const lv_img_dsc_t * const ICONES[IC_COUNT][3];', '',
         '#ifdef __cplusplus', '}', '#endif']
    open(path_h, 'w').write('\n'.join(h) + '\n')


def folha(path_png):
    """Folha de contato para conferir os desenhos."""
    cols = 6
    rows = (len(ICONES) + cols - 1) // cols
    cel = 140
    im = Image.new('RGB', (cols * cel, rows * cel), (18, 18, 22))
    for k, (nome, fn) in enumerate(ICONES):
        a = alpha(fn(), 96)
        fundo = Image.new('RGB', a.size, (255, 159, 10))
        x, y = (k % cols) * cel + 22, (k // cols) * cel + 22
        im.paste(fundo, (x, y), a)
    im.save(path_png)


if __name__ == '__main__':
    saida = sys.argv[1]
    gerar_c(f'{saida}/src/icons.c', f'{saida}/src/icons.h')
    folha('/home/claude/tools/folha.png')
    print('ok')
