#ifndef KEYS_H
#define KEYS_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>

gboolean keys_handle_key(GtkNotebook *notebook, GtkWidget *command_bar, guint keyval, GdkModifierType state_mask);

#endif // KEYS_H
