#ifndef TAB_H
#define TAB_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>

GtkWidget *tab_manager_create_notebook(void);
GtkWidget *tab_manager_add_tab(GtkWidget *notebook, const char *url, GCallback key_press_cb, gpointer user_data);
void tab_manager_close_current_tab(GtkWidget *notebook, GtkWidget *window);
WebKitWebView *tab_manager_get_active_web_view(GtkNotebook *notebook);
gboolean tab_manager_is_editable_focused(GtkNotebook *notebook);

#endif // TAB_H
