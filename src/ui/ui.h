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

#ifndef WIHOTSPOT_UI_H
#define WIHOTSPOT_UI_H

#include <gtk/gtk.h>

#include "read_config.h"

typedef struct {
    GtkEntry *ssid;
    GtkEntry *pass;
} WIData;

int initUi(int argc, char *argv[]);

void init_ui_from_config();

static void* run_create_hp_shell(void *cmd);

void init_interface_list();

void* init_running_info();

static gboolean update_progress_in_timeout (gpointer pbar);

void lock_all_views(gboolean set_lock);

void lock_running_views(gboolean set_lock);

static guint start_pb_pulse();

static void on_create_hp_clicked(GtkWidget *widget,gpointer data);

static void *stopHp();

static int init_config_val_input(ConfigValues* cv);

static gboolean validator(ConfigValues *cv);

static void set_error_text(char * text);

gchar* get_accepted_macs();

static void set_connected_devices_label(); // new

static void on_refresh_clicked(GtkWidget *widget, gpointer data);

static void on_cb_open_clicked(GtkWidget *widget, gpointer data);

static void clear_connecetd_devices_list();

#endif //WIHOTSPOT_UI_H
