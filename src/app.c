#include "app.h"
#include "window.h"
#include "storage.h"

void app_activate(GtkApplication *app, gpointer user_data) {
    const char *url = (const char *)user_data;

    storage_init_persistent();

    GtkWidget *window = window_create(app, url);
    gtk_window_present(GTK_WINDOW(window));
}
