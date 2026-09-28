# Painel de Canais — ESP32-4848S040C(E) para a Soundcraft Ui24R

Interface de controle para a mesa **Soundcraft Ui24R**, rodando no display
**ESP32-4848S040C(E)** (ESP32-S3, tela 480×480 IPS com painel ST7701 em modo
RGB, toque capacitivo GT911). Feita com **LVGL 8.3** + **Arduino_GFX**, via
**PlatformIO**.

## Estado atual (importante)

Como pedido, esta primeira versão **não conversa com a mesa ainda**:

- A tela **conecta de verdade no Wi-Fi** da mesa (rede `Soundcraft Ui24`,
  aberta) e mostra o status real dessa conexão (conectando / conectado /
  desconectado) no topo da tela principal.
- Só o **Wi-Fi** é real. O **estado dos canais (mute/ativo) é 100% local**:
  tocar num botão só muda a cor na tela, nada é enviado para a mesa. A
  Ui24R usa um protocolo próprio (não é OSC nem algo documentado
  publicamente pela Soundcraft), então essa parte fica para quando você
  tiver a mesa por perto para testar e capturar o protocolo.
- O ponto de extensão já está pronto: a função `enviar_mute_para_mesa()`
  em `src/state.cpp` é chamada toda vez que o usuário muda o estado de um
  canal pela tela — é só implementar o envio real ali dentro. Da mesma
  forma, `mixer_link.cpp` é onde entraria a leitura do estado real dos
  canais vindo da mesa (hoje só cuida do Wi-Fi).

## O que a interface faz

- **Tela principal**: status da conexão (Wi-Fi + estado) e 4 blocos
  grandes, um por grupo de canais (dados vindos de `Canais.xlsx`):
  **Vocal** (canais 1–4), **Instrumentos** (7–11), **Bateria** (14–20) e
  **Sem Fio** (5–6). Os canais 12, 13, 21 e 22 (Mídia, Talkback, VS L,
  VS R) não entram em nenhum desses 4 grupos — eles só aparecem em
  "Todos os Canais".
  - **Toque curto** no bloco: muta/desmuta o grupo inteiro de uma vez.
  - **Toque e segure (1,5 s)**: abre a tela daquele grupo, com um botão
    para cada canal individualmente.
  - Botão no canto superior direito: abre **"Todos os Canais"** (todos
    os 22, cada um controlável separadamente).
- **Telas de canais** (grupo ou "Todos"): botão no canto superior direito
  volta para a tela principal. Tocar num canal muta/desmuta só ele. Todas
  as telas mostram todos os botões de uma vez, numa grade que se ajusta
  ao número de canais — **sem rolagem, nunca precisa arrastar** para ver
  o resto. Na tela "Todos os Canais" (22 canais, sem espaço para nome
  por extenso de cada um) os botões ficam mais compactos, só com ícone e
  número — o nome completo de cada canal continua na tela do grupo dele.
- **Visual**: tema escuro, cada grupo/canal com uma cor de identificação;
  canal **mutado** = só a borda colorida; canal **ativo** = botão
  totalmente preenchido com a cor. Ícones para cada instrumento/tipo de
  canal (nada de texto cortado por acento faltando — ver nota abaixo).

## Por que os textos não usam fonte "de verdade" do LVGL

O conversor de fonte oficial do LVGL (`lv_font_conv`) precisa de Node.js e
não estava disponível para gerar isso aqui. Para não perder acentos do
português (Violão, Mídia, "conexão", "à mesa"...) — que **não** existem nos
fontes Montserrat embutidos do LVGL (só ASCII) — cada texto da interface
foi pré-renderizado como uma **imagem bitmap** (mesma técnica dos ícones),
com a fonte Poppins, por `tools/gen_texts.py`. Isso já está pronto e
gerado em `src/texts.c/h` — você não precisa rodar nada para isso
funcionar. Só rótulos curtos sem acento (o número do canal, "MUTE") usam
fonte de verdade do LVGL, porque esses continuam editáveis/dinâmicos.

Se um dia você quiser trocar algum texto, edite a lista `MANIFESTO` em
`tools/gen_texts.py` e rode `python3 tools/gen_texts.py src` de novo (o
mesmo vale para ícones novos em `tools/gen_icons.py`).

## Como compilar

Precisa do [PlatformIO](https://platformio.org/) (CLI ou extensão do
VS Code). Não precisa instalar Git nem nada manualmente além dele — o
`platformio.ini` já lista as 3 bibliotecas usadas (Arduino_GFX, LVGL
8.3, TAMC_GT911) pelo **registro do PlatformIO**, então o PlatformIO
baixa tudo sozinho (como um zip, sem clonar repositório) na primeira
compilação.

```bash
cd painel-canais-ui24r
pio run                 # compila
pio run -t upload       # grava no ESP32 (USB)
pio device monitor      # abre o serial (115200 baud), opcional
```

## Pinagem usada (ESP32-4848S040C/CE)

| Sinal | Pino | | Sinal | Pino |
|---|---|---|---|---|
| Retroiluminação | 38 | | Toque SDA | 19 |
| LCD CS | 39 | | Toque SCL | 45 |
| LCD SCK | 48 | | DE | 18 |
| LCD SDA | 47 | | VSYNC | 17 |
| R0–R4 | 11,12,13,14,0 | | HSYNC | 16 |
| G0–G5 | 8,20,3,46,9,10 | | PCLK | 21 |
| B0–B4 | 4,5,6,7,15 | | | |

Todos os valores estão centralizados em `src/board_config.h`. Eles foram
conferidos cruzando vários projetos independentes para esta mesma placa,
mas **placas vendidas como "4848S040C" e "4848S040CE" têm pequenas
variações de lote** — se algo não bater, é só esse arquivo que precisa
mudar.

## Se algo não funcionar de primeira

- **Imagem "de lado" (texto na vertical)**: em `src/board_config.h`, mude
  `PAINEL_ROTACAO` — tente `0`, depois `1`, `2` e `3`, recompilando e
  regravando a cada tentativa, até o texto "Painel de Canais" aparecer
  na horizontal e do jeito certo.
- **Cores erradas / "em negativo"**: já era um problema conhecido nesta
  combinação de placa+biblioteca — a tabela de inicialização do painel
  que a maioria dos exemplos por aí usa (`st7701_type1_init_operations`)
  deixa as cores invertidas nesta placa especificamente. O projeto já
  vem configurado com a tabela certa (`st7701_type9_init_operations`).
  Se mesmo assim as cores ainda saírem erradas, tente estas duas coisas,
  uma de cada vez:
  1. `PAINEL_INVERTER_CORES` para `1` em `src/board_config.h`;
  2. `LV_COLOR_16_SWAP` para `1` em `include/lv_conf.h`.
- **Toque no lugar errado (espelhado, trocado, etc.)**: com
  `DEBUG_TOQUE 1` (já é o padrão) em `src/board_config.h`, abra o
  monitor serial (`pio device monitor`, 115200 baud) e toque nos 4
  cantos da tela — ele mostra as coordenadas cruas de cada toque. Use
  isso pra descobrir a combinação certa de `TOQUE_TROCAR_XY`,
  `TOQUE_INVERTER_X` e `TOQUE_INVERTER_Y` (também em `board_config.h`):
  se tocar na esquerda e acender um botão da direita, é `X` invertido;
  se tocar em cima e acender embaixo, é `Y` invertido; se os toques
  saem "girados 90°" (o eixo X responde como se fosse o Y), é
  `TOQUE_TROCAR_XY`. Ajuste **depois** de já ter acertado a
  `PAINEL_ROTACAO` acima, porque mudar a rotação da tela muda qual
  combinação de toque é a certa.
- **Wi-Fi da mesa continua funcionando "sozinho"?**: a rede
  `Soundcraft Ui24` só existe quando a mesa está ligada e com o Wi-Fi dela
  ativo. Enquanto isso, a tela mostra "Desconectado" normalmente — é
  esperado, não afeta o uso da interface (o clique nos canais continua
  funcionando local).
- **Erro de compilação `esp32-hal-periman.h: No such file or directory`**:
  já corrigido na versão atual (`platform = espressif32@6.9.0` fixo no
  `platformio.ini`, junto com versões travadas das 3 bibliotecas). O
  que acontecia: a versão mais nova do `espressif32` vem com o core
  Arduino-ESP32 3.x (baseado no ESP-IDF 5), que reorganizou vários
  headers internos — e a Arduino_GFX/TAMC_GT911 ainda esperam a
  estrutura do core 2.0.x. Se você já tinha rodado `pio run` antes com
  a versão antiga do projeto, apague a pasta `.pio` (`rm -rf .pio` ou
  pelo Explorer mesmo) antes de compilar de novo, senão ele pode reusar
  os pacotes errados já baixados em cache.
- **`UserSideException: Please install Git client` no PlatformIO, mesmo
  com o Git instalado**: isso já foi resolvido na versão atual do
  projeto — as 3 bibliotecas agora vêm do registro do PlatformIO, não
  de um link do GitHub, então ele não precisa mais chamar o `git`. Se
  você baixou uma cópia antiga deste projeto, pegue o `platformio.ini`
  de novo. Se o erro aparecer por outro motivo, normalmente é o `git`
  não estar no PATH do terminal ainda — feche e abra de novo o
  terminal/VS Code (às vezes precisa reiniciar o Windows) e confirme
  com `git --version` num terminal novo.

## Estrutura do projeto

```
platformio.ini        configuração da placa e bibliotecas
include/lv_conf.h      configuração do LVGL
src/
  board_config.h        pinagem e parâmetros de calibração do toque
  display.h / .cpp       inicialização do painel (Arduino_GFX) + toque (GT911) + LVGL
  data.h                 tabela de canais e grupos (a partir de Canais.xlsx)
  state.h / .cpp          estado local de mute + ponto de extensão p/ a mesa
  mixer_link.h / .cpp     conexão Wi-Fi com a mesa (protocolo da mesa: TODO)
  ui.h / .cpp             as telas em si (LVGL)
  icons.c / .h            ícones (gerados por tools/gen_icons.py)
  texts.c / .h            textos em bitmap, com acentos (tools/gen_texts.py)
  main.cpp                setup()/loop()
tools/
  gen_icons.py           gerador dos ícones (não precisa rodar de novo)
  gen_texts.py           gerador dos textos (não precisa rodar de novo)
```
