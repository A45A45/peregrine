#include "keys.h"
#include "tab.h"
#include "hint.h"
#include <jsc/jsc.h>

static gboolean g_pending = FALSE;
static gboolean g_hint_mode = FALSE;

static void keys_enter_hint_mode(WebKitWebView *web_view) {
    if (!web_view) return;
    g_hint_mode = TRUE;
    hint_enter_mode(web_view);
}

static void keys_exit_hint_mode(WebKitWebView *web_view) {
    if (!web_view) return;
    g_hint_mode = FALSE;
    hint_exit_mode(web_view);
}

static void on_hint_char_evaluated(GObject *source, GAsyncResult *res, gpointer user_data) {
    JSCValue *val = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(source), res, NULL);
    if (val && jsc_value_is_string(val)) {
        g_autofree gchar *str = jsc_value_to_string(val);
        if (g_str_equal(str, "activated") || g_str_equal(str, "cancelled")) {
            g_hint_mode = FALSE;
        }
    }
    if (val) g_object_unref(val);
}

gboolean keys_handle_key(GtkNotebook *notebook, GtkWidget *command_bar, guint keyval, GdkModifierType state_mask) {
    if (gtk_widget_get_visible(command_bar) || tab_manager_is_editable_focused(notebook)) {
        g_pending = FALSE;
        if (g_hint_mode) {
            WebKitWebView *web_view = tab_manager_get_active_web_view(notebook);
            keys_exit_hint_mode(web_view);
        }
        return FALSE;
    }

    WebKitWebView *web_view = tab_manager_get_active_web_view(notebook);
    gboolean ctrl = (state_mask & GDK_CONTROL_MASK) != 0;
    gboolean shift = (state_mask & GDK_SHIFT_MASK) != 0;
    gboolean alt = (state_mask & GDK_ALT_MASK) != 0;

    if (alt) {
        g_pending = FALSE;
        if (keyval >= GDK_KEY_1 && keyval <= GDK_KEY_9) {
            int target_page = keyval - GDK_KEY_1;
            int n_pages = gtk_notebook_get_n_pages(notebook);
            if (target_page < n_pages) {
                gtk_notebook_set_current_page(notebook, target_page);
            }
            return TRUE;
        }
        return FALSE;
    }

    if (g_hint_mode) {
        if (keyval == GDK_KEY_Escape || (ctrl && keyval == GDK_KEY_bracketleft)) {
            keys_exit_hint_mode(web_view);
            return TRUE;
        }

        gunichar uch = gdk_keyval_to_unicode(keyval);
        if (uch != 0 && g_unichar_isprint(uch)) {
            g_autofree gchar *js = g_strdup_printf("if (window.__peregrine_handle_char) { window.__peregrine_handle_char('%c'); } else { 'cancelled'; }", (char)uch);
            webkit_web_view_evaluate_javascript(web_view, js, -1, NULL, NULL, NULL, on_hint_char_evaluated, NULL);
        }
        return TRUE;
    }

    if (ctrl) {
        g_pending = FALSE;
        if (keyval == GDK_KEY_d || keyval == GDK_KEY_D) {
            if (web_view) webkit_web_view_evaluate_javascript(web_view, "window.scrollBy(0, window.innerHeight / 2);", -1, NULL, NULL, NULL, NULL, NULL);
            return TRUE;
        }
        if (keyval == GDK_KEY_u || keyval == GDK_KEY_U) {
            if (web_view) webkit_web_view_evaluate_javascript(web_view, "window.scrollBy(0, -window.innerHeight / 2);", -1, NULL, NULL, NULL, NULL, NULL);
            return TRUE;
        }
        if (keyval == GDK_KEY_Tab) {
            int n_pages = gtk_notebook_get_n_pages(notebook);
            int current = gtk_notebook_get_current_page(notebook);
            if (shift) {
                int target = (current > 0) ? current - 1 : n_pages - 1;
                gtk_notebook_set_current_page(notebook, target);
            } else {
                int target = (current < n_pages - 1) ? current + 1 : 0;
                gtk_notebook_set_current_page(notebook, target);
            }
            return TRUE;
        }
        if (keyval == GDK_KEY_ISO_Left_Tab) {
            int n_pages = gtk_notebook_get_n_pages(notebook);
            int current = gtk_notebook_get_current_page(notebook);
            int target = (current > 0) ? current - 1 : n_pages - 1;
            gtk_notebook_set_current_page(notebook, target);
            return TRUE;
        }
        return FALSE;
    }

    if (g_pending) {
        g_pending = FALSE;
        if (keyval == GDK_KEY_g) {
            if (web_view) {
                webkit_web_view_evaluate_javascript(web_view, "window.scrollTo(0, 0);", -1, NULL, NULL, NULL, NULL, NULL);
            }
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
        case GDK_KEY_f:
            if (!ctrl && !shift) {
                keys_enter_hint_mode(web_view);
                return TRUE;
            }
            break;

        case GDK_KEY_g:
            g_pending = TRUE;
            return TRUE;

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
            if (web_view) webkit_web_view_evaluate_javascript(web_view, "window.scrollTo(80, 0);", -1, NULL, NULL, NULL, NULL, NULL);
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

        default:
            break;
    }

    return FALSE;
}
