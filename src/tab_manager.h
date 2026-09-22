#ifndef TAB_MANAGER_H
#define TAB_MANAGER_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>

GtkWidget *tab_manager_create_notebook(void);
GtkWidget *tab_manager_add_tab(GtkWidget *notebook, const char *url, GCallback key_press_cb, gpointer user_data);
void tab_manager_close_current_tab(GtkWidget *notebook, GtkWidget *window);
gboolean tab_manager_is_editable_focused(GtkNotebook *notebook);
char *tab_manager_normalize_url(const char *url);

#endif // TAB_MANAGER_H
