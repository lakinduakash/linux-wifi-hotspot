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



#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <pwd.h>
#include <unistd.h>

#include "read_config.h"

#define CONFIG_KEY_COUNT 30
#define STRING_MAX_LENGTH 100
#define BUFSIZE 150

extern "C" {

ConfigValues configValues;

char configs[CONFIG_KEY_COUNT][STRING_MAX_LENGTH];

int read_config_file() {
    // std::ifstream is RAII, i.e. no need to call close
    std::ifstream cFile(get_config_file(CONFIG_FILE_NAME));
    if (cFile.is_open()) {
        std::string line;

        int i=0;
        while (getline(cFile, line)) {
            auto delimiterPos = line.find('=');
            if (!(line.find("SSID") < delimiterPos) && !(line.find("PASSPHRASE") < delimiterPos)) {
                line.erase(std::remove_if(line.begin(), line.end(), isspace),
                           line.end());
            }
            if (line[0] == '#' || line.empty())
                continue;
            delimiterPos = line.find('='); //check again in case it changed
            auto name = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);

            strcpy(configs[i],value.c_str());
            setConfigValues(name.c_str(),configs[i]);
            //std::cout << name << " " << value << '\n';
            ++i;
        }

    } else {
        std::cerr << "Couldn't open config file for reading.\n";
        return READ_CONFIG_FILE_FAIL;
    }

    return READ_CONFIG_FILE_SUCCESS;
}

ConfigValues* getConfigValues(){
    return &configValues;
}


static void setConfigValues(const char * key, char *value){

    if( !strcmp ( SSID, key ))
        configValues.ssid = value;

    if( !strcmp ( PASSPHRASE, key ))
        configValues.pass = value;

    if( !strcmp ( WIFI_IFACE, key ))
        configValues.iface_wifi = value;

    if( !strcmp ( INTERNET_IFACE, key ))
        configValues.iface_inet = value;

    if( !strcmp ( HIDDEN, key ))
        configValues.hidden = value;

    if( !strcmp ( NO_HAVEGED, key ))
        configValues.no_haveged = value;

    if( !strcmp ( NO_VIRT, key ))
        configValues.no_virt = value;

    if( !strcmp ( NEW_MACADDR, key ))
        configValues.mac = value;

    if( !strcmp ( CHANNEL, key ))
        configValues.channel = value;

    if( !strcmp ( FREQ_BAND, key ))
        configValues.freq = value;

    if( !strcmp ( USE_PSK, key ))
        configValues.use_psk = value;

    if( !strcmp ( MAC_FILTER, key ))
        configValues.mac_filter = value;

    if( !strcmp ( MAC_FILTER_ACCEP, key ))
        configValues.accepted_mac_file = value;

    if( !strcmp ( IEEE80211N, key ))
        configValues.ieee80211n = value;

    if( !strcmp ( IEEE80211AC, key ))
        configValues.ieee80211ac = value;

    if( !strcmp ( IEEE80211AX, key ))
        configValues.ieee80211ax = value;
        
    if( !strcmp ( GATEWAY, key ))
        configValues.gateway = value;

}


const char* get_config_file(const char* file){
    static char *homedir;

    static char a[BUFSIZE];

    //    if ((homedir = getenv("HOME")) == nullptr) {
    //        homedir = getpwuid(getuid())->pw_dir;
    //    }
    //    snprintf(a,BUFSIZE,"%s%s%s",homedir,"/",file);

    snprintf(a,BUFSIZE,"%s",file);

    return (const char*)a;
}

}




