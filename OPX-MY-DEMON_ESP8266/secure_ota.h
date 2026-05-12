#ifndef SECURE_OTA_H
#define SECURE_OTA_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <bearssl/bearssl.h>
#include <LittleFS.h>
#include "config.h"

#define OTA_SIGNATURE_SIZE 32
#define OTA_HASH_SIZE 32
#define OTA_CHUNK_SIZE 4096

static uint8_t otaDeviceSecret[32] = {0};
static bool otaSecretInitialized = false;

static void initOTASecret() {
  if (otaSecretInitialized) return;
  uint32_t chipId = ESP.getChipId();
  uint8_t mac[6];
  WiFi.macAddress(mac);
  for (int i = 0; i < 32; i++) {
    otaDeviceSecret[i] = (uint8_t)(chipId >> ((i * 11) % 32)) ^ mac[i % 6] ^ (uint8_t)(0xDE + i * 0xAD);
  }
  otaSecretInitialized = true;
}

static void computeHMACSHA256(const uint8_t* data, size_t len, const uint8_t* key, size_t keyLen, uint8_t* hmacOut) {
  uint8_t inner[64], outer[64];
  memset(inner, 0, 64);
  memset(outer, 0, 64);
  for (size_t i = 0; i < 64; i++) {
    inner[i] = (i < keyLen) ? key[i] ^ 0x36 : 0x36;
    outer[i] = (i < keyLen) ? key[i] ^ 0x5C : 0x5C;
  }
  br_sha256_context ctx;
  uint8_t innerHash[32];
  br_sha256_init(&ctx);
  br_sha256_update(&ctx, inner, 64);
  br_sha256_update(&ctx, data, len);
  br_sha256_out(&ctx, innerHash);
  br_sha256_init(&ctx);
  br_sha256_update(&ctx, outer, 64);
  br_sha256_update(&ctx, innerHash, 32);
  br_sha256_out(&ctx, hmacOut);
}

static bool isValidHexChar(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static uint8_t hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return 0;
}

#endif
