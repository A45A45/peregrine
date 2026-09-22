#include "utils.h"

gchar *utils_ensure_url_scheme(const char *url) {
    if (!url || *url == '\0') {
        return NULL;
    }
    while (*url == ' ') {
        url++;
    }
    if (*url == '\0') {
        return NULL;
    }
    if (g_str_has_prefix(url, "http://") || g_str_has_prefix(url, "https://")) {
        return g_strdup(url);
    }
    return g_strconcat("https://", url, NULL);
}
