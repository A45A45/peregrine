#ifndef WINDOW_H
#define WINDOW_H

#include <gtk/gtk.h>

GtkWidget *window_create(GtkApplication *app, const char *initial_url);

#endif // WINDOW_H
