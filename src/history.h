#ifndef HISTORY_H
#define HISTORY_H

#include <glib.h>

void history_init(void);
void history_record(const char *url, const char *title);
gchar *history_build_page(const char *query);

#endif // HISTORY_H
