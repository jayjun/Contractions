#include <pebble.h>
#include "store.h"
#include "delete_contraction.h"

typedef enum {
  // EditRow,
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
  char date_time[] = "Jan 01 , 00:00 AM";
  store_date_time_for_contraction(date_time, sizeof(date_time), key);
  menu_cell_basic_header_draw(ctx, cell_layer, date_time);
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->row) {
    // case EditRow:
    //   menu_cell_basic_draw(ctx, cell_layer, "Edit", NULL, NULL);
    //   break;

    case DeleteRow: {
      menu_cell_basic_draw(ctx, cell_layer, "Delete", NULL, trash_icon);
    } break;

    default:
      break;
  }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->row) {
    // case EditRow:
    //   break;

    case DeleteRow:
      show_delete_contraction(key);
      break;

    default:
      break;
  }
}

// Window callbacks
static void window_load() {
  trash_icon = gbitmap_create_with_resource(RESOURCE_ID_MENU_ICON_TRASH);

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

static void window_unload() {
  menu_layer_destroy(menu_layer);
}

void show_contraction_menu(uint32_t contraction_key) {
  key = contraction_key;
  window_stack_push(window, true);
}

void contraction_menu_init() {
  window = window_create();

  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
}

void contraction_menu_deinit() {
  window_destroy(window);
}