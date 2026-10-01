#include "adblock.h"
#include "adblock_c.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>

/* ---- Brave (adblock-rust) engine: EasyList .txt, navigation-level blocking ---- */

static void load_easylist_filters(void) {
    g_autofree gchar *config_dir = g_build_filename(g_get_user_config_dir(), "peregrine", "filters", NULL);
    g_mkdir_with_parents(config_dir, 0700);

    GPtrArray *lists_array = g_ptr_array_new_with_free_func(g_free);

    GDir *dir = g_dir_open(config_dir, 0, NULL);
    if (dir) {
        const gchar *filename;
        while ((filename = g_dir_read_name(dir)) != NULL) {
            if (g_str_has_suffix(filename, ".txt")) {
                g_autofree gchar *file_path = g_build_filename(config_dir, filename, NULL);
                gchar *contents = NULL;
                GError *error = NULL;
                if (g_file_get_contents(file_path, &contents, NULL, &error)) {
                    g_ptr_array_add(lists_array, contents);
                } else if (error) {
                    g_warning("adblock: failed to read '%s': %s", file_path, error->message);
                    g_error_free(error);
                }
            }
        }
        g_dir_close(dir);
    }

    if (lists_array->len > 0) {
        adblock_init_engine((const char **)lists_array->pdata, lists_array->len);
    }

    g_ptr_array_free(lists_array, TRUE);
}

/* ---- WebKit native content filters: .json, real per-resource blocking ---- */

static WebKitUserContentFilterStore *s_filter_store = NULL;
static GPtrArray *s_native_filters = NULL; /* owns WebKitUserContentFilter refs */

static void on_filter_loaded(WebKitUserContentFilterStore *store, GAsyncResult *result, gpointer user_data) {
    GError *error = NULL;
    WebKitUserContentFilter *filter = webkit_user_content_filter_store_load_finish(store, result, &error);
    if (filter) {
        g_ptr_array_add(s_native_filters, filter);
    } else if (error) {
        g_warning("adblock: failed to load native filter: %s", error->message);
        g_error_free(error);
    }
}

static void on_filter_saved(WebKitUserContentFilterStore *store, GAsyncResult *result, gpointer user_data) {
    g_autofree gchar *identifier = (gchar *)user_data;
    GError *error = NULL;
    WebKitUserContentFilter *filter = webkit_user_content_filter_store_save_finish(store, result, &error);
    if (filter) {
        g_ptr_array_add(s_native_filters, filter);
    } else {
        if (error) {
            g_warning("adblock: failed to compile native filter '%s': %s", identifier, error->message);
            g_error_free(error);
        }
        if (identifier) {
            webkit_user_content_filter_store_load(store, identifier, NULL, (GAsyncReadyCallback)on_filter_loaded, NULL);
        }
    }
}

static void load_native_filters(void) {
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

/* ---- Public API ---- */

void adblock_init(void) {
    if (!s_native_filters) {
        s_native_filters = g_ptr_array_new_with_free_func((GDestroyNotify)webkit_user_content_filter_unref);
    }
    load_easylist_filters();
    load_native_filters();
}

void adblock_reload_filters(void) {
    if (s_native_filters) {
        g_ptr_array_set_size(s_native_filters, 0);
    }
    load_easylist_filters();
    load_native_filters();
    /* Note: tabs already open keep their old native filters until reopened. */
}

void adblock_attach_filters(WebKitUserContentManager *ucm) {
    if (!s_native_filters) return;
    for (guint i = 0; i < s_native_filters->len; i++) {
        webkit_user_content_manager_add_filter(ucm, g_ptr_array_index(s_native_filters, i));
    }
}
