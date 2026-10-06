#include "webview.h"
#include "adblock.h"
#include "config.h"
#include <jsc/jsc.h>

typedef struct {
    gboolean editable_focused;
} WebViewContext;

static void on_script_message_received(WebKitUserContentManager *manager, JSCValue *result, gpointer user_data) {
    WebViewContext *ctx = (WebViewContext *)user_data;
    if (jsc_value_is_string(result)) {
        g_autofree gchar *str = jsc_value_to_string(result);
        ctx->editable_focused = (g_strcmp0(str, "1") == 0);
    }
}

static gchar *youtube_to_piped_url(const char *uri) {
    if (!uri) return NULL;

    GUri *parsed = g_uri_parse(uri, G_URI_FLAGS_NONE, NULL);
    if (!parsed) return NULL;

    const char *host = g_uri_get_host(parsed);
    gchar *result = NULL;

    if (host && (g_str_has_suffix(host, "youtube.com") || g_str_equal(host, "youtu.be"))) {
        const char *path = g_uri_get_path(parsed);
        const char *query = g_uri_get_query(parsed);

        if (g_str_equal(host, "youtu.be") && path && *path == '/' && path[1] != '\0') {
            /* youtu.be/<id> -> /watch?v=<id> */
            result = g_strdup_printf("%s/watch?v=%s", PEREGRINE_PIPED_INSTANCE, path + 1);
        } else {
            result = g_strdup_printf("%s%s%s%s",
                                      PEREGRINE_PIPED_INSTANCE,
                                      path ? path : "",
                                      query ? "?" : "",
                                      query ? query : "");
        }
    }

    g_uri_unref(parsed);
    return result;
}

static gboolean on_decide_policy(WebKitWebView *web_view,
                                  WebKitPolicyDecision *decision,
                                  WebKitPolicyDecisionType type,
                                  gpointer user_data) {
    if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION &&
        type != WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION) {
        return FALSE;
    }

    WebKitNavigationPolicyDecision *nav_decision = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
    WebKitNavigationAction *action = webkit_navigation_policy_decision_get_navigation_action(nav_decision);
    WebKitURIRequest *request = webkit_navigation_action_get_request(action);
    const char *uri = webkit_uri_request_get_uri(request);

    g_autofree gchar *piped_url = youtube_to_piped_url(uri);
    if (piped_url) {
        webkit_policy_decision_ignore(decision);
        webkit_web_view_load_uri(web_view, piped_url);
        return TRUE;
    }

    const char *source_uri = webkit_web_view_get_uri(web_view);
    if (!source_uri) source_uri = "";

    if (uri && adblock_should_block(uri, source_uri, "document")) {
        webkit_policy_decision_ignore(decision);
        return TRUE;
    }

    webkit_policy_decision_use(decision);
    return TRUE;
}

GtkWidget *webview_create(void) {
    WebKitUserContentManager *ucm = webkit_user_content_manager_new();
    GtkWidget *web_view = g_object_new(WEBKIT_TYPE_WEB_VIEW,
                                        "user-content-manager", ucm,
                                        NULL);

    gtk_widget_set_vexpand(web_view, TRUE);
    gtk_widget_set_hexpand(web_view, TRUE);

    WebViewContext *ctx = g_new0(WebViewContext, 1);
    ctx->editable_focused = FALSE;
    g_object_set_data_full(G_OBJECT(web_view), "webview-context", ctx, g_free);

    webkit_user_content_manager_register_script_message_handler(ucm, "peregrineFocus", NULL);
    g_signal_connect(ucm, "script-message-received::peregrineFocus", G_CALLBACK(on_script_message_received), ctx);
    g_signal_connect(web_view, "decide-policy", G_CALLBACK(on_decide_policy), NULL);

    adblock_attach_filters(ucm);

    const char *script_source =
        "let __peregrine_last_editable = false;"
        "function checkEditable() {"
        "    let el = document.activeElement;"
        "    while (el && el.shadowRoot && el.shadowRoot.activeElement) {"
        "        el = el.shadowRoot.activeElement;"
        "    }"
        "    if (!el) return false;"
        "    const tag = el.tagName;"
        "    return el.isContentEditable || tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'SELECT' || el.getAttribute('role') === 'textbox' || el.getAttribute('contenteditable') === 'true';"
        "}"
        "function updateFocus() {"
        "    const isEditable = checkEditable();"
        "    if (isEditable !== __peregrine_last_editable) {"
        "        __peregrine_last_editable = isEditable;"
        "        window.webkit.messageHandlers.peregrineFocus.postMessage(isEditable ? '1' : '0');"
        "    }"
        "}"
        "window.addEventListener('focusin', updateFocus, true);"
        "window.addEventListener('focusout', () => { setTimeout(updateFocus, 0); }, true);"
        "document.addEventListener('selectionchange', updateFocus, true);";

    WebKitUserScript *user_script = webkit_user_script_new(
        script_source,
        WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        NULL, NULL
    );
    webkit_user_content_manager_add_script(ucm, user_script);
    webkit_user_script_unref(user_script);

    return web_view;
}

gboolean webview_is_editable_focused(WebKitWebView *web_view) {
    if (!web_view) return FALSE;
    WebViewContext *ctx = g_object_get_data(G_OBJECT(web_view), "webview-context");
    if (ctx) {
        return ctx->editable_focused;
    }
    return FALSE;
}
