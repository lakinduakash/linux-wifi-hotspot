#include <gtk/gtk.h>
#include <stdlib.h>

typedef struct{
    GtkEntry* ssid;
    GtkEntry* pass;
} WIData;


static void print_hello(GtkEntry *widget,
            gpointer data) {
    const char *name;
    name = gtk_entry_get_text (widget);

    g_print ("\nHello %s!\n\n", name);
}

static void on_create_hp_clicked(GtkWidget *widget,
                         gpointer data) {

    WIData* d= (WIData*)data;
    printf ("Entry contents: %s\n", gtk_entry_get_text(d->ssid));
    printf ("Entry contents: %s\n", gtk_entry_get_text(d->pass));
}



int main(int argc,char *argv[]) {
    GtkBuilder *builder;
    GObject *window;
    GtkButton *button_create_hp;
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
    g_signal_connect (entry_ssd, "activate", G_CALLBACK(print_hello), NULL);

    entry_pass = (GtkEntry*)gtk_builder_get_object(builder, "entry_pass");
    g_signal_connect (entry_pass, "activate", G_CALLBACK(print_hello), NULL);



    WIData wiData={
            .pass= entry_pass,
            .ssid= entry_ssd
    };

    button_create_hp = (GtkButton*)gtk_builder_get_object(builder, "button_create_hp");
    g_signal_connect (button_create_hp, "clicked", G_CALLBACK(on_create_hp_clicked),&wiData);


    gtk_main();

    return 0;
}