#ifndef SECURE_OTA_H
#define SECURE_OTA_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
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

static void computeSHA256(Stream& data, size_t len, uint8_t* hashOut) {
  SHA256 sha256;
  sha256.begin();
  uint8_t buf[64];
  size_t remaining = len;
  while (remaining > 0) {
    size_t toRead = (remaining > 64) ? 64 : remaining;
    size_t read = data.readBytes(buf, toRead);
    if (read == 0) break;
    sha256.update(buf, read);
    remaining -= read;
  }
  memcpy(hashOut, sha256.finalize(HASH_SIZE), OTA_HASH_SIZE);
}

static void computeHMACSHA256(const uint8_t* data, size_t len, const uint8_t* key, size_t keyLen, uint8_t* hmacOut) {
  uint8_t inner[64], outer[64];
  memset(inner, 0, 64);
  memset(outer, 0, 64);
  for (size_t i = 0; i < 64; i++) {
    inner[i] = (i < keyLen) ? key[i] ^ 0x36 : 0x36;
    outer[i] = (i < keyLen) ? key[i] ^ 0x5C : 0x5C;
  }
  SHA256 sha;
  sha.begin();
  sha.update(inner, 64);
  sha.update(data, len);
  uint8_t innerHash[32];
  memcpy(innerHash, sha.finalize(HASH_SIZE), 32);
  sha.begin();
  sha.update(outer, 64);
  sha.update(innerHash, 32);
  memcpy(hmacOut, sha.finalize(HASH_SIZE), OTA_SIGNATURE_SIZE);
}

static bool verifyOTASignature(const uint8_t* firmwareHash, const uint8_t* signature) {
  initOTASecret();
  uint8_t expectedSig[OTA_SIGNATURE_SIZE];
  computeHMACSHA256(firmwareHash, OTA_HASH_SIZE, otaDeviceSecret, 32, expectedSig);
  return memcmp(expectedSig, signature, OTA_SIGNATURE_SIZE) == 0;
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

static String sanitizeHexString(const String& input) {
  String result;
  result.reserve(input.length());
  for (size_t i = 0; i < input.length(); i++) {
    if (isValidHexChar(input[i])) result += input[i];
  }
  return result;
}

#endif
