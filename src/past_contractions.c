#include <pebble.h>
#include "past_contractions.h"
#include "contraction_menu.h"
#include "store.h"

static Window *window;
static MenuLayer *menu_layer;
static TextLayer *empty_menu_layer;

static int number_of_contractions;

// Menu layer callbacks
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return store_number_of_date_sections();
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return store_number_of_contractions_for_date_section(section_index);
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  char date[] = "Jan 01";
  store_date_for_date_section(date, sizeof(date), section_index);

  menu_cell_basic_header_draw(ctx, cell_layer, date);
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  Contraction contraction;
  int status = store_contraction_for_date_section_index(cell_index->section, cell_index->row, &contraction);

  char title_text[] = "00:00 XX";
  char subtitle_text[32];

  if (status == sizeof(Contraction)) {
    struct tm *start_datetime = localtime(&contraction.start_time);
    int seconds_elapsed = contraction.seconds_elapsed;

    int hour = start_datetime->tm_hour;
    int minute = start_datetime->tm_min;

    store_time_for_hour_minute(title_text, sizeof(title_text), hour, minute);

    char lasted_text[] = "Lasted";
    char minute_text[] = "min";
    char second_text[] = "sec";
    char plural_suffix[] = "s";

    int minutes = seconds_elapsed / 60;
    int seconds = seconds_elapsed % 60;

    if (seconds_elapsed > 60) {
      snprintf(
        subtitle_text,
        sizeof(subtitle_text),
        "%s %d %s%s %d %s%s",
        lasted_text,
        minutes,
        minute_text,
        minutes == 1 ? "" : plural_suffix,
        seconds,
        second_text,
        seconds == 1 ? "" : plural_suffix);
    } else {
      snprintf(
        subtitle_text,
        sizeof(subtitle_text),
        "%s %d %s%s",
        lasted_text,
        seconds_elapsed,
        second_text,
        seconds == 1 ? "" : plural_suffix);
    }
  }

  menu_cell_basic_draw(ctx, cell_layer, title_text, subtitle_text, NULL);
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  if (number_of_contractions > 0) {
    show_contraction_menu(store_contraction_key(cell_index->section, cell_index->row));
  }
}

// Window handlers
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  // Menu layer
  menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });
  menu_layer_set_click_config_onto_window(menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(menu_layer));

  // Empty menu layer
  const int empty_menu_height = 28 * 2;
  empty_menu_layer = text_layer_create(GRect(0, (bounds.size.h - empty_menu_height) / 2, bounds.size.w, empty_menu_height));
  text_layer_set_text(empty_menu_layer, "No contractions recorded yet.");
  text_layer_set_text_alignment(empty_menu_layer, GTextAlignmentCenter);
  text_layer_set_font(empty_menu_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(empty_menu_layer));
}

static void window_appear(Window *window) {
  number_of_contractions = store_number_of_past_contractions();
  if (number_of_contractions > 0) {
    menu_layer_reload_data(menu_layer);
    layer_set_hidden(text_layer_get_layer(empty_menu_layer), true);
  } else {
    layer_set_hidden(text_layer_get_layer(empty_menu_layer), false);
  }
}

static void window_unload(Window *window) {
  menu_layer_destroy(menu_layer);
  text_layer_destroy(empty_menu_layer);
}

void show_past_contractions() {
  window_stack_push(window, true);
}

void past_contractions_init() {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .appear = window_appear,
    .unload = window_unload
  });
}

void past_contractions_deinit() {
  window_destroy(window);
}