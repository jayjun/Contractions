#include <pebble.h>
#pragma once

typedef struct {
  time_t start_time;
  int seconds_elapsed;
} Contraction;

typedef struct {
  int count;
  int average_duration_in_seconds;
  int average_interval_in_seconds;
} SummaryResult;

void store_time_for_hour_minute(char *buffer, size_t size, int hour, int minute);
void store_date_time_for_contraction(char *buffer, size_t size, int contraction_key);

int store_number_of_past_contractions();

int store_number_of_date_sections();
void store_date_for_date_section(char *date_as_string, size_t num, int date_section);

int store_number_of_contractions_for_date_section(int date_section);
int store_contraction_for_date_section_index(int date_section, int contraction_index, Contraction *contraction);
uint32_t store_contraction_key(int date_section, int contraction_index);
status_t store_contraction_for_key(uint32_t contraction_key, Contraction *contraction);

void store_insert_contraction(time_t start_time, int seconds_elapsed);
void store_remove_contraction(time_t start_time);
void store_remove_all_contractions();

SummaryResult store_calculate_summary(int minutes);

bool store_should_show_disclaimer();
void store_set_disclaimer_shown(bool shown);

void store_init();
void store_deinit();