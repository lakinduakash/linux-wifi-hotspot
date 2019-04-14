//
// Created by lakinduakash on 13/04/19.
//



#include <gtk/gtk.h>
#include <stdlib.h>

#include "h_prop.h"
#include "ui.h"
#include "read_config.h"

GtkBuilder *builder;
GObject *window;
GtkButton *button_create_hp;
GtkButton *button_stop_hp;
GtkEntry *entry_ssd;
GtkEntry *entry_pass;

GtkComboBox *combo_wifi;
GtkComboBox *combo_internet;

GError *error = NULL;


void init_interface_list();

void *threadFunc(void *args) {
    startShell(args);
    return 0;
}

void *stopHp() {
    startShell("sudo create_ap --stop ap0");
}

static void on_create_hp_clicked(GtkWidget *widget,
                                 gpointer data) {

    WIData *d = (WIData *) data;

    if (gtk_combo_box_get_active (combo_wifi) >= 0 && gtk_combo_box_get_active (combo_internet) >= 0) {


        gchar *wifi = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(combo_wifi));
        gchar *internet = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(combo_internet));

        //Remove trailing new lines
        wifi[strcspn(wifi, "\n")] = 0;
        internet[strcspn(internet, "\n")] = 0;

        startShell(build_wh_mkconfig_command(wifi, internet, (char *) gtk_entry_get_text(d->ssid),
                                             (char *) gtk_entry_get_text(d->pass)));

        g_thread_new("shell_create_hp", threadFunc, (void*)build_wh_from_config());

        g_free (wifi);
        g_free (internet);
    } else{

        g_print("Please select Wifi and Internet interfaces\n");
    }


}

static void on_stop_hp_clicked(GtkWidget *widget, gpointer data) {
    g_thread_new("shell2", stopHp, NULL);

}

int initUi(int argc, char *argv[]){


    gtk_init(&argc, &argv);

    /* Construct a GtkBuilder instance and load our UI description */
    builder = gtk_builder_new();
    if (gtk_builder_add_from_file(builder, "glade/wifih.ui", &error) == 0) {
        g_printerr("Error loading file: %s\n", error->message);
        g_clear_error(&error);
        return 1;
    }

    /* Connect signal handlers to the constructed widgets. */
    window = gtk_builder_get_object(builder, "window");
    g_signal_connect (window, "destroy", G_CALLBACK(gtk_main_quit), NULL);


    entry_ssd = (GtkEntry *) gtk_builder_get_object(builder, "entry_ssid");
    entry_pass = (GtkEntry *) gtk_builder_get_object(builder, "entry_pass");

    combo_wifi = (GtkComboBox *) gtk_builder_get_object(builder, "combo_wifi");
    combo_internet = (GtkComboBox *) gtk_builder_get_object(builder, "combo_internet");





    WIData wiData = {
            .pass= entry_pass,
            .ssid= entry_ssd
    };

    init_ui_from_config(&wiData);

    button_create_hp = (GtkButton *) gtk_builder_get_object(builder, "button_create_hp");
    g_signal_connect (button_create_hp, "clicked", G_CALLBACK(on_create_hp_clicked), &wiData);

    button_stop_hp = (GtkButton *) gtk_builder_get_object(builder, "button_stop_hp");
    g_signal_connect (button_stop_hp, "clicked", G_CALLBACK(on_stop_hp_clicked), NULL);

    init_interface_list();

    gtk_main();

    return 0;
}


void init_ui_from_config(WIData* data){

    if(read_config_file()==READ_CONFIG_FILE_SUCCESS){

        ConfigValues *values=getConfigValues();


        if(values->ssid!=NULL)
            gtk_entry_set_text(data->ssid,values->ssid);
        if(values->pass!=NULL)
            gtk_entry_set_text(data->pass,values->pass);

    }
}

void init_interface_list(){

    int length=0;
    const char** list=(const char**)get_interface_list(&length);

    for (int i = 0; i < length; i++){
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_wifi), list[i]);
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_internet), list[i]);
    }

}