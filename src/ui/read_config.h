//
// Created by lakinduakash on 14/04/19.
//
/*
Copyright (c) 2019, lakinduaksh
        All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
        FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
        CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#ifndef WIHOTSPOT_READ_CONFIG_H
#define WIHOTSPOT_READ_CONFIG_H


#define READ_CONFIG_FILE_SUCCESS 0
#define READ_CONFIG_FILE_FAIL 1



#define CHANNEL          "CHANNEL"
#define GATEWAY          "GATEWAY"
#define WPA_VERSION      "WPA_VERSION"
#define ETC_HOSTS        "ETC_HOSTS"
#define DHCP_DNS         "DHCP_DNS"
#define NO_DNS           "NO_DNS"
#define NO_DNSMASQ       "NO_DNSMASQ"
#define HIDDEN           "HIDDEN"
#define MAC_FILTER       "MAC_FILTER"
#define MAC_FILTER_ACCEP "MAC_FILTER_ACCEPT"
#define ISOLATE_CLIENTS  "ISOLATE_CLIENTS"
#define SHARE_METHOD     "SHARE_METHOD"
#define IEEE80211N       "IEEE80211N"
#define IEEE80211AC      "IEEE80211AC"
#define HT_CAPAB         "HT_CAPAB"
#define VHT_CAPAB        "VHT_CAPAB"
#define DRIVER           "DRIVER"
#define NO_VIRT          "NO_VIRT"
#define COUNTRY          "COUNTRY"
#define FREQ_BAND        "FREQ_BAND"
#define NEW_MACADDR      "NEW_MACADDR"
#define DAEMONIZE        "DAEMONIZE"
#define NO_HAVEGED       "NO_HAVEGED"
#define WIFI_IFACE       "WIFI_IFACE"
#define INTERNET_IFACE   "INTERNET_IFACE"
#define SSID             "SSID"
#define PASSPHRASE       "PASSPHRASE"
#define USE_PSK          "USE_PSK"



#define CONFIG_FILE_NAME "/etc/create_ap.conf"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
    char *ssid;
    char *pass;
    char *iface_wifi;
    char *iface_inet;
    char *no_virt;
    char *use_psk;
    char *channel;
    char *freq;
    char *hidden;
    char *mac;
    char *mac_filter;
    char *accepted_mac_file;
    char *accepted_macs;
} ConfigValues;


int read_config_file();
static void setConfigValues(const char * key, char *value);
ConfigValues* getConfigValues(void);
const char *get_config_file(const char* file);

#ifdef __cplusplus
}
#endif



#endif //WIHOTSPOT_READ_CONFIG_H
