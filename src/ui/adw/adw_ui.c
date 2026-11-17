#include "adw_ui.h"
#include <stdio.h>
#include <stdlib.h>

static gchar* find_target_binary(const gchar *target) {
    gchar *self_exe = g_file_read_link("/proc/self/exe", NULL);
    if (self_exe) {
        gchar *dir = g_path_get_dirname(self_exe);
        gchar *sibling = g_build_filename(dir, target, NULL);
        g_free(dir);
        g_free(self_exe);
        if (g_file_test(sibling, G_FILE_TEST_IS_EXECUTABLE)) {
            return sibling;
        }
        g_free(sibling);
    }

    const gchar *local_paths[] = { "./build", ".", "../build" };
    for (size_t i = 0; i < G_N_ELEMENTS(local_paths); i++) {
        gchar *candidate = g_build_filename(local_paths[i], target, NULL);
        if (g_file_test(candidate, G_FILE_TEST_IS_EXECUTABLE)) {
            return candidate;
        }
        g_free(candidate);
    }

    gchar *system_path = g_find_program_in_path(target);
    if (system_path) {
        return system_path;
    }

    return g_strdup(target);
}

static void on_switch_ui_action(GSimpleAction *action, GVariant *param, gpointer user_data) {
    (void)action;
    (void)param;
    GtkWindow *window = GTK_WINDOW(user_data);
    GtkApplication *app = gtk_window_get_application(window);
    g_print("Switching to classic GTK3 UI...\n");

    const gchar *config_dir = g_get_user_config_dir();
    gchar *wihotspot_dir = g_build_filename(config_dir, "wihotspot", NULL);
    g_mkdir_with_parents(wihotspot_dir, 0755);
    gchar *pref_file = g_build_filename(wihotspot_dir, "ui", NULL);
    g_file_set_contents(pref_file, "gui\n", -1, NULL);
    g_free(pref_file);
    g_free(wihotspot_dir);

    gchar *target_cmd = find_target_binary("wihotspot-gui");
    const gchar *argv[] = { target_cmd, NULL };
    GError *error = NULL;
    if (g_spawn_async(NULL, (gchar **)argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error)) {
        g_application_quit(G_APPLICATION(app));
    } else {
        g_warning("Failed to launch wihotspot-gui: %s", error ? error->message : "unknown error");
        if (error) {
            g_error_free(error);
        }
    }
    g_free(target_cmd);
}

static void on_about_action(GSimpleAction *action, GVariant *param, gpointer user_data) {
    (void)action;
    (void)param;
    GtkWindow *window = GTK_WINDOW(user_data);

    GtkBuilder *builder = gtk_builder_new_from_resource("/org/gtk/wihotspot/adw/window.ui");
    AdwDialog *about = ADW_DIALOG(gtk_builder_get_object(builder, "about_dialog"));
    if (about) {
        adw_about_dialog_add_link(ADW_ABOUT_DIALOG(about), "Star on GitHub", "https://github.com/lakinduakash/linux-wifi-hotspot");
        adw_dialog_present(about, GTK_WIDGET(window));
    }
    g_object_unref(builder);
}

static const GActionEntry win_entries[] = {
    { "switch-ui", on_switch_ui_action, NULL, NULL, NULL },
    { "about", on_about_action, NULL, NULL, NULL },
};

void activate_adw_ui(AdwApplication *app, gpointer user_data) {
    (void)user_data;

    GtkBuilder *builder = gtk_builder_new_from_resource("/org/gtk/wihotspot/adw/window.ui");
    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));

    if (!window) {
        g_critical("Could not load 'window' from resource /org/gtk/wihotspot/adw/window.ui");
        g_object_unref(builder);
        return;
    }

    gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(app));
    g_action_map_add_action_entries(G_ACTION_MAP(window), win_entries, G_N_ELEMENTS(win_entries), window);

    gtk_window_present(GTK_WINDOW(window));
    g_object_unref(builder);
}
