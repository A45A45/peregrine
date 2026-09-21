#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include "command_bar.h"

static void activate(GtkApplication *app, gpointer user_data) {
    const char *url = (const char *)user_data;

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Peregrine");
    gtk_window_set_default_size(GTK_WINDOW(window), 1024, 768);

    GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), root_box);

    GtkWidget *web_view = webkit_web_view_new();
    gtk_widget_set_vexpand(web_view, TRUE);
    gtk_box_append(GTK_BOX(root_box), web_view);

    command_bar_init(window, web_view, root_box);

    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(web_view), url);
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
