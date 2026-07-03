//
// Created by lakinduakash on 13/04/19.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>
//#include <libconfig.h>

#include "h_prop.h"
#include "read_config.h"
#include "qrgen.h"


#define BUFSIZE 2048


#define SUDO "pkexec --user root"
#define CREATE_AP "create_ap"

#define MKCONFIG "--mkconfig"
#define LOAD_CONFIG "--config"
#define STOP "--stop"
#define SHOW_INFO "--show-info"


static char cmd_start[BUFSIZE];
static char cmd_mkconfig[BUFSIZE];
static char cmd_config[BUFSIZE];
static char cmd_kill[BUFSIZE];
static char cmd_write_mac[BUFSIZE];

static char h_running_info[BUFSIZE];
static char interface_list[BUFSIZE];
static char wifi_interface_list[BUFSIZE];
static char accepted_macs[BUFSIZE];

static const char* g_ssid=NULL;
static const char* g_pass=NULL;

//config_t cfg;

static char create_ap_invocation[64];

static const char *get_create_ap_invocation(void) {
    if (geteuid() == 0) {
        snprintf(create_ap_invocation, sizeof(create_ap_invocation), "%s", CREATE_AP);
    } else {
        snprintf(create_ap_invocation, sizeof(create_ap_invocation), "%s %s", SUDO, CREATE_AP);
    }
    return create_ap_invocation;
}

static int parse_output(const char *cmd) {

    char buf[BUFSIZE];
    FILE *fp;

    if ((fp = popen(cmd, "r")) == NULL) {
        printf("Error opening pipe!\n");
        return -1;
    }

    while (fgets(buf, BUFSIZE, fp) != NULL) {
        // Do whatever you want here...
        printf("%s", buf);
    }

    if (pclose(fp)) {
        printf("Command not found or exited with error status\n");
        return -1;
    }

    return 0;
}


const char *build_wh_start_command(char *iface_src, char *iface_dest, char *ssid, char *pass) {

    snprintf(cmd_start, BUFSIZE, "%s %s %s %s %s", get_create_ap_invocation(),
             iface_src, iface_dest, ssid, pass);

    return cmd_start;
}


const char *build_wh_mkconfig_command(ConfigValues* cv){

    const char* config_ffile_name=get_config_file(CONFIG_FILE_NAME);

    snprintf(cmd_mkconfig, BUFSIZE, "%s %s %s '%s' '%s' %s %s",
             get_create_ap_invocation(), cv->iface_wifi, cv->iface_inet,
             cv->ssid, cv->pass, MKCONFIG, config_ffile_name);

    if(cv->freq!=NULL){
        strcat(cmd_mkconfig," --freq-band ");
        strcat(cmd_mkconfig,cv->freq);
    }

    if(cv->no_virt!=NULL && (strcmp(cv->no_virt,"1") == 0))
        strcat(cmd_mkconfig," --no-virt ");

    if(cv->use_psk!=NULL && (strcmp(cv->use_psk,"1") == 0))
        strcat(cmd_mkconfig," --psk ");

    if(cv->hidden!=NULL && (strcmp(cv->hidden,"1") == 0))
        strcat(cmd_mkconfig," --hidden ");

    if(cv->no_haveged!=NULL && (strcmp(cv->no_haveged,"1") == 0))
        strcat(cmd_mkconfig," --no-haveged ");

    if(cv->channel!=NULL && (strcmp(cv->channel,"default") != 0) && (cv->freq==NULL||(strcmp(cv->freq,"2.4") == 0)|| (strcmp(cv->freq,"5") == 0))){

            strcat(cmd_mkconfig," -c ");
            strcat(cmd_mkconfig,cv->channel);
    }

    if(cv->ieee80211n!=NULL && (strcmp(cv->ieee80211n,"1") == 0)){
        strcat(cmd_mkconfig," --ieee80211n ");
    }
    
    if(cv->ieee80211ac!=NULL && (strcmp(cv->ieee80211ac,"1") == 0)){
        strcat(cmd_mkconfig," --ieee80211ac ");
    }

    if(cv->ieee80211ax!=NULL && (strcmp(cv->ieee80211ax,"1") == 0)){
        strcat(cmd_mkconfig," --ieee80211ax ");
    }

    if(cv->mac!=NULL) {
        strcat(cmd_mkconfig, " --mac ");
        strcat(cmd_mkconfig, cv->mac);
    }

    if(cv->gateway!=NULL) {
        strcat(cmd_mkconfig, " -g ");
        strcat(cmd_mkconfig, cv->gateway);
    }

    if(cv->mac_filter!=NULL && (strcmp(cv->mac_filter,"1") == 0)){
        strcat(cmd_mkconfig, " --mac-filter ");
        strcat(cmd_mkconfig, cv->mac_filter);
        write_accepted_macs(cv->accepted_mac_file,cv->accepted_macs);
    }

    printf("%s \n",cmd_mkconfig);
    return cmd_mkconfig;

}

const char *build_wh_from_config(){

    snprintf(cmd_config, BUFSIZE, "%s %s %s", get_create_ap_invocation(),
             LOAD_CONFIG, get_config_file(CONFIG_FILE_NAME));
    return cmd_config;

}

int startShell(const char *cmd) {
    return parse_output(cmd);
}


const char* build_kill_create_ap_command(char* id){
    snprintf(cmd_kill, BUFSIZE, "%s %s %s", get_create_ap_invocation(), STOP, id);
    return cmd_kill;
}

void write_accepted_macs(char* filename, char* accepted_macs){

    printf("mac filter file %s \n",filename);

    if (geteuid() == 0)
        snprintf(cmd_write_mac, BUFSIZE, "echo '%s' | tee %s", accepted_macs, filename);
    else
        snprintf(cmd_write_mac, BUFSIZE, "%s '%s' %s %s", "echo", accepted_macs, "| pkexec -u root tee", filename);
    int r=system(cmd_write_mac);

}

char * read_mac_filter_file(char * filename){

    char ch;
    FILE *fp;

    fp = fopen(filename, "r"); // read mode

    if (fp == NULL)
    {
        return NULL;
    }

    while((ch = (char)fgetc(fp)) != EOF)
        strcat(accepted_macs, &ch);

   fclose(fp);
   return accepted_macs;
}

//int write_config(char* file){
//
//    config_t cfg;
//    config_setting_t *root, *setting, *group, *array;
//    int i;
//
//    config_init(&cfg);
//    root = config_root_setting(&cfg);
//
//    /* Add some settings to the configuration. */
//
//
//    setting = config_setting_add(root, SSID, CONFIG_TYPE_STRING);
//    config_setting_set_string(setting, "myssid");
//
//    setting = config_setting_add(root, PASSPHRASE, CONFIG_TYPE_STRING);
//    config_setting_set_string(setting, "123456789");
//
//    /* Write out the new configuration. */
//    if(! config_write_file(&cfg, get_config_file(CONFIG_FILE_NAME)))
//    {
//        fprintf(stderr, "Error while writing file.\n");
//        config_destroy(&cfg);
//        return(EXIT_FAILURE);
//    }
//
//    fprintf(stderr, "New configuration successfully written to: %s\n",
//            get_config_file(CONFIG_FILE_NAME));
//
//    config_destroy(&cfg);
//    return(EXIT_SUCCESS);
//}


static int init_get_running(){

    char cmd[BUFSIZE];
    snprintf(cmd, BUFSIZE, "%s --list-running", get_create_ap_invocation());

    FILE *fp;

    if ((fp = popen(cmd, "r")) == NULL) {
        printf("Error opening pipe!\n");
        return -1;
    }

    h_running_info[0] = '\0';

    while (fgets(h_running_info + strlen(h_running_info),
                 BUFSIZE - strlen(h_running_info), fp) != NULL) {
        if (strlen(h_running_info) >= BUFSIZE - 1)
            break;
    }

    if (pclose(fp)) {
        if (h_running_info[0] == '\0') {
            printf("Command not found or exited with error status\n");
            return -1;
        }
    }

    return 0;

}

static void parse_running_line(const char *line, RunningHotspot *hotspot) {
    char buffer[BUFSIZE];
    char *open_paren;
    char *close_paren;

    memset(hotspot, 0, sizeof(*hotspot));
    strncpy(buffer, line, BUFSIZE - 1);
    buffer[BUFSIZE - 1] = '\0';
    buffer[strcspn(buffer, "\n")] = 0;

    open_paren = strchr(buffer, '(');
    if (open_paren != NULL) {
        close_paren = strchr(open_paren, ')');
        if (close_paren != NULL) {
            *close_paren = '\0';
            snprintf(hotspot->ap_iface, sizeof(hotspot->ap_iface), "%s", open_paren + 1);
        }
        *open_paren = '\0';
    }

    if (sscanf(buffer, "%31s %31s", hotspot->pid, hotspot->phy_iface) < 2) {
        hotspot->pid[0] = '\0';
        return;
    }

    if (hotspot->ap_iface[0] == '\0')
        snprintf(hotspot->ap_iface, sizeof(hotspot->ap_iface), "%s", hotspot->phy_iface);
}

int get_running_hotspots(RunningHotspot **hotspots, int *count) {
    char *line;
    char *saveptr;
    char buffer[BUFSIZE];
    int capacity = 4;
    int n = 0;
    RunningHotspot *list;

    *hotspots = NULL;
    *count = 0;

    if (init_get_running() != 0 || h_running_info[0] == '\0')
        return -1;

    list = malloc(capacity * sizeof(RunningHotspot));
    if (list == NULL)
        return -1;

    strncpy(buffer, h_running_info, BUFSIZE - 1);
    buffer[BUFSIZE - 1] = '\0';

    line = strtok_r(buffer, "\n", &saveptr);
    while (line != NULL) {
        while (*line == ' ' || *line == '\t')
            line++;

        if (*line != '\0') {
            if (n >= capacity) {
                capacity *= 2;
                RunningHotspot *grown = realloc(list, capacity * sizeof(RunningHotspot));
                if (grown == NULL) {
                    free(list);
                    return -1;
                }
                list = grown;
            }
            parse_running_line(line, &list[n]);
            if (list[n].pid[0] != '\0')
                n++;
        }
        line = strtok_r(NULL, "\n", &saveptr);
    }

    if (n == 0) {
        free(list);
        return 0;
    }

    *hotspots = list;
    *count = n;
    return 0;
}

void free_running_hotspots(RunningHotspot *hotspots) {
    free(hotspots);
}

static int is_live_pid_str(const char *pid_str) {
    char path[64];

    if (pid_str == NULL || pid_str[0] == '\0')
        return 0;

    snprintf(path, sizeof(path), "/proc/%s", pid_str);
    return access(path, F_OK) == 0;
}

static int read_text_file(const char *path, char *buf, size_t size) {
    FILE *fp;

    if (size == 0)
        return -1;

    buf[0] = '\0';
    fp = fopen(path, "r");
    if (fp == NULL)
        return -1;

    if (fgets(buf, (int) size, fp) == NULL) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    buf[strcspn(buf, "\n")] = '\0';
    return 0;
}

static int find_running_confdir(const char *id, char *confdir_out, size_t confdir_size) {
    glob_t g;
    size_t i;
    char pid[32];

    if (id == NULL || id[0] == '\0' || confdir_out == NULL || confdir_size == 0)
        return -1;

    if (glob("/tmp/create_ap.*", GLOB_ONLYDIR, NULL, &g) != 0)
        return -1;

    for (i = 0; i < g.gl_pathc; i++) {
        char pid_path[BUFSIZE];

        snprintf(pid_path, sizeof(pid_path), "%s/pid", g.gl_pathv[i]);
        if (read_text_file(pid_path, pid, sizeof(pid)) != 0)
            continue;
        if (!is_live_pid_str(pid))
            continue;

        if (strcmp(id, pid) == 0) {
            snprintf(confdir_out, confdir_size, "%s", g.gl_pathv[i]);
            globfree(&g);
            return 0;
        }

        if (strncmp(g.gl_pathv[i], "/tmp/create_ap.", 15) == 0) {
            const char *p = g.gl_pathv[i] + 15;
            const char *dot = strstr(p, ".conf.");
            if (dot != NULL) {
                char iface[32];
                size_t len = (size_t) (dot - p);

                if (len > 0 && len < sizeof(iface)) {
                    memcpy(iface, p, len);
                    iface[len] = '\0';
                    if (strcmp(id, iface) == 0) {
                        snprintf(confdir_out, confdir_size, "%s", g.gl_pathv[i]);
                        globfree(&g);
                        return 0;
                    }
                }
            }
        }
    }

    globfree(&g);
    return -1;
}

static void set_key_value(const char *line, const char *key, char *dest, size_t dest_size) {
    size_t key_len;

    if (dest == NULL || dest_size == 0 || line == NULL || key == NULL)
        return;

    key_len = strlen(key);
    if (strncmp(line, key, key_len) == 0 && line[key_len] == '=')
        snprintf(dest, dest_size, "%s", line + key_len + 1);
}

static void set_encryption_label(HotspotDetails *details, const char *wpa,
                                 const char *key_mgmt, const char *passphrase,
                                 const char *psk) {
    if ((passphrase == NULL || passphrase[0] == '\0')
        && (psk == NULL || psk[0] == '\0')) {
        snprintf(details->encryption, sizeof(details->encryption), "Open");
        return;
    }

    if (key_mgmt != NULL && strstr(key_mgmt, "SAE") != NULL) {
        snprintf(details->encryption, sizeof(details->encryption), "WPA2/WPA3");
        return;
    }

    if (wpa != NULL && strcmp(wpa, "3") == 0) {
        snprintf(details->encryption, sizeof(details->encryption), "WPA3");
        return;
    }

    if (wpa != NULL && strcmp(wpa, "2") == 0) {
        snprintf(details->encryption, sizeof(details->encryption), "WPA2-PSK");
        return;
    }

    if (wpa != NULL && strcmp(wpa, "1") == 0) {
        snprintf(details->encryption, sizeof(details->encryption), "WPA-PSK");
        return;
    }

    if (psk != NULL && psk[0] != '\0')
        snprintf(details->encryption, sizeof(details->encryption), "WPA-PSK (hex)");
    else
        snprintf(details->encryption, sizeof(details->encryption), "WPA-PSK");
}

static void parse_hostapd_conf(const char *path, HotspotDetails *details) {
    FILE *fp;
    char line[BUFSIZE];
    char wpa[8] = "";
    char key_mgmt[64] = "";
    char passphrase[128] = "";
    char psk[128] = "";
    char hw_mode[8] = "";
    char hidden[8] = "";

    fp = fopen(path, "r");
    if (fp == NULL)
        return;

    while (fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        set_key_value(line, "ssid", details->ssid, sizeof(details->ssid));
        set_key_value(line, "interface", details->ap_iface, sizeof(details->ap_iface));
        set_key_value(line, "channel", details->channel, sizeof(details->channel));
        set_key_value(line, "wpa", wpa, sizeof(wpa));
        set_key_value(line, "wpa_key_mgmt", key_mgmt, sizeof(key_mgmt));
        set_key_value(line, "wpa_passphrase", passphrase, sizeof(passphrase));
        set_key_value(line, "wpa_psk", psk, sizeof(psk));
        set_key_value(line, "hw_mode", hw_mode, sizeof(hw_mode));
        set_key_value(line, "ignore_broadcast_ssid", hidden, sizeof(hidden));
    }

    fclose(fp);

    if (passphrase[0] != '\0')
        snprintf(details->passphrase, sizeof(details->passphrase), "%s", passphrase);
    else if (psk[0] != '\0')
        snprintf(details->passphrase, sizeof(details->passphrase), "%s", psk);

    if (strcmp(hw_mode, "a") == 0)
        snprintf(details->band, sizeof(details->band), "5 GHz");
    else if (strcmp(hw_mode, "g") == 0)
        snprintf(details->band, sizeof(details->band), "2.4 GHz");
    else
        snprintf(details->band, sizeof(details->band), "Unknown");

    if (strcmp(hidden, "1") == 0)
        snprintf(details->hidden, sizeof(details->hidden), "Yes");
    else
        snprintf(details->hidden, sizeof(details->hidden), "No");

    set_encryption_label(details, wpa, key_mgmt, passphrase, psk);
}

static void parse_dnsmasq_conf(const char *path, HotspotDetails *details) {
    FILE *fp;
    char line[BUFSIZE];

    fp = fopen(path, "r");
    if (fp == NULL)
        return;

    while (fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        set_key_value(line, "listen-address", details->gateway, sizeof(details->gateway));
    }

    fclose(fp);
}

static int join_conf_path(char *dest, size_t dest_size, const char *confdir, const char *suffix) {
    size_t dir_len;
    size_t suffix_len;

    if (dest == NULL || confdir == NULL || suffix == NULL || dest_size == 0)
        return -1;

    dir_len = strlen(confdir);
    suffix_len = strlen(suffix);
    if (dir_len + suffix_len + 1 > dest_size)
        return -1;

    memcpy(dest, confdir, dir_len);
    memcpy(dest + dir_len, suffix, suffix_len + 1);
    return 0;
}

static void apply_show_info_line(const char *line, HotspotDetails *details) {
    const char *value;

    if (line == NULL || details == NULL)
        return;

    value = strchr(line, '=');
    if (value == NULL)
        return;
    value++;

    if (strncmp(line, "PID=", 4) == 0)
        snprintf(details->pid, sizeof(details->pid), "%s", value);
    else if (strncmp(line, "PHY_IFACE=", 10) == 0)
        snprintf(details->phy_iface, sizeof(details->phy_iface), "%s", value);
    else if (strncmp(line, "AP_IFACE=", 9) == 0)
        snprintf(details->ap_iface, sizeof(details->ap_iface), "%s", value);
    else if (strncmp(line, "INTERNET_IFACE=", 15) == 0)
        snprintf(details->internet_iface, sizeof(details->internet_iface), "%s", value);
    else if (strncmp(line, "SSID=", 5) == 0)
        snprintf(details->ssid, sizeof(details->ssid), "%s", value);
    else if (strncmp(line, "PASSPHRASE=", 11) == 0)
        snprintf(details->passphrase, sizeof(details->passphrase), "%s", value);
    else if (strncmp(line, "ENCRYPTION=", 11) == 0)
        snprintf(details->encryption, sizeof(details->encryption), "%s", value);
    else if (strncmp(line, "GATEWAY=", 8) == 0)
        snprintf(details->gateway, sizeof(details->gateway), "%s", value);
    else if (strncmp(line, "CHANNEL=", 8) == 0)
        snprintf(details->channel, sizeof(details->channel), "%s", value);
    else if (strncmp(line, "BAND=", 5) == 0)
        snprintf(details->band, sizeof(details->band), "%s", value);
    else if (strncmp(line, "HIDDEN=", 7) == 0)
        snprintf(details->hidden, sizeof(details->hidden), "%s", value);
}

static int fetch_hotspot_details_via_create_ap(const char *id, HotspotDetails *details) {
    char cmd[BUFSIZE];
    char line[BUFSIZE];
    FILE *fp;

    snprintf(cmd, BUFSIZE, "%s %s %s", get_create_ap_invocation(), SHOW_INFO, id);

    fp = popen(cmd, "r");
    if (fp == NULL)
        return -1;

    while (fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\n")] = 0;
        apply_show_info_line(line, details);
    }

    pclose(fp);
    return 0;
}

static void fill_details_from_saved_config(HotspotDetails *details) {
    ConfigValues *cv;

    if (read_config_file() != READ_CONFIG_FILE_SUCCESS)
        return;

    cv = getConfigValues();
    if (cv == NULL)
        return;

    if (details->ssid[0] == '\0' && cv->ssid != NULL && cv->ssid[0] != '\0')
        snprintf(details->ssid, sizeof(details->ssid), "%s", cv->ssid);

    if ((details->passphrase[0] == '\0' || strcmp(details->passphrase, "(none)") == 0)
        && cv->pass != NULL && cv->pass[0] != '\0')
        snprintf(details->passphrase, sizeof(details->passphrase), "%s", cv->pass);

    if (details->gateway[0] == '\0' && cv->gateway != NULL && cv->gateway[0] != '\0')
        snprintf(details->gateway, sizeof(details->gateway), "%s", cv->gateway);

    if (details->channel[0] == '\0' && cv->channel != NULL && cv->channel[0] != '\0')
        snprintf(details->channel, sizeof(details->channel), "%s", cv->channel);

    if (details->band[0] == '\0' && cv->freq != NULL) {
        if (strcmp(cv->freq, "5") == 0)
            snprintf(details->band, sizeof(details->band), "5 GHz");
        else if (strcmp(cv->freq, "2.4") == 0)
            snprintf(details->band, sizeof(details->band), "2.4 GHz");
    }

    if (details->encryption[0] == '\0') {
        if (cv->pass != NULL && cv->pass[0] != '\0')
            snprintf(details->encryption, sizeof(details->encryption), "WPA2-PSK");
        else
            snprintf(details->encryption, sizeof(details->encryption), "Open");
    }
}

static void finalize_hotspot_details(HotspotDetails *details) {
    if (details->hidden[0] == '\0')
        snprintf(details->hidden, sizeof(details->hidden), "No");

    if (details->encryption[0] == '\0') {
        if (details->passphrase[0] != '\0' && strcmp(details->passphrase, "(none)") != 0)
            snprintf(details->encryption, sizeof(details->encryption), "WPA2-PSK");
        else
            snprintf(details->encryption, sizeof(details->encryption), "Open");
    }

    if (details->passphrase[0] == '\0')
        snprintf(details->passphrase, sizeof(details->passphrase), "(none)");

    if (details->internet_iface[0] == '\0')
        snprintf(details->internet_iface, sizeof(details->internet_iface), "none");
}

int get_hotspot_details(const char *id, HotspotDetails *details) {
    char confdir[512];
    char path[512];

    if (details == NULL)
        return -1;

    memset(details, 0, sizeof(*details));

    if (find_running_confdir(id, confdir, sizeof(confdir)) != 0)
        return -1;

    if (join_conf_path(path, sizeof(path), confdir, "/pid") != 0)
        return -1;
    read_text_file(path, details->pid, sizeof(details->pid));

    if (strncmp(confdir, "/tmp/create_ap.", 15) == 0) {
        const char *p = confdir + 15;
        const char *dot = strstr(p, ".conf.");
        if (dot != NULL) {
            size_t len = (size_t) (dot - p);
            if (len > 0 && len < sizeof(details->phy_iface)) {
                memcpy(details->phy_iface, p, len);
                details->phy_iface[len] = '\0';
            }
        }
    }

    if (join_conf_path(path, sizeof(path), confdir, "/wifi_iface") != 0)
        return -1;
    read_text_file(path, details->ap_iface, sizeof(details->ap_iface));

    if (join_conf_path(path, sizeof(path), confdir, "/nat_internet_iface") != 0)
        return -1;
    if (read_text_file(path, details->internet_iface, sizeof(details->internet_iface)) != 0)
        snprintf(details->internet_iface, sizeof(details->internet_iface), "none");

    if (join_conf_path(path, sizeof(path), confdir, "/hostapd.conf") != 0)
        return -1;
    parse_hostapd_conf(path, details);

    if (join_conf_path(path, sizeof(path), confdir, "/dnsmasq.conf") != 0)
        return -1;
    parse_dnsmasq_conf(path, details);

    if (details->ssid[0] == '\0' || details->encryption[0] == '\0')
        fetch_hotspot_details_via_create_ap(id, details);

    if (details->ssid[0] == '\0')
        fill_details_from_saved_config(details);

    finalize_hotspot_details(details);
    return 0;
}


// Ex:
// char *a[3];
//get_h_running_info(a);
//printf("%s",a[0]);

int get_h_running_info(char* a[3]){

    if(init_get_running()==0){
        char * pch;
        pch = strtok (h_running_info," ");
        int i=0;
        while (pch != NULL && i<3)
        {
            a[i] = strdup(pch);
            pch = strtok (NULL, " ");
            i++;
        }

        return 0;
    }

    return 1;
}


static int init_get_interface_list(){
    const char* cmd="echo $( ls /sys/class/net ) ";

    FILE *fp;
    char temp_buff[1024];

    if ((fp = popen(cmd, "r")) == NULL) {
        printf("Error opening pipe!\n");
        return -1;
    }

    while (fgets(temp_buff, sizeof(temp_buff), fp) != NULL) {

        strcat(interface_list,temp_buff);
    }

    if (pclose(fp)) {
        printf("Command not found or exited with error status\n");
        return -1;
    }


    return 0;
}

//int i=0;
//char ** a=get_interface_list(&i);
//
//for(int j=0;j<i;j++){
//printf("%s ",a[j]);
//}

char** get_interface_list(int *length){

    if(init_get_interface_list()==0){

        char *a=strdup(interface_list);
        char *b=strdup(interface_list);

        char * pch;
        pch = strtok (a," ");
        int i=0;
        while (pch != NULL)
        {
            pch = strtok (NULL, " ");
            i++;
        }

        static char** arr;
        arr =malloc(i * sizeof(char*));

        free(a);

        pch = strtok (b," ");
        i=0;
        while (pch != NULL)
        {
            arr[i]=strdup(pch);
            pch = strtok (NULL, " \n");
            i++;
        }

        *length= i;

        return arr;

    }

    return NULL;

}


static int init_get_wifi_interface_list(){
    const char* cmd="iw dev | awk '$1==\"Interface\"{print $2}' ";

    FILE *fp;

    char temp_buff[1048];

    if ((fp = popen(cmd, "r")) == NULL) {
        printf("Error opening pipe!\n");
        return -1;
    }


    while (fgets(temp_buff, sizeof(temp_buff), fp) != NULL) {

        strcat(wifi_interface_list,temp_buff);
    }

    if (pclose(fp)) {
        printf("Command not found or exited with error status\n");
        return -1;
    }


    return 0;
}

char** get_wifi_interface_list(int *length){

    if(init_get_wifi_interface_list()==0){

        char *a=strdup(wifi_interface_list);
        char *b=strdup(wifi_interface_list);

        char * pch;
        pch = strtok (a,"\n");
        int i=0;
        while (pch != NULL)
        {
            pch = strtok (NULL, "\n");
            i++;
        }

        static char** arr;
        arr =malloc(i * sizeof(char*));

        free(a);

        pch = strtok (b,"\n");
        i=0;
        while (pch != NULL)
        {

            arr[i]=strdup(pch);
            pch = strtok (NULL, "\n");
            i++;
        }

        *length= i;

        return arr;

    }

    return NULL;

}

static char qr_image_path[128];

char* generate_qr_image(char* ssid,char* type,char *password){
    char cmd[BUFSIZE];
    const char *ssid_safe = ssid ? ssid : "";
    const char *type_safe = type ? type : "WPA";
    const char *pass_safe = password ? password : "";

    snprintf(qr_image_path, sizeof(qr_image_path), "/tmp/wihotspot_qr_%d.png", getuid());
    unlink(qr_image_path);

    if (ssid_safe[0] == '\0')
        return NULL;

    if (strcmp(type_safe, "nopass") == 0)
        snprintf(cmd, BUFSIZE, "WIFI:T:nopass;S:%s;;", ssid_safe);
    else
        snprintf(cmd, BUFSIZE, "WIFI:T:%s;S:%s;P:%s;;", type_safe, ssid_safe, pass_safe);

    if (qr_to_png(cmd, qr_image_path) != 0)
        return NULL;

    return qr_image_path;
}

Node get_connected_devices(char *PID)
{
    char cmd[BUFSIZE];
    snprintf(cmd, BUFSIZE, "%s --list-clients %s", get_create_ap_invocation(), PID);
    FILE *fp;
    Node l = (struct Device *)malloc(sizeof(struct Device));
    Position head = l;
    fp = popen(cmd, "r");
    char line[BUFSIZE];

    int _n = 0; //Device number
    while (fgets(line, BUFSIZE, fp) != NULL)
    {
        if (strstr(line, "MAC") != NULL)
            continue;

        _n++;
        int size = strlen(line);
        int marker[3] = {0};
        int n = 0;             // For marker
        line[size - 1] = '\0'; // Remove "\n"
        for (int i = 0; i < size; i++)
        {
            if (*(line + i) != ' ' && *(line + i + 1) == ' ')
            {
                // End
                *(line + i + 1) = '\0';
                i++;
            }
            if (*(line + i) == ' ' && *(line + i + 1) != ' ')
            {
                // Head
                *(line + i) = '\0';
                marker[++n] = i + 1;
            }
        }
        l = add_device_node(l, _n, line, marker);
    }
    return head;
}

PtrToNode add_device_node(PtrToNode l, int number, char line[BUFSIZE], int marker[3])
{
    Node next = (PtrToNode)malloc(sizeof(struct Device));
    strcpy(next->MAC, line);
    strcpy(next->IP, line + marker[1]);
    strcpy(next->HOSTNAME, line + marker[2]);
    next->Number = number;
    next->Next = NULL;
    l->Next = next;
    return next;
}
