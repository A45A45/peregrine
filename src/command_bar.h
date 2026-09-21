#ifndef COMMAND_BAR_H
#define COMMAND_BAR_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *command_bar;
    GtkWidget *entry;
} AppState;

GtkWidget *app_add_tab(GtkWidget *notebook, const char *url);
AppState *command_bar_init(GtkWidget *window, GtkWidget *notebook, GtkWidget *root_box);

#endif // COMMAND_BAR_H
