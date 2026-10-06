#include "storage.h"
#include <string.h>

WebKitWebsiteDataManager *storage_get_website_data_manager(void) {
    WebKitNetworkSession *default_session = webkit_network_session_get_default();
    return webkit_network_session_get_website_data_manager(default_session);
}

WebKitWebContext *storage_get_web_context(void) {
    return webkit_web_context_get_default();
}

void storage_init_persistent(void) {
    WebKitNetworkSession *session = webkit_network_session_get_default();
    WebKitCookieManager *cookie_manager = webkit_network_session_get_cookie_manager(session);

    g_autofree gchar *data_dir = g_build_filename(g_get_user_data_dir(), "peregrine", NULL);
    g_mkdir_with_parents(data_dir, 0700);
    g_autofree gchar *cookie_file = g_build_filename(data_dir, "cookies.sqlite", NULL);

    webkit_cookie_manager_set_persistent_storage(
        cookie_manager, cookie_file, WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);

    webkit_cookie_manager_set_accept_policy(
        cookie_manager, WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY);
}

static gchar *unique_destination_path(const char *dir, const char *filename) {
    g_autofree gchar *base_path = g_build_filename(dir, filename, NULL);
    if (!g_file_test(base_path, G_FILE_TEST_EXISTS)) {
        return g_steal_pointer(&base_path);
    }

    const char *dot = strrchr(filename, '.');
    g_autofree gchar *stem = dot ? g_strndup(filename, dot - filename) : g_strdup(filename);
    const char *ext = dot ? dot : "";

    for (int i = 1; i < 1000; i++) {
        g_autofree gchar *candidate_name = g_strdup_printf("%s (%d)%s", stem, i, ext);
        gchar *candidate_path = g_build_filename(dir, candidate_name, NULL);
        if (!g_file_test(candidate_path, G_FILE_TEST_EXISTS)) {
            return candidate_path;
        }
        g_free(candidate_path);
    }
    return g_build_filename(dir, filename, NULL); /* give up, allow overwrite */
}

static gboolean on_decide_destination(WebKitDownload *download, const gchar *suggested_filename, gpointer user_data) {
    const gchar *downloads_dir = g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD);
    if (!downloads_dir) downloads_dir = g_get_home_dir();
    g_mkdir_with_parents(downloads_dir, 0700);

    g_autofree gchar *dest_path = unique_destination_path(downloads_dir, suggested_filename);
    g_autofree gchar *uri = g_filename_to_uri(dest_path, NULL, NULL);
    if (uri) {
        webkit_download_set_destination(download, uri);
    }
    return TRUE;
}

static void on_download_started(WebKitNetworkSession *session, WebKitDownload *download, gpointer user_data) {
    g_signal_connect(download, "decide-destination", G_CALLBACK(on_decide_destination), NULL);
}

void storage_init_downloads(void) {
    WebKitNetworkSession *session = webkit_network_session_get_default();
    g_signal_connect(session, "download-started", G_CALLBACK(on_download_started), NULL);
}
