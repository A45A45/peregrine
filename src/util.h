#ifndef UTIL_H
#define UTIL_H

#include <glib.h>

char *util_normalize_url(const char *url);
gboolean util_is_bare_url(const char *text);
char *util_build_search_url(const char *query);

#endif // UTIL_H
