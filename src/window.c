#include "window.h"
#include "tab.h"
#include "command.h"
#include "config.h"

GtkWidget *window_create(GtkApplication *app, const char *initial_url) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), PEREGRINE_DEFAULT_TITLE);
    gtk_window_set_default_size(GTK_WINDOW(window), 1024, 768);

    GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), root_box);

    GtkWidget *notebook = tab_manager_create_notebook();
    gtk_box_append(GTK_BOX(root_box), notebook);

    AppState *state = command_bar_init(window, notebook, root_box);

    tab_manager_add_tab(notebook, initial_url, G_CALLBACK(on_key_pressed), state);

    return window;
}
