#include <pebble.h>
#include "new_contraction.h"
#include "store.h"

typedef enum {
  Start,
  TimerStarted,
  TimerStopped
} ScreenState;

static ScreenState screenState;
static struct tm *start_time;
static int seconds_elapsed;

static GBitmap *action_icon_play;
static GBitmap *action_icon_stop;
static GBitmap *action_icon_ok;
static GBitmap *action_icon_cancel;

static Window *window;
static ActionBarLayer *action_bar_layer;

static char timer_title_text[] = "TIME ELAPSED";
static TextLayer *timer_title_layer;
static char timer_text[] = "00:00";
static TextLayer *timer_layer;

static char start_text[] = "START";
static char stop_text[] = "STOP";
static char save_text[] = "SAVE";
static char cancel_text[] = "CANCEL";
static TextLayer *up_button_text_layer;
static TextLayer *down_button_text_layer;

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (start_time == NULL) {
    start_time = tick_time;
  }

  seconds_elapsed++;

  int minutes_elapsed = seconds_elapsed / 60;
  int seconds_remainder = seconds_elapsed - minutes_elapsed * 60;

  snprintf(timer_text, sizeof(timer_text), "%02d:%02d", minutes_elapsed, seconds_remainder);
  text_layer_set_text(timer_layer, timer_text);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (screenState == TimerStopped) {
    window_stack_pop(window);
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  switch (screenState) {
    case Start:
      tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
      action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_UP, action_icon_stop);
      text_layer_set_text(up_button_text_layer, stop_text);
      screenState = TimerStarted;
      break;

    case TimerStarted:
      tick_timer_service_unsubscribe();
      action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_UP, action_icon_ok);
      text_layer_set_text(up_button_text_layer, save_text);
      action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_DOWN, action_icon_cancel);
      text_layer_set_text(down_button_text_layer, cancel_text);
      screenState = TimerStopped;
      break;

    case TimerStopped: {
      store_insert_contraction(time(NULL), seconds_elapsed);
      down_click_handler(recognizer, context);
    } break;
  }
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler)up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler)down_click_handler);
}

static void reset() {
  screenState = Start;
  start_time = NULL;
  seconds_elapsed = 0;

  snprintf(timer_text, sizeof(timer_text), "00:00");
  text_layer_set_text(timer_layer, timer_text);

  text_layer_set_text(up_button_text_layer, start_text);
  text_layer_set_text(down_button_text_layer, NULL);

  action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_UP, action_icon_play);
  action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_SELECT, NULL);
  action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_DOWN, NULL);
}

// Window callbacks
static void window_load(Window *window) {
  action_bar_layer = action_bar_layer_create();
  action_bar_layer_add_to_window(action_bar_layer, window);
  action_bar_layer_set_click_config_provider(action_bar_layer, click_config_provider);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  const int16_t width = bounds.size.w - ACTION_BAR_WIDTH;
  const int16_t height = bounds.size.h;

  const int16_t total_timer_text_height = 18 + 34;
  const int16_t timer_title_y = (height - total_timer_text_height) / 2;

  timer_title_layer = text_layer_create(GRect(0, timer_title_y - 6, width, 18));
  text_layer_set_text(timer_title_layer, timer_title_text);
  text_layer_set_font(timer_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(timer_title_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(timer_title_layer));

  timer_layer = text_layer_create(GRect(0, timer_title_y + 18 - 6, width, 34));
  text_layer_set_font(timer_layer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
  text_layer_set_text_alignment(timer_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(timer_layer));

  up_button_text_layer = text_layer_create(GRect(0, 19, width - 4, 14));
  text_layer_set_font(up_button_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(up_button_text_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(up_button_text_layer));

  down_button_text_layer = text_layer_create(GRect(0, height - 13 - 24, width - 4, 14));
  text_layer_set_font(down_button_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(down_button_text_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(down_button_text_layer));
}

static void window_appear(Window *window) {
  reset();
}

static void window_disappear(Window *window) {
  tick_timer_service_unsubscribe();
}

static void window_unload(Window *window) {
  text_layer_destroy(timer_title_layer);
  text_layer_destroy(timer_layer);
  text_layer_destroy(up_button_text_layer);
  text_layer_destroy(down_button_text_layer);
  action_bar_layer_destroy(action_bar_layer);
}

void show_new_contraction() {
  window_stack_push(window, true);
}

void new_contraction_init() {
  action_icon_play = gbitmap_create_with_resource(RESOURCE_ID_ACTION_ICON_PLAY);
  action_icon_stop = gbitmap_create_with_resource(RESOURCE_ID_ACTION_ICON_STOP);
  action_icon_ok = gbitmap_create_with_resource(RESOURCE_ID_ACTION_ICON_OK);
  action_icon_cancel = gbitmap_create_with_resource(RESOURCE_ID_ACTION_ICON_CANCEL);

  window = window_create();

  window_set_window_handlers(window, (WindowHandlers){
    .load = window_load,
    .appear = window_appear,
    .disappear = window_disappear,
    .unload = window_unload,
  });
}

void new_contraction_deinit() {
  window_destroy(window);
}