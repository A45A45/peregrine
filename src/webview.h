#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>

GtkWidget *webview_create(void);
gboolean webview_is_editable_focused(WebKitWebView *web_view);

#endif // WEBVIEW_H
