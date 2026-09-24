#ifndef STORAGE_H
#define STORAGE_H

#include <webkit/webkit.h>

WebKitWebsiteDataManager *storage_get_website_data_manager(void);
WebKitWebContext *storage_get_web_context(void);

#endif // STORAGE_H
