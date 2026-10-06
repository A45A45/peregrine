#include "history.h"
#include <sqlite3.h>
#include <glib/gstdio.h>

static sqlite3 *s_db = NULL;

void history_init(void) {
    g_autofree gchar *data_dir = g_build_filename(g_get_user_data_dir(), "peregrine", NULL);
    g_mkdir_with_parents(data_dir, 0700);
    g_autofree gchar *db_path = g_build_filename(data_dir, "history.sqlite", NULL);

    if (sqlite3_open(db_path, &s_db) != SQLITE_OK) {
        g_warning("history: failed to open database: %s", s_db ? sqlite3_errmsg(s_db) : "unknown error");
        if (s_db) sqlite3_close(s_db);
        s_db = NULL;
        return;
    }

    const char *create_sql =
        "CREATE TABLE IF NOT EXISTS history ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  url TEXT NOT NULL,"
        "  title TEXT,"
        "  visited_at INTEGER NOT NULL"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_history_visited_at ON history(visited_at);";

    char *err = NULL;
    if (sqlite3_exec(s_db, create_sql, NULL, NULL, &err) != SQLITE_OK) {
        g_warning("history: failed to create schema: %s", err ? err : "unknown error");
        sqlite3_free(err);
    }
}

void history_record(const char *url, const char *title) {
    if (!s_db || !url || !*url) return;

    const char *sql = "INSERT INTO history (url, title, visited_at) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(s_db, sql, -1, &stmt, NULL) != SQLITE_OK) return;

    sqlite3_bind_text(stmt, 1, url, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, title ? title : "", -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)(g_get_real_time() / 1000000));

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

gchar *history_build_page(const char *query) {
    GString *html = g_string_new(
        "<!DOCTYPE html><html><head><meta charset='utf-8'>"
        "<title>History</title>"
        "<style>"
        "body { font-family: sans-serif; max-width: 700px; margin: 2em auto; }"
        "a { display: block; padding: 6px 0; text-decoration: none; color: #1a0dab; }"
        "small { color: #666; display: block; }"
        "</style></head><body><h2>History</h2>"
    );

    if (!s_db) {
        g_string_append(html, "<p>History database unavailable.</p></body></html>");
        return g_string_free(html, FALSE);
    }

    sqlite3_stmt *stmt = NULL;
    gboolean has_query = query && *query;
    const char *sql = has_query
        ? "SELECT url, title FROM history WHERE url LIKE ?1 OR title LIKE ?1 ORDER BY visited_at DESC LIMIT 100;"
        : "SELECT url, title FROM history ORDER BY visited_at DESC LIMIT 100;";

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
        }
        sqlite3_finalize(stmt);
    }

    g_string_append(html, "</body></html>");
    return g_string_free(html, FALSE);
}
