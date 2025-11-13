#include "lvgl.h"
#include "ui_manager.h"
#include <stdio.h>
#include <string.h>

// ------------------- Internal state -------------------
static lv_obj_t *calc_label;
static char calc_buffer[64] = "";

// ------------------- Helpers -------------------
static void calc_press(lv_event_t *e);
static void swipe_back_cb(lv_event_t *e);

// ------------------- Styling -------------------
static void style_button(lv_obj_t *btn, bool is_operator)
{
    lv_obj_set_size(btn, 55, 55);
    lv_obj_set_style_radius(btn, 28, 0);
    lv_obj_set_style_border_width(btn, 0, 0);

    if (is_operator) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF9A00), 0);   // orange
        lv_obj_set_style_text_color(btn, lv_color_white(), 0);
    } else {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xFFFFFF), 0);   // white
        lv_obj_set_style_text_color(btn, lv_color_hex(0x000000), 0);
    }

    lv_obj_set_style_text_font(btn, &lv_font_montserrat_20, 0);
}

// ------------------- Calculator Screen -------------------
lv_obj_t *build_calculator_screen(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // ----- Display label -----
    calc_label = lv_label_create(scr);
    lv_label_set_text(calc_label, "0");
    lv_obj_set_style_text_color(calc_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(calc_label, &lv_font_montserrat_28, 0);
    lv_obj_align(calc_label, LV_ALIGN_TOP_RIGHT, -12, 12);

    // Divider line
    lv_obj_t *line = lv_obj_create(scr);
    lv_obj_set_style_bg_color(line, lv_color_white(), 0);
    lv_obj_set_size(line, 220, 2);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_border_width(line, 0, 0);

    // ---------------- Grid container ----------------
    lv_obj_t *grid = lv_obj_create(scr);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_size(grid, 220, 210);
    lv_obj_align(grid, LV_ALIGN_BOTTOM_MID, 0, -5);

    static lv_coord_t cols[] = {55, 55, 55, 55, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t rows[] = {55, 55, 55, 55, LV_GRID_TEMPLATE_LAST};

    lv_obj_set_grid_dsc_array(grid, cols, rows);

    // --------------- Button map (matches your UI) ---------------
    const char *labels[4][4] = {
        {"AC", "/",  "x",  "-"},
        {"7",  "8",  "9",  "+"},
        {"4",  "5",  "6",  "="},
        {"1",  "2",  "3",  "0/."}
    };

    // Operators for orange buttons
    const char *ops = "AC/ x-+=0/.";

    // ------------- Create grid buttons -------------
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {

            lv_obj_t *btn = lv_btn_create(grid);
            bool is_op = (strchr("/x-+=AC", labels[r][c][0]) != NULL);
            style_button(btn, is_op);

            lv_obj_set_grid_cell(btn,
                                 LV_GRID_ALIGN_CENTER, c, 1,
                                 LV_GRID_ALIGN_CENTER, r, 1);

            lv_obj_t *lbl = lv_label_create(btn);
            lv_label_set_text(lbl, labels[r][c]);
            lv_obj_center(lbl);

            lv_obj_add_event_cb(btn, calc_press, LV_EVENT_CLICKED, (void *)labels[r][c]);
        }
    }

    // Swipe-back gesture
    lv_obj_add_event_cb(scr, swipe_back_cb, LV_EVENT_GESTURE, NULL);

    return scr;
}

// ------------------- Button Handler -------------------
static void calc_press(lv_event_t *e)
{
    const char *txt = lv_event_get_user_data(e);

    if (strcmp(txt, "AC") == 0) {
        calc_buffer[0] = 0;
    }
    else if (strcmp(txt, "=") == 0) {
        double a, b; char op;
        if (sscanf(calc_buffer, "%lf %c %lf", &a, &op, &b) == 3) {
            double r = 0;
            switch (op) {
                case '+': r = a + b; break;
                case '-': r = a - b; break;
                case 'x': r = a * b; break;
                case '/': r = b != 0 ? a / b : 0; break;
            }
            snprintf(calc_buffer, sizeof(calc_buffer), "%.2f", r);
        }
    }
    else {
        // Append number or operator
        if (strlen(calc_buffer) < sizeof(calc_buffer) - 4) {
            strcat(calc_buffer, txt);
            if (strlen(txt) == 1 && strchr("+-/x", txt[0]))
                strcat(calc_buffer, " ");
        }
    }

    lv_label_set_text(calc_label,
                      calc_buffer[0] ? calc_buffer : "0");
}

// ------------------- Swipe Back -------------------
static void swipe_back_cb(lv_event_t *e)
{
    if (lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT)
        ui_show_menu();
}
