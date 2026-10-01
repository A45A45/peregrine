#ifndef ADBLOCK_H
#define ADBLOCK_H

#include <webkit/webkit.h>

void adblock_init(void);
void adblock_reload_filters(void);
void adblock_attach_filters(WebKitUserContentManager *ucm);

#endif // ADBLOCK_H
