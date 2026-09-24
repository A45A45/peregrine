#include "storage.h"

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

    /* Cookies are NOT persisted by default, even on the default
     * (non-ephemeral) network session -- you have to opt in explicitly. */
    g_autofree gchar *data_dir = g_build_filename(g_get_user_data_dir(), "peregrine", NULL);
    g_mkdir_with_parents(data_dir, 0700);
    g_autofree gchar *cookie_file = g_build_filename(data_dir, "cookies.sqlite", NULL);

    webkit_cookie_manager_set_persistent_storage(
        cookie_manager, cookie_file, WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);

    /* Make sure the policy actually allows cookies to be set in the
     * first place (first-party always, third-party blocked by default
     * is usually what you want for a general browser). */
    webkit_cookie_manager_set_accept_policy(
        cookie_manager, WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY);
}
