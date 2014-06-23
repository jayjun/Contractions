#pragma once

#define DELETE_ALL_CONTRACTIONS_KEY 0

void show_delete_contraction(uint32_t contraction_key, bool is_corrupted);

void delete_contraction_init();
void delete_contraction_deinit();