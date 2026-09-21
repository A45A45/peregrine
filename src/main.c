#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include "tab_manager.h"
#include "command_bar.h"

static void activate(GtkApplication *app, gpointer user_data) {
    const char *url = (const char *)user_data;

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Peregrine");
    gtk_window_set_default_size(GTK_WINDOW(window), 1920, 1080);

    GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), root_box);

    GtkWidget *notebook = tab_manager_create_notebook();
    gtk_box_append(GTK_BOX(root_box), notebook);

    tab_manager_add_tab(notebook, url);
    command_bar_init(window, notebook, root_box);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    const char *url = (argc > 1) ? argv[1] : "https://webkitgtk.org";

    GtkApplication *app = gtk_application_new("org.peregrine.browser", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), (gpointer)url);

    int status = g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);
    return status;
}
