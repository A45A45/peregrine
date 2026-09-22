#include "command_bar.h"
#include "tab_manager.h"
#include "vim_bindings.h"
#include "utils.h"

#define DEFAULT_HOME_URL "https://webkitgtk.org"

static WebKitWebView *get_active_web_view(GtkNotebook *notebook) {
    int current_page = gtk_notebook_get_current_page(notebook);
    if (current_page < 0) return NULL;
    GtkWidget *child = gtk_notebook_get_nth_page(notebook, current_page);
    return WEBKIT_WEB_VIEW(child);
}

static void hide_command_bar_and_focus_web_view(AppState *state) {
    gtk_editable_set_text(GTK_EDITABLE(state->entry), "");
    gtk_widget_set_visible(state->command_bar, FALSE);
    WebKitWebView *current_wv = get_active_web_view(GTK_NOTEBOOK(state->notebook));
    if (current_wv) {
        gtk_widget_grab_focus(GTK_WIDGET(current_wv));
    }
}

static const char *extract_arg(const char *text, const char *prefix) {
    size_t len = strlen(prefix);
    if (strncmp(text, prefix, len) != 0) return NULL;
    const char *arg = text + len;
    while (*arg == ' ') arg++;
    return arg;
}

static void handle_open_command(AppState *state, const char *url_part) {
    g_autofree gchar *final_url = utils_ensure_url_scheme(url_part);
    if (final_url) {
        WebKitWebView *current_wv = get_active_web_view(GTK_NOTEBOOK(state->notebook));
        if (current_wv) {
            webkit_web_view_load_uri(current_wv, final_url);
        }
    }
}

static void handle_newtab_command(AppState *state, const char *url_part) {
    const char *target_url = (url_part && *url_part != '\0') ? url_part : DEFAULT_HOME_URL;
    tab_manager_add_tab(state->notebook, target_url, G_CALLBACK(on_key_pressed), state);
}

static void on_entry_activate(GtkEntry *entry, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));

    if (text && *text != '\0') {
        const char *arg = NULL;
        if ((arg = extract_arg(text, "open ")) || (arg = extract_arg(text, "o "))) {
            if (*arg != '\0') {
                handle_open_command(state, arg);
            }
        } else if ((arg = extract_arg(text, "newtab ")) || (arg = extract_arg(text, "nt "))) {
            handle_newtab_command(state, arg);
        } else if (g_str_equal(text, "newtab") || g_str_equal(text, "nt")) {
            handle_newtab_command(state, NULL);
        } else if (g_str_equal(text, "closetab") || g_str_equal(text, "close") || g_str_equal(text, "ct")) {
            tab_manager_close_current_tab(state->notebook, state->window);
        }
    }

    hide_command_bar_and_focus_web_view(state);
}

gboolean on_key_pressed(GtkEventControllerKey *controller,
                        guint keyval,
                        guint keycode,
                        GdkModifierType state_mask,
                        gpointer user_data) {
    AppState *state = (AppState *)user_data;
    gboolean bar_visible = gtk_widget_get_visible(state->command_bar);

    if (!bar_visible) {
        if ((state_mask & GDK_CONTROL_MASK) != 0) {
            if (keyval == GDK_KEY_t || keyval == GDK_KEY_T) {
                tab_manager_add_tab(state->notebook, "https://webkitgtk.org", G_CALLBACK(on_key_pressed), state);
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
            hide_command_bar_and_focus_web_view(state);
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

    return state;
}
