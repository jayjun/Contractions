#include <pebble.h>
#include "delete_contraction.h"
#include "store.h"

static int number_of_contractions;
static uint32_t key;
static bool corrupted;

static GBitmap *action_icon_ok;
static GBitmap *action_icon_cancel;

static Window *window;
static ActionBarLayer *action_bar_layer;
static Layer *delete_layer;

static TextLayer *warning_title_layer;
static char warning_title_warning_text[] = "WARNING!";
static char warning_title_corrupted_text[] = "ERROR!";
static TextLayer *warning_layer;
static char warning_delete_all_text[] = "Delete ALL contractions?";
static char warning_delete_corrupted_text[] = "Data corrupted. Delete?";
static char warning_delete_text[] = "Delete this contraction?";

static TextLayer *up_button_text_layer;
static TextLayer *down_button_text_layer;
static char confirm_text[] = "CONFIRM";
static char cancel_text[] = "CANCEL";

static TextLayer *nothing_text_layer;
static char nothing_text[] = "No contractions recorded\nto delete.";

// Static methods
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (number_of_contractions > 0) {
    window_stack_pop(window);
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (number_of_contractions) {
    if (key) {
      store_remove_contraction(key);

      if (store_number_of_past_contractions() == 0) {
        window_stack_pop(false);
      }
      window_stack_pop(true);
    } else {
      store_remove_all_contractions();
    }
    window_stack_pop(true);
  }
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler)up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler)down_click_handler);
}

// Window callbacks
static void window_load(Window *window) {
  action_bar_layer = action_bar_layer_create();
  action_bar_layer_add_to_window(action_bar_layer, window);
  action_bar_layer_set_click_config_provider(action_bar_layer, click_config_provider);

  action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_UP, action_icon_ok);
  action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_DOWN, action_icon_cancel);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  const int16_t width = bounds.size.w - ACTION_BAR_WIDTH;
  const int16_t height = bounds.size.h;

  const int16_t total_warning_text_height = 24 * 3;
  const int16_t warning_title_y = (height - total_warning_text_height) / 2;

  delete_layer = layer_create(bounds);
  layer_add_child(window_layer, delete_layer);

  warning_title_layer = text_layer_create(GRect(0, warning_title_y - 6, width, 24));
  text_layer_set_text(warning_title_layer, warning_title_warning_text);
  text_layer_set_font(warning_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(warning_title_layer, GTextAlignmentCenter);
  layer_add_child(delete_layer, text_layer_get_layer(warning_title_layer));

  warning_layer = text_layer_create(GRect(0, warning_title_y + 24 - 6, width, 24 * 2));
  text_layer_set_font(warning_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(warning_layer, GTextAlignmentCenter);
  layer_add_child(delete_layer, text_layer_get_layer(warning_layer));

  up_button_text_layer = text_layer_create(GRect(0, 19, width - 4, 14));
  text_layer_set_text(up_button_text_layer, confirm_text);
  text_layer_set_font(up_button_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(up_button_text_layer, GTextAlignmentRight);
  layer_add_child(delete_layer, text_layer_get_layer(up_button_text_layer));

  down_button_text_layer = text_layer_create(GRect(0, height - 13 - 24, width - 4, 14));
  text_layer_set_text(down_button_text_layer, cancel_text);
  text_layer_set_font(down_button_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(down_button_text_layer, GTextAlignmentRight);
  layer_add_child(delete_layer, text_layer_get_layer(down_button_text_layer));

  const int nothing_text_height = 24 * 3;
  const int nothing_text_y = (height - nothing_text_height) / 2 - 6;
  nothing_text_layer = text_layer_create(GRect(0, nothing_text_y, bounds.size.w, nothing_text_height));
  text_layer_set_text(nothing_text_layer, nothing_text);
  text_layer_set_font(nothing_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(nothing_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(nothing_text_layer));
}

static void window_appear(Window *window) {
  number_of_contractions = store_number_of_past_contractions();
  if (number_of_contractions > 0) {
    layer_set_hidden(text_layer_get_layer(nothing_text_layer), true);
    layer_set_hidden(delete_layer, false);
    layer_set_hidden(action_bar_layer_get_layer(action_bar_layer), false);

    switch (key) {
      case DELETE_ALL_CONTRACTIONS_KEY:
        text_layer_set_text(warning_title_layer, warning_title_warning_text);
        text_layer_set_text(warning_layer, warning_delete_all_text);
        break;

      default:
        if (corrupted) {
          text_layer_set_text(warning_title_layer, warning_title_corrupted_text);
          text_layer_set_text(warning_layer, warning_delete_corrupted_text);
        } else {
          text_layer_set_text(warning_title_layer, warning_title_warning_text);
          text_layer_set_text(warning_layer, warning_delete_text);
        }
    }
  } else {
    layer_set_hidden(text_layer_get_layer(nothing_text_layer), false);
    layer_set_hidden(delete_layer, true);
    layer_set_hidden(action_bar_layer_get_layer(action_bar_layer), true);
  }
}

static void window_unload(Window *window) {
  text_layer_destroy(warning_title_layer);
  text_layer_destroy(warning_layer);
  text_layer_destroy(up_button_text_layer);
  text_layer_destroy(down_button_text_layer);
  text_layer_destroy(nothing_text_layer);
  layer_destroy(delete_layer);
  action_bar_layer_destroy(action_bar_layer);
}

// Non-static methods
void show_delete_contraction(uint32_t contraction_key, bool is_corrupted) {
  key = contraction_key;
  corrupted = is_corrupted;
  window_stack_push(window, true);
}

void delete_contraction_init() {
  action_icon_ok = gbitmap_create_with_resource(RESOURCE_ID_ACTION_ICON_OK);
  action_icon_cancel = gbitmap_create_with_resource(RESOURCE_ID_ACTION_ICON_CANCEL);

  window = window_create();

  window_set_window_handlers(window, (WindowHandlers){
    .load = window_load,
    .appear = window_appear,
    .unload = window_unload,
  });
}

void delete_contraction_deinit() {
  gbitmap_destroy(action_icon_ok);
  gbitmap_destroy(action_icon_cancel);
  window_destroy(window);
}