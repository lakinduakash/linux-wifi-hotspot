//
// Created by lakinduakash on 13/04/19.
//

#ifndef WIHOTSPOT_H_PROP_H
#define WIHOTSPOT_H_PROP_H


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

#define CONFIG_FILE "wh.config"

static int parse_output(char *);

char *build_command(char *, char *, char *, char *);

int startShell(char *);

int create_config(char *);

int init_read_wh_config(void);


void set_ssid(const char* ssid);
const char* get_ssid(void);

void set_pass(const char* pass);
const char* get_pass(void);

#endif //WIHOTSPOT_H_PROP_H
