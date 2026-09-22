#include "vim_bindings.h"
#include "tab_manager.h"

static gboolean g_pending = FALSE;

// Pre-defined static JS strings for fast evaluation without allocation
static const char JS_SCROLL_DOWN_HALF[] = "window.scrollBy(0, window.innerHeight / 2);";
static const char JS_SCROLL_UP_HALF[]   = "window.scrollBy(0, -window.innerHeight / 2);";
static const char JS_SCROLL_TOP[]       = "window.scrollTo(0, 0);";
static const char JS_SCROLL_BOTTOM[]    = "window.scrollTo(0, document.body.scrollHeight);";
static const char JS_SCROLL_DOWN[]      = "window.scrollBy(0, 80);";
static const char JS_SCROLL_UP[]        = "window.scrollBy(0, -80);";
static const char JS_SCROLL_LEFT[]      = "window.scrollBy(-80, 0);";
static const char JS_SCROLL_RIGHT[]     = "window.scrollBy(80, 0);";

static WebKitWebView *get_current_web_view(GtkNotebook *notebook) {
    int current_page = gtk_notebook_get_current_page(notebook);
    if (current_page < 0) return NULL;
    GtkWidget *child = gtk_notebook_get_nth_page(notebook, current_page);
    return WEBKIT_WEB_VIEW(child);
}

static inline void eval_js(WebKitWebView *web_view, const char *script) {
    if (web_view) {
        webkit_web_view_evaluate_javascript(web_view, script, -1, NULL, NULL, NULL, NULL, NULL);
    }
}

gboolean vim_bindings_handle_key(GtkNotebook *notebook, GtkWidget *command_bar, guint keyval, GdkModifierType state_mask) {
    if (gtk_widget_get_visible(command_bar) || tab_manager_is_editable_focused(notebook)) {
        g_pending = FALSE;
        return FALSE;
    }

    WebKitWebView *web_view = get_current_web_view(notebook);
    gboolean ctrl = (state_mask & GDK_CONTROL_MASK) != 0;
    gboolean shift = (state_mask & GDK_SHIFT_MASK) != 0;

    if (ctrl) {
        g_pending = FALSE;
        if (keyval == GDK_KEY_d || keyval == GDK_KEY_D) {
            eval_js(web_view, JS_SCROLL_DOWN_HALF);
            return TRUE;
        }
        if (keyval == GDK_KEY_u || keyval == GDK_KEY_U) {
            eval_js(web_view, JS_SCROLL_UP_HALF);
            return TRUE;
        }
        return FALSE;
    }

    if (g_pending) {
        g_pending = FALSE;
        if (keyval == GDK_KEY_g) {
            eval_js(web_view, JS_SCROLL_TOP);
            return TRUE;
        } else if (keyval == GDK_KEY_t) {
            gtk_notebook_next_page(notebook);
            return TRUE;
        } else if (keyval == GDK_KEY_T || (shift && keyval == GDK_KEY_t)) {
            gtk_notebook_prev_page(notebook);
            return TRUE;
        }
    }

    switch (keyval) {
        case GDK_KEY_g:
            g_pending = TRUE;
            return TRUE;

        case GDK_KEY_j:
        case GDK_KEY_J:
            if (shift) {
                gtk_notebook_next_page(notebook);
            } else {
                eval_js(web_view, JS_SCROLL_DOWN);
            }
            return TRUE;

        case GDK_KEY_k:
        case GDK_KEY_K:
            if (shift) {
                gtk_notebook_prev_page(notebook);
            } else {
                eval_js(web_view, JS_SCROLL_UP);
            }
            return TRUE;

        case GDK_KEY_h:
            eval_js(web_view, JS_SCROLL_LEFT);
            return TRUE;

        case GDK_KEY_l:
            eval_js(web_view, JS_SCROLL_RIGHT);
            return TRUE;

        case GDK_KEY_G:
            eval_js(web_view, JS_SCROLL_BOTTOM);
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

        default:
            break;
    }

    return FALSE;
}
