/*
 * mixer_link.cpp - Conexao Wi-Fi com a rede aberta gerada pela
 * Soundcraft Ui24R ou hotspot do celular.
 *
 * O que este arquivo FAZ hoje: conecta de verdade nessa rede Wi-Fi e
 * informa o status (conectando/conectado/desconectado) para a tela.
 *
 * O que ele NAO faz ainda: falar o protocolo da mesa em si (a Ui24R
 * usa um protocolo proprio, nao documentado publicamente pela
 * Soundcraft, normalmente via TCP na porta usada pelo app oficial).
 * Isso fica para quando voce tiver a mesa por perto para testar -
 * o ponto de entrada e a funcao enviar_mute_para_mesa() em state.cpp.
 *
 * Se quiser testar so a interface, sem tentar nenhuma conexao Wi-Fi,
 * mude WIFI_HABILITADO para 0 abaixo.
 */
#include "mixer_link.h"
#include "state.h"
#include "ui.h"
#include <WiFi.h>
#include <WebSocketsClient.h>

#define WIFI_HABILITADO 1
#define WIFI_SSID       "Soundcraft Ui24"
#define WIFI_SENHA      "scuiwlan"
#define WIFI_TIMEOUT_MS 8000
#define WIFI_RETENTAR_MS 6000
#define MESA_IP          "10.10.1.1"
#define MESA_WS_PORT     80
#define MESA_WS_PATH     "/socket.io/1/websocket/"

static StatusMesa status_atual = STATUS_MODO_TESTE;
static uint32_t inicio_tentativa = 0;
static WebSocketsClient web_socket;
static bool websocket_iniciado = false;
static uint32_t mgmask_atual = 0;
static bool mgmask_valido = false;

static void tratar_mensagem_mesa(const char *mensagem) {
  if (strncmp(mensagem, "2::", 3) == 0) {
    web_socket.sendTXT("2::");
    return;
  }
  if (strncmp(mensagem, "3:::", 4) != 0) return;

  const char *linha = mensagem + 4;
  if(strncmp(linha, "SETD^i", 6) == 0) {
    Serial.printf("[RAW MESA] %s\n",linha);
    web_socket.sendTXT("2::");
    return;
  }
  
  while (*linha) {
    if (strncmp(linha, "SETD^mgmask^", 12) == 0) {
      uint32_t recebido = strtoul(linha + 12, NULL, 10);
      mgmask_atual = recebido;
      mgmask_valido = true;
      estado_aplicar_mgmask(recebido);
      ui_refresh_state();
      Serial.printf("[mesa] mgmask=%lu\n", (unsigned long)recebido);
    } 
    const char *proxima = strchr(linha, '\n');
    if (!proxima) break;
    linha = proxima + 1;
  }
  
}

static void websocket_event(WStype_t tipo, uint8_t *payload, size_t length) {
  (void)length;
  switch (tipo) {
    case WStype_CONNECTED:
      status_atual = STATUS_CONECTADO;
      Serial.println("[mesa] WebSocket conectado");
      web_socket.sendTXT("3:::ALIVE");
      break;
    case WStype_DISCONNECTED:
      status_atual = STATUS_DESCONECTADO;
      Serial.println("[mesa] WebSocket desconectado");
      break;
    case WStype_TEXT:
      tratar_mensagem_mesa((const char *)payload);
      break;
    default:
      break;
  }
}

static void diagnosticar_redes_wifi() {
  Serial.println("[wifi] iniciando varredura...");
  int total = WiFi.scanNetworks(false, true);
  if (total < 0) {
    Serial.printf("[wifi] falha na varredura: %d\n", total);
  } else if (total == 0) {
    Serial.println("[wifi] nenhuma rede encontrada");
  } else {
    for (int i = 0; i < total; i++) {
      Serial.printf("[wifi] rede: '%s', canal=%d, RSSI=%d dBm\n",
                    WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i));
    }
  }
  WiFi.scanDelete();
}

void mixer_link_init() {
#if WIFI_HABILITADO
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
  Serial.printf("[wifi] procurando SSID '%s' (ESP32 usa 2,4 GHz)\n", WIFI_SSID);
  diagnosticar_redes_wifi();
  WiFi.begin(WIFI_SSID, WIFI_SENHA);
  status_atual = STATUS_CONECTANDO;
  inicio_tentativa = millis();
#else
  status_atual = STATUS_MODO_TESTE;
#endif
}

void mixer_link_loop() {
#if WIFI_HABILITADO
  if (WiFi.status() == WL_CONNECTED) {
    static bool informou_conexao = false;
    if (!informou_conexao) {
      Serial.printf("[wifi] conectado, IP=%s\n", WiFi.localIP().toString().c_str());
      informou_conexao = true;
    }
    if (!websocket_iniciado) {
      web_socket.begin(MESA_IP, MESA_WS_PORT, MESA_WS_PATH);
      web_socket.onEvent(websocket_event);
      web_socket.setReconnectInterval(WIFI_RETENTAR_MS);
      websocket_iniciado = true;
      status_atual = STATUS_CONECTANDO;
      Serial.printf("[mesa] conectando WebSocket ws://%s%s\n", MESA_IP, MESA_WS_PATH);
    }
    web_socket.loop();
    return;
  }

  if (status_atual == STATUS_CONECTADO) {
    /* estava conectado e caiu */
    Serial.printf("[wifi] conexao perdida, status=%d\n", WiFi.status());
    status_atual = STATUS_DESCONECTADO;
    inicio_tentativa = millis();
    return;
  }

  if (millis() - inicio_tentativa > WIFI_TIMEOUT_MS) {
    static bool informou_falha = false;
    status_atual = STATUS_DESCONECTADO;
    if (!informou_falha) {
      Serial.printf("[wifi] falha ao conectar, status=%d\n", WiFi.status());
      if (WiFi.status() == WL_NO_SSID_AVAIL) {
        Serial.printf("[wifi] SSID '%s' nao foi encontrado; confira o nome anunciado pelo celular\n", WIFI_SSID);
      }
      informou_falha = true;
    }
  }

  if (millis() - inicio_tentativa > WIFI_TIMEOUT_MS + WIFI_RETENTAR_MS) {
    web_socket.disconnect();
    websocket_iniciado = false;
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_SENHA);
    Serial.println("[wifi] nova tentativa");
    status_atual = STATUS_CONECTANDO;
    inicio_tentativa = millis();
  }
#endif
}

StatusMesa mixer_link_status() {
  return status_atual;
}

void mixer_link_set_group_mute(int grupo_id, bool mutado) {
  if (grupo_id < 0 || grupo_id >= GRP_COUNT || !websocket_iniciado || !mgmask_valido) return;
  if (mutado) mgmask_atual |= (1UL << grupo_id);
  else mgmask_atual &= ~(1UL << grupo_id);

  String comando = "3:::SETD^mgmask^" + String(mgmask_atual);
  web_socket.sendTXT(comando);
  Serial.printf("[mesa] enviando mgmask=%lu\n", (unsigned long)mgmask_atual);
}

void mixer_link_set_channel_unmute(uint8_t numero_canal, bool ativo) {
  if (!websocket_iniciado || status_atual != STATUS_CONECTADO) return;

  String comando = "3:::SETD^i." + String(numero_canal-1) + ".forceunmute^" +
                   String(ativo ? 1 : 0);
  web_socket.sendTXT(comando);
  String comando2 = "3:::SETD^i." + String(numero_canal-1) + ".mute^" +
                   String(ativo ? 0 : 1);
  web_socket.sendTXT(comando2);
  Serial.printf("[mesa] enviando canal=%u forceunmute=%d\n",
                numero_canal, ativo ? 1 : 0);
}
