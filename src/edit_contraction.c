#include <pebble.h>
#include "store.h"
#include "contraction_menu.h"
#include "edit_contraction.h"
#include "delete_contraction.h"

#define LONG_CLICK_DELAY 1000
#define SMALL_DELTA_IN_SECONDS 1
#define BIG_DELTA_IN_SECONDS 60

typedef enum {
  EditingState,
  SavingState
} ScreenState;

static ScreenState screen_state;
static bool showing_discard_changes;

static EditMode mode;
static uint32_t current_contraction_key;
static Contraction current_contraction;
static Contraction modified_contraction;
static Contraction previous_contraction;
static Contraction next_contraction;
static bool has_previous_contraction;
static bool has_next_contraction;

static GBitmap *action_icon_increment;
static GBitmap *action_icon_decrement;
static GBitmap *action_icon_ok;
static GBitmap *action_icon_yes;
static GBitmap *action_icon_no;

static Window *window;
static ActionBarLayer *action_bar_layer;
static TextLayer *title_text_layer;
static char title_start_time_text[] = "START TIME";
static char title_duration_text[] = "DURATION";

static TextLayer *big_text_layer;
static char big_text[32];

static TextLayer *small_text_layer;
static char small_text[32];

static TextLayer *error_title_text_layer;
static TextLayer *error_message_text_layer;
static char error_title_max_text[] = "MAXIMUM";
static char error_title_min_text[] = "MINIMUM";
static char error_message_min_text[] = "Reached previous log.";
static char error_message_max_start_time_text[] = "Reached stopped time.";
static char error_message_max_duration_text[] = "Reached next log.";
static char error_message_min_interval_text[] = "Cannot be negative.";

static TextLayer *up_button_text_layer;
static TextLayer *down_button_text_layer;

// Static functions
static bool data_is_modified() {
  return (mode == EditStartTime && current_contraction.start_time != modified_contraction.start_time) || 
         (mode == EditInterval && current_contraction.seconds_elapsed != modified_contraction.seconds_elapsed);
}

static void reset_ui() {
  screen_state = EditingState;
  showing_discard_changes = false;
}

static void update_ui() {
  if (screen_state == EditingState) {
    layer_set_hidden(text_layer_get_layer(error_title_text_layer), false);
    layer_set_hidden(text_layer_get_layer(error_message_text_layer), false);
    layer_set_hidden(text_layer_get_layer(down_button_text_layer), true);

    text_layer_set_text(up_button_text_layer, NULL);
    text_layer_set_text(down_button_text_layer, NULL);

    if (mode == EditStartTime) {
      if (current_contraction.start_time == modified_contraction.start_time) {
        action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_SELECT, NULL);
      } else {
        action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_SELECT, action_icon_ok);
      }

    } else if (mode == EditInterval) {
      if (current_contraction.seconds_elapsed == modified_contraction.seconds_elapsed) {
        action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_SELECT, NULL);
      } else {
        action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_SELECT, action_icon_ok);
      }
    }

    action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_UP, action_icon_increment);
    action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_DOWN, action_icon_decrement);

  } else if (screen_state == SavingState) {
    layer_set_hidden(text_layer_get_layer(error_title_text_layer), true);
    layer_set_hidden(text_layer_get_layer(error_message_text_layer), true);
    layer_set_hidden(text_layer_get_layer(down_button_text_layer), false);

    text_layer_set_text(up_button_text_layer, "SAVE CHANGES");
    if (showing_discard_changes) {
      text_layer_set_text(down_button_text_layer, "DISCARD CHANGES");
    } else {
      text_layer_set_text(down_button_text_layer, "RESET");
    }

    action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_UP, action_icon_yes);
    action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_SELECT, NULL);
    action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_DOWN, action_icon_no);
  }

  if (mode == EditStartTime) {
    struct tm *date_time = localtime(&modified_contraction.start_time);
    int hour = date_time->tm_hour;
    int minute = date_time->tm_min;
    int second = date_time->tm_sec;

    store_time_for_time(big_text, sizeof(big_text), hour, minute, second);

    // store_duration_for_seconds_elapsed(small_text, sizeof(small_text), modified_contraction.seconds_elapsed);
    int delta = modified_contraction.start_time - current_contraction.start_time;
    int positive_delta = abs(delta);
    int seconds = positive_delta % 60;
    int minutes = positive_delta / 60; 
    char changed_text[] = "Changed";

    if (delta > 0) {
      snprintf(small_text, sizeof(small_text), "%s +%d:%02d", changed_text, minutes, seconds);
    } else if (delta < 0) {
      snprintf(small_text, sizeof(small_text), "%s -%d:%02d", changed_text, minutes, seconds);
    } else {
      snprintf(small_text, sizeof(small_text), " ");
    }

    text_layer_set_text(title_text_layer, title_start_time_text);
    text_layer_set_text(big_text_layer, big_text);
    text_layer_set_text(small_text_layer, small_text);
    
  } else if (mode == EditInterval) {
    int seconds = modified_contraction.seconds_elapsed;
    int minutes = seconds / 60;

    modified_contraction.start_time += modified_contraction.seconds_elapsed;
    struct tm *date_time = localtime(&modified_contraction.start_time);
    modified_contraction.start_time -= modified_contraction.seconds_elapsed;

    int hour = date_time->tm_hour;
    int minute = date_time->tm_min;
    int second = date_time->tm_sec;

    if (minutes == 0) {
      snprintf(big_text, sizeof(big_text), "%d sec", seconds);
    } else {
      snprintf(big_text, sizeof(big_text), "%d min %d sec", minutes % 60, seconds % 60);
    }
    char end_time_text[16];
    store_time_for_time(end_time_text, sizeof(end_time_text), hour, minute, second);
    snprintf(small_text, sizeof(small_text), "Stops %s", end_time_text);

    text_layer_set_text(title_text_layer, title_duration_text);
    text_layer_set_text(big_text_layer, big_text);
    text_layer_set_text(small_text_layer, small_text);
  }
}

static void update_contraction_start_time(int delta) {
  int start_time = modified_contraction.start_time + delta;
  int seconds_elapsed = modified_contraction.seconds_elapsed - delta;

  if (delta < 0) {
    if (has_previous_contraction && (start_time < (previous_contraction.start_time + previous_contraction.seconds_elapsed))) {
      int end_time = modified_contraction.start_time + modified_contraction.seconds_elapsed;
      modified_contraction.start_time = previous_contraction.start_time + previous_contraction.seconds_elapsed;
      modified_contraction.seconds_elapsed = end_time - modified_contraction.start_time;
      text_layer_set_text(error_title_text_layer, error_title_min_text);
      text_layer_set_text(error_message_text_layer, error_message_min_text);

    } else {
      modified_contraction.start_time = start_time;
      modified_contraction.seconds_elapsed = seconds_elapsed;
      text_layer_set_text(error_title_text_layer, NULL);
      text_layer_set_text(error_message_text_layer, NULL);
    }

  } else if (delta > 0) {
    if (seconds_elapsed < 0) {
      modified_contraction.start_time = current_contraction.start_time + current_contraction.seconds_elapsed;
      modified_contraction.seconds_elapsed = 0;
      text_layer_set_text(error_title_text_layer, error_title_max_text);
      text_layer_set_text(error_message_text_layer, error_message_max_start_time_text);
      
    } else {
      modified_contraction.start_time = start_time;
      modified_contraction.seconds_elapsed = seconds_elapsed;
      text_layer_set_text(error_title_text_layer, NULL);
      text_layer_set_text(error_message_text_layer, NULL);
    }
  }
}

static void update_contraction_seconds_elapsed(int delta) {
  int result = modified_contraction.seconds_elapsed + delta;

  if (delta < 0) {
    if (result < 0) {
      modified_contraction.seconds_elapsed = 0;
      text_layer_set_text(error_title_text_layer, error_title_min_text);
      text_layer_set_text(error_message_text_layer, error_message_min_interval_text);

    } else {
      modified_contraction.seconds_elapsed = result;
      text_layer_set_text(error_title_text_layer, NULL);
      text_layer_set_text(error_message_text_layer, NULL);
    }

  } else if (delta > 0) {
    if (has_next_contraction && (modified_contraction.start_time + result >= next_contraction.start_time)) {
      modified_contraction.seconds_elapsed = next_contraction.start_time - modified_contraction.start_time;
      text_layer_set_text(error_title_text_layer, error_title_max_text);
      text_layer_set_text(error_message_text_layer, error_message_max_duration_text);

    } else {
      modified_contraction.seconds_elapsed = result;
      text_layer_set_text(error_title_text_layer, NULL);
      text_layer_set_text(error_message_text_layer, NULL);
    }
  }
}

// Click handlers
static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  switch(screen_state) {
    case EditingState:
      if (!data_is_modified()) {
        window_stack_pop(true);
      } else {
        // Save
        screen_state = SavingState;
        showing_discard_changes = true;
      }
      break;

    case SavingState:
      // Discard changes
      if (screen_state == SavingState && showing_discard_changes) {
        window_stack_pop(true);
      }
      break;
  }

  update_ui();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  switch (screen_state) {
    case EditingState:
      // Increment
      if (mode == EditStartTime) {
        update_contraction_start_time(SMALL_DELTA_IN_SECONDS);
      } else if (mode == EditInterval) {
        update_contraction_seconds_elapsed(SMALL_DELTA_IN_SECONDS);
      }
      break;

    case SavingState:
      // Confirm save
      pop_to_contraction_menu(store_replace_contraction(current_contraction_key, modified_contraction.start_time, modified_contraction.seconds_elapsed));
      break;
  }

  update_ui();
}

static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (screen_state == EditingState) {
    // Big increment
    if (mode == EditStartTime) {
      update_contraction_start_time(BIG_DELTA_IN_SECONDS);
    } else if (mode == EditInterval) {
      update_contraction_seconds_elapsed(BIG_DELTA_IN_SECONDS);
    }
  }
  update_ui();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (screen_state == EditingState && !showing_discard_changes) {
    if (data_is_modified()) {
      screen_state = SavingState;
    }
  }
  update_ui();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (screen_state == EditingState) {
    // Decrement
    if (mode == EditStartTime) {
      update_contraction_start_time(-SMALL_DELTA_IN_SECONDS);
    } else if (mode == EditInterval) {
      update_contraction_seconds_elapsed(-SMALL_DELTA_IN_SECONDS);
    }

  } else if (screen_state == SavingState) {
    if (showing_discard_changes) {
      window_stack_pop(true);

    } else {
      // Reset changes
      memcpy(&modified_contraction, &current_contraction, sizeof(Contraction));      
      reset_ui();
    }
  }
  update_ui();
}

static void down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (screen_state == EditingState) {
    // Big decrement
    if (mode == EditStartTime) {
      update_contraction_start_time(-BIG_DELTA_IN_SECONDS);
    } else if (mode == EditInterval) {
      update_contraction_seconds_elapsed(-BIG_DELTA_IN_SECONDS);
    }
  }
  update_ui();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_BACK, (ClickHandler)back_click_handler);

  window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler)up_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, LONG_CLICK_DELAY, (ClickHandler)up_long_click_handler, NULL);

  window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler)select_click_handler);

  window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler)down_click_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, LONG_CLICK_DELAY, (ClickHandler)down_long_click_handler, NULL);
}

// Window callbacks
static void window_load(Window *window) {
  action_bar_layer = action_bar_layer_create();
  action_bar_layer_add_to_window(action_bar_layer, window);
  action_bar_layer_set_click_config_provider(action_bar_layer, click_config_provider);

  // UI
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  const int16_t width = bounds.size.w - ACTION_BAR_WIDTH;
  const int16_t height = bounds.size.h;
  const int16_t info_padding = -2;
  const int16_t error_padding = -4;

  const int16_t total_text_layers_height = 18 + 28 + 18;
  const int16_t title_text_y = (height - total_text_layers_height) / 2 + info_padding;

  GRect big_text_frame = GRect(0, title_text_y + 14, width, 28);
  big_text_layer = text_layer_create(big_text_frame);
  text_layer_set_text(big_text_layer, big_text);
  text_layer_set_font(big_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(big_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(big_text_layer));

  GRect title_text_frame = GRect(0, title_text_y, width, 18);
  title_text_layer = text_layer_create(title_text_frame);
  text_layer_set_font(title_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(title_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(title_text_layer));

  GRect small_text_frame = GRect(0, title_text_y + 14 + 28, width, 18 + 4);
  small_text_layer = text_layer_create(small_text_frame);
  text_layer_set_text(small_text_layer, small_text);
  text_layer_set_font(small_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(small_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(small_text_layer));

  error_title_text_layer = text_layer_create(GRect(0, height - 14 * 2 + error_padding, width, 14));
  text_layer_set_font(error_title_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(error_title_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(error_title_text_layer));

  error_message_text_layer = text_layer_create(GRect(0, height - 14 + error_padding, width, 18));
  text_layer_set_font(error_message_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(error_message_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(error_message_text_layer));

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
  reset_ui();
  update_ui();
}

static void window_unload(Window *window) {
  text_layer_destroy(title_text_layer);
  text_layer_destroy(big_text_layer);
  text_layer_destroy(small_text_layer);
  text_layer_destroy(error_title_text_layer);
  text_layer_destroy(error_message_text_layer);
  text_layer_destroy(up_button_text_layer);
  text_layer_destroy(down_button_text_layer);
  action_bar_layer_destroy(action_bar_layer);
}

// Non-static methods
void show_edit_contraction(uint32_t contraction_key, EditMode edit_mode) {
  status_t status = store_contractions_for_key(contraction_key, &current_contraction, &previous_contraction, &next_contraction);
  if (status != sizeof(Contraction) || current_contraction.start_time == 0) {
    // Data corrupted, show delete alert
    show_delete_contraction(contraction_key, true);

  } else {
    current_contraction_key = contraction_key;
    mode = edit_mode;
    memcpy(&modified_contraction, &current_contraction, sizeof(Contraction));

    if (previous_contraction.start_time == 0) {
      has_previous_contraction = false;
    } else {
      has_previous_contraction = true;
    }

    if (next_contraction.start_time == 0) {
      has_next_contraction = false;
    } else {
      has_next_contraction = true;
    }

    window_stack_push(window, true);
  }
}

void edit_contraction_init() {
  action_icon_increment = gbitmap_create_with_resource(RESOURCE_ID_ACTION_ICON_INCREMENT);
  action_icon_decrement = gbitmap_create_with_resource(RESOURCE_ID_ACTION_ICON_DECREMENT);
  action_icon_ok = gbitmap_create_with_resource(RESOURCE_ID_ACTION_ICON_OK);
  action_icon_yes = gbitmap_create_with_resource(RESOURCE_ID_ACTION_ICON_YES);
  action_icon_no = gbitmap_create_with_resource(RESOURCE_ID_ACTION_ICON_NO);

  window = window_create();

  window_set_window_handlers(window, (WindowHandlers){
    .load = window_load,
    .appear = window_appear,
    .unload = window_unload,
  });
}

void edit_contraction_deinit() {
  gbitmap_destroy(action_icon_increment);
  gbitmap_destroy(action_icon_decrement);
  gbitmap_destroy(action_icon_ok);
  gbitmap_destroy(action_icon_yes);
  gbitmap_destroy(action_icon_no);
  window_destroy(window);  
}