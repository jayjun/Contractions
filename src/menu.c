#include <pebble.h>
#include "store.h"
#include "disclaimer.h"
#include "menu.h"
#include "summary.h"
#include "new_contraction.h"
#include "past_contractions.h"
#include "delete_contraction.h"

#define TIMER_LAYER_HEIGHT 28
#define TITLE_LAYER_HEIGHT 17

typedef enum {
  LogSection,
  SectionCount
} Section;

typedef enum {
  NewContractionRow,
  PastContractionsRow,
  SummaryRow,
  ClearRow,
  HelpRow,
  LogSectionRowCount
} LogSectionRow;

static GBitmap *report_icon;
static GBitmap *trash_icon;
static GBitmap *disclaimer_icon;

static Window *window;
static MenuLayer *menu_layer;

// Menu layer callbacks
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return SectionCount;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case LogSection:
      return LogSectionRowCount;

    default:
      return 0;
  }
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case LogSection:
      return MENU_CELL_BASIC_HEADER_HEIGHT;

    default:
      return 0;
  }
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case LogSection:
      menu_cell_basic_header_draw(ctx, cell_layer, "Labor Log");
      break;

    default:
      break;
  }
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->section) {
    case LogSection:
      switch (cell_index->row) {
        case NewContractionRow:
          menu_cell_basic_draw(ctx, cell_layer, "New Contraction", NULL, NULL);
          break;

        case PastContractionsRow: {
          char subtitle_text[] = "XX/64 recorded";
          snprintf(subtitle_text, sizeof(subtitle_text), "%d/64 recorded", store_number_of_past_contractions());
          menu_cell_basic_draw(ctx, cell_layer, "Past Contractions", subtitle_text, NULL);
        } break;

        case SummaryRow:
          menu_cell_basic_draw(ctx, cell_layer, "Summary", NULL, report_icon);
          break;

        case ClearRow:
          menu_cell_basic_draw(ctx, cell_layer, "Delete All", NULL, trash_icon);
          break;

        case HelpRow:
          menu_cell_basic_draw(ctx, cell_layer, "Disclaimer", NULL, disclaimer_icon);
          break;
      }
      break;
  }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->section) {
    case LogSection:
      switch (cell_index->row) {
        case NewContractionRow:
          show_new_contraction();
          break;

        case PastContractionsRow:
          show_past_contractions();
          break;

        case SummaryRow:
          show_summary();
          break;

        case ClearRow:
          show_delete_contraction(0);
          break;

        case HelpRow:
          show_disclaimer();
          break;

        default:
          break;
      }
      break;
  }
}

// Window callbacks
static void window_load(Window *window) {
  report_icon = gbitmap_create_with_resource(RESOURCE_ID_MENU_ICON_REPORT);
  trash_icon = gbitmap_create_with_resource(RESOURCE_ID_MENU_ICON_TRASH);
  disclaimer_icon = gbitmap_create_with_resource(RESOURCE_ID_MENU_ICON_DISCLAIMER);

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

static void window_unload(Window *window) {
  menu_layer_destroy(menu_layer);
}

// Non-static methods
void show_menu() {
  window_stack_push(window, true);
}

void menu_init() {
  window = window_create();

  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
}

void menu_deinit() {
  window_destroy(window);
}