#include "adblock.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>

static WebKitUserContentFilterStore *s_filter_store = NULL;
static GPtrArray *s_filters = NULL;

static void on_filter_loaded(WebKitUserContentFilterStore *store, GAsyncResult *result, gpointer user_data) {
    GError *error = NULL;
    WebKitUserContentFilter *filter = webkit_user_content_filter_store_load_finish(store, result, &error);
    if (filter) {
        g_ptr_array_add(s_filters, filter);
    } else if (error) {
        g_warning("adblock: failed to load filter: %s", error->message);
        g_error_free(error);
    }
}

static void on_filter_saved(WebKitUserContentFilterStore *store, GAsyncResult *result, gpointer user_data) {
    g_autofree gchar *identifier = (gchar *)user_data;
    GError *error = NULL;
    WebKitUserContentFilter *filter = webkit_user_content_filter_store_save_finish(store, result, &error);
    if (filter) {
        g_ptr_array_add(s_filters, filter);
    } else {
        if (error) {
            g_warning("adblock: failed to save filter '%s': %s", identifier, error->message);
            g_error_free(error);
        }
        if (identifier) {
            webkit_user_content_filter_store_load(store, identifier, NULL, (GAsyncReadyCallback)on_filter_loaded, NULL);
        }
    }
}

static void load_filters_from_dir(void) {
    g_autofree gchar *config_dir = g_build_filename(g_get_user_config_dir(), "peregrine", "filters", NULL);
    g_mkdir_with_parents(config_dir, 0700);

    if (!s_filter_store) {
        g_autofree gchar *store_path = g_build_filename(g_get_user_data_dir(), "peregrine", "filter_store", NULL);
        s_filter_store = webkit_user_content_filter_store_new(store_path);
    }

    GDir *dir = g_dir_open(config_dir, 0, NULL);
    if (!dir) return;

    const gchar *filename;
    while ((filename = g_dir_read_name(dir)) != NULL) {
        if (g_str_has_suffix(filename, ".json")) {
            g_autofree gchar *json_path = g_build_filename(config_dir, filename, NULL);
            g_autofree gchar *identifier = g_strdup(filename);
            char *ext = strrchr(identifier, '.');
            if (ext) *ext = '\0';

            g_autofree gchar *contents = NULL;
            gsize length = 0;
            GError *error = NULL;

            if (g_file_get_contents(json_path, &contents, &length, &error)) {
                GBytes *data = g_bytes_new_take(contents, length);
                contents = NULL;
                webkit_user_content_filter_store_save(
                    s_filter_store,
                    identifier,
                    data,
                    NULL,
                    (GAsyncReadyCallback)on_filter_saved,
                    g_strdup(identifier)
                );
                g_bytes_unref(data);
            } else if (error) {
                g_warning("adblock: failed to read '%s': %s", json_path, error->message);
                g_error_free(error);
            }
        }
    }
    g_dir_close(dir);
}

void adblock_init(void) {
    if (!s_filters) {
        s_filters = g_ptr_array_new_with_free_func((GDestroyNotify)webkit_user_content_filter_unref);
    }
    load_filters_from_dir();
}

void adblock_attach_filters(WebKitUserContentManager *ucm) {
    if (!s_filters) return;
    for (guint i = 0; i < s_filters->len; i++) {
        webkit_user_content_manager_add_filter(ucm, g_ptr_array_index(s_filters, i));
    }
}

void adblock_reload_filters(void) {
    if (s_filters) {
        g_ptr_array_set_size(s_filters, 0);
    }
    load_filters_from_dir();
}
