#include "adblock.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>

typedef struct {
    gchar *identifier;
    gint64 mtime;
} FilterSaveCtx;

static gchar *mtimes_path(void) {
    return g_build_filename(g_get_user_data_dir(), "peregrine", "filter_mtimes.ini", NULL);
}

static void record_mtime(const char *identifier, gint64 mtime) {
    g_autofree gchar *path = mtimes_path();
    GKeyFile *kf = g_key_file_new();
    g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL);
    g_autofree gchar *mtime_str = g_strdup_printf("%" G_GINT64_FORMAT, mtime);
    g_key_file_set_string(kf, "mtimes", identifier, mtime_str);
    g_key_file_save_to_file(kf, path, NULL);
    g_key_file_free(kf);
}

static gboolean mtime_unchanged(const char *identifier, gint64 mtime) {
    g_autofree gchar *path = mtimes_path();
    GKeyFile *kf = g_key_file_new();
    gboolean unchanged = FALSE;
    if (g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL)) {
        g_autofree gchar *stored = g_key_file_get_string(kf, "mtimes", identifier, NULL);
        if (stored) {
            unchanged = (g_ascii_strtoll(stored, NULL, 10) == mtime);
        }
    }
    g_key_file_free(kf);
    return unchanged;
}

static WebKitUserContentFilterStore *s_filter_store = NULL;
static GPtrArray *s_native_filters = NULL;

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
    FilterSaveCtx *ctx = (FilterSaveCtx *)user_data;
    GError *error = NULL;
    WebKitUserContentFilter *filter = webkit_user_content_filter_store_save_finish(store, result, &error);
    if (filter) {
        g_ptr_array_add(s_native_filters, filter);
        record_mtime(ctx->identifier, ctx->mtime);
    } else {
        if (error) {
            g_warning("adblock: failed to compile native filter '%s': %s", ctx->identifier, error->message);
            g_error_free(error);
        }
        webkit_user_content_filter_store_load(store, ctx->identifier, NULL, (GAsyncReadyCallback)on_filter_loaded, NULL);
    }
    g_free(ctx->identifier);
    g_free(ctx);
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

            GStatBuf stat_buf;
            gint64 mtime = (g_stat(json_path, &stat_buf) == 0) ? (gint64)stat_buf.st_mtime : 0;

            if (mtime_unchanged(identifier, mtime)) {
                webkit_user_content_filter_store_load(s_filter_store, identifier, NULL, (GAsyncReadyCallback)on_filter_loaded, NULL);
                continue;
            }

            g_autofree gchar *contents = NULL;
            gsize length = 0;
            GError *error = NULL;

            if (g_file_get_contents(json_path, &contents, &length, &error)) {
                GBytes *data = g_bytes_new_take(contents, length);
                contents = NULL;
                FilterSaveCtx *ctx = g_new0(FilterSaveCtx, 1);
                ctx->identifier = g_strdup(identifier);
                ctx->mtime = mtime;
                webkit_user_content_filter_store_save(
                    s_filter_store,
                    identifier,
                    data,
                    NULL,
                    (GAsyncReadyCallback)on_filter_saved,
                    ctx
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
    if (!s_native_filters) {
        s_native_filters = g_ptr_array_new_with_free_func((GDestroyNotify)webkit_user_content_filter_unref);
    }
    load_native_filters();
}

void adblock_reload_filters(void) {
    if (s_native_filters) {
        g_ptr_array_set_size(s_native_filters, 0);
    }
    load_native_filters();
}

void adblock_attach_filters(WebKitUserContentManager *ucm) {
    if (!s_native_filters) return;
    for (guint i = 0; i < s_native_filters->len; i++) {
        webkit_user_content_manager_add_filter(ucm, g_ptr_array_index(s_native_filters, i));
    }
}
