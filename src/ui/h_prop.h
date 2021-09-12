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

#ifndef WIHOTSPOT_H_PROP_H
#define WIHOTSPOT_H_PROP_H


#include "read_config.h"

typedef struct Device *PtrToNode;
struct Device
{
        char HOSTNAME[2048];
        char IP[2048];
        char MAC[2048];
        unsigned int Number;
        PtrToNode Next;
}; // Head node is null
typedef PtrToNode Position;
typedef PtrToNode Node;

static int parse_output(const char *);

const char *build_wh_start_command(char *, char *, char *, char *);
const char *build_wh_from_config(void);

int startShell(const char *);

int write_config(char *);

int get_h_running_info(char** a);
static int init_get_running();

static int init_get_interface_list();
char** get_interface_list(int*);
const char* build_kill_create_ap_command(char* pid);

const char *build_wh_mkconfig_command(ConfigValues* cv);

char** get_wifi_interface_list(int *length);

void write_accepted_macs(char* filename, char* accepted_macs);

char * read_mac_filter_file(char * filename);

Node get_connected_devices(char *PID);
PtrToNode add_device_node(Node l, int number, char *line, int marker[3]);

#endif //WIHOTSPOT_H_PROP_H
