//
// Created by lakinduakash on 13/04/19.
//
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libconfig.h>

#include "h_prop.h"


#define BUFSIZE 1024


#define SUDO "pkexec --user root"
#define CREATE_AP "create_ap"


char cmd[BUFSIZE];

const char* g_ssid=NULL;
const char* g_pass=NULL;

config_t cfg;

static int parse_output(char *cmd) {

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


char *build_command(char *iface_src, char *iface_dest, char *ssid, char *pass) {

    snprintf(cmd, BUFSIZE, "%s %s %s %s %s %s", SUDO, CREATE_AP, iface_src, iface_dest, ssid, pass);


    return cmd;
}


int startShell(char *cmd) {
    parse_output(cmd);
    return 0;
}


int create_config(char* file){

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
    if(! config_write_file(&cfg, CONFIG_FILE))
    {
        fprintf(stderr, "Error while writing file.\n");
        config_destroy(&cfg);
        return(EXIT_FAILURE);
    }

    fprintf(stderr, "New configuration successfully written to: %s\n",
            CONFIG_FILE);

    config_destroy(&cfg);
    return(EXIT_SUCCESS);
}


int init_read_wh_config(){


    config_init(&cfg);

    if (!config_read_file(&cfg, CONFIG_FILE)) {
        fprintf(stderr, "%s:%d - %s\n",
                config_error_file(&cfg),
                config_error_line(&cfg),
                config_error_text(&cfg));
        config_destroy(&cfg);
        return(EXIT_FAILURE);
    }


    if (config_lookup_string(&cfg, SSID, &g_ssid))
        printf("SSID: %s", g_ssid);
    else
        printf("SSID is not defined\n");

    if (config_lookup_string(&cfg, PASSPHRASE, &g_pass))
        printf("PASS: %s", g_pass);
    else
        printf("PASS is not defined\n");





    //config_destroy(cfg);
    return 0;


}



void set_ssid(const char* ssid){

    g_ssid=ssid;
}

const char* get_ssid(){

    return g_ssid;
}


void set_pass(const char* pass){

    g_pass=pass;
}


const char* get_pass(){

    return g_pass;
}
