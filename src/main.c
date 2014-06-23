#include <pebble.h>
#include "store.h"
#include "disclaimer.h"
#include "menu.h"
#include "summary.h"
#include "new_contraction.h"
#include "past_contractions.h"
#include "contraction_menu.h"
#include "edit_contraction.h"
#include "delete_contraction.h"

static bool should_show_disclaimer = false;

static void init() {
  store_init();

  disclaimer_init();
  menu_init();
  summary_init();
  new_contraction_init();
  past_contractions_init();
  contraction_menu_init();
  edit_contraction_init();
  delete_contraction_init();

  if (store_should_show_disclaimer()) {
    show_disclaimer();
  } else {
    show_menu();
  }
}

static void deinit() {
  store_deinit();

  if (should_show_disclaimer) {
    disclaimer_deinit();
  }
  
  menu_deinit();
  summary_deinit();
  new_contraction_deinit();
  past_contractions_deinit();
  contraction_menu_deinit();
  edit_contraction_deinit();
  delete_contraction_deinit();
}

int main(void) {
  init();
  app_event_loop();
  deinit();

  return 0;
}