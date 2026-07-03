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
#include <unistd.h>
#include "qr_ui.h"

void open_qr(GtkWidget *widget, gpointer data, char *image_path) {
    GtkBuilder *builder;
    GtkWidget *dialog;
    GtkImage *qr_image;
    GError *error = NULL;
    GtkWidget *toplevel;

    (void)data;

    if (image_path == NULL || access(image_path, R_OK) != 0)
        return;

    builder = gtk_builder_new();
    if (!gtk_builder_add_from_resource(builder, "/org/gtk/wihotspot/qr.glade", &error)) {
        g_warning("Failed to load QR dialog: %s", error ? error->message : "unknown");
        g_clear_error(&error);
        g_object_unref(builder);
        return;
    }

    dialog = GTK_WIDGET(gtk_builder_get_object(builder, "dialog_qr"));
    qr_image = GTK_IMAGE(gtk_builder_get_object(builder, "image_qr"));
    if (dialog == NULL || qr_image == NULL) {
        g_object_unref(builder);
        return;
    }

    if (widget != NULL) {
        toplevel = gtk_widget_get_toplevel(widget);
        if (GTK_IS_WINDOW(toplevel))
            gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(toplevel));
    }

    gtk_image_set_from_file(qr_image, image_path);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_object_unref(builder);
}
