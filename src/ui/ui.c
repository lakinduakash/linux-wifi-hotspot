//
// Created by lakinduakash on 13/04/19.
//



#include <gtk/gtk.h>
#include <stdlib.h>

#include "h_prop.h"
#include "ui.h"
#include "read_config.h"


void init_interface_list();
static int find_str(char *find, const char **array, int length);
void* init_running_info();

GtkBuilder *builder;
GObject *window;
GtkButton *button_create_hp;
GtkButton *button_stop_hp;
GtkEntry *entry_ssd;
GtkEntry *entry_pass;

GtkComboBox *combo_wifi;
GtkComboBox *combo_internet;

GtkLabel *label_status;

GError *error = NULL;


const char** iface_list;
int iface_list_length;
char* running_info[3];


void *threadFunc(void *args) {
    startShell(args);
    return 0;
}

void *stopHp() {
    if(running_info[0]!=NULL)
        startShell(build_kill_create_ap_command(running_info[0]));
    return 0;
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


    label_status = (GtkLabel *) gtk_builder_get_object(builder, "label_status");





    WIData wiData = {
            .pass= entry_pass,
            .ssid= entry_ssd
    };

    g_thread_new("init_running",init_running_info,NULL);
    //init_running_info();

    init_interface_list();

    init_ui_from_config(&wiData);

    button_create_hp = (GtkButton *) gtk_builder_get_object(builder, "button_create_hp");
    g_signal_connect (button_create_hp, "clicked", G_CALLBACK(on_create_hp_clicked), &wiData);

    button_stop_hp = (GtkButton *) gtk_builder_get_object(builder, "button_stop_hp");
    g_signal_connect (button_stop_hp, "clicked", G_CALLBACK(on_stop_hp_clicked), NULL);


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

        if(values->iface_wifi!=NULL){
            int idw=find_str(values->iface_wifi,iface_list,iface_list_length);

            if(idw !=-1){
                gtk_combo_box_set_active(combo_wifi,idw);
            }
        }

        if(values->iface_inet!=NULL){
            int idw=find_str(values->iface_inet,iface_list,iface_list_length);

            if(idw !=-1){
                gtk_combo_box_set_active(combo_internet,idw);
            }
        }

    }
}

void init_interface_list(){

    iface_list_length=0;
    iface_list=(const char**)get_interface_list(&iface_list_length);

    for (int i = 0; i < iface_list_length; i++){
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_wifi), iface_list[i]);
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_internet), iface_list[i]);
    }

}


void* init_running_info(){
    get_running_info(running_info);

    if(running_info[0]!=NULL){

        gtk_label_set_label(label_status,running_info[0]);
    }

}

int find_str(char *find, const char **array, int length) {
    int i;

    for ( i = 0; i < length; i++ ) {
        if (strcmp(array[i], find) == 0) {
            return i;
        }
    }


    return -1;

}