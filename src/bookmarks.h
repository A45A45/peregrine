#ifndef BOOKMARKS_H
#define BOOKMARKS_H

#include <glib.h>
#include <webkit/webkit.h>

void bookmarks_init(void);
gboolean bookmarks_add_current(WebKitWebView *web_view);
gboolean bookmarks_remove_current(WebKitWebView *web_view);
gchar *bookmarks_build_page(const char *query);

#endif // BOOKMARKS_H
