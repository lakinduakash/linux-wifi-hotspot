#include <gtk/gtk.h>
#include <stdlib.h>
#include <pthread.h>

#include "h_prop.h"

#define BUFSIZE 1024


typedef struct{
    GtkEntry* ssid;
    GtkEntry* pass;
} WIData;


void* threadFunc(void* args){
    startShell(args);
    //parse_output("sudo create_ap wlp3s0 wlp3s0 my 12345679");
    return 0;
}

void* stopHp(){
    startShell("sudo create_ap --stop ap0");
}

static void on_create_hp_clicked(GtkWidget *widget,
                         gpointer data) {

    WIData* d= (WIData*)data;
    printf ("Entry contents: %s\n", gtk_entry_get_text(d->ssid));
    printf ("Entry contents: %s\n", gtk_entry_get_text(d->pass));

    g_thread_new("shell",threadFunc,build_command("wlp3s0","wlp3s0",(char*)gtk_entry_get_text(d->ssid),(char*)gtk_entry_get_text(d->pass)));


}

static void on_stop_hp_clicked(GtkWidget *widget,gpointer data){
    g_thread_new("shell2",stopHp,NULL);

}


int main(int argc,char *argv[]) {
    GtkBuilder *builder;
    GObject *window;
    GtkButton *button_create_hp;
    GtkButton *button_stop_hp;
    GtkEntry *entry_ssd;
    GtkEntry *entry_pass;
    GError *error = NULL;

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



    entry_ssd = (GtkEntry*)gtk_builder_get_object(builder, "entry_ssid");
    entry_pass = (GtkEntry*)gtk_builder_get_object(builder, "entry_pass");


    WIData wiData={
            .pass= entry_pass,
            .ssid= entry_ssd
    };

    button_create_hp = (GtkButton*)gtk_builder_get_object(builder, "button_create_hp");
    g_signal_connect (button_create_hp, "clicked", G_CALLBACK(on_create_hp_clicked),&wiData);

    button_stop_hp = (GtkButton*)gtk_builder_get_object(builder, "button_stop_hp");
    g_signal_connect (button_stop_hp, "clicked", G_CALLBACK(on_stop_hp_clicked),NULL);

    gtk_main();


    return 0;
}
