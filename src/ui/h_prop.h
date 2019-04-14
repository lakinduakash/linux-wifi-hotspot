//
// Created by lakinduakash on 13/04/19.
//

#ifndef WIHOTSPOT_H_PROP_H
#define WIHOTSPOT_H_PROP_H


static int parse_output(const char *);

const char *build_wh_start_command(char *, char *, char *, char *);
const char *build_wh_mkconfig_command(char *, char *, char *, char *);
const char *build_wh_from_config(void);

int startShell(const char *);

int write_config(char *);

int get_running_info(char** a);
static int init_get_running();

static int init_get_interface_list();
char** get_interface_list(int*);
const char* build_kill_create_ap_command(char* pid);

#endif //WIHOTSPOT_H_PROP_H
