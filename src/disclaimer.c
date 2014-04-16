#include <pebble.h>
#include "disclaimer.h"
#include "store.h"
#include "menu.h"

static char disclaimer_title_text[] = "DISCLAIMER";
static char disclaimer_text[] =
    "By proceeding, I acknowledge that this app DOES NOT provide medical advice.\n\n"
    "Please consult your physician or obstetrician for proper guidance at all times.\n\n"
    "USE AT YOUR\nOWN RISK!";

static GBitmap *action_icon_ok;
static GBitmap *action_icon_cancel;

static Window *window;
static TextLayer *disclaimer_title_layer;
static TextLayer *disclaimer_layer;
static ScrollLayer *scroll_layer;

// Static methods
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  store_set_disclaimer_shown(true);
  window_stack_pop(true);
  show_menu();
}

static void scroll_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler)select_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  const int padding = 6;
  const int width = bounds.size.w - padding * 2;

  const int disclaimer_title_height = 28;
  disclaimer_title_layer = text_layer_create(GRect(padding, 0, width, disclaimer_title_height));
  text_layer_set_text(disclaimer_title_layer, disclaimer_title_text);
  text_layer_set_font(disclaimer_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(disclaimer_title_layer, GTextAlignmentCenter);

  const int disclaimer_height = 18 * 13 + padding;
  disclaimer_layer = text_layer_create(GRect(padding, disclaimer_title_height + padding, width, disclaimer_height));
  text_layer_set_text(disclaimer_layer, disclaimer_text);
  text_layer_set_font(disclaimer_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(disclaimer_layer, GTextAlignmentCenter);

  GSize content_size = GSize(width, disclaimer_title_height + disclaimer_height + padding);
  scroll_layer = scroll_layer_create(bounds);
  scroll_layer_add_child(scroll_layer, text_layer_get_layer(disclaimer_title_layer));
  scroll_layer_add_child(scroll_layer, text_layer_get_layer(disclaimer_layer));
  scroll_layer_set_click_config_onto_window(scroll_layer, window);
  scroll_layer_set_callbacks(scroll_layer, (ScrollLayerCallbacks) {
    .click_config_provider = scroll_click_config_provider
  });
  scroll_layer_set_content_size(scroll_layer, content_size);
  
  layer_add_child(window_layer, scroll_layer_get_layer(scroll_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(disclaimer_title_layer);
  text_layer_destroy(disclaimer_layer);
  scroll_layer_destroy(scroll_layer);
}

// Non-static methods
void show_disclaimer() {
  window_stack_push(window, true);
}

void disclaimer_init() {
  window = window_create();

  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
}

void disclaimer_deinit() {
  window_destroy(window);
}