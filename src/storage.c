#include "storage.h"
#include <webkit/webkit.h>

#include <webkit/webkit.h>

WebKitWebsiteDataManager *storage_get_website_data_manager(void) {
    WebKitNetworkSession *default_session = webkit_network_session_get_default();
    return webkit_network_session_get_website_data_manager(default_session);
}


WebKitWebContext *storage_get_web_context(void) {
    return webkit_web_context_get_default();
}
