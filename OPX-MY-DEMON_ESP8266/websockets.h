#ifndef WEBSOCKETS_H
#define WEBSOCKETS_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include "config.h"
#include "attacks.h"

extern unsigned long startTime;

static AsyncWebSocket ws("/ws");

static void broadcastStatus() {
  if (ws.count() == 0) return;
  String json = getAttackStatusJSON();
  json.remove(json.length() - 1);
  json += ",\"heap\":" + String(ESP.getFreeHeap());
  json += ",\"heapMin\":" + String(heapLowMark == 0xFFFFFFFF ? ESP.getFreeHeap() : heapLowMark);
  json += ",\"uptime\":" + String((millis() - startTime) / 1000);
  json += "}";
  ws.textAll(json);
}

static void broadcastLog(const String& msg, uint8_t level) {
  if (ws.count() == 0) return;
  String json = "{\"type\":\"log\",\"t\":" + String(millis() / 1000);
  json += ",\"l\":" + String(level);
  json += ",\"m\":\"" + escapeJSON(msg) + "\"}";
  ws.textAll(json);
}

static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    broadcastStatus();
  }
}

static void initWebSockets(AsyncWebServer* srv) {
  ws.onEvent(onWsEvent);
  srv->addHandler(&ws);
}

static void wsLoop() {
  ws.cleanupClients();
}

#endif
