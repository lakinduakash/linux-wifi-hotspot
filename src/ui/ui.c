//
// Created by lakinduakash on 13/04/19.
//
/*
Copyright (c) 2019, lakinduaksh
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
#include <stdlib.h>
#include <X11/Xlib.h>
#include <regex.h>

#include "h_prop.h"
#include "ui.h"
#include "read_config.h"
#include "util.h"
#include "about_ui.h"
#include "qr_ui.h"

#define BUFSIZE 512
#define CMD_BUFSIZE 2304
#define AP_ENABLED "AP-ENABLED"

#define ERROR_SSID_MSG "SSID must not be empty"
#define ERROR_PASS_MSG "Password must contain 8 characters"
#define ERROR_CHANNEL_MSG "Invalid channel number"
#define ERROR_CHANNEL_MSG_2 "Channel must be 1-11"
#define ERROR_CHANNEL_MSG_5 "Channel must be 1-196"
#define ERROR_MAC_MSG "Invalid Mac address"
#define ERROR_GATEWAY_MSG "Invalid gateway IP address"
#define ERROR_IFACE_MSG "Please select WiFi and Internet interfaces"
#define ERROR_START_MSG "Could not start the hotspot. Run wihotspot from a terminal for details."
#define ERROR_CONFIG_MSG "Could not write the configuration file"

#define DEFAULT_GATEWAY_IP "192.168.12.1"

#define SET_GTK_WIDGET(name, type) \
    name = (type *) gtk_builder_get_object(builder, #name);


GtkBuilder *builder;
GtkWindow *window;
GtkButton *button_create_hp;
GtkButton *button_stop_hp;
GtkButton *button_about;
GtkButton *button_qr_open;
GtkButton *button_refresh;

GtkGrid *grid_devices;
GtkWidget *label_cd_hostname;
GtkWidget *label_cd_ip;
GtkWidget *label_cd_mac;
GtkWidget *label_cd_number;
PtrToNode device_list;

GtkEntry *entry_ssid;
GtkEntry *entry_pass;
GtkEntry *entry_mac;
GtkEntry *entry_channel;
GtkEntry *entry_gateway;
GtkTextView *tv_mac_filter;

GtkTextBuffer *buffer_mac_filter;

GtkDropDown *combo_wifi;
GtkDropDown *combo_internet;

GtkCheckButton *rb_freq_auto;
GtkCheckButton *rb_freq_2;
GtkCheckButton *rb_freq_5;

GtkCheckButton *cb_hidden;
GtkCheckButton *cb_no_haveged;
GtkCheckButton *cb_use_psk;
GtkCheckButton *cb_gateway;
GtkCheckButton *cb_mac;
GtkCheckButton *cb_random_mac;
GtkCheckButton *cb_no_virt;
GtkCheckButton *cb_channel;
GtkCheckButton *cb_open;
GtkCheckButton *cb_mac_filter;
GtkCheckButton *cb_ieee80211n;
GtkCheckButton *cb_ieee80211ac;
GtkCheckButton *cb_ieee80211ax;

GtkProgressBar *progress_bar;

GtkLabel *label_status;
GtkTextView *textview_log;
GtkExpander *expander_log;
GtkLabel *label_input_error;

GtkCssProvider* provider;
GdkDisplay *display;

GError *error = NULL;


const char** iface_list;
const char** wifi_iface_list;
gchar *accepted_macs;
int iface_list_length;
int wifi_iface_list_length;
char* running_info[3];
guint pb_pulse_id;
static ConfigValues configValues;



static void *stopHp(void *) {
    if(running_info[0]!=NULL){
        gtk_label_set_label(label_status,"Stopping ...");
        start_pb_pulse();
        lock_all_views(TRUE);
        startShell(build_kill_create_ap_command(running_info[0]));
        g_thread_new("init_running",init_running_info,NULL);
    }
    return 0;
}

static void on_create_hp_clicked(GtkWidget *widget, gpointer data) {


    if(init_config_val_input(&configValues) != 0){
        set_error_text(ERROR_IFACE_MSG);
        return;
    }


    if(validator(&configValues) == FALSE){
        set_error_text("Some inputs are not valid!");
        return;
    }
    else{
        set_error_text("");
    }


    if(startShell(build_wh_mkconfig_command(&configValues)) != 0){
        show_shell_error(ERROR_CONFIG_MSG);
        return;
    }

    clear_log();
    gtk_label_set_label(label_status, "Starting hotspot...");
    g_thread_new("shell_create_hp", run_create_hp_shell, (void*)build_wh_from_config());


}

static void on_stop_hp_clicked(GtkWidget *widget, gpointer data) {
    g_thread_new("shell2", stopHp, NULL);

}

static void on_about_clicked(GtkWidget *widget, gpointer data){
    show_info(widget,data);
}

static void on_qr_open_clicked(GtkWidget *widget, gpointer data){

    char* image_path = generate_qr_image(configValues.ssid,"WPA",configValues.pass);
    open_qr(widget,data,image_path);
}


static void loadStyles(){
    provider = gtk_css_provider_new();
    display = gdk_display_get_default();

    gtk_style_context_add_provider_for_display(
        display,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );

    // Load CSS description from compiled resource
    gtk_css_provider_load_from_resource(
        provider,
        "/css/style.css"
    );

    g_object_unref(provider);
    provider = NULL;
}

static void set_error_text(const char * text){
    gtk_label_set_label(label_input_error,text);
}

/* create_ap's output is read on a worker thread, and GTK may only be touched
   from the main loop, so every log update is marshalled through g_idle_add(). */
static gboolean append_log_line_idle(gpointer data){
    char *line = data;
    GtkTextBuffer *tb;
    GtkTextIter end;
    GtkTextMark *mark;

    if(textview_log == NULL){
        g_free(line);
        return G_SOURCE_REMOVE;
    }

    tb = gtk_text_view_get_buffer(textview_log);
    gtk_text_buffer_get_end_iter(tb, &end);
    gtk_text_buffer_insert(tb, &end, line, -1);
    gtk_text_buffer_insert(tb, &end, "\n", -1);

    /* keep the newest line in view */
    gtk_text_buffer_get_end_iter(tb, &end);
    mark = gtk_text_buffer_create_mark(tb, NULL, &end, FALSE);
    gtk_text_view_scroll_mark_onscreen(textview_log, mark);
    gtk_text_buffer_delete_mark(tb, mark);

    g_free(line);
    return G_SOURCE_REMOVE;
}

void append_log_line(const char *line){
    if(line == NULL)
        return;
    g_idle_add(append_log_line_idle, g_strdup(line));
}

/* Called from a GTK signal handler, i.e. already on the main loop. Clearing
   inline rather than through g_idle_add() also guarantees the reset lands
   before any line the worker thread queues after it. */
void clear_log(void){
    if(textview_log != NULL)
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(textview_log), "", -1);
}

static gboolean expand_log_idle(gpointer data){
    (void) data;
    if(expander_log != NULL)
        gtk_expander_set_expanded(expander_log, TRUE);
    return G_SOURCE_REMOVE;
}

/* Open the log on failure: the one-line error label rarely carries enough to
   act on, and the reason is always somewhere in the output. */
static void expand_log(void){
    g_idle_add(expand_log_idle, NULL);
}

/* Show whatever create_ap complained about, falling back to a generic hint
   when it died without a recognisable message. */
static void show_shell_error(const char *fallback){
    const char *err = get_last_shell_error();
    set_error_text(err != NULL ? err : fallback);
    expand_log();
}

static void* entry_mac_warn(GtkWidget *widget, gpointer data) {
    const char *mac = gtk_editable_get_text(GTK_EDITABLE(widget));

    if (is_toggle_active(cb_mac) && (mac == NULL || isValidMacAddress(mac) != 1)) {
        set_error_text(ERROR_MAC_MSG);
        gtk_widget_add_css_class(GTK_WIDGET(entry_mac), "entry-error");

        return NULL;
    }

    gtk_widget_remove_css_class(GTK_WIDGET(entry_mac), "entry-error");
    set_error_text("");
    return NULL;
}

static void* entry_ssid_warn(GtkWidget *widget, gpointer data) {
    const char *ssid = gtk_editable_get_text(GTK_EDITABLE(widget));

    if (ssid == NULL || !*ssid) {
        gtk_widget_add_css_class(GTK_WIDGET(entry_ssid), "entry-error");
        set_error_text(ERROR_SSID_MSG);
        return NULL;
    }

    gtk_widget_remove_css_class(GTK_WIDGET(entry_ssid), "entry-error");
    set_error_text("");
    return NULL;
}

static void* entry_pass_warn(GtkWidget *widget, gpointer data) {
    const char *pass = gtk_editable_get_text(GTK_EDITABLE(widget));

    if (pass == NULL || (strlen(pass) > 0 && strlen(pass) < 8)) {
        gtk_widget_add_css_class(GTK_WIDGET(entry_pass), "entry-error");
        set_error_text(ERROR_PASS_MSG);
        return NULL;
    }

    gtk_widget_remove_css_class(GTK_WIDGET(entry_pass), "entry-error");
    set_error_text("");
    return NULL;
}

static void* entry_channel_warn(GtkWidget *widget, gpointer data)
{
    const char *channel = gtk_editable_get_text(GTK_EDITABLE(widget));

    if (!is_toggle_active(cb_channel))
        goto valid;

    if (channel == NULL || !*channel)
        goto invalid;

    char *end;
    long ch = strtol(channel, &end, 10);

    if (end == channel || *end != '\0')
        goto invalid;

    long max_channel = 196;
    const char *error_msg = ERROR_CHANNEL_MSG;

    if (configValues.freq != NULL) {
        if (strcmp(configValues.freq, "2.4") == 0) {
            max_channel = 11;
            error_msg = ERROR_CHANNEL_MSG_2;
        }
        else if (strcmp(configValues.freq, "5") == 0) {
            error_msg = ERROR_CHANNEL_MSG_5;
        }
    }

    if (ch < 1 || ch > max_channel) {
        gtk_widget_add_css_class(GTK_WIDGET(entry_channel), "entry-error");
        set_error_text(error_msg);
        return NULL;
    }

    valid:
        gtk_widget_remove_css_class(GTK_WIDGET(entry_channel), "entry-error");
        set_error_text("");
        return NULL;

    invalid:
        gtk_widget_add_css_class(GTK_WIDGET(entry_channel), "entry-error");
        set_error_text(ERROR_CHANNEL_MSG);
        return NULL;
}

static void* tv_mac_filter_warn() {
    if (is_toggle_active(cb_mac_filter) &&
        isValidAcceptedMacs(get_accepted_macs()) == -1) {

        gtk_widget_add_css_class(GTK_WIDGET(tv_mac_filter), "tv-mac-error");
        set_error_text(ERROR_MAC_MSG);
        return NULL;
    }

    gtk_widget_remove_css_class(GTK_WIDGET(tv_mac_filter), "tv-mac-error");
    set_error_text("");
    return NULL;
}

static void* entry_gateway_warn(GtkWidget *widget, gpointer data) {
    const char *gateway = gtk_editable_get_text(GTK_EDITABLE(widget));

    if (is_toggle_active(cb_gateway) && isValidIPaddress(gateway) == -1) {
        gtk_widget_add_css_class(GTK_WIDGET(entry_gateway), "entry-error");
        set_error_text(ERROR_GATEWAY_MSG);
        return NULL;
    }

    gtk_widget_remove_css_class(GTK_WIDGET(entry_gateway), "entry-error");
    set_error_text("");
    return NULL;
}


static void update_freq_toggle(){
    if (is_toggle_active(rb_freq_2))
        configValues.freq = "2.4";
    else if (is_toggle_active(rb_freq_5))
        configValues.freq = "5";
    else
        configValues.freq = NULL;
}

static void assign_gtk_widgets(void) {
    /* Buttons */
    SET_GTK_WIDGET(button_create_hp, GtkButton);
    SET_GTK_WIDGET(button_stop_hp, GtkButton);
    SET_GTK_WIDGET(button_about, GtkButton);
    SET_GTK_WIDGET(button_qr_open, GtkButton);
    SET_GTK_WIDGET(button_refresh, GtkButton);

    /* Containers */
    SET_GTK_WIDGET(grid_devices, GtkGrid);

    /* Entries */
    SET_GTK_WIDGET(entry_ssid, GtkEntry);
    SET_GTK_WIDGET(entry_pass, GtkEntry);
    SET_GTK_WIDGET(entry_mac, GtkEntry);
    SET_GTK_WIDGET(entry_channel, GtkEntry);
    SET_GTK_WIDGET(entry_gateway, GtkEntry);

    /* Text views */
    SET_GTK_WIDGET(tv_mac_filter, GtkTextView);
    SET_GTK_WIDGET(textview_log, GtkTextView);

    /* Dropdowns */
    SET_GTK_WIDGET(combo_internet, GtkDropDown);
    SET_GTK_WIDGET(combo_wifi, GtkDropDown);

    /* Check buttons */
    SET_GTK_WIDGET(cb_hidden, GtkCheckButton);
    SET_GTK_WIDGET(cb_no_haveged, GtkCheckButton);
    SET_GTK_WIDGET(cb_use_psk, GtkCheckButton);
    SET_GTK_WIDGET(cb_mac, GtkCheckButton);
    SET_GTK_WIDGET(cb_random_mac, GtkCheckButton);
    SET_GTK_WIDGET(cb_no_virt, GtkCheckButton);
    SET_GTK_WIDGET(cb_channel, GtkCheckButton);
    SET_GTK_WIDGET(cb_gateway, GtkCheckButton);
    SET_GTK_WIDGET(cb_open, GtkCheckButton);
    SET_GTK_WIDGET(cb_mac_filter, GtkCheckButton);
    SET_GTK_WIDGET(cb_ieee80211n, GtkCheckButton);
    SET_GTK_WIDGET(cb_ieee80211ac, GtkCheckButton);
    SET_GTK_WIDGET(cb_ieee80211ax, GtkCheckButton);
    SET_GTK_WIDGET(rb_freq_auto, GtkCheckButton);
    SET_GTK_WIDGET(rb_freq_2, GtkCheckButton);
    SET_GTK_WIDGET(rb_freq_5, GtkCheckButton);

    /* Labels */
    SET_GTK_WIDGET(label_status, GtkLabel);
    SET_GTK_WIDGET(label_input_error, GtkLabel);

    /* Other widgets */
    SET_GTK_WIDGET(expander_log, GtkExpander);
    SET_GTK_WIDGET(progress_bar, GtkProgressBar);

    buffer_mac_filter = gtk_text_view_get_buffer(tv_mac_filter);
}

static void connect_callbacks(void) {
    /* Buttons */
    CONNECT_BUTTON_CLICKED(create_hp);
    CONNECT_BUTTON_CLICKED(stop_hp);
    CONNECT_BUTTON_CLICKED(about);
    CONNECT_BUTTON_CLICKED(qr_open);
    CONNECT_BUTTON_CLICKED(refresh);

    /* Configuration toggles */
    CONNECT_TOGGLED(cb_mac, entry_mac);
    CONNECT_TOGGLED(cb_channel, entry_channel);
    CONNECT_TOGGLED(cb_mac_filter, tv_mac_filter);
    CONNECT_TOGGLED(cb_gateway, entry_gateway);

    CONNECT_SIGNAL(cb_open, "toggled", on_cb_open_toggle);
    CONNECT_SIGNAL(cb_random_mac, "toggled", on_cb_random_mac_toggle);

    /* Frequency selection */
    CONNECT_SIGNAL(rb_freq_2, "toggled", update_freq_toggle);
    CONNECT_SIGNAL(rb_freq_5, "toggled", update_freq_toggle);
    CONNECT_SIGNAL(rb_freq_auto, "toggled", update_freq_toggle);

    /* Input validation */
    CONNECT_SIGNAL(buffer_mac_filter, "changed", tv_mac_filter_warn);

    CONNECT_ENTRY_CHANGED(mac);
    CONNECT_ENTRY_CHANGED(ssid);
    CONNECT_ENTRY_CHANGED(pass);
    CONNECT_ENTRY_CHANGED(channel);
    CONNECT_ENTRY_CHANGED(gateway);
}

gboolean is_toggle_active(GtkCheckButton *widget) {
    return gtk_check_button_get_active(widget);
}

gboolean is_widget_active(GtkWidget *widget) {
    return is_toggle_active(GTK_CHECK_BUTTON(widget));
}


int initUi(GtkApplication *app) {
    GError *error = NULL;

    XInitThreads();

    // Construct a GtkBuilder instance
    builder = gtk_builder_new();

    // Load UI description from compiled resource
    gtk_builder_add_from_resource(builder,"/org/gtk/wihotspot/wifih.ui",&error);

    /* Connect signal handlers to the constructed widgets. */
    window = (GtkWindow *) gtk_builder_get_object(builder, "window");

    gtk_window_set_application(window, app);

    assign_gtk_widgets();

    loadStyles();

    gtk_widget_add_css_class(
        GTK_WIDGET(label_input_error), "label-error");

    connect_callbacks();

    start_pb_pulse();

    init_interface_list();
    init_ui_from_config();
    select_default_interfaces();

    gtk_window_present(window);

    g_thread_new("init_running", init_running_info, NULL);

    return 0;
}


void init_ui_from_config() {

    if (read_config_file() != READ_CONFIG_FILE_SUCCESS)
        return;

    ConfigValues *values = getConfigValues();

    if (values->ssid != NULL)
        gtk_editable_set_text(GTK_EDITABLE(entry_ssid), values->ssid);

    if (values->pass != NULL)
        gtk_editable_set_text(GTK_EDITABLE(entry_pass), values->pass);

    gboolean open_network =
        (values->pass == NULL || strcmp(values->pass,"")==0);

    gtk_check_button_set_active(cb_open, open_network);

    if (values->iface_wifi != NULL) {
        int idw = find_str(
            values->iface_wifi,
            wifi_iface_list,
            wifi_iface_list_length
        );

        if (idw >= 0)
            gtk_drop_down_set_selected(combo_wifi, idw);
    }

    if (values->iface_inet != NULL) {
        int idw = find_str(
            values->iface_inet,
            iface_list,
            iface_list_length
        );

        if (idw >= 0)
            gtk_drop_down_set_selected(combo_internet, idw);
    }

    /* Restore boolean configuration options. */
    #define SET_CONFIG_TOGGLE(key) \
        gtk_check_button_set_active( \
            GTK_CHECK_BUTTON(cb_##key), \
            g_strcmp0(values->key, "1") == 0 \
        )

    SET_CONFIG_TOGGLE(hidden);
    SET_CONFIG_TOGGLE(no_haveged);
    SET_CONFIG_TOGGLE(use_psk);
    SET_CONFIG_TOGGLE(no_virt);
    SET_CONFIG_TOGGLE(ieee80211n);
    SET_CONFIG_TOGGLE(ieee80211ac);
    SET_CONFIG_TOGGLE(ieee80211ax);

    #undef SET_CONFIG_TOGGLE

    /* Channel configuration. */
    if (values->channel != NULL &&
        values->channel[0] != '\0' &&
        strcmp(values->channel, "default") != 0) {

        gtk_check_button_set_active(cb_channel, TRUE);
        gtk_editable_set_text(GTK_EDITABLE(entry_channel), values->channel);
    } else {
        gtk_widget_set_sensitive(
            GTK_WIDGET(entry_channel), FALSE
        );
    }

    /* Frequency band. */
    if (g_strcmp0(values->freq, "2.4") == 0)
        gtk_check_button_set_active(rb_freq_2, TRUE);
    else if (g_strcmp0(values->freq, "5") == 0)
        gtk_check_button_set_active(rb_freq_5, TRUE);

    /* MAC address. */
    if (values->mac != NULL && values->mac[0] != '\0') {
        gtk_check_button_set_active(cb_mac, TRUE);
        gtk_editable_set_text(GTK_EDITABLE(entry_mac), values->mac);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(entry_mac), FALSE);
    }

    /* Gateway configuration. */
    gboolean custom_gateway =
        values->gateway != NULL &&
        strcmp(values->gateway, DEFAULT_GATEWAY_IP) != 0;

    gtk_check_button_set_active(cb_gateway, custom_gateway);
    gtk_widget_set_sensitive(GTK_WIDGET(entry_gateway), custom_gateway);

    gtk_editable_set_text(
        GTK_EDITABLE(entry_gateway),
        values->gateway ? values->gateway : DEFAULT_GATEWAY_IP
    );

    /* MAC filter. */
    if (g_strcmp0(values->mac_filter, "1") == 0) {
        gtk_check_button_set_active(cb_mac_filter, TRUE);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(tv_mac_filter), FALSE);
    }

    /* Load accepted MAC addresses. */
    if (values->accepted_mac_file != NULL) {
        char *macs = read_mac_filter_file(values->accepted_mac_file);

        if (macs != NULL && strlen(macs) > 0) {
            gtk_text_buffer_set_text(buffer_mac_filter, macs, strlen(macs));
        }
    }
}

void init_interface_list()
{
    iface_list_length = 0;
    wifi_iface_list_length = 0;

    iface_list = (const char **)get_interface_list(&iface_list_length);
    wifi_iface_list = (const char **)get_wifi_interface_list(&wifi_iface_list_length);

    GtkStringList *internet_model = gtk_string_list_new(NULL);
    GtkStringList *wifi_model = gtk_string_list_new(NULL);

    for (int i = 0; i < iface_list_length; i++) {
        gtk_string_list_append(internet_model, iface_list[i]);
    }

    for (int i = 0; i < wifi_iface_list_length; i++) {
        gtk_string_list_append(wifi_model, wifi_iface_list[i]);
    }

    gtk_drop_down_set_model(combo_internet, G_LIST_MODEL(internet_model));
    gtk_drop_down_set_model(combo_wifi, G_LIST_MODEL(wifi_model));

    g_object_unref(internet_model);
    g_object_unref(wifi_model);
}


/* The shipped config names wlan0/eth0, which exist on hardly any machine, so
   neither combo ends up selected and Create silently does nothing. Fall back
   to the interfaces this machine actually has. */
void select_default_interfaces(){

    if(gtk_drop_down_get_selected(combo_wifi) < 0 && wifi_iface_list_length > 0)
        gtk_drop_down_set_selected(combo_wifi, 0);

    if(gtk_drop_down_get_selected(combo_internet) < 0 && iface_list_length > 0){

        const char *route_iface = get_default_route_interface();
        int id = -1;

        // the interface holding the default route is the one with Internet
        if(route_iface != NULL)
            id = find_str((char*)route_iface, iface_list, iface_list_length);

        // no default route: anything but loopback is a better guess than lo
        for (int i = 0; id == -1 && i < iface_list_length; i++){
            if(strcmp(iface_list[i], "lo") != 0)
                id = i;
        }

        if(id != -1)
            gtk_drop_down_set_selected(combo_internet, id);
    }
}


void lock_all_views(gboolean set_lock){
    if(set_lock){
        gtk_editable_set_editable( (GtkEditable*)entry_ssid,FALSE);
        gtk_editable_set_editable( (GtkEditable*)entry_pass,FALSE);
        gtk_widget_set_sensitive ((GtkWidget*)button_create_hp, FALSE);
        gtk_widget_set_sensitive ((GtkWidget*)button_stop_hp, FALSE);
        gtk_widget_set_sensitive ((GtkWidget*)combo_internet, FALSE);
        gtk_widget_set_sensitive ((GtkWidget*)combo_wifi, FALSE);
        gtk_widget_set_sensitive ((GtkWidget*)tv_mac_filter, FALSE);
    } else{
        gtk_editable_set_editable( (GtkEditable*)entry_ssid,TRUE);
        gtk_editable_set_editable( (GtkEditable*)entry_pass,TRUE);
        gtk_widget_set_sensitive ((GtkWidget*)button_create_hp, TRUE);
        gtk_widget_set_sensitive ((GtkWidget*)button_stop_hp, TRUE);
        gtk_widget_set_sensitive ((GtkWidget*)button_stop_hp, TRUE);
        gtk_widget_set_sensitive ((GtkWidget*)combo_internet, TRUE);
        gtk_widget_set_sensitive ((GtkWidget*)combo_wifi, TRUE);
        if (is_toggle_active(cb_mac_filter))
            gtk_widget_set_sensitive ((GtkWidget*)tv_mac_filter, TRUE);
    }
}


void lock_running_views(gboolean set_lock){
    if(set_lock){
        gtk_editable_set_editable( (GtkEditable*)entry_ssid,FALSE);
        gtk_editable_set_editable( (GtkEditable*)entry_pass,FALSE);
        gtk_widget_set_sensitive ((GtkWidget*)button_create_hp, FALSE);

        gtk_widget_set_sensitive ((GtkWidget*)button_stop_hp, TRUE);
        gtk_widget_set_sensitive ((GtkWidget*)button_qr_open, TRUE);

        gtk_widget_set_sensitive ((GtkWidget*)combo_internet, FALSE);
        gtk_widget_set_sensitive ((GtkWidget*)combo_wifi, FALSE);

        gtk_widget_set_sensitive ((GtkWidget*)tv_mac_filter, FALSE);
    } else{
        gtk_editable_set_editable( (GtkEditable*)entry_ssid,TRUE);
        gtk_editable_set_editable( (GtkEditable*)entry_pass,TRUE);
        gtk_widget_set_sensitive ((GtkWidget*)button_create_hp, TRUE);

        gtk_widget_set_sensitive ((GtkWidget*)button_stop_hp, FALSE);
        gtk_widget_set_sensitive ((GtkWidget*)button_qr_open, FALSE);

        gtk_widget_set_sensitive ((GtkWidget*)combo_internet, TRUE);
        gtk_widget_set_sensitive ((GtkWidget*)combo_wifi, TRUE);

        if (is_toggle_active(cb_mac_filter))
            gtk_widget_set_sensitive ((GtkWidget*)tv_mac_filter, TRUE);
    }
}

static guint start_pb_pulse(){
    gtk_widget_set_visible((GtkWidget*)progress_bar,TRUE);
    return pb_pulse_id= g_timeout_add (100, update_progress_in_timeout, progress_bar);
}

static void stop_pb_pulse(){
    g_source_remove(pb_pulse_id);
    gtk_widget_set_visible((GtkWidget*)progress_bar,FALSE);
}

static gboolean update_progress_in_timeout (gpointer pbar)
{
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(pbar));
    return G_SOURCE_CONTINUE; /* keep running */
}


void clear_running_info(){

        g_free(running_info[0]);
        running_info[0]=NULL;
}

void* init_running_info(void *){

    clear_running_info();
    lock_all_views(TRUE);

    gtk_label_set_label(label_status,"Getting running info...");

    get_h_running_info(running_info);

    if(running_info[0]!=NULL){

        char a[BUFSIZE];
        snprintf(a,BUFSIZE,"Running as PID: %s",running_info[0]);
        gtk_label_set_label(label_status,a);

        lock_all_views(FALSE);
        lock_running_views(TRUE);


    } else{

        gtk_label_set_label(label_status,"Not running");
        lock_all_views(FALSE);
        lock_running_views(FALSE);
    }

    stop_pb_pulse();
    return 0;
}


static void *run_create_hp_shell(void *cmd) {

    char buf[BUFSIZE];
    char cmd_line[CMD_BUFSIZE];
    FILE *fp;

    clear_last_shell_error();

    /* create_ap reports every failure on stderr, so fold it into the pipe -
       without it a failed start leaves the user with no reason at all */
    if(is_explicit_freq_band(configValues.freq))
        snprintf(cmd_line, CMD_BUFSIZE, "{ %s --freq-band %s ; } 2>&1", (char*)cmd, configValues.freq);
    else
        snprintf(cmd_line, CMD_BUFSIZE, "{ %s ; } 2>&1", (char*)cmd);

    if ((fp = popen(cmd_line, "r")) == NULL) {
        printf("Error opening pipe!\n");
        set_error_text(ERROR_START_MSG);
        init_running_info(NULL);
        return NULL;
    }

    int ap_enabled = 0;

    start_pb_pulse();

    /* keep draining until create_ap exits: returning early would leak the
       popen() handle and can block the child on a full pipe buffer */
    while (fgets(buf, BUFSIZE, fp) != NULL) {
        buf[strcspn(buf, "\n")] = 0;
        printf("%s\n", buf);
        append_log_line(buf);
        record_shell_error_line(buf);

        if (!ap_enabled && strstr(buf, AP_ENABLED) != NULL) {
            ap_enabled = 1;
            init_running_info(NULL);
        }
    }

    if (pclose(fp)) {
        printf("Command not found or exited with error status\n");
        show_shell_error(ERROR_START_MSG);
        init_running_info(NULL);
        return NULL;
    }

    init_running_info(NULL);
    return 0;
}

static gboolean validator(ConfigValues *cv) {
    if (cv->pass != NULL && *cv->pass && strlen(cv->pass) < 8)
        return FALSE;

    if (cv->ssid == NULL || !*cv->ssid)
        return FALSE;

    if (is_toggle_active(cb_mac) &&
        (cv->mac == NULL || isValidMacAddress(cv->mac) != 1)) {

        set_error_text(ERROR_MAC_MSG);
        gtk_widget_add_css_class(GTK_WIDGET(entry_mac), "entry-error");
        return FALSE;
    }

    if (is_toggle_active(cb_channel)) {
        if (cv->channel == NULL || !*cv->channel)
            return FALSE;

        char *end;
        long channel = strtol(cv->channel, &end, 10);

        if (end == cv->channel || *end != '\0')
            return FALSE;

        long max_channel = 196;

        if (cv->freq != NULL && strcmp(cv->freq, "2.4") == 0)
            max_channel = 11;

        if (channel < 1 || channel > max_channel)
            return FALSE;
    }

    if (is_toggle_active(cb_mac_filter) &&
        isValidAcceptedMacs(get_accepted_macs()) == -1)
        return FALSE;

    if (is_toggle_active(cb_gateway) &&
        isValidIPaddress(cv->gateway) == -1)
        return FALSE;

    return TRUE;
}


static int init_config_val_input(ConfigValues *cv) {
    GtkStringObject *wifi_item;
    GtkStringObject *internet_item;

    guint wifi_selected = gtk_drop_down_get_selected(combo_wifi);
    guint internet_selected = gtk_drop_down_get_selected(combo_internet);

    if (wifi_selected == GTK_INVALID_LIST_POSITION ||
        internet_selected == GTK_INVALID_LIST_POSITION) {

        g_print("Please select Wifi and Internet interfaces\n");
        return 1;
    }

    wifi_item = GTK_STRING_OBJECT(
        gtk_drop_down_get_selected_item(combo_wifi)
    );

    internet_item = GTK_STRING_OBJECT(
        gtk_drop_down_get_selected_item(combo_internet)
    );

    if (wifi_item == NULL || internet_item == NULL)
        return 1;

    cv->iface_wifi = (char *)gtk_string_object_get_string(wifi_item);
    cv->iface_inet = (char *)gtk_string_object_get_string(internet_item);

    cv->ssid = (char *)gtk_editable_get_text(
        GTK_EDITABLE(entry_ssid)
    );

    cv->pass = is_toggle_active(cb_open)
        ? ""
        : (char *)gtk_editable_get_text(GTK_EDITABLE(entry_pass));

    if (is_toggle_active(rb_freq_2))
        cv->freq = "2.4";
    else if (is_toggle_active(rb_freq_5))
        cv->freq = "5";
    else
        cv->freq = NULL;

    cv->channel = is_toggle_active(cb_channel)
        ? (char *)gtk_editable_get_text(GTK_EDITABLE(entry_channel))
        : NULL;

    cv->gateway = is_toggle_active(cb_gateway)
        ? (char *)gtk_editable_get_text(GTK_EDITABLE(entry_gateway))
        : NULL;

    if (is_toggle_active(cb_random_mac)) {
        char *new_mac = rand_mac();

        if (new_mac != NULL) {
            gtk_editable_set_text(GTK_EDITABLE(entry_mac), new_mac);
            free(new_mac);
        }
    }

    cv->mac = is_toggle_active(cb_mac)
        ? (char *)gtk_editable_get_text(GTK_EDITABLE(entry_mac))
        : NULL;

    #define IF_TOGGLED_USE_PARAM(value) \
        cv->value = is_toggle_active(cb_##value) ? "1" : NULL;

    IF_TOGGLED_USE_PARAM(hidden);
    IF_TOGGLED_USE_PARAM(no_haveged);
    IF_TOGGLED_USE_PARAM(no_virt);
    IF_TOGGLED_USE_PARAM(use_psk);
    IF_TOGGLED_USE_PARAM(ieee80211n);
    IF_TOGGLED_USE_PARAM(ieee80211ac);
    IF_TOGGLED_USE_PARAM(ieee80211ax);

    #undef IF_TOGGLED_USE_PARAM

    if (is_toggle_active(cb_mac_filter)) {
        cv->mac_filter = "1";
        cv->accepted_macs = get_accepted_macs();
    } else {
        cv->mac_filter = NULL;
        cv->accepted_macs = NULL;
    }

    return 0;
}


gchar* get_accepted_macs() {
    GtkTextIter start;
    GtkTextIter end;

    /* Obtain iters for the start and end of points of the buffer */
    gtk_text_buffer_get_start_iter (buffer_mac_filter, &start);
    gtk_text_buffer_get_end_iter (buffer_mac_filter, &end);
    accepted_macs=gtk_text_buffer_get_text (buffer_mac_filter,&start,&end,TRUE);

    return accepted_macs;
}

/**
 * Clear device list
*/
static void clear_connected_devices_list(void) {
    GtkWidget *child;

    while ((child = gtk_widget_get_first_child(
                GTK_WIDGET(grid_devices))) != NULL) {
        gtk_grid_remove(grid_devices, child);
    }
}

/**
 * Set connected device list
*/
static void set_connected_devices_label() {
    Position tmp;
    device_list = get_connected_devices(running_info[0]); // running_info[0] PID

    clear_connected_devices_list();

    while (device_list->Next != NULL)
    {
        tmp = device_list; // Save the last one
        device_list = device_list->Next;
        char number[2];
        sprintf(number, "%d", device_list->Number);
        label_cd_number = gtk_label_new(number);
        label_cd_hostname = gtk_label_new(device_list->HOSTNAME);
        label_cd_ip = gtk_label_new(device_list->IP);
        label_cd_mac = gtk_label_new(device_list->MAC);

        gtk_grid_attach(grid_devices, label_cd_number, 0, device_list->Number, 1, 1);
        gtk_grid_attach(grid_devices, label_cd_hostname, 1, device_list->Number, 1, 1);
        gtk_grid_attach(grid_devices, label_cd_ip, 2, device_list->Number, 1, 1);
        gtk_grid_attach(grid_devices, label_cd_mac, 3, device_list->Number, 1, 1);
        gtk_widget_set_visible(GTK_WIDGET(grid_devices), TRUE);
        free(tmp); // Free the last pointer
    }
}

/**
 * When `widget` is toggled, enable `data` entry
 */
static void on_toggle_changed(GtkWidget *widget, gpointer data) {
    gtk_widget_set_sensitive(GTK_WIDGET(data), is_widget_active(widget));
}

/**
 * When connected devices refresh button clicked.
*/
static void on_refresh_clicked(GtkWidget *widget, gpointer data) {
    if (running_info[0] != NULL)
        set_connected_devices_label();
    else
        clear_connected_devices_list();
}

/**
 * When open password is toggled, disable password entry.
*/
static void on_cb_open_toggle(GtkWidget *widget, gpointer data) {
    gtk_widget_set_sensitive(GTK_WIDGET(entry_pass), !is_widget_active(widget));
}

/**
 * When random mac is toggled, disable set mac label and its entry.
*/
static void on_cb_random_mac_toggle(GtkWidget *widget, gpointer data) {
    if (is_widget_active(widget)) {
        gtk_widget_set_sensitive(GTK_WIDGET(cb_mac), FALSE);
        gtk_check_button_set_active(GTK_CHECK_BUTTON(cb_mac), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(entry_mac), FALSE);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(cb_mac), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(entry_mac), is_toggle_active(cb_mac));
    }
}
