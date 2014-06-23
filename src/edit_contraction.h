#pragma once

typedef enum {
  EditStartTime,
  EditInterval
} EditMode;

void show_edit_contraction(uint32_t contraction_key, EditMode edit_mode);

void edit_contraction_init();
void edit_contraction_deinit();