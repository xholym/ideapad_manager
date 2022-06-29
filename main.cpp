//
// This program is not longer ideapad specific. But the name stays so far.
// TODO: Add custom icons to items and to app tray idicator.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include "libappindicator/app-indicator.h"

#define TRAY_ICON "emblem-system-symbolic"

enum Power_Profile{
    Low_Power = 0,
    Balanced = 1,
    Performance = 2
};

static int  g_refresh_rate = 0;
static bool g_battery_conservation_on = false;
static Power_Profile g_active_profile;

static AppIndicator* g_tray;
static GtkWidget* g_refresh_rate_item = NULL;
static GtkWidget* g_profile_items[3]  = {};
static GtkWidget* g_battery_con_item  = NULL;

static Power_Profile g_profile_values[] = {Low_Power, Balanced, Performance};

const char * battery_con_lbl_format  = "";

void str_rtrim(char str[]) {
    char *end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    end[1] = '\0';
}

const char* power_profile_name(Power_Profile profile) {
    switch (profile) {
        case Low_Power:   return "Low power";
        case Balanced:    return "Balanced";
        case Performance: return "Performance";
    }
}

void get_command_output(const char command[], char result[], int size) {

    FILE* pipe = popen(command, "r");
    if (!pipe) {
        printf("Failed to execute command %s: popen failed. ", command);
        exit(-1);
    }

    if (fgets(result, size, pipe) == NULL) {
        printf("Failed to execute command %s: fgets failed. ", command);
        exit(-1);
    }

    pclose(pipe);
}

const char* get_refresh_rate_label(int refresh_rate) {
    assert(refresh_rate == 60 || refresh_rate == 90);

    return refresh_rate == 90 ? "Use 60 Hz" : "Use 90 Hz";
}

const char* get_battery_con_label(bool isOn) {
    return isOn ? "Battery Conservation On" : "Battery Conservation Off";
}

int load_refresh_rate() {
    char refresh_rate[3];
    get_command_output("xrandr | rg -o \"\\b\\d+\\.\\d+\\b\\*\"", refresh_rate, 3);
    return atoi(refresh_rate);
}

Power_Profile load_active_profile() {
    char buffer[12];
    memset(buffer, 0, 12);
    get_command_output("cat /sys/firmware/acpi/platform_profile", buffer, 12);
    str_rtrim(buffer);
    if (strcmp(buffer, "low-power") == 0)
        return Low_Power;
    if (strcmp(buffer, "balanced") == 0)
        return Balanced;
    if (strcmp(buffer, "performance") == 0)
        return Performance;

    printf("Unknown power profile - %s.", buffer);
    exit(-1);
}

bool load_battery_conservation_on() {
    char buffer[2];
    get_command_output("cat /sys/bus/platform/drivers/ideapad_acpi/VPC2004:00/conservation_mode", buffer, 2);
    // buffer[1] = 0;
    return strcmp(buffer, "1") == 0;
}

static void reload(bool load_values) {
    if (load_values) {
        g_refresh_rate = load_refresh_rate();
        g_active_profile = load_active_profile();
        g_battery_conservation_on = load_battery_conservation_on();
    }

    const char* battery_conservation_title = g_battery_conservation_on ? "Conservation" : "";
    char title[64];
    sprintf(title, "%s, %s, %d Hz",
            power_profile_name(g_active_profile), battery_conservation_title, g_refresh_rate);
    // TODO: Would be nice to have a subtitle like "Ideapad manager".
    app_indicator_set_title(g_tray, title);


    {
        const char* refresh_rate_lbl = get_refresh_rate_label(g_refresh_rate);
        gtk_menu_item_set_label(GTK_MENU_ITEM(g_refresh_rate_item), refresh_rate_lbl);

        for (int profile = Low_Power; profile <= Performance; profile++) {
            auto profile_item = g_profile_items[profile];

            if (profile_item) {
                int sensitive = profile == g_active_profile ? FALSE : TRUE;
                gtk_widget_set_sensitive(profile_item, sensitive);
            }
        }

        const char* battery_con_label = get_battery_con_label(g_battery_conservation_on);
        gtk_menu_item_set_label(GTK_MENU_ITEM(g_battery_con_item), battery_con_label);
    }
}

static void reload_callback(GtkWidget *item, gpointer data) {
    reload(true);
}

static void quit(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

static void toggle_refresh_rate(GtkWidget *item, gpointer* data) {
    int rate_wanted = g_refresh_rate == 60 ? 90 : 60;

    char command[64];
    sprintf(command, "xrandr --rate %d", rate_wanted);

    system(command);

    g_refresh_rate = rate_wanted;
    reload(false);
}

static void change_profile(GtkWidget *item, gpointer data) {
    Power_Profile profile = *(Power_Profile*) data;

    char profile_name[12];
    if (profile == Low_Power)
        strcpy(profile_name, "low-power");
    else if (profile == Balanced)
        strcpy(profile_name, "balanced");
    else if (profile == Performance)
        strcpy(profile_name, "performance");

    printf("Switching to %s\n", profile_name);
    char command[92];
    sprintf(command, "pkexec sh -c 'echo %s > /sys/firmware/acpi/platform_profile'", profile_name);
    system(command);

    g_active_profile = profile;
    reload(false);
}

static void toggle_battery_conservation(GtkWidget *item, gpointer data) {
    bool turn_on = !g_battery_conservation_on;
    if (turn_on)
        system("pkexec sh -c 'echo 1 > /sys/bus/platform/drivers/ideapad_acpi/VPC2004:00/conservation_mode'");
    else
        system("pkexec sh -c 'echo 0 > /sys/bus/platform/drivers/ideapad_acpi/VPC2004:00/conservation_mode'");

    g_battery_conservation_on = !g_battery_conservation_on;
    reload(false);
}


int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // TODO: Add icons to menu items.

    g_tray = app_indicator_new(
        "ideapad_manager",
        TRAY_ICON,
        APP_INDICATOR_CATEGORY_APPLICATION_STATUS
    );
    app_indicator_set_status(g_tray, APP_INDICATOR_STATUS_ACTIVE);

    // Configure Menu and it's items.
    auto menu = gtk_menu_new();

    // Refresh rate item
    {
        g_refresh_rate_item = gtk_image_menu_item_new();
        auto refresh_rate_icon = gtk_image_new_from_icon_name("video-display", GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(g_refresh_rate_item), refresh_rate_icon);
        g_signal_connect(g_refresh_rate_item, "activate", G_CALLBACK(toggle_refresh_rate), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), g_refresh_rate_item);
        gtk_widget_show(g_refresh_rate_item);
    }

    // Power Profiles items
    {
        for (int profile = Low_Power; profile <= Performance; profile++) {

            char profile_item_lbl[32];
            strcpy(profile_item_lbl, power_profile_name((Power_Profile) profile));

            auto profile_item = gtk_image_menu_item_new_with_label(profile_item_lbl);
            GtkWidget* profile_icon;
            switch (profile) {
                // TODO: add custom icons for these.
                case Low_Power:
                    profile_icon = gtk_image_new_from_icon_name("security-high", GTK_ICON_SIZE_MENU);
                    break;
                case Balanced:
                    profile_icon = gtk_image_new_from_icon_name("security-medium", GTK_ICON_SIZE_MENU);
                    break;
                case Performance:
                    profile_icon = gtk_image_new_from_icon_name("security-low", GTK_ICON_SIZE_MENU);
                    break;
                default:
                    assert(false); // It should never go here.
            }
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(profile_item), profile_icon);
            g_signal_connect(G_OBJECT(profile_item), "activate", G_CALLBACK(change_profile), (gpointer) &g_profile_values[profile]);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), profile_item);

            if (profile == g_active_profile)
                gtk_widget_set_sensitive(profile_item, FALSE);

            g_profile_items[profile] = profile_item;
            gtk_widget_show(profile_item);
        }
    }

    // Battery Conservation item
    {
        g_battery_con_item = gtk_image_menu_item_new();
        auto battery_icon = gtk_image_new_from_icon_name("battery", GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(g_battery_con_item), battery_icon);
        g_signal_connect(G_OBJECT(g_battery_con_item), "activate", G_CALLBACK(toggle_battery_conservation), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), g_battery_con_item);
        gtk_widget_show(g_battery_con_item);
    }

    // Reload item
    {
        auto reload_item = gtk_image_menu_item_new_with_label("Reload");
        auto reload_icon = gtk_image_new_from_icon_name("view-refresh", GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(reload_item), reload_icon);
        g_signal_connect(G_OBJECT(reload_item), "activate", G_CALLBACK(reload_callback), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), reload_item);
        gtk_widget_show(reload_item);
    }

    // Quit item
    {
        auto quit_item = gtk_image_menu_item_new_with_label("Quit");
        auto quit_icon = gtk_image_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(quit_item), quit_icon);
        g_signal_connect(quit_item, "activate", G_CALLBACK(quit), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit_item);
        gtk_widget_show(quit_item);
    }
    app_indicator_set_menu(g_tray, GTK_MENU(menu));

    // Load all ideapad profile data.
    // Set menu item labels.
    reload(true);


    gtk_main();

    return 0;
}

