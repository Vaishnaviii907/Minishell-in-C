#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include "shell_interface.h"

#define MAX_HISTORY 100

static char *history[MAX_HISTORY];
static int history_len = 0;
static int history_pos = 0;
static char *in_progress_input = NULL;
static int prompt_end_offset = 0;

typedef struct {
    GtkWidget *entry1;
    GtkWidget *combo;
    GtkWidget *entry2;
    GtkWidget *result_label;
    GtkTextBuffer *buffer;
} CalculatorWidgets;

static char* get_dynamic_prompt() {
    static char prompt[PATH_MAX + 4];
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
        snprintf(prompt, sizeof(prompt), "%s$ ", cwd);
    else
        snprintf(prompt, sizeof(prompt), "minishell$ ");
    return prompt;
}

static void insert_colored_text(GtkTextBuffer *buffer, const char *text, const char *tag) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    if (tag && strcmp(tag, "prompt") == 0) {
        gtk_text_buffer_insert_with_tags_by_name(buffer, &end, text, -1, tag, NULL);
        gtk_text_buffer_get_end_iter(buffer, &end);
        prompt_end_offset = gtk_text_iter_get_offset(&end);
    } else if (tag) {
        gtk_text_buffer_insert_with_tags_by_name(buffer, &end, text, -1, tag, NULL);
    } else {
        gtk_text_buffer_insert(buffer, &end, text, -1);
    }
}

static gchar *get_current_input(GtkTextBuffer *buffer) {
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gchar *all_text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    char *prompt = get_dynamic_prompt();
    char *last_prompt = NULL;
    char *search = all_text;
    int last_offset = -1;
    while ((search = strstr(search, prompt)) != NULL) {
        last_prompt = search;
        last_offset = search - all_text;
        search += strlen(prompt);
    }
    gchar *result = NULL;
    if (last_prompt && last_offset >= 0)
        result = g_strdup(last_prompt + strlen(prompt));
    g_free(all_text);
    return result;
}

static void replace_input_after_prompt(GtkTextBuffer *buffer, const char *new_input) {
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gchar *all_text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    char *prompt = get_dynamic_prompt();
    char *last_prompt = NULL;
    char *search = all_text;
    int last_offset = -1;
    while ((search = strstr(search, prompt)) != NULL) {
        last_prompt = search;
        last_offset = search - all_text;
        search += strlen(prompt);
    }
    if (last_prompt && last_offset >= 0) {
        GtkTextIter prompt_iter, after_prompt, buffer_end;
        gtk_text_buffer_get_iter_at_offset(buffer, &prompt_iter, last_offset);
        after_prompt = prompt_iter;
        gtk_text_iter_forward_chars(&after_prompt, strlen(prompt));
        gtk_text_buffer_get_end_iter(buffer, &buffer_end);
        gtk_text_buffer_delete(buffer, &after_prompt, &buffer_end);
        if (new_input)
            insert_colored_text(buffer, new_input, "cmd");
        prompt_end_offset = last_offset + strlen(prompt);
    }
    g_free(all_text);
}

static void move_cursor_to_end(GtkTextView *textview) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    GtkTextMark *mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_buffer_place_cursor(buffer, &end);
    gtk_text_view_scroll_mark_onscreen(textview, mark);
}

static void on_calculate_clicked(GtkButton *btn, gpointer user_data) {
    CalculatorWidgets *widgets = (CalculatorWidgets *)user_data;
    const char *num1_str = gtk_entry_get_text(GTK_ENTRY(widgets->entry1));
    const char *op = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widgets->combo));
    const char *num2_str = gtk_entry_get_text(GTK_ENTRY(widgets->entry2));
    GtkWidget *result_label = widgets->result_label;
    GtkTextBuffer *buffer = widgets->buffer;

    double a = atof(num1_str);
    double b = atof(num2_str);
    double res = 0;
    char result[128];
    int valid = 1;
    if (strcmp(op, "+") == 0) res = a + b;
    else if (strcmp(op, "-") == 0) res = a - b;
    else if (strcmp(op, "*") == 0) res = a * b;
    else if (strcmp(op, "/") == 0) {
        if (b == 0) { valid = 0; snprintf(result, sizeof(result), "Error: Division by zero"); }
        else res = a / b;
    }
    else valid = 0;
    if (valid && !(strcmp(op, "/") == 0 && b == 0))
        snprintf(result, sizeof(result), "Result: %.2f", res);
    gtk_label_set_text(GTK_LABEL(result_label), result);
    // Also print to shell output, always on a new line
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, "\n", -1); // Insert newline first
    gtk_text_buffer_insert_with_tags_by_name(buffer, &end, result, -1, "output", NULL);
    gtk_text_buffer_insert(buffer, &end, "\n", -1);
}

static void show_calculator_dialog(GtkTextView *textview, GtkTextBuffer *buffer) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Calculator", GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(textview))),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        ("_Close"), GTK_RESPONSE_CLOSE,
        NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_container_add(GTK_CONTAINER(content), grid);

    GtkWidget *label1 = gtk_label_new("Number 1:");
    GtkWidget *entry1 = gtk_entry_new();
    GtkWidget *label2 = gtk_label_new("Operator:");
    GtkWidget *combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "+");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "-");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "*");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "/");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    GtkWidget *label3 = gtk_label_new("Number 2:");
    GtkWidget *entry2 = gtk_entry_new();
    GtkWidget *result_label = gtk_label_new("");
    GtkWidget *calc_btn = gtk_button_new_with_label("Calculate");

    gtk_grid_attach(GTK_GRID(grid), label1, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry1, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label2, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), combo, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label3, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry2, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), calc_btn, 0, 3, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), result_label, 0, 4, 2, 1);

    CalculatorWidgets *widgets = g_new0(CalculatorWidgets, 1);
    widgets->entry1 = entry1;
    widgets->combo = combo;
    widgets->entry2 = entry2;
    widgets->result_label = result_label;
    widgets->buffer = buffer;

    g_signal_connect(calc_btn, "clicked", G_CALLBACK(on_calculate_clicked), widgets);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_free(widgets);
}

static void run_command(GtkTextView *textview, GtkTextBuffer *buffer) {
    gchar *input = get_current_input(buffer);
    if (!input || strlen(input) == 0) {
        g_free(input);
        insert_colored_text(buffer, "\n", NULL);
        insert_colored_text(buffer, get_dynamic_prompt(), "prompt");
        move_cursor_to_end(textview);
        return;
    }

    // Special handling for 'clr' or 'clear' in GUI mode
    if (strcmp(input, "clr") == 0 || strcmp(input, "clear") == 0) {
        gtk_text_buffer_set_text(buffer, "", -1);
        insert_colored_text(buffer, get_dynamic_prompt(), "prompt");
        move_cursor_to_end(textview);
        g_free(input);
        return;
    }

    if (strcmp(input, "calculator") == 0) {
        show_calculator_dialog(textview, buffer);
        insert_colored_text(buffer, get_dynamic_prompt(), "prompt");
        move_cursor_to_end(textview);
        g_free(input);
        return;
    }

    if (history_len == 0 || strcmp(input, history[history_len-1]) != 0) {
        if (history_len < MAX_HISTORY) {
            history[history_len++] = g_strdup(input);
        } else {
            g_free(history[0]);
            memmove(&history[0], &history[1], sizeof(char*) * (MAX_HISTORY-1));
            history[MAX_HISTORY-1] = g_strdup(input);
        }
    }
    history_pos = history_len;

    char output[4096];
    minishell_execute(input, output, sizeof(output));
    insert_colored_text(buffer, "\n", NULL);
    if (strlen(output) > 0)
        insert_colored_text(buffer, output, "output");
    insert_colored_text(buffer, get_dynamic_prompt(), "prompt");
    move_cursor_to_end(textview);
    g_free(input);
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    GtkTextView *textview = GTK_TEXT_VIEW(widget);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);
    GtkTextIter insert_iter;
    gtk_text_buffer_get_iter_at_mark(buffer, &insert_iter, gtk_text_buffer_get_insert(buffer));
    int cursor_offset = gtk_text_iter_get_offset(&insert_iter);
    if ((event->keyval == GDK_KEY_BackSpace || event->keyval == GDK_KEY_Delete) &&
        cursor_offset <= prompt_end_offset) {
        return TRUE;
    }
    if (event->keyval == GDK_KEY_Home) {
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_get_iter_at_offset(buffer, &insert_iter, prompt_end_offset);
        gtk_text_buffer_place_cursor(buffer, &insert_iter);
        GtkTextMark *mark = gtk_text_buffer_get_insert(buffer);
        gtk_text_view_scroll_mark_onscreen(textview, mark);
        return TRUE;
    }
    switch (event->keyval) {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
            if (in_progress_input) { g_free(in_progress_input); in_progress_input = NULL; }
            run_command(textview, buffer);
            return TRUE;
        case GDK_KEY_Up:
            if (history_len > 0 && history_pos > 0) {
                if (history_pos == history_len && !in_progress_input) {
                    in_progress_input = get_current_input(buffer);
                }
                history_pos--;
                replace_input_after_prompt(buffer, history[history_pos]);
                move_cursor_to_end(textview);
            }
            return TRUE;
        case GDK_KEY_Down:
            if (history_len > 0 && history_pos < history_len - 1) {
                history_pos++;
                replace_input_after_prompt(buffer, history[history_pos]);
                move_cursor_to_end(textview);
            } else if (history_pos == history_len - 1) {
                history_pos = history_len;
                replace_input_after_prompt(buffer, in_progress_input ? in_progress_input : "");
                move_cursor_to_end(textview);
            }
            return TRUE;
    }
    return FALSE;
}

static gboolean on_textview_insert(GtkTextBuffer *buffer, GtkTextIter *location, gchar *text, gint len, gpointer user_data) {
    if (gtk_text_iter_get_offset(location) < prompt_end_offset) {
        return TRUE;
    }
    return FALSE;
}

static gboolean on_textview_delete(GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end, gpointer user_data) {
    if (gtk_text_iter_get_offset(start) < prompt_end_offset) {
        return TRUE;
    }
    return FALSE;
}

static void on_cursor_moved(GtkTextBuffer *buffer, const GtkTextIter *location, GtkTextMark *mark, gpointer user_data) {
    int offset = gtk_text_iter_get_offset(location);
    if (offset < prompt_end_offset) {
        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_offset(buffer, &iter, prompt_end_offset);
        gtk_text_buffer_place_cursor(buffer, &iter);
    }
}

static void on_maximize_restore(GtkButton *btn, gpointer user_data) {
    GtkWindow *win = GTK_WINDOW(user_data);
    if (gtk_window_is_maximized(win))
        gtk_window_unmaximize(win);
    else
        gtk_window_maximize(win);
}

int main(int argc, char *argv[]) {
    setenv("MINISHELL_GUI", "1", 1);
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Minishell v1.0");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 600);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    gtk_window_set_deletable(GTK_WINDOW(window), TRUE);

    GtkWidget *header = gtk_header_bar_new();
    // Create a custom label for the title
    GtkWidget *title_label = gtk_label_new("SheLL Coders Minishell");
    gtk_widget_set_halign(title_label, GTK_ALIGN_CENTER); // Center align
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.5);   // Center text
    gtk_widget_set_hexpand(title_label, TRUE);           // Allow to expand
    // Set bold, large font via CSS
    GtkCssProvider *title_css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(title_css,
        "label#header-title {"
        "  color: #ffde59;"
        "  font-weight: 900;"
        "  font-size: 22pt;"
        "  font-family: 'Segoe UI', 'Cantarell', 'Arial', 'sans-serif';"
        "  letter-spacing: 2px;"
        "  text-shadow: 1px 2px 6px #00000088;"
        "}"
        , -1, NULL);
    gtk_widget_set_name(title_label, "header-title");
    GtkStyleContext *title_ctx = gtk_widget_get_style_context(title_label);
    gtk_style_context_add_provider(title_ctx, GTK_STYLE_PROVIDER(title_css), GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(title_css);
    gtk_header_bar_set_custom_title(GTK_HEADER_BAR(header), title_label);
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
    gtk_window_set_titlebar(GTK_WINDOW(window), header);

    // Update headerbar CSS: remove ellipsize line
    GtkCssProvider *header_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(header_provider,
        "headerbar {"
        "  background: linear-gradient(90deg, #181825 0%, #232347 100%);"
        "  color: #ffde59;"
        "  font-family: 'Segoe UI', 'Cantarell', 'Arial', 'sans-serif';"
        "  font-weight: 900;"
        "  font-size: 22pt;"
        "  letter-spacing: 2px;"
        "  text-shadow: 1px 2px 6px #00000088;"
        "  border-bottom: 2px solid #ffcc00;"
        "  min-width: 600px;"
        "}"
        "headerbar .title {"
        "  color: #ffde59;"
        "  font-weight: 900;"
        "  font-size: 22pt;"
        "  font-family: 'Segoe UI', 'Cantarell', 'Arial', 'sans-serif';"
        "  letter-spacing: 2px;"
        "  text-shadow: 1px 2px 6px #00000088;"
        "  min-width: 500px;"
        "}"
        "headerbar button.titlebutton.close {"
        "  background: #ff3333;"
        "  color: #fff;"
        "  border-radius: 6px;"
        "  min-width: 36px;"
        "  min-height: 36px;"
        "  font-size: 18pt;"
        "  box-shadow: 0 2px 8px #0008;"
        "}"
        "headerbar button.titlebutton.close:hover {"
        "  background: #ff0000;"
        "  color: #fff;"
        "}"
        , -1, NULL);
    GtkStyleContext *header_context = gtk_widget_get_style_context(header);
    gtk_style_context_add_provider(header_context, GTK_STYLE_PROVIDER(header_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(header_provider);

    // Create custom minimize and maximize/restore buttons with standard icons
    GtkWidget *min_btn = gtk_button_new_with_label("-");
    gtk_widget_set_tooltip_text(min_btn, "Minimize");
    g_signal_connect_swapped(min_btn, "clicked", G_CALLBACK(gtk_window_iconify), window);
    gtk_widget_set_name(min_btn, "minimize-btn");
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), min_btn);

    GtkWidget *max_btn = gtk_button_new_with_label("☐"); // or "□"
    gtk_widget_set_tooltip_text(max_btn, "Maximize/Restore");
    g_signal_connect(max_btn, "clicked", G_CALLBACK(on_maximize_restore), window);
    gtk_widget_set_name(max_btn, "maximize-btn");
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), max_btn);

    // Find the close button in the header bar and set its name and label
    GtkWidget *close_btn = NULL;
    GList *children = gtk_container_get_children(GTK_CONTAINER(header));
    for (GList *l = children; l != NULL; l = l->next) {
        GtkWidget *child = GTK_WIDGET(l->data);
        if (GTK_IS_BUTTON(child) && gtk_widget_get_tooltip_text(child) &&
            strstr(gtk_widget_get_tooltip_text(child), "Close")) {
            close_btn = child;
            gtk_widget_set_name(close_btn, "close-btn");
            gtk_button_set_label(GTK_BUTTON(close_btn), "×");
            break;
        }
    }
    g_list_free(children);

    // Add minimal, standard CSS for the custom buttons (all three)
    GtkCssProvider *btns_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(btns_provider,
        "button#minimize-btn, button#maximize-btn, button#close-btn {"
        "  background: #232347;"
        "  color: #ffde59;"
        "  border-radius: 0px;"
        "  min-width: 32px;"
        "  min-height: 32px;"
        "  font-size: 16pt;"
        "  font-family: 'monospace', 'Consolas', 'Liberation Mono', 'Arial';"
        "  font-weight: normal;"
        "  margin-left: 2px;"
        "  margin-right: 2px;"
        "  border: none;"
        "  box-shadow: none;"
        "}"
        "button#minimize-btn:hover, button#maximize-btn:hover {"
        "  background: #35355a;"
        "  color: #fff;"
        "}"
        "button#close-btn:hover {"
        "  background: #ffdddd;"
        "  color: #c00;"
        "}"
        , -1, NULL);
    GtkStyleContext *min_ctx = gtk_widget_get_style_context(min_btn);
    GtkStyleContext *max_ctx = gtk_widget_get_style_context(max_btn);
    gtk_style_context_add_provider(min_ctx, GTK_STYLE_PROVIDER(btns_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_style_context_add_provider(max_ctx, GTK_STYLE_PROVIDER(btns_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    if (close_btn) {
        GtkStyleContext *close_ctx = gtk_widget_get_style_context(close_btn);
        gtk_style_context_add_provider(close_ctx, GTK_STYLE_PROVIDER(btns_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    }
    g_object_unref(btns_provider);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    GtkWidget *textview = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_CHAR);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(textview), TRUE);
    gtk_container_add(GTK_CONTAINER(scrolled), textview);

    // Dark background + bright text
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "textview, textview text, textview > * {"
        "  background-color: #0a0a0e;"
        "  color: #f0f0f0;"
        "  font-family: 'Fira Mono', 'JetBrains Mono', 'Consolas', 'monospace';"
        "  font-size: 16pt;"
        "  padding: 16px;"
        "  caret-color: #ffcc00;"
        "}",
        -1, NULL);
    GtkStyleContext *context = gtk_widget_get_style_context(textview);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(buffer);

    GtkTextTag *tag_prompt = gtk_text_tag_new("prompt");
    g_object_set(tag_prompt, "foreground", "#539bf5", "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_tag_table_add(tag_table, tag_prompt);

    GtkTextTag *tag_cmd = gtk_text_tag_new("cmd");
    g_object_set(tag_cmd, "foreground", "#7fffd4", "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_tag_table_add(tag_table, tag_cmd);

    GtkTextTag *tag_output = gtk_text_tag_new("output");
    g_object_set(tag_output, "foreground", "#f5f5f5", NULL);
    gtk_text_tag_table_add(tag_table, tag_output);

    GtkTextTag *tag_error = gtk_text_tag_new("error");
    g_object_set(tag_error, "foreground", "#ff6f6f", "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_tag_table_add(tag_table, tag_error);

    GtkTextTag *tag_welcome = gtk_text_tag_new("welcome");
    g_object_set(tag_welcome, "foreground", "#ffcc00", "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_tag_table_add(tag_table, tag_welcome);

    GtkTextTag *tag_info = gtk_text_tag_new("info");
    g_object_set(tag_info, "foreground", "#aaaaaa", NULL);
    gtk_text_tag_table_add(tag_table, tag_info);

    insert_colored_text(buffer, "Welcome to Minishell!\n", "welcome");
    insert_colored_text(buffer, "Minishell Initialized. Type 'help' for a list of commands.\n\n", "info");
    insert_colored_text(buffer, get_dynamic_prompt(), "prompt");
    move_cursor_to_end(GTK_TEXT_VIEW(textview));

    g_signal_connect(textview, "key-press-event", G_CALLBACK(on_key_press), NULL);
    g_signal_connect(buffer, "insert-text", G_CALLBACK(on_textview_insert), NULL);
    g_signal_connect(buffer, "delete-range", G_CALLBACK(on_textview_delete), NULL);
    g_signal_connect(buffer, "mark-set", G_CALLBACK(on_cursor_moved), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    for (int i = 0; i < history_len; ++i) g_free(history[i]);
    if (in_progress_input) g_free(in_progress_input);
    return 0;
}
