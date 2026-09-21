#include "command_bar.h"

static void on_entry_activate(GtkEntry *entry, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));

    if (text && *text != '\0') {
        const char *url_part = NULL;
        if (g_str_has_prefix(text, "open ")) {
            url_part = text + 5;
        } else if (g_str_has_prefix(text, "o ")) {
            url_part = text + 2;
        } else if (g_str_equal(text, "open") || g_str_equal(text, "o")) {
            url_part = "";
        }

        if (url_part) {
            while (*url_part == ' ') {
                url_part++;
            }
            if (*url_part != '\0') {
                g_autofree gchar *final_url = NULL;
                if (g_str_has_prefix(url_part, "http://") || g_str_has_prefix(url_part, "https://")) {
                    final_url = g_strdup(url_part);
                } else {
                    final_url = g_strconcat("https://", url_part, NULL);
                }
                webkit_web_view_load_uri(WEBKIT_WEB_VIEW(state->web_view), final_url);
            }
        }
    }

    gtk_editable_set_text(GTK_EDITABLE(entry), "");
    gtk_widget_set_visible(state->command_bar, FALSE);
    gtk_widget_grab_focus(state->web_view);
}

static gboolean on_key_pressed(GtkEventControllerKey *controller,
                               guint keyval,
                               guint keycode,
                               GdkModifierType state_mask,
                               gpointer user_data) {
    AppState *state = (AppState *)user_data;
    gboolean bar_visible = gtk_widget_get_visible(state->command_bar);

    if (!bar_visible) {
        if (keyval == GDK_KEY_colon) {
            gtk_widget_set_visible(state->command_bar, TRUE);
            gtk_widget_grab_focus(state->entry);
            gtk_editable_set_text(GTK_EDITABLE(state->entry), "");
            return TRUE;
        }
    } else {
        if (keyval == GDK_KEY_Escape) {
            gtk_editable_set_text(GTK_EDITABLE(state->entry), "");
            gtk_widget_set_visible(state->command_bar, FALSE);
            gtk_widget_grab_focus(state->web_view);
            return TRUE;
        }
    }
    return FALSE;
}

AppState *command_bar_init(GtkWidget *window, GtkWidget *web_view, GtkWidget *root_box) {
    GtkWidget *command_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(command_bar, 8);
    gtk_widget_set_margin_end(command_bar, 8);
    gtk_widget_set_margin_top(command_bar, 6);
    gtk_widget_set_margin_bottom(command_bar, 6);
    gtk_widget_set_visible(command_bar, FALSE);

    GtkWidget *prompt_label = gtk_label_new(":");
    gtk_box_append(GTK_BOX(command_bar), prompt_label);

    GtkWidget *entry = gtk_entry_new();
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_box_append(GTK_BOX(command_bar), entry);

    gtk_box_append(GTK_BOX(root_box), command_bar);

    AppState *state = g_new0(AppState, 1);
    state->window = window;
    state->web_view = web_view;
    state->command_bar = command_bar;
    state->entry = entry;

    g_signal_connect_swapped(window, "destroy", G_CALLBACK(g_free), state);
    g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(on_entry_activate), state);

    GtkEventController *key_controller = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(key_controller, GTK_PHASE_CAPTURE);
    g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_key_pressed), state);
    gtk_widget_add_controller(window, key_controller);

    return state;
}
