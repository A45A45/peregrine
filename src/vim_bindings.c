#include "vim_bindings.h"
#include "tab_manager.h"

static WebKitWebView *get_current_web_view(GtkNotebook *notebook) {
    int current_page = gtk_notebook_get_current_page(notebook);
    if (current_page < 0) return NULL;
    GtkWidget *child = gtk_notebook_get_nth_page(notebook, current_page);
    return WEBKIT_WEB_VIEW(child);
}

gboolean vim_bindings_handle_key(GtkNotebook *notebook, GtkWidget *command_bar, guint keyval, GdkModifierType state_mask) {
    if (gtk_widget_get_visible(command_bar)) {
        return FALSE;
    }

    WebKitWebView *web_view = get_current_web_view(notebook);
    gboolean ctrl = (state_mask & GDK_CONTROL_MASK) != 0;
    gboolean shift = (state_mask & GDK_SHIFT_MASK) != 0;

    if (ctrl) {
        if (keyval == GDK_KEY_d || keyval == GDK_KEY_D) {
            if (web_view) webkit_web_view_evaluate_javascript(web_view, "window.scrollBy(0, window.innerHeight / 2);", -1, NULL, NULL, NULL, NULL, NULL);
            return TRUE;
        }
        if (keyval == GDK_KEY_u || keyval == GDK_KEY_U) {
            if (web_view) webkit_web_view_evaluate_javascript(web_view, "window.scrollBy(0, -window.innerHeight / 2);", -1, NULL, NULL, NULL, NULL, NULL);
            return TRUE;
        }
        return FALSE;
    }

    switch (keyval) {
        case GDK_KEY_j:
        case GDK_KEY_J:
            if (shift) {
                gtk_notebook_next_page(notebook);
            } else {
                if (web_view) webkit_web_view_evaluate_javascript(web_view, "window.scrollBy(0, 80);", -1, NULL, NULL, NULL, NULL, NULL);
            }
            return TRUE;

        case GDK_KEY_k:
        case GDK_KEY_K:
            if (shift) {
                gtk_notebook_prev_page(notebook);
            } else {
                if (web_view) webkit_web_view_evaluate_javascript(web_view, "window.scrollBy(0, -80);", -1, NULL, NULL, NULL, NULL, NULL);
            }
            return TRUE;

        case GDK_KEY_h:
            if (web_view) webkit_web_view_evaluate_javascript(web_view, "window.scrollBy(-80, 0);", -1, NULL, NULL, NULL, NULL, NULL);
            return TRUE;

        case GDK_KEY_l:
            if (web_view) webkit_web_view_evaluate_javascript(web_view, "window.scrollBy(80, 0);", -1, NULL, NULL, NULL, NULL, NULL);
            return TRUE;

        case GDK_KEY_G:
            if (web_view) webkit_web_view_evaluate_javascript(web_view, "window.scrollTo(0, document.body.scrollHeight);", -1, NULL, NULL, NULL, NULL, NULL);
            return TRUE;

        case GDK_KEY_H:
            if (web_view && webkit_web_view_can_go_back(web_view)) {
                webkit_web_view_go_back(web_view);
            }
            return TRUE;

        case GDK_KEY_L:
            if (web_view && webkit_web_view_can_go_forward(web_view)) {
                webkit_web_view_go_forward(web_view);
            }
            return TRUE;

        case GDK_KEY_r:
        case GDK_KEY_R:
            if (web_view) {
                webkit_web_view_reload(web_view);
            }
            return TRUE;

        case GDK_KEY_g:
            if (web_view) {
                webkit_web_view_evaluate_javascript(web_view, "window.scrollTo(0, 0);", -1, NULL, NULL, NULL, NULL, NULL);
            }
            return TRUE;

        default:
            break;
    }

    return FALSE;
}
