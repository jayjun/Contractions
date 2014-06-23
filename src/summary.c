#include <pebble.h>
#include "store.h"
#include "new_contraction.h"

static Window *window;

static ActionBarLayer *action_bar_layer;
static GBitmap *action_icon_play;
static GBitmap *action_icon_60;
static GBitmap *action_icon_30;

static TextLayer *title_layer;
static char title_1_hour_text[] = "PAST 1 HOUR";
static char title_30_minutes_text[] = "PAST 30 MINS";

static Layer *line_layer;

static TextLayer *total_layer;
static TextLayer *total_title_layer;
static char total_text[] = "XX";
static char total_title_text_1[] = "CONTRACTION";
static char total_title_text_n[] = "CONTRACTIONS";

static TextLayer *average_duration_layer;
static TextLayer *average_duration_title_layer;
static char average_duration_text[] = "00:00";
static char average_duration_title[] = "AVERAGE\nDURATION";

static TextLayer *average_interval_layer;
static TextLayer *average_interval_title_layer;
static char average_interval_text[] = "00:00";
static char average_interval_title[] = "AVERAGE\nINTERVAL";

static int range_in_minutes = 60;

// Click handlers
static void update_text_layer_titles() {
  SummaryResult result = store_calculate_summary(range_in_minutes);

  int count = result.count;
  if (count == 1) {
    text_layer_set_text(total_title_layer, total_title_text_1);
  } else {
    text_layer_set_text(total_title_layer, total_title_text_n);
  }
  snprintf(total_text, sizeof(total_text), "%d", count);
  text_layer_set_text(total_layer, total_text);

  int average_duration_minutes = result.average_duration_in_seconds / 60;
  int average_duration_seconds = result.average_duration_in_seconds % 60;
  snprintf(average_duration_text, sizeof(average_duration_text), "%02d:%02d", average_duration_minutes, average_duration_seconds);
  text_layer_set_text(average_duration_layer, average_duration_text);

  int average_interval_minutes = result.average_interval_in_seconds / 60;
  int average_interval_seconds = result.average_interval_in_seconds % 60;
  snprintf(average_interval_text, sizeof(average_interval_text), "%02d:%02d", average_interval_minutes, average_interval_seconds);
  text_layer_set_text(average_interval_layer, average_interval_text);
}

static void show_new_contraction_handler(ClickRecognizerRef recognizer, void *context) {
  show_new_contraction();
}

static void show_60_mins_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(title_layer, title_1_hour_text);
  range_in_minutes = 60;
  update_text_layer_titles();
}

static void show_30_mins_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(title_layer, title_30_minutes_text);
  range_in_minutes = 30;
  update_text_layer_titles();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler)show_new_contraction_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler)show_60_mins_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler)show_30_mins_handler);
}

// Layer callbacks
static void line_layer_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

// Window handlers
static void window_load(Window *window) {
  // Action Bar
  action_bar_layer = action_bar_layer_create();

  action_bar_layer_add_to_window(action_bar_layer, window);
  action_bar_layer_set_click_config_provider(action_bar_layer, click_config_provider);

  action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_UP, action_icon_play);
  action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_SELECT, action_icon_60);
  action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_DOWN, action_icon_30);

  // UI
  Layer *window_layer = window_get_root_layer(window);
  GRect window_frame = layer_get_frame(window_layer);

  const int16_t width = window_frame.size.w - ACTION_BAR_WIDTH;
  const int16_t padding = 6;

  GRect title_frame = GRect(0, 0, width, 18);
  title_layer = text_layer_create(title_frame);
  text_layer_set_text(title_layer, title_1_hour_text);
  text_layer_set_font(title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(title_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(title_layer));

  GRect line_frame = GRect(padding, title_frame.origin.y + title_frame.size.h + padding, width - padding * 2, 1);
  line_layer = layer_create(line_frame);
  layer_set_update_proc(line_layer, line_layer_draw);
  layer_add_child(window_layer, line_layer);

  // Timer
  GRect total_frame = GRect(0, line_frame.origin.y + line_frame.size.h, width, 30);
  total_layer = text_layer_create(total_frame);
  text_layer_set_text(total_layer, total_text);
  text_layer_set_font(total_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(total_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(total_layer));

  GRect total_title_frame = GRect(0, total_frame.origin.y + total_frame.size.h, width, 18);
  total_title_layer = text_layer_create(total_title_frame);
  text_layer_set_text(total_title_layer, total_title_text_1);
  text_layer_set_font(total_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(total_title_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(total_title_layer));
  
  // Duration
  GRect average_duration_frame = GRect(0, total_title_frame.origin.y + total_title_frame.size.h + padding, width / 2, 28);
  average_duration_layer = text_layer_create(average_duration_frame);
  text_layer_set_text(average_duration_layer, average_duration_text);
  text_layer_set_font(average_duration_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(average_duration_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(average_duration_layer));

  GRect average_duration_title_frame = GRect(0, average_duration_frame.origin.y + average_duration_frame.size.h, width / 2, 14 * 2);
  average_duration_title_layer = text_layer_create(average_duration_title_frame);
  text_layer_set_text(average_duration_title_layer, average_duration_title);
  text_layer_set_font(average_duration_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(average_duration_title_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(average_duration_title_layer));

  // Interval
  GRect average_interval_frame = GRect(width / 2, average_duration_frame.origin.y, width / 2, 28);
  average_interval_layer = text_layer_create(average_interval_frame);
  text_layer_set_text(average_interval_layer, average_interval_text);
  text_layer_set_font(average_interval_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(average_interval_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(average_interval_layer));

  GRect average_interval_title_frame = GRect(width / 2, average_duration_title_frame.origin.y, width / 2, 14 * 2);
  average_interval_title_layer = text_layer_create(average_interval_title_frame);
  text_layer_set_text(average_interval_title_layer, average_interval_title);
  text_layer_set_font(average_interval_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(average_interval_title_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(average_interval_title_layer));

  update_text_layer_titles();
}


static void window_appear(Window *window) {
  update_text_layer_titles();
}

static void window_unload(Window *window) {
  text_layer_destroy(title_layer);
  layer_destroy(line_layer);
  text_layer_destroy(total_layer);
  text_layer_destroy(total_title_layer);
  text_layer_destroy(average_duration_layer);
  text_layer_destroy(average_duration_title_layer);
  text_layer_destroy(average_interval_layer);
  text_layer_destroy(average_interval_title_layer);

  // Action Bar
  action_bar_layer_destroy(action_bar_layer);
}

void show_summary(void) {
  window_stack_push(window, true);
}

void summary_init(void) {
  action_icon_play = gbitmap_create_with_resource(RESOURCE_ID_ACTION_ICON_PLAY);
  action_icon_60 = gbitmap_create_with_resource(RESOURCE_ID_ACTION_ICON_60);
  action_icon_30 = gbitmap_create_with_resource(RESOURCE_ID_ACTION_ICON_30);

  window = window_create();

  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .appear = window_appear,
    .unload = window_unload
  });
}

void summary_deinit(void) {
  gbitmap_destroy(action_icon_play);
  gbitmap_destroy(action_icon_60);
  gbitmap_destroy(action_icon_30);
  window_destroy(window);
}