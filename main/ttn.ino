#include <hal/hal.h>
#include <SPI.h>
#include <vector>
#include <Preferences.h>
#include "configuration.h"
#include "credentials.h"

const lmic_pinmap lmic_pins = {
    .nss = NSS_GPIO,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = RESET_GPIO,
    .dio = {DIO0_GPIO, DIO1_GPIO, DIO2_GPIO},
};

static RTC_DATA_ATTR uint32_t count = 0;

#ifdef USE_ABP
void os_getArtEui(u1_t *buf) {}
void os_getDevEui(u1_t *buf) {}
void os_getDevKey(u1_t *buf) {}
#endif

std::vector<void (*)(uint8_t message)> _lmic_callbacks;

void _ttn_callback(uint8_t message) {
    for (uint8_t i = 0; i < _lmic_callbacks.size(); i++) {
        (_lmic_callbacks[i])(message);
    }
}

void forceTxSingleChannelDr() {
#ifdef SINGLE_CHANNEL_GATEWAY
    for (int i = 0; i < 9; i++) {
        if (i != SINGLE_CHANNEL_GATEWAY) {
            LMIC_disableChannel(i);
        }
    }
#endif
    ttn_sf(LORAWAN_SF);
}

static void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16) DEBUG_PORT.print('0');
    DEBUG_PORT.print(v, HEX);
}

bool ttn_setup() {
    Preferences p;
    p.begin("loratest", false);
    count = p.getUInt("counttest", 0);
    p.end();

    SPI.begin(SCK_GPIO, MISO_GPIO, MOSI_GPIO, NSS_GPIO);
    bool initSuccess = (1 == os_init_ex((const void *)&lmic_pins));
    DEBUG_PORT.println(initSuccess ? "Init Success" : "Init Fail");
    return initSuccess;
}

void ttn_join() {
    LMIC_reset();
#ifdef CLOCK_ERROR
    LMIC_setClockError(MAX_CLOCK_ERROR * CLOCK_ERROR / 100);
#endif
#if defined(CFG_eu868)
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK, DR_FSK), BAND_MILLI);
#endif
    LMIC_setLinkCheckMode(0);
#ifdef SINGLE_CHANNEL_GATEWAY
    forceTxSingleChannelDr();
#else
    ttn_sf(LORAWAN_SF);
#endif
#if defined(USE_ABP)
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession(0x1, DEVADDR, nwkskey, appskey);
    LMIC.dn2Dr = DR_SF9;
    _ttn_callback(EV_JOINED);
#endif
}

void ttn_sf(unsigned char sf) {
    LMIC_setDrTxpow(sf, 14);
}

void ttn_adr(bool enabled) {
    LMIC_setAdrMode(enabled);
    LMIC_setLinkCheckMode(enabled);
}

uint32_t ttn_get_count() {
    return count;
}

static void ttn_set_cnt() {
    LMIC_setSeqnoUp(count);
    static uint32_t lastWriteMsec = UINT32_MAX;
    uint32_t now = millis();
    if (now < lastWriteMsec || (now - lastWriteMsec) > 5 * 60 * 1000L) {
        lastWriteMsec = now;
        Preferences p;
        if (p.begin("loratest", false)) {
            p.putUInt("counttest", count);
            p.end();
        }
    }
}

void ttn_send(uint8_t *data, uint8_t data_size, uint8_t port, bool confirmed) {
    ttn_set_cnt();
    if (LMIC.opmode & OP_TXRXPEND) {
        _ttn_callback(EV_PENDING);
        return;
    }
    LMIC_setTxData2(port, data, data_size, confirmed ? 1 : 0);
    _ttn_callback(EV_QUEUED);
    count++;
}

void ttn_loop() {
    os_runloop_once();
}

void ttn_register(void (*callback)(uint8_t message)) {
    _lmic_callbacks.push_back(callback);
}

size_t ttn_response_len() {
    return LMIC.dataLen;
}

void ttn_response(uint8_t *buffer, size_t len) {
    for (uint8_t i = 0; i < LMIC.dataLen; i++) {
        buffer[i] = LMIC.frame[LMIC.dataBeg + i];
    }
}

void ttn_erase_prefs() {
    Preferences p;
    if (p.begin("loratest", false)) {
        p.clear();
        p.end();
    }
}

void onEvent(ev_t event) {
    switch (event) {
    case EV_JOINED: {
#ifdef SINGLE_CHANNEL_GATEWAY
        forceTxSingleChannelDr();
#endif
        if (!LORAWAN_ADR) {
            LMIC_setLinkCheckMode(0);
        }
        DEBUG_PORT.println(F("EV_JOINED"));
        u4_t netid = 0;
        devaddr_t devaddr = 0;
        u1_t nwkKey[16];
        u1_t artKey[16];
        LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
        DEBUG_PORT.print("netid: ");
        DEBUG_PORT.println(netid, DEC);
        DEBUG_PORT.print("devaddr: ");
        DEBUG_PORT.println(devaddr, HEX);
        DEBUG_PORT.print("AppSKey: ");
        for (size_t i = 0; i < sizeof(artKey); ++i) {
            if (i != 0) DEBUG_PORT.print("-");
            printHex2(artKey[i]);
        }
        DEBUG_PORT.println("");
        DEBUG_PORT.print("NwkSKey: ");
        for (size_t i = 0; i < sizeof(nwkKey); ++i) {
            if (i != 0) DEBUG_PORT.print("-");
            printHex2(nwkKey[i]);
        }
        DEBUG_PORT.println();
        Preferences p;
        if (p.begin("lora", false)) {
            p.putUInt("netId", netid);
            p.putUInt("devAddr", devaddr);
            p.putBytes("nwkKey", nwkKey, sizeof(nwkKey));
            p.putBytes("artKey", artKey, sizeof(artKey));
            p.end();
        }
        break;
    }
    case EV_TXCOMPLETE:
        DEBUG_PORT.println(F("EV_TXCOMPLETE (inc. RX win. wait)"));
        if (LMIC.txrxFlags & TXRX_ACK) {
            DEBUG_PORT.println(F("Received ack"));
            _ttn_callback(EV_ACK);
        }
        if (LMIC.dataLen) {
            DEBUG_PORT.print(F("Data Received: "));
            DEBUG_PORT.write(LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
            DEBUG_PORT.println();
            _ttn_callback(EV_RESPONSE);
        }
        break;
    default:
        break;
    }
    _ttn_callback(event);
}