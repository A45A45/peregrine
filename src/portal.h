#ifndef PORTAL_H
#define PORTAL_H

#include <glib.h>

typedef void (*PortalDetectedFunc)(const char *url, gpointer user_data);

void portal_init(PortalDetectedFunc func, gpointer user_data);
void portal_check_now(gboolean force);

#endif // PORTAL_H
