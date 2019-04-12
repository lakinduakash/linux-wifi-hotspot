//
// Created by lakinduakash on 13/04/19.
//
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "h_prop.h"



#define BUFSIZE 1024


#define SUDO "sudo"
#define CREATE_AP "create_ap"

char cmd[BUFSIZE];

int parse_output(char *cmd) {

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

    if(pclose(fp))  {
        printf("Command not found or exited with error status\n");
        return -1;
    }

    return 0;
}


char * build_command(char* iface_src,char* iface_dest,char* ssid, char* pass){

    snprintf(cmd,BUFSIZE,"%s %s %s %s %s %s",SUDO,CREATE_AP,iface_src,iface_dest,ssid,pass);


    return cmd;
}


int startShell(char* cmd){
    parse_output(cmd);
}