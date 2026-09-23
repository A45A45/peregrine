#ifndef HINT_H
#define HINT_H

#include <webkit/webkit.h>

extern const char *HINT_MODE_JS;

void hint_enter_mode(WebKitWebView *web_view);
void hint_exit_mode(WebKitWebView *web_view);

#endif // HINT_H
