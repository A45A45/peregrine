#ifndef VIM_BINDINGS_H
#define VIM_BINDINGS_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>

gboolean vim_bindings_handle_key(GtkNotebook *notebook, GtkWidget *command_bar, guint keyval, GdkModifierType state_mask);

#endif // VIM_BINDINGS_H
