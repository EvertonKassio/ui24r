/*
 * mixer_link.cpp - Integracao com a Soundcraft Ui24R.
 *
 * Protocolo (o mesmo usado pela interface web da mesa), sobre um
 * WebSocket comum em ws://<ip-da-mesa>:80/ :
 *   - a mesa so continua informando o estado se receber o texto
 *     3:::ALIVE  - enviado aqui a cada 1 s (ALIVE_INTERVALO_MS);
 *     "2::" recebido e respondido com "2::"
 *   - comando de mute:   3:::SETD^i.<canal-1>.mute^<0|1>
 *                        (canal contado a partir de 0; 1 = mutado)
 *   - estado vindo da mesa: linhas 3:::SETD^i.N.mute^V. Ao conectar a
 *     mesa despeja o estado atual de tudo; depois avisa cada mudanca
 *     (inclusive as feitas por outros aparelhos ou pela propria mesa).
 *
 * O estado de cada canal na tela e SEMPRE o que a mesa informou (cada
 * canal e lido individualmente - nunca deduzido do grupo). Para o caso
 * de alguma atualizacao se perder, ha duas redes de seguranca:
 *   - vigia: sem receber nada da mesa por WATCHDOG_MS -> reconecta;
 *   - releitura periodica: a cada RESYNC_INTERVALO_MS (com a tela
 *     "parada", sem toques recentes) reconecta em silencio, o que faz a
 *     mesa despejar o estado completo de novo. 0 desliga.
 *
 * Na rede propria da mesa ("Soundcraft Ui24", sem senha) o IP dela e
 * 10.10.1.1. Se usar a mesa numa rede existente, mude MESA_IP.
 *
 * Mapeamento de canais: canal N da planilha -> entrada "i.(N-1)".
 *
 * Para testar so a interface, sem nenhuma conexao, WIFI_HABILITADO 0.
 */
#include "mixer_link.h"
#include "state.h"
#include "data.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <string.h>
#include <stdlib.h>

#define WIFI_HABILITADO      1
#define WIFI_SSID            "Soundcraft Ui24"
#define WIFI_SENHA           "" /* rede aberta, sem senha */
#define MESA_IP              "10.10.1.1"
#define MESA_PORTA           80
#define WIFI_TIMEOUT_MS      8000
#define WIFI_RETENTAR_MS     6000
#define WS_TIMEOUT_MS        8000
#define ALIVE_INTERVALO_MS   1000
#define WATCHDOG_MS          15000  /* sem nada da mesa -> reconecta */
#define RESYNC_INTERVALO_MS  30000  /* releitura completa; 0 = desligada */
#define RESYNC_OCIOSO_MS     2000   /* so releia se nao houve toque recente */
#define DEBUG_MESA           1      /* resumo no serial a cada 5 s */

static WebSocketsClient ws;
static StatusMesa status_atual = STATUS_MODO_TESTE;
static bool ws_iniciado = false;
static bool ws_conectado = false;
static bool resync_em_andamento = false;
static uint32_t inicio_wifi = 0;
static uint32_t inicio_ws = 0;
static uint32_t ultimo_alive = 0;
static uint32_t ultimo_rx = 0;
static uint32_t ultimo_envio = 0;
static uint32_t ultimo_resync = 0;
static uint32_t rx_mensagens = 0;
static uint32_t rx_mutes = 0;
static uint32_t ultimo_debug = 0;

/* ---------------------------------------------------------- entrada */
static void aplicar_mute_recebido(int numero_canal, bool ativo) {
  for (int i = 0; i < NUM_CANAIS; i++) {
    if (CANAIS[i].canal == numero_canal) {
      rx_mutes++;
      canal_set_ativo_remoto(i, ativo);
      return;
    }
  }
}

static void processar_linha(const char *l, size_t n) {
  if (n >= 3 && n <= 4 && strncmp(l, "2::", 3) == 0) {
    ws.sendTXT("2::"); /* ping-pong da interface original */
    return;
  }
  char buf[96];
  if (n == 0 || n >= sizeof(buf)) return;
  memcpy(buf, l, n);
  buf[n] = 0;

  const char *s = strstr(buf, "SETD^i.");
  if (!s) return;
  s += 7;
  char *fim;
  long idx = strtol(s, &fim, 10);
  if (fim == s || strncmp(fim, ".mute^", 6) != 0) return;
  float v = atof(fim + 6);
  aplicar_mute_recebido((int)idx + 1, v < 0.5f); /* 1 = mutado -> inativo */
}

static void processar_texto(const char *txt, size_t len) {
  const char *p = txt, *fim = txt + len;
  while (p < fim) {
    const char *nl = (const char *)memchr(p, '\n', fim - p);
    const char *fim_linha = nl ? nl : fim;
    processar_linha(p, fim_linha - p);
    p = nl ? nl + 1 : fim;
  }
}

static void evento_ws(WStype_t tipo, uint8_t *payload, size_t len) {
  switch (tipo) {
    case WStype_CONNECTED:
      ws_conectado = true;
      resync_em_andamento = false;
      ws.sendTXT("3:::ALIVE");
      ultimo_alive = ultimo_rx = ultimo_resync = millis();
      Serial.println("[mesa] WebSocket conectado");
      break;
    case WStype_DISCONNECTED:
      if (ws_conectado && !resync_em_andamento) Serial.println("[mesa] WebSocket desconectado");
      ws_conectado = false;
      if (!resync_em_andamento) inicio_ws = millis();
      break;
    case WStype_TEXT:
      ultimo_rx = millis();
      rx_mensagens++;
      processar_texto((const char *)payload, len);
      break;
    default:
      break;
  }
}

/* Reconecta o WebSocket: a mesa despeja o estado completo ao conectar. */
static void reler_estado(const char *motivo) {
  Serial.printf("[mesa] releitura do estado (%s)\n", motivo);
  resync_em_andamento = true;
  inicio_ws = millis();
  ws.disconnect();
  ws.begin(MESA_IP, MESA_PORTA, "/");
  ws_conectado = false;
}

/* ----------------------------------------------------------- API */
void mixer_link_init() {
#if WIFI_HABILITADO
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); /* menor latencia nos comandos */
  WiFi.begin(WIFI_SSID, WIFI_SENHA);
  status_atual = STATUS_CONECTANDO;
  inicio_wifi = millis();
#else
  status_atual = STATUS_MODO_TESTE;
#endif
}

void mixer_link_loop() {
#if WIFI_HABILITADO
  if (WiFi.status() == WL_CONNECTED) {
    if (!ws_iniciado) {
      ws.begin(MESA_IP, MESA_PORTA, "/");
      ws.onEvent(evento_ws);
      ws.setReconnectInterval(1500);
      ws_iniciado = true;
      inicio_ws = millis();
    }
    ws.loop();
    uint32_t agora = millis();

    if (ws_conectado) {
      status_atual = STATUS_CONECTADO;

      /* mantem a mesa informando o estado dos canais */
      if (agora - ultimo_alive >= ALIVE_INTERVALO_MS) {
        ws.sendTXT("3:::ALIVE");
        ultimo_alive = agora;
      }

      if (agora - ultimo_rx > WATCHDOG_MS) {
        reler_estado("sem dados da mesa");
      } else if (RESYNC_INTERVALO_MS > 0 &&
                 agora - ultimo_resync >= RESYNC_INTERVALO_MS &&
                 agora - ultimo_envio >= RESYNC_OCIOSO_MS) {
        reler_estado("periodica");
      }
    } else if (resync_em_andamento && agora - inicio_ws < 4000) {
      /* releitura em silencio: mantem o status como esta */
    } else {
      resync_em_andamento = false;
      status_atual = (agora - inicio_ws > WS_TIMEOUT_MS) ? STATUS_DESCONECTADO
                                                          : STATUS_CONECTANDO;
    }

#if DEBUG_MESA
    if (agora - ultimo_debug >= 5000) {
      ultimo_debug = agora;
      Serial.printf("[mesa] ws=%d msgs=%lu mutes=%lu ultimo_rx=%lums\n",
                    (int)ws_conectado, (unsigned long)rx_mensagens,
                    (unsigned long)rx_mutes, (unsigned long)(agora - ultimo_rx));
    }
#endif
    return;
  }

  /* sem Wi-Fi */
  ws_conectado = false;
  resync_em_andamento = false;
  if (millis() - inicio_wifi > WIFI_TIMEOUT_MS) status_atual = STATUS_DESCONECTADO;
  if (millis() - inicio_wifi > WIFI_TIMEOUT_MS + WIFI_RETENTAR_MS) {
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_SENHA);
    status_atual = STATUS_CONECTANDO;
    inicio_wifi = millis();
  }
#endif
}

StatusMesa mixer_link_status() {
  return status_atual;
}

void mixer_link_enviar_mute(int numero_canal, bool ativo) {
#if WIFI_HABILITADO
  if (!ws_conectado) return;
  char msg[40];
  snprintf(msg, sizeof(msg), "3:::SETD^i.%d.mute^%d", numero_canal - 1, ativo ? 0 : 1);
  ws.sendTXT(msg);
  ultimo_envio = millis();
#else
  (void)numero_canal; (void)ativo;
#endif
}
