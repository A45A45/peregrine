#include <gtk/gtk.h>
#include "app.h"
#include "adblock.h"
#include "config.h"

int main(int argc, char **argv) {
    const char *url = (argc > 1) ? argv[1] : PEREGRINE_DEFAULT_URL;

    adblock_init();

    GtkApplication *app = gtk_application_new("org.peregrine.browser", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate), (gpointer)url);

    int status = g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);
    return status;
}
