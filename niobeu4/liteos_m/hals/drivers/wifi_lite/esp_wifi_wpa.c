/*
 * Copyright (c) 2022 Hunan OpenValley Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>

struct wpa_funcs {
    bool (*wpa_sta_init)(void);
    bool (*wpa_sta_deinit)(void);
    void (*wpa_sta_connect)(uint8_t *bssid);
    void (*wpa_sta_disconnected_cb)(uint8_t reason_code);
    int (*wpa_sta_rx_eapol)(uint8_t *src_addr, uint8_t *buf, uint32_t len);
    bool (*wpa_sta_in_4way_handshake)(void);
    void *(*wpa_ap_init)(void);
    bool (*wpa_ap_deinit)(char *data);
    bool (*wpa_ap_join)(char **sm, uint8_t *bssid, uint8_t *wpa_ie, uint8_t wpa_ie_len);
    bool (*wpa_ap_remove)(char *sm);
    uint8_t *(*wpa_ap_get_wpa_ie)(uint8_t *len);
    bool (*wpa_ap_rx_eapol)(char *hapd_data, char *sm, uint8_t *data, size_t data_len);
    void (*wpa_ap_get_peer_spp_msg)(char *sm, bool *spp_cap, bool *spp_req);
    char *(*wpa_config_parse_string)(const char *value, size_t *len);
    int (*wpa_parse_wpa_ie)(const uint8_t *wpa_ie, size_t wpa_ie_len, char *data);
    int (*wpa_config_bss)(uint8_t *bssid);
    int (*wpa_michael_mic_failure)(uint16_t is_unicast);
    uint8_t *(*wpa3_build_sae_msg)(uint8_t *bssid, uint32_t type, size_t *len);
    int (*wpa3_parse_sae_msg)(uint8_t *buf, size_t len, uint32_t type, uint16_t status);
    int (*resfunc)(void);
    void (*wpa_config_done)(void);
};

struct {
    uint32_t size;
    uint32_t version;
    char *func[24];
}const g_wifi_default_wpa_crypto_funcs = {
    .size = sizeof(g_wifi_default_wpa_crypto_funcs),
    .version = 1,
    {NULL}
};

const char *g_wifi_default_mesh_crypto_funcs[2] = { NULL, NULL};
int esp_wifi_register_wpa_cb_internal(struct wpa_funcs *cb);
int esp_wifi_unregister_wpa_cb_internal(void);

static bool wpa_attach(void)
{
    return true;
}

static bool wpa_deattach(void)
{
    return true;
}

static void wpa_sta_connect(uint8_t *bssid)
{
}

static void wpa_sta_disconnected_cb(uint8_t reason_code)
{
}

static int wpa_parse_wpa_ie(const uint8_t *wpa_ie, size_t wpa_ie_len, char *data)
{
    return 0;
}

static void wpa_config_done(void)
{
}

int esp_supplicant_init(void)
{
    struct wpa_funcs *wpa_cb;

    wpa_cb = (struct wpa_funcs *)malloc(sizeof(struct wpa_funcs));
    if (!wpa_cb) {
        return -1;
    }
    (void)memset_s(wpa_cb, sizeof(struct wpa_funcs), 0, sizeof(struct wpa_funcs));
    wpa_cb->wpa_sta_init       = wpa_attach;
    wpa_cb->wpa_sta_deinit     = wpa_deattach;
    wpa_cb->wpa_sta_connect    = wpa_sta_connect;
    wpa_cb->wpa_sta_disconnected_cb = wpa_sta_disconnected_cb;

    wpa_cb->wpa_parse_wpa_ie  = wpa_parse_wpa_ie;
    wpa_cb->wpa_config_done = wpa_config_done;
    esp_wifi_register_wpa_cb_internal(wpa_cb);
    return 0;
}

int esp_supplicant_deinit(void)
{
    return esp_wifi_unregister_wpa_cb_internal();
}

int hexstr2bin(const char *hex, unsigned char *buf, size_t len)
{
    unsigned char t = 0;

    for (size_t i = 0; i < len; i++) {
        unsigned char c = hex[i];
        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (c >= 'a' && c <= 'f') {
            c -= 'W';
        } else if (c >= 'A' && c <= 'F') {
            c -= '7';
        } else {
            return -1;
        }
        if (i & 1) {
            *buf++ = (t << '\x4') | c;
        } else {
            t = c;
        }
    }
    return 0;
}
