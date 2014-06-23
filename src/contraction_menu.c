#include <pebble.h>
#include "store.h"
#include "contraction_menu.h"
#include "edit_contraction.h"
#include "delete_contraction.h"

typedef enum {
  EditStartTimeRow,
  EditIntervalRow,
  DeleteRow,
  RowCount
} Row;

static GBitmap *trash_icon;

static Window *window;
static MenuLayer *menu_layer;

static uint32_t key;

// Menu layer callbacks
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return 1;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return RowCount;
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  menu_cell_basic_header_draw(ctx, cell_layer, "Options");
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  char start_time_text[32];
  char end_time_text[32];

  store_time_text_for_contraction(
    start_time_text,
    sizeof(start_time_text),
    end_time_text,
    sizeof(end_time_text),
    key);

  char edit_start_time_detail[32];
  snprintf(edit_start_time_detail, sizeof(edit_start_time_detail), "Started %s", start_time_text);

  char edit_interval_detail[32];
  snprintf(edit_interval_detail, sizeof(edit_interval_detail), "Stopped %s", end_time_text);

  switch (cell_index->row) {
    case EditStartTimeRow:
      menu_cell_basic_draw(ctx, cell_layer, "Edit Start Time", edit_start_time_detail, NULL);
      break;

    case EditIntervalRow:
      menu_cell_basic_draw(ctx, cell_layer, "Edit Duration", edit_interval_detail, NULL);
      break;

    case DeleteRow:
      menu_cell_basic_draw(ctx, cell_layer, "Delete", NULL, trash_icon);
      break;

    default:
      break;
  }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->row) {
    case EditStartTimeRow:
      show_edit_contraction(key, EditStartTime);
      break;

    case EditIntervalRow:
      show_edit_contraction(key, EditInterval);
      break;

    case DeleteRow:
      show_delete_contraction(key, false);
      break;

    default:
      break;
  }
}

// Window callbacks
static void window_load() {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

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
}

static void window_appear() {
  menu_layer_reload_data(menu_layer);
}

static void window_unload() {
  menu_layer_destroy(menu_layer);
}

// Non-static functions
void show_contraction_menu(uint32_t contraction_key) {
  key = contraction_key;
  window_stack_push(window, true);
}

void pop_to_contraction_menu(uint32_t contraction_key) {
  key = contraction_key;
  window_stack_pop(true);
}

void contraction_menu_init() {
  trash_icon = gbitmap_create_with_resource(RESOURCE_ID_MENU_ICON_TRASH);

  window = window_create();

  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .appear = window_appear,
    .unload = window_unload
  });
}

void contraction_menu_deinit() {
  gbitmap_destroy(trash_icon);
  window_destroy(window);
}