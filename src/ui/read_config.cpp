//
// Created by lakinduakash on 14/04/19.
//



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
            line.erase(std::remove_if(line.begin(), line.end(), isspace),
                       line.end());
            if (line[0] == '#' || line.empty())
                continue;
            auto delimiterPos = line.find('=');
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
}


const char* get_config_file(const char* file){
    static char *homedir;

    static char a[BUFSIZE];

    if ((homedir = getenv("HOME")) == nullptr) {
        homedir = getpwuid(getuid())->pw_dir;
    }
    snprintf(a,BUFSIZE,"%s%s%s",homedir,"/",file);

    //printf(" from %s \n",a);
    return (const char*)a;
}

}




