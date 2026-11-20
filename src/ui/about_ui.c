//
// Created by lakinduakash on 13/04/19.
//
/*
Copyright (c) 2021, lakinduaksh
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



#include <gtk/gtk.h>
#include "about_ui.h"

static GtkBuilder *builder;
static GError *error = NULL;

void show_info(GtkWidget *widget, gpointer root) {

    builder = gtk_builder_new();

    // Load UI description from compiled resource
    gtk_builder_add_from_resource(builder,"/org/gtk/wihotspot/about.ui",&error);

    GtkWidget *dialog =
        GTK_WIDGET(gtk_builder_get_object(builder, "about_dialog"));

    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(root));

    // Release builder when dialog is closed
    g_signal_connect_swapped(dialog, "close-request",
                             G_CALLBACK(g_object_unref), builder);

    gtk_window_present(GTK_WINDOW(dialog));
}