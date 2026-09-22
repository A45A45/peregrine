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

AppState *command_bar_init(GtkWidget *window, GtkWidget *notebook, GtkWidget *root_box);
gboolean on_key_pressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state_mask, gpointer user_data);

#endif // COMMAND_BAR_H
