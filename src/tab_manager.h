#ifndef TAB_MANAGER_H
#define TAB_MANAGER_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>

GtkWidget *tab_manager_create_notebook(void);
GtkWidget *tab_manager_add_tab(GtkWidget *notebook, const char *url);
void tab_manager_close_current_tab(GtkWidget *notebook, GtkWidget *window);

#endif // TAB_MANAGER_H
