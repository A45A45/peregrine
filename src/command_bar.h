#ifndef COMMAND_BAR_H
#define COMMAND_BAR_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *web_view;
    GtkWidget *command_bar;
    GtkWidget *entry;
} AppState;

AppState *command_bar_init(GtkWidget *window, GtkWidget *web_view, GtkWidget *root_box);

#endif // COMMAND_BAR_H
