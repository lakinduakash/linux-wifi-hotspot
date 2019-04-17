//
// Created by lakinduakash on 13/04/19.
//

#ifndef WIHOTSPOT_UI_H
#define WIHOTSPOT_UI_H

#include <gtk/gtk.h>

#include "read_config.h"

typedef struct {
    GtkEntry *ssid;
    GtkEntry *pass;
} WIData;

int initUi(int argc, char *argv[]);

void init_ui_from_config(WIData* data);

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

#endif //WIHOTSPOT_UI_H
