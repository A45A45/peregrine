#include "bookmarks.h"
#include "config.h"
#include <gtk/gtk.h>
#include <sqlite3.h>
#include <glib/gstdio.h>

static sqlite3 *s_db = NULL;

void bookmarks_init(void) {
    g_autofree gchar *data_dir = g_build_filename(g_get_user_data_dir(), "peregrine", NULL);
    g_mkdir_with_parents(data_dir, 0700);
    g_autofree gchar *db_path = g_build_filename(data_dir, "bookmarks.sqlite", NULL);

    if (sqlite3_open(db_path, &s_db) != SQLITE_OK) {
        g_warning("bookmarks: failed to open database: %s", s_db ? sqlite3_errmsg(s_db) : "unknown error");
        if (s_db) sqlite3_close(s_db);
        s_db = NULL;
        return;
    }

    const char *create_sql =
        "CREATE TABLE IF NOT EXISTS bookmarks ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  url TEXT NOT NULL UNIQUE,"
        "  title TEXT,"
        "  added_at INTEGER NOT NULL"
        ");";

    char *err = NULL;
    if (sqlite3_exec(s_db, create_sql, NULL, NULL, &err) != SQLITE_OK) {
        g_warning("bookmarks: failed to create schema: %s", err ? err : "unknown error");
        sqlite3_free(err);
    }
}

static gboolean restore_title(gpointer data) {
    GtkWindow *window = GTK_WINDOW(data);
    gtk_window_set_title(window, PEREGRINE_DEFAULT_TITLE);
    g_object_unref(window);
    return G_SOURCE_REMOVE;
}

static void flash_message(GtkWidget *widget, const char *msg) {
    GtkRoot *root = gtk_widget_get_root(widget);
    if (!root || !GTK_IS_WINDOW(root)) return;
    gtk_window_set_title(GTK_WINDOW(root), msg);
    g_timeout_add(2000, restore_title, g_object_ref(root));
}

gboolean bookmarks_add_current(WebKitWebView *web_view) {
    if (!web_view) return FALSE;
    const char *uri = webkit_web_view_get_uri(web_view);
    const char *title = webkit_web_view_get_title(web_view);

    if (!s_db || !uri || !*uri || g_str_equal(uri, "about:blank")) {
        flash_message(GTK_WIDGET(web_view), "Nothing to bookmark");
        return FALSE;
    }

    const char *sql =
        "INSERT INTO bookmarks (url, title, added_at) VALUES (?, ?, ?) "
        "ON CONFLICT(url) DO UPDATE SET title = excluded.title;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(s_db, sql, -1, &stmt, NULL) != SQLITE_OK) return FALSE;

    sqlite3_bind_text(stmt, 1, uri, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, title ? title : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)(g_get_real_time() / 1000000));

    gboolean ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (ok) {
        g_autofree gchar *msg = g_strdup_printf("Bookmarked: %s", (title && *title) ? title : uri);
        flash_message(GTK_WIDGET(web_view), msg);
    }
    return ok;
}

gboolean bookmarks_remove_current(WebKitWebView *web_view) {
    if (!web_view || !s_db) return FALSE;
    const char *uri = webkit_web_view_get_uri(web_view);
    if (!uri || !*uri) return FALSE;

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(s_db, "DELETE FROM bookmarks WHERE url = ?;", -1, &stmt, NULL) != SQLITE_OK) return FALSE;
    sqlite3_bind_text(stmt, 1, uri, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    gboolean removed = sqlite3_changes(s_db) > 0;
    flash_message(GTK_WIDGET(web_view), removed ? "Bookmark removed" : "Page isn't bookmarked");
    return removed;
}

gchar *bookmarks_build_page(const char *query) {
    GString *html = g_string_new(
        "<!DOCTYPE html><html><head><meta charset='utf-8'>"
        "<title>Bookmarks</title>"
        "<style>"
        ":root { color-scheme: light dark; }"
        "body { font-family: sans-serif; max-width: 700px; margin: 2em auto; }"
        "a { display: block; padding: 6px 0; text-decoration: none; }"
        "small { opacity: 0.6; display: block; }"
        "</style></head><body><h2>Bookmarks</h2>"
    );

    if (!s_db) {
        g_string_append(html, "<p>Bookmarks database unavailable.</p></body></html>");
        return g_string_free(html, FALSE);
    }

    gboolean has_query = query && *query;
    const char *sql = has_query
        ? "SELECT url, title FROM bookmarks WHERE url LIKE ?1 OR title LIKE ?1 ORDER BY added_at DESC LIMIT 500;"
        : "SELECT url, title FROM bookmarks ORDER BY added_at DESC LIMIT 500;";

    int count = 0;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(s_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (has_query) {
            g_autofree gchar *pattern = g_strdup_printf("%%%s%%", query);
            sqlite3_bind_text(stmt, 1, pattern, -1, SQLITE_TRANSIENT);
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *url = (const char *)sqlite3_column_text(stmt, 0);
            const char *title = (const char *)sqlite3_column_text(stmt, 1);
            g_autofree gchar *esc_url = g_markup_escape_text(url ? url : "", -1);
            g_autofree gchar *esc_title = g_markup_escape_text((title && *title) ? title : (url ? url : ""), -1);
            g_string_append_printf(html, "<a href=\"%s\">%s<small>%s</small></a>", esc_url, esc_title, esc_url);
            count++;
        }
        sqlite3_finalize(stmt);
    }

    if (count == 0) {
        g_string_append(html, has_query
            ? "<p>No bookmarks match that search.</p>"
            : "<p>No bookmarks yet. Press <b>B</b> on a page or use <code>:bm</code>.</p>");
    }

    g_string_append(html, "</body></html>");
    return g_string_free(html, FALSE);
}
