//
// Created by lakinduakash on 13/04/19.
//
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libconfig.h>

#include "h_prop.h"
#include "read_config.h"


#define BUFSIZE 1024


#define SUDO "pkexec --user root"
#define CREATE_AP "create_ap"

#define MKCONFIG "--mkconfig"
#define LOAD_CONFIG "--config"


char cmd_start[BUFSIZE];
char cmd_mkconfig[BUFSIZE];
char cmd_config[BUFSIZE];

const char* g_ssid=NULL;
const char* g_pass=NULL;

config_t cfg;

static int parse_output(const char *cmd) {

    char buf[BUFSIZE];
    FILE *fp;

    if ((fp = popen(cmd, "r")) == NULL) {
        printf("Error opening pipe!\n");
        return -1;
    }

    while (fgets(buf, BUFSIZE, fp) != NULL) {
        // Do whatever you want here...
        printf("OUTPUT: %s", buf);
    }

    if (pclose(fp)) {
        printf("Command not found or exited with error status\n");
        return -1;
    }

    return 0;
}


const char *build_wh_start_command(char *iface_src, char *iface_dest, char *ssid, char *pass) {

    snprintf(cmd_start, BUFSIZE, "%s %s %s %s %s %s", SUDO, CREATE_AP, iface_src, iface_dest, ssid, pass);

    return cmd_start;
}

const char *build_wh_mkconfig_command(char *iface_src, char *iface_dest, char *ssid, char *pass){

    const char* a=get_config_file(CONFIG_FILE_NAME);

    snprintf(cmd_mkconfig, BUFSIZE, "%s %s %s %s %s %s %s", CREATE_AP, iface_src, iface_dest, ssid, pass,MKCONFIG,a);
    printf("%s \n",cmd_mkconfig);
    return cmd_mkconfig;

}

const char *build_wh_from_config(){

    snprintf(cmd_config, BUFSIZE, "%s %s %s %s", SUDO, CREATE_AP,LOAD_CONFIG,get_config_file(CONFIG_FILE_NAME));
    return cmd_config;

}

int startShell(const char *cmd) {
    parse_output(cmd);
    return 0;
}


int write_config(char* file){

    config_t cfg;
    config_setting_t *root, *setting, *group, *array;
    int i;

    config_init(&cfg);
    root = config_root_setting(&cfg);

    /* Add some settings to the configuration. */


    setting = config_setting_add(root, SSID, CONFIG_TYPE_STRING);
    config_setting_set_string(setting, "myssid");

    setting = config_setting_add(root, PASSPHRASE, CONFIG_TYPE_STRING);
    config_setting_set_string(setting, "123456789");

    /* Write out the new configuration. */
    if(! config_write_file(&cfg, get_config_file(CONFIG_FILE_NAME)))
    {
        fprintf(stderr, "Error while writing file.\n");
        config_destroy(&cfg);
        return(EXIT_FAILURE);
    }

    fprintf(stderr, "New configuration successfully written to: %s\n",
            get_config_file(CONFIG_FILE_NAME));

    config_destroy(&cfg);
    return(EXIT_SUCCESS);
}

