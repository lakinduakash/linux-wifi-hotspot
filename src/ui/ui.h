//
// Created by lakinduakash on 13/04/19.
//

#ifndef WIHOTSPOT_UI_H
#define WIHOTSPOT_UI_H

#include <gtk/gtk.h>

typedef struct {
    GtkEntry *ssid;
    GtkEntry *pass;
} WIData;

int initUi(int argc, char *argv[]);
void init_ui_from_config(WIData* data);

#endif //WIHOTSPOT_UI_H
