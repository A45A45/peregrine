#include "portal.h"
#include "config.h"
#include <gio/gio.h>
#include <libsoup/soup.h>
#include <string.h>

static SoupSession *s_session = NULL;
static PortalDetectedFunc s_func = NULL;
static gpointer s_func_data = NULL;
static gboolean s_portal_active = FALSE;
static gboolean s_in_flight = FALSE;
static gboolean s_monitor_connected = FALSE;
static guint s_timer_id = 0;

typedef struct {
    SoupMessage *msg;
    gboolean force;
} ProbeCtx;

static void on_probe_done(GObject *source, GAsyncResult *res, gpointer user_data) {
    ProbeCtx *ctx = (ProbeCtx *)user_data;
    GError *error = NULL;
    GBytes *body = soup_session_send_and_read_finish(SOUP_SESSION(source), res, &error);
    s_in_flight = FALSE;

    if (error) {
        g_error_free(error);
    } else {
        guint status = soup_message_get_status(ctx->msg);
        gsize len = 0;
        const char *data = body ? g_bytes_get_data(body, &len) : NULL;
        gboolean online = (status == 200 && data && len >= 7 && memcmp(data, "success", 7) == 0);

        if (online) {
            s_portal_active = FALSE;
        } else if (s_func && (!s_portal_active || ctx->force)) {
            s_portal_active = TRUE;
            s_func(PEREGRINE_PORTAL_PROBE_URL, s_func_data);
        }
    }

    if (body) g_bytes_unref(body);
    g_object_unref(ctx->msg);
    g_free(ctx);
}

void portal_check_now(gboolean force) {
    if (s_in_flight) return;

    if (!s_session) {
        s_session = soup_session_new();
        soup_session_set_timeout(s_session, 5);
    }

    SoupMessage *msg = soup_message_new("GET", PEREGRINE_PORTAL_PROBE_URL);
    if (!msg) return;
    soup_message_set_flags(msg, SOUP_MESSAGE_NO_REDIRECT);

    ProbeCtx *ctx = g_new0(ProbeCtx, 1);
    ctx->msg = msg;
    ctx->force = force;

    s_in_flight = TRUE;
    soup_session_send_and_read_async(s_session, msg, G_PRIORITY_DEFAULT, NULL, on_probe_done, ctx);
}

static gboolean probe_timer_cb(gpointer data) {
    s_timer_id = 0;
    portal_check_now(FALSE);
    return G_SOURCE_REMOVE;
}

static void schedule_probe(guint seconds) {
    if (s_timer_id) g_source_remove(s_timer_id);
    s_timer_id = g_timeout_add_seconds(seconds, probe_timer_cb, NULL);
}

static void on_network_changed(GNetworkMonitor *monitor, gboolean available, gpointer user_data) {
    if (!available) {
        s_portal_active = FALSE;
        return;
    }
    schedule_probe(2);
}

void portal_init(PortalDetectedFunc func, gpointer user_data) {
    s_func = func;
    s_func_data = user_data;

    if (!s_monitor_connected) {
        g_signal_connect(g_network_monitor_get_default(), "network-changed",
                         G_CALLBACK(on_network_changed), NULL);
        s_monitor_connected = TRUE;
    }
    schedule_probe(3);
}
