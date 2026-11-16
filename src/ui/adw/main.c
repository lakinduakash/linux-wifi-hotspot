#include <adwaita.h>
#include "adw_ui.h"

int main(int argc, char *argv[]) {
    g_autoptr(AdwApplication) app = adw_application_new(
        "org.gtk.wihotspot.adw",
        G_APPLICATION_DEFAULT_FLAGS
    );

    g_signal_connect(app, "activate", G_CALLBACK(activate_adw_ui), NULL);

    return g_application_run(G_APPLICATION(app), argc, argv);
}
