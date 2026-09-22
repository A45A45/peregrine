#include "vim_bindings.h"
#include "tab_manager.h"
#include <jsc/jsc.h>

static gboolean g_pending = FALSE;
static gboolean g_hint_mode = FALSE;

static const char *HINT_MODE_JS =
"(function() {\n"
"    if (window.__peregrine_hints_active) return;\n"
"    window.__peregrine_hints_active = true;\n"
"    window.__peregrine_typed = '';\n"
"    \n"
"    const hintChars = 'asdfghjkl';\n"
"    \n"
"    function generateHintLabels(count) {\n"
"        if (count <= hintChars.length) {\n"
"            return hintChars.split('').slice(0, count);\n"
"        }\n"
"        let labels = [];\n"
"        for (let i = 0; i < hintChars.length; i++) {\n"
"            for (let j = 0; j < hintChars.length; j++) {\n"
"                labels.push(hintChars[i] + hintChars[j]);\n"
"                if (labels.length >= count) return labels;\n"
"            }\n"
"        }\n"
"        return labels;\n"
"    }\n"
"    \n"
"    function isElementVisible(el, rect) {\n"
"        if (rect.width === 0 || rect.height === 0) return false;\n"
"        if (rect.bottom < 0 || rect.right < 0 || rect.top > window.innerHeight || rect.left > window.innerWidth) return false;\n"
"        const style = window.getComputedStyle(el);\n"
"        if (style.visibility === 'hidden' || style.opacity === '0' || style.display === 'none') return false;\n"
"        return true;\n"
"    }\n"
"    \n"
"    const selector = \"a, button, input:not([type='hidden']), textarea, select, [tabindex], [onclick], [role='button'], [role='link']\";\n"
"    const allElements = document.querySelectorAll(selector);\n"
"    const visibleElements = [];\n"
"    \n"
"    for (let el of allElements) {\n"
"        const rect = el.getBoundingClientRect();\n"
"        if (isElementVisible(el, rect)) {\n"
"            visibleElements.push({ el: el, rect: rect });\n"
"        }\n"
"    }\n"
"    \n"
"    const labels = generateHintLabels(visibleElements.length);\n"
"    const container = document.createElement('div');\n"
"    container.id = '__peregrine_hint_container';\n"
"    container.style.position = 'absolute';\n"
"    container.style.top = '0';\n"
"    container.style.left = '0';\n"
"    container.style.width = '100%';\n"
"    container.style.height = '100%';\n"
"    container.style.pointerEvents = 'none';\n"
"    container.style.zIndex = '2147483647';\n"
"    document.body.appendChild(container);\n"
"    \n"
"    window.__peregrine_hint_map = {};\n"
"    \n"
"    visibleElements.forEach((item, index) => {\n"
"        if (index >= labels.length) return;\n"
"        const label = labels[index];\n"
"        window.__peregrine_hint_map[label] = item.el;\n"
"        \n"
"        const hintDiv = document.createElement('div');\n"
"        hintDiv.className = 'peregrine-hint';\n"
"        hintDiv.textContent = label.toUpperCase();\n"
"        hintDiv.style.position = 'absolute';\n"
"        hintDiv.style.left = (window.scrollX + item.rect.left) + 'px';\n"
"        hintDiv.style.top = (window.scrollY + item.rect.top) + 'px';\n"
"        hintDiv.style.background = '#ffff00';\n"
"        hintDiv.style.color = '#000000';\n"
"        hintDiv.style.border = '1px solid #d4a017';\n"
"        hintDiv.style.padding = '1px 3px';\n"
"        hintDiv.style.fontSize = '11px';\n"
"        hintDiv.style.fontWeight = 'bold';\n"
"        hintDiv.style.fontFamily = 'monospace';\n"
"        hintDiv.style.boxShadow = '0 2px 4px rgba(0,0,0,0.3)';\n"
"        hintDiv.style.borderRadius = '2px';\n"
"        hintDiv.style.zIndex = '2147483647';\n"
"        container.appendChild(hintDiv);\n"
"    });\n"
"    \n"
"    window.__peregrine_handle_char = function(ch) {\n"
"        ch = ch.toLowerCase();\n"
"        window.__peregrine_typed += ch;\n"
"        const typed = window.__peregrine_typed;\n"
"        \n"
"        if (window.__peregrine_hint_map[typed]) {\n"
"            const target = window.__peregrine_hint_map[typed];\n"
"            window.__peregrine_exit_hint_mode();\n"
"            target.focus();\n"
"            target.click();\n"
"            return 'activated';\n"
"        }\n"
"        \n"
"        let matchFound = false;\n"
"        const hints = container.querySelectorAll('.peregrine-hint');\n"
"        hints.forEach(hint => {\n"
"            const text = hint.textContent.toLowerCase();\n"
"            if (text.startsWith(typed)) {\n"
"                matchFound = true;\n"
"                hint.style.display = 'inline-block';\n"
"                const matchedPart = text.substring(0, typed.length);\n"
"                const restPart = text.substring(typed.length);\n"
"                hint.innerHTML = `<span style=\"color: #ff0000;\">${matchedPart.toUpperCase()}</span>${restPart.toUpperCase()}`;\n"
"            } else {\n"
"                hint.style.display = 'none';\n"
"            }\n"
"        });\n"
"        \n"
"        if (!matchFound) {\n"
"            window.__peregrine_exit_hint_mode();\n"
"            return 'cancelled';\n"
"        }\n"
"        return 'continue';\n"
"    };\n"
"    \n"
"    window.__peregrine_exit_hint_mode = function() {\n"
"        if (!window.__peregrine_hints_active) return;\n"
"        window.__peregrine_hints_active = false;\n"
"        const c = document.getElementById('__peregrine_hint_container');\n"
"        if (c) c.remove();\n"
"        delete window.__peregrine_hint_container;\n"
"        delete window.__peregrine_hint_map;\n"
"        delete window.__peregrine_handle_char;\n"
"        delete window.__peregrine_exit_hint_mode;\n"
"        delete window.__peregrine_typed;\n"
"    };\n"
"})();";

static WebKitWebView *get_current_web_view(GtkNotebook *notebook) {
    int current_page = gtk_notebook_get_current_page(notebook);
    if (current_page < 0) return NULL;
    GtkWidget *child = gtk_notebook_get_nth_page(notebook, current_page);
    return WEBKIT_WEB_VIEW(child);
}

static void vim_bindings_enter_hint_mode(WebKitWebView *web_view) {
    if (!web_view) return;
    g_hint_mode = TRUE;
    webkit_web_view_evaluate_javascript(web_view, HINT_MODE_JS, -1, NULL, NULL, NULL, NULL, NULL);
}

static void vim_bindings_exit_hint_mode(WebKitWebView *web_view) {
    if (!web_view) return;
    g_hint_mode = FALSE;
    webkit_web_view_evaluate_javascript(web_view, "if (window.__peregrine_exit_hint_mode) window.__peregrine_exit_hint_mode();", -1, NULL, NULL, NULL, NULL, NULL);
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

gboolean vim_bindings_handle_key(GtkNotebook *notebook, GtkWidget *command_bar, guint keyval, GdkModifierType state_mask) {
    if (gtk_widget_get_visible(command_bar) || tab_manager_is_editable_focused(notebook)) {
        g_pending = FALSE;
        if (g_hint_mode) {
            WebKitWebView *web_view = get_current_web_view(notebook);
            vim_bindings_exit_hint_mode(web_view);
        }
        return FALSE;
    }

    WebKitWebView *web_view = get_current_web_view(notebook);
    gboolean ctrl = (state_mask & GDK_CONTROL_MASK) != 0;
    gboolean shift = (state_mask & GDK_SHIFT_MASK) != 0;

    if (g_hint_mode) {
        if (keyval == GDK_KEY_Escape || (ctrl && (keyval == GDK_KEY_bracketleft || keyval == GDK_KEY_bracketleft))) {
            vim_bindings_exit_hint_mode(web_view);
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
                vim_bindings_enter_hint_mode(web_view);
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
