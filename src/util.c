#include "util.h"
#include "config.h"
#include <string.h>

char *util_normalize_url(const char *url) {
    if (!url || *url == '\0') {
        return g_strdup(PEREGRINE_FALLBACK_URL);
    }
    if (g_str_has_prefix(url, "http://") || 
        g_str_has_prefix(url, "https://") || 
        g_str_has_prefix(url, "file://")) {
        return g_strdup(url);
    }
    return g_strconcat("https://", url, NULL);
}

gboolean util_is_bare_url(const char *text) {
    if (!text || *text == '\0') return FALSE;
    if (g_str_has_prefix(text, "http://") || 
        g_str_has_prefix(text, "https://") || 
        g_str_has_prefix(text, "file://") ||
        g_str_has_prefix(text, "www.")) {
        return TRUE;
    }
    if (strchr(text, '.') != NULL && strchr(text, ' ') == NULL) {
        return TRUE;
    }
    return FALSE;
}
