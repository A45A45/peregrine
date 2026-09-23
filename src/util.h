#ifndef UTIL_H
#define UTIL_H

#include <glib.h>

char *util_normalize_url(const char *url);
gboolean util_is_bare_url(const char *text);

#endif // UTIL_H
