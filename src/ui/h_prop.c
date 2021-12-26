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

static char* qr_image_path;

//config_t cfg;

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

    snprintf(cmd_start, BUFSIZE, "%s %s %s %s %s %s", SUDO, CREATE_AP, iface_src, iface_dest, ssid, pass);

    return cmd_start;
}


const char *build_wh_mkconfig_command(ConfigValues* cv){

    const char* config_ffile_name=get_config_file(CONFIG_FILE_NAME);

    snprintf(cmd_mkconfig, BUFSIZE, "%s %s %s %s '%s' '%s' %s %s",SUDO, CREATE_AP, cv->iface_wifi, cv->iface_inet, cv->ssid, cv->pass,MKCONFIG,config_ffile_name);

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

    if(cv->mac!=NULL) {
        strcat(cmd_mkconfig, " --mac ");
        strcat(cmd_mkconfig, cv->mac);
    }

    if(cv->gateway!=NULL) {
        strcat(cmd_mkconfig, " --gateway ");
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

    snprintf(cmd_config, BUFSIZE, "%s %s %s %s", SUDO, CREATE_AP,LOAD_CONFIG,get_config_file(CONFIG_FILE_NAME));
    return cmd_config;

}

int startShell(const char *cmd) {
    return parse_output(cmd);
}


const char* build_kill_create_ap_command(char* pid){
    snprintf(cmd_kill, BUFSIZE, "%s %s %s %s", SUDO, CREATE_AP,STOP,pid);
    return cmd_kill;
}

void write_accepted_macs(char* filename, char* accepted_macs){

    printf("mac filter file %s \n",filename);

    snprintf(cmd_write_mac,BUFSIZE,"%s '%s' %s %s","echo", accepted_macs, "| pkexec -u root tee", filename);
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
    snprintf(cmd, BUFSIZE, "%s %s --list-running",SUDO, CREATE_AP);

    FILE *fp;

    if ((fp = popen(cmd, "r")) == NULL) {
        printf("Error opening pipe!\n");
        return -1;
    }

    // Clear buffer - Otherwise old one is used
    h_running_info[0] = '\0';

    while (fgets(h_running_info, BUFSIZE, fp) != NULL) {
        // Do whatever you want here...
        //printf("%s", h_running_info);
    }

    if (pclose(fp)) {
        printf("Command not found or exited with error status\n");
        return -1;
    }

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

char* generate_qr_image(char* ssid,char* type,char *password){
    char cmd[BUFSIZE];

    qr_image_path = "/tmp/wihotspot_qr.png";

    // snprintf(cmd, BUFSIZE, "%s -s 10 -d 256 -o %s 'WIFI:S:%s;T:%s;P:%s;;' ","qrencode",qr_image_path, ssid,type,password);

    // FILE *fp;

    // char temp_buff[1048];

    // if ((fp = popen(cmd, "r")) == NULL) {
    //     printf("Error opening pipe!\n");
        
    // }


    // while (fgets(temp_buff, sizeof(temp_buff), fp) != NULL) {
    
    //     printf("%s", temp_buff);
    // }

    // if (pclose(fp)) {
    //     printf("Error executing qrencode\n");
        
    // }

    snprintf(cmd, BUFSIZE, "WIFI:S:%s;T:%s;P:%s;;",ssid,type,password);

    qr_to_png(cmd,qr_image_path);
    
    return qr_image_path;
}

Node get_connected_devices(char *PID)
{
    char cmd[BUFSIZE];
    snprintf(cmd, BUFSIZE, "%s %s --list-clients %s", SUDO, CREATE_AP, PID);
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
