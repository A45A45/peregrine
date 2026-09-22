#include "tab_manager.h"

typedef struct {
    GtkWidget *notebook;
    GtkWidget *web_view;
    GtkWidget *label;
    gboolean editable_focused;
} TabContext;

char *tab_manager_normalize_url(const char *url) {
    if (!url || *url == '\0') {
        return g_strdup("https://webkitgtk.org");
    }
    if (g_str_has_prefix(url, "http://") || g_str_has_prefix(url, "https://") || g_str_has_prefix(url, "file://")) {
        return g_strdup(url);
    }
    return g_strconcat("https://", url, NULL);
}

static void on_script_message_received(WebKitUserContentManager *manager, JSCValue *result, gpointer user_data) {
    TabContext *ctx = (TabContext *)user_data;
    if (jsc_value_is_string(result)) {
        g_autofree gchar *str = jsc_value_to_string(result);
        ctx->editable_focused = (g_strcmp0(str, "1") == 0);
    }
}

static void on_title_changed(WebKitWebView *web_view, GParamSpec *pspec, gpointer user_data) {
    GtkLabel *label = GTK_LABEL(user_data);
    const char *title = webkit_web_view_get_title(web_view);
    if (title && *title != '\0') {
        if (g_utf8_strlen(title, -1) > 20) {
            g_autofree gchar *truncated = g_utf8_substring(title, 0, 18);
            g_autofree gchar *display_title = g_strconcat(truncated, "...", NULL);
            gtk_label_set_text(label, display_title);
        } else {
            gtk_label_set_text(label, title);
        }
    } else {
        gtk_label_set_text(label, "New Tab");
    }
}

static void on_close_clicked(GtkButton *button, gpointer user_data) {
    TabContext *ctx = (TabContext *)user_data;
    int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(ctx->notebook));
    if (n_pages > 1) {
        int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(ctx->notebook), ctx->web_view);
        if (page_num >= 0) {
            gtk_notebook_remove_page(GTK_NOTEBOOK(ctx->notebook), page_num);
        }
    } else {
        GtkWidget *window = GTK_WIDGET(gtk_widget_get_root(ctx->notebook));
        if (GTK_IS_WINDOW(window)) {
            gtk_window_close(GTK_WINDOW(window));
        }
    }
}

GtkWidget *tab_manager_create_notebook(void) {
    GtkWidget *notebook = gtk_notebook_new();
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), TRUE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    gtk_widget_set_vexpand(notebook, TRUE);
    gtk_widget_set_hexpand(notebook, TRUE);
    return notebook;
}

GtkWidget *tab_manager_add_tab(GtkWidget *notebook, const char *url, GCallback key_press_cb, gpointer user_data) {
    GtkWidget *web_view = webkit_web_view_new();
    gtk_widget_set_vexpand(web_view, TRUE);
    gtk_widget_set_hexpand(web_view, TRUE);

    GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new("New Tab");
    gtk_box_append(GTK_BOX(tab_box), label);

    GtkWidget *close_btn = gtk_button_new_from_icon_name("window-close-symbolic");
    gtk_widget_set_valign(close_btn, GTK_ALIGN_CENTER);
    gtk_button_set_has_frame(GTK_BUTTON(close_btn), FALSE);
    gtk_widget_set_size_request(close_btn, 16, 16);
    gtk_box_append(GTK_BOX(tab_box), close_btn);

    gtk_widget_set_visible(tab_box, TRUE);
    gtk_widget_set_visible(label, TRUE);
    gtk_widget_set_visible(close_btn, TRUE);

    TabContext *ctx = g_new0(TabContext, 1);
    ctx->notebook = notebook;
    ctx->web_view = web_view;
    ctx->label = label;
    ctx->editable_focused = FALSE;

    g_object_set_data(G_OBJECT(web_view), "tab-context", ctx);

    WebKitUserContentManager *ucm = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(web_view));
    webkit_user_content_manager_register_script_message_handler(ucm, "peregrineFocus", NULL);
    g_signal_connect(ucm, "script-message-received::peregrineFocus", G_CALLBACK(on_script_message_received), ctx);

    // Optimized script tracking editable state to avoid redundant IPC messages
    const char *script_source =
        "let __peregrine_last_editable = false;"
        "window.addEventListener('focusin', (e) => {"
        "    const tag = e.target.tagName;"
        "    const isEditable = e.target.isContentEditable || tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'SELECT';"
        "    if (isEditable !== __peregrine_last_editable) {"
        "        __peregrine_last_editable = isEditable;"
        "        window.webkit.messageHandlers.peregrineFocus.postMessage(isEditable ? '1' : '0');"
        "    }"
        "}, true);"
        "window.addEventListener('focusout', (e) => {"
        "    setTimeout(() => {"
        "        if (!document.activeElement || document.activeElement === document.body) {"
        "            if (__peregrine_last_editable) {"
        "                __peregrine_last_editable = false;"
        "                window.webkit.messageHandlers.peregrineFocus.postMessage('0');"
        "            }"
        "        }"
        "    }, 0);"
        "}, true);";

    WebKitUserScript *user_script = webkit_user_script_new(
        script_source,
        WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        NULL, NULL
    );
    webkit_user_content_manager_add_script(ucm, user_script);
    webkit_user_script_unref(user_script);

    g_signal_connect_swapped(web_view, "destroy", G_CALLBACK(g_free), ctx);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), ctx);
    g_signal_connect(web_view, "notify::title", G_CALLBACK(on_title_changed), label);

    if (key_press_cb) {
        GtkEventController *key_controller = gtk_event_controller_key_new();
        gtk_event_controller_set_propagation_phase(key_controller, GTK_PHASE_CAPTURE);
        g_signal_connect(key_controller, "key-pressed", key_press_cb, user_data);
        gtk_widget_add_controller(web_view, key_controller);
    }

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), web_view, tab_box);
    gtk_widget_set_visible(web_view, TRUE);

    int page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) - 1;
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page_num);

    g_autofree gchar *final_url = tab_manager_normalize_url(url);
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(web_view), final_url);

    return web_view;
}

void tab_manager_close_current_tab(GtkWidget *notebook, GtkWidget *window) {
    int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook));
    if (n_pages > 1) {
        int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
        if (current_page >= 0) {
            gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), current_page);
        }
    } else {
        if (window && GTK_IS_WINDOW(window)) {
            gtk_window_close(GTK_WINDOW(window));
        }
    }
}

gboolean tab_manager_is_editable_focused(GtkNotebook *notebook) {
    int current_page = gtk_notebook_get_current_page(notebook);
    if (current_page < 0) return FALSE;
    GtkWidget *child = gtk_notebook_get_nth_page(notebook, current_page);
    TabContext *ctx = g_object_get_data(G_OBJECT(child), "tab-context");
    if (ctx) {
        return ctx->editable_focused;
    }
    return FALSE;
}
