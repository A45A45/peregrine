#include "command_bar.h"
#include "tab_manager.h"
#include "vim_bindings.h"

static WebKitWebView *get_current_web_view(GtkNotebook *notebook) {
    int current_page = gtk_notebook_get_current_page(notebook);
    if (current_page < 0) return NULL;
    GtkWidget *child = gtk_notebook_get_nth_page(notebook, current_page);
    return WEBKIT_WEB_VIEW(child);
}

static void on_entry_activate(GtkEntry *entry, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));

    if (text && *text != '\0') {
        if (g_str_has_prefix(text, "open ")) {
            const char *url_part = text + 5;
            while (*url_part == ' ') url_part++;
            if (*url_part != '\0') {
                g_autofree gchar *final_url = NULL;
                if (g_str_has_prefix(url_part, "http://") || g_str_has_prefix(url_part, "https://")) {
                    final_url = g_strdup(url_part);
                } else {
                    final_url = g_strconcat("https://", url_part, NULL);
                }
                WebKitWebView *current_wv = get_current_web_view(GTK_NOTEBOOK(state->notebook));
                if (current_wv) {
                    webkit_web_view_load_uri(current_wv, final_url);
                }
            }
        } else if (g_str_has_prefix(text, "o ")) {
            const char *url_part = text + 2;
            while (*url_part == ' ') url_part++;
            if (*url_part != '\0') {
                g_autofree gchar *final_url = NULL;
                if (g_str_has_prefix(url_part, "http://") || g_str_has_prefix(url_part, "https://")) {
                    final_url = g_strdup(url_part);
                } else {
                    final_url = g_strconcat("https://", url_part, NULL);
                }
                WebKitWebView *current_wv = get_current_web_view(GTK_NOTEBOOK(state->notebook));
                if (current_wv) {
                    webkit_web_view_load_uri(current_wv, final_url);
                }
            }
        } else if (g_str_has_prefix(text, "newtab ")) {
            const char *url_part = text + 7;
            while (*url_part == ' ') url_part++;
            tab_manager_add_tab(state->notebook, *url_part != '\0' ? url_part : "https://webkitgtk.org");
        } else if (g_str_has_prefix(text, "nt ")) {
            const char *url_part = text + 3;
            while (*url_part == ' ') url_part++;
            tab_manager_add_tab(state->notebook, *url_part != '\0' ? url_part : "https://webkitgtk.org");
        } else if (g_str_equal(text, "newtab") || g_str_equal(text, "nt")) {
            tab_manager_add_tab(state->notebook, "https://webkitgtk.org");
        } else if (g_str_equal(text, "closetab") || g_str_equal(text, "close") || g_str_equal(text, "ct")) {
            tab_manager_close_current_tab(state->notebook, state->window);
        }
    }

    gtk_editable_set_text(GTK_EDITABLE(entry), "");
    gtk_widget_set_visible(state->command_bar, FALSE);
    WebKitWebView *current_wv = get_current_web_view(GTK_NOTEBOOK(state->notebook));
    if (current_wv) {
        gtk_widget_grab_focus(GTK_WIDGET(current_wv));
    }
}

static gboolean on_key_pressed(GtkEventControllerKey *controller,
                               guint keyval,
                               guint keycode,
                               GdkModifierType state_mask,
                               gpointer user_data) {
    AppState *state = (AppState *)user_data;
    gboolean bar_visible = gtk_widget_get_visible(state->command_bar);

    if (!bar_visible) {
        if ((state_mask & GDK_CONTROL_MASK) != 0) {
            if (keyval == GDK_KEY_t || keyval == GDK_KEY_T) {
                tab_manager_add_tab(state->notebook, "https://webkitgtk.org");
                return TRUE;
            } else if (keyval == GDK_KEY_w || keyval == GDK_KEY_W) {
                tab_manager_close_current_tab(state->notebook, state->window);
                return TRUE;
            }
        }

        if (keyval == GDK_KEY_colon) {
            gtk_widget_set_visible(state->command_bar, TRUE);
            gtk_widget_grab_focus(state->entry);
            gtk_editable_set_text(GTK_EDITABLE(state->entry), "");
            return TRUE;
        }

        if (vim_bindings_handle_key(GTK_NOTEBOOK(state->notebook), state->command_bar, keyval, state_mask)) {
            return TRUE;
        }
    } else {
        if (keyval == GDK_KEY_Escape) {
            gtk_editable_set_text(GTK_EDITABLE(state->entry), "");
            gtk_widget_set_visible(state->command_bar, FALSE);
            WebKitWebView *current_wv = get_current_web_view(GTK_NOTEBOOK(state->notebook));
            if (current_wv) {
                gtk_widget_grab_focus(GTK_WIDGET(current_wv));
            }
            return TRUE;
        }
    }
    return FALSE;
}

AppState *command_bar_init(GtkWidget *window, GtkWidget *notebook, GtkWidget *root_box) {
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
    state->notebook = notebook;
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
