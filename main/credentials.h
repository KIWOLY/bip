/*

Credentials file

*/

#pragma once

// Only one of these settings must be defined
#define USE_ABP
// #define USE_OTAA

#ifdef USE_ABP

// LoRaWAN NwkSKey, network session key
static const u1_t PROGMEM NWKSKEY[16] = {0x78, 0xB5, 0xCA, 0x1C, 0x20, 0x83, 0x97, 0x0F, 0x45, 0x15, 0x9E, 0xA2, 0x13, 0xA0, 0x12, 0x3B};
// LoRaWAN AppSKey, application session key
static const u1_t PROGMEM APPSKEY[16] = {0x6F, 0xD3, 0xC4, 0x2F, 0x98, 0x5D, 0xEB, 0x4D, 0x5A, 0xD5, 0x18, 0xC8, 0x82, 0x1E, 0x03, 0x97};
// LoRaWAN end-device address (DevAddr)
// This has to be unique for every node
static const u4_t DEVADDR = 0x260B305B;

#endif

#ifdef USE_OTAA

// This EUI must be in little-endian format, so least-significant-byte (lsb)
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0x00, 0x00,
// 0x00.
static const u1_t PROGMEM APPEUI[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// This should also be in little endian format (lsb), see above.
// Note: You do not need to set this field, if unset it will be generated automatically based on the device macaddr
static u1_t DEVEUI[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// This key should be in big endian format (msb) (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
// The key shown here is the semtech default key.
static const u1_t PROGMEM APPKEY[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#endif
