#include "webview.h"
#include "storage.h"
#include "adblock.h"
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
