#include "store.h"

#define CONTRACTIONS_KEY 0
#define DISCLAIMER_SHOWN_KEY 1
#define MAX_NUMBER_OF_DATE_SECTIONS 32
#define MAX_NUMBER_OF_CONTRACTIONS 64

typedef struct {
  uint8_t location;
  uint8_t length;
  uint8_t month;
  uint8_t day;
  char as_string[8];
} DateRange;

static int number_of_contractions;
static uint32_t contraction_keys[MAX_NUMBER_OF_CONTRACTIONS];

static int number_of_dates;
static DateRange dates[MAX_NUMBER_OF_DATE_SECTIONS];

// Debug
// static void log_contraction_keys() {
//   for (int i = 0; i < number_of_contractions; i++) {
//     app_log(APP_LOG_LEVEL_INFO, "store.c", 166, "contraction_keys[%d] = %d", i, (int)contraction_keys[i]);
//   }
// }

// static void log_dates() {
//   // app_log(APP_LOG_LEVEL_INFO, "store.c", 29, "No of dates: %d", number_of_dates);
//   for (int i = 0; i < number_of_dates; i++) {
//     DateRange range = dates[i];
//     app_log(APP_LOG_LEVEL_INFO, "store.c", 29, "dates[%d] = { .month = %d, .day = %d, .location = %d, .length = %d }", i, range.month, range.day, range.location, range.length);
//   }
// }

// Static functions
static uint32_t generate_key_from_time(time_t start_time) {
  char key[] = "MMDDHHMMSS";

  struct tm *start_datetime = localtime(&start_time);
  snprintf(key, sizeof(key), "%02d%02d%02d%02d%02d",
    start_datetime->tm_mon,
    start_datetime->tm_mday,
    start_datetime->tm_hour,
    start_datetime->tm_min,
    start_datetime->tm_sec);

  return atoi(key);
}

static DateRange make_date_range(int location, int month, int day) {
  DateRange range;
  range.location = location;
  range.length = 1;
  range.month = month;
  range.day = day;

  store_date_for_month_day(range.as_string, sizeof(range.as_string), month, day);

  return range;
}

static bool date_section_is_valid(int date_section) {
  return date_section >= 0 && date_section < MAX_NUMBER_OF_DATE_SECTIONS;
}

static void cleanup_contractions() {
  for (int i = 0; i < MAX_NUMBER_OF_CONTRACTIONS; i++) {
    uint32_t contraction_key = contraction_keys[i];
    if (contraction_key != 0) {
      Contraction contraction;
      status_t status = store_contraction_for_key(contraction_key, &contraction);
      if (status != sizeof(Contraction) || contraction.start_time == 0) {
        // If contraction key does not retrieve usable data, remove contraction key
        contraction_keys[i] = 0;
      }
    }
  }
}

static void sort_contractions() {
  uint32_t temp;
  int j;

  // Insert sort
  for (int i = 1; i < MAX_NUMBER_OF_CONTRACTIONS; i++) {
    temp = contraction_keys[i];
    j = i - 1;
    while (temp > contraction_keys[j] && j >= 0) {
      contraction_keys[j + 1] = contraction_keys[j];
      j--;
    }
    contraction_keys[j + 1] = temp;
  }

  number_of_contractions = 0;
  for (int i = 0; i < MAX_NUMBER_OF_CONTRACTIONS; i++) {
    if (contraction_keys[i] != 0) {
      number_of_contractions++;
    }
  }
}

static void rebuild_dates() {
  number_of_dates = 0;
  memset(dates, 0, sizeof(dates));

  for (int i = 0; i < number_of_contractions; i++) {
    uint32_t contraction_key = contraction_keys[i];

    Contraction contraction;
    status_t status = store_contraction_for_key(contraction_key, &contraction);

    if (status == sizeof(Contraction)) {
      time_t start_time = contraction.start_time;
      struct tm *date_time = localtime(&start_time);

      int month = date_time->tm_mon;
      int day = date_time->tm_mday;

      DateRange range;
      if (number_of_dates > 0) {
        range = dates[number_of_dates - 1];
        if (range.month == month && range.day == day) {
          range.length++;
          dates[number_of_dates - 1] = range;
        } else if (number_of_dates < MAX_NUMBER_OF_DATE_SECTIONS) {
          range = make_date_range(i, month, day);
          dates[number_of_dates] = range;
          number_of_dates++;
        }
      } else {
        range = make_date_range(i, month, day);
        dates[0] = range;
        number_of_dates++;
      }
    }
  }
  // log_dates();
}

// Non-static functions
void store_time_for_hour_minute(char *buffer, size_t size, int hour, int minute) {
  if (clock_is_24h_style()) {
    snprintf(buffer, size, "%2d:%02d", hour, minute);
  } else {
    if (hour > 12) {
      snprintf(buffer, size, "%2d:%02d PM", hour % 12, minute);
    } else if (hour == 0) {
      snprintf(buffer, size, "12:%02d AM", minute);
    } else {
      snprintf(buffer, size, "%2d:%02d AM", hour, minute);
    }
  }
}

void store_time_for_time(char *buffer, size_t size, int hour, int minute, int second) {
  if (clock_is_24h_style()) {
    snprintf(buffer, size, "%2d:%02d:%02d", hour, minute, second);
  } else {
    if (hour > 12) {
      snprintf(buffer, size, "%2d:%02d:%02d PM", hour % 12, minute, second);
    } else if (hour == 0) {
      snprintf(buffer, size, "12:%02d:%02d AM", minute, second);
    } else {
      snprintf(buffer, size, "%2d:%02d:%02d AM", hour, minute, second);
    }
  }
}

void store_date_for_month_day(char *buffer, size_t size, int month, int day) {
  char month_name[4];
  
  switch (month) {
    case 0:
      snprintf(month_name, sizeof(month_name), "Jan");
      break;

    case 1:
      snprintf(month_name, sizeof(month_name), "Feb");
      break;

    case 2:
      snprintf(month_name, sizeof(month_name), "Mar");
      break;

    case 3:
      snprintf(month_name, sizeof(month_name), "Apr");
      break;

    case 4:
      snprintf(month_name, sizeof(month_name), "May");
      break;

    case 5:
      snprintf(month_name, sizeof(month_name), "Jun");
      break;

    case 6:
      snprintf(month_name, sizeof(month_name), "Jul");
      break;

    case 7:
      snprintf(month_name, sizeof(month_name), "Aug");
      break;

    case 8:
      snprintf(month_name, sizeof(month_name), "Sep");
      break;

    case 9:
      snprintf(month_name, sizeof(month_name), "Oct");
      break;

    case 10:
      snprintf(month_name, sizeof(month_name), "Nov");
      break;

    default:
      snprintf(month_name, sizeof(month_name), "Dec");
      break;
  }

  snprintf(buffer, size, "%s %d", month_name, day);
}

void store_time_text_for_contraction(char *start_time_buffer, size_t start_time_size, char *end_time_buffer, size_t end_time_size, int contraction_key) {
  Contraction contraction;
  status_t status = store_contraction_for_key(contraction_key, &contraction);

  if (status == sizeof(Contraction)) {
    struct tm *start_datetime = localtime(&contraction.start_time);
    store_time_for_time(start_time_buffer, start_time_size, start_datetime->tm_hour, start_datetime->tm_min, start_datetime->tm_sec);

    contraction.start_time += contraction.seconds_elapsed;

    struct tm *end_datetime = localtime(&contraction.start_time);
    store_time_for_time(end_time_buffer, end_time_size, end_datetime->tm_hour, end_datetime->tm_min, end_datetime->tm_sec);
  }
}

void store_duration_for_seconds_elapsed(char *buffer, size_t size, int seconds_elapsed) {
  char prefix_text[] = "Lasted ";
  char minute_text[] = "min";
  char second_text[] = "sec";
  char plural_suffix[] = "s";

  int minutes = seconds_elapsed / 60;
  int seconds = seconds_elapsed % 60;

  if (seconds_elapsed > 60) {
    snprintf(
      buffer,
      size,
      "%s%d %s%s %d%s%s",
      prefix_text,
      minutes,
      minute_text,
      minutes == 1 ? "" : plural_suffix,
      seconds,
      second_text,
      seconds == 1 ? "" : plural_suffix);
  } else {
    snprintf(
      buffer,
      size,
      "%s%d %s%s",
      prefix_text,
      seconds_elapsed,
      second_text,
      seconds == 1 ? "" : plural_suffix);
  }
}

int store_number_of_past_contractions() {
  return number_of_contractions;
}

int store_number_of_date_sections() {
  return number_of_dates;
}

void store_date_for_date_section(char *date_as_string, size_t num, int date_section) {
  if (date_as_string != NULL && num > 0 && date_section_is_valid(date_section)) {
    char date_text[] = "Jan 01";
    DateRange range = dates[date_section];
    strcpy(date_text, range.as_string);

    time_t current_time = time(NULL);
    struct tm *now = localtime(&current_time);

    if (range.month == now->tm_mon && range.day == now->tm_mday) {
      snprintf(date_as_string, num, "Today, %s", date_text);
    } else {
      snprintf(date_as_string, num, "%s", date_text);
    }
  }
}

int store_number_of_contractions_for_date_section(int date_section) {
  if (date_section_is_valid(date_section)) {
    DateRange range = dates[date_section];
    return range.length;
  }
  return 0;
}

int store_contraction_for_date_section_index(int date_section, int contraction_index, Contraction *contraction) {
  if (date_section_is_valid(date_section)) {
    DateRange range = dates[date_section];

    if (contraction_index < range.length) {
      uint32_t contraction_key = contraction_keys[range.location + contraction_index];
      return store_contraction_for_key(contraction_key, contraction);    
    } else {
      return E_INVALID_ARGUMENT;
    }
  }
  return E_INVALID_ARGUMENT;
}

uint32_t store_contraction_key(int date_section, int contraction_index) {
  DateRange range = dates[date_section];
  return contraction_keys[range.location + contraction_index];
}

status_t store_contraction_for_key(uint32_t contraction_key, Contraction *contraction) {
  return persist_read_data(contraction_key, contraction, sizeof(Contraction));
}

status_t store_contractions_for_key(
  uint32_t contraction_key,
  Contraction *contraction,
  Contraction *previous_contraction,
  Contraction *next_contraction) {

  sort_contractions();
  previous_contraction->start_time = 0;
  next_contraction->start_time = 0;

  for (int i = 0; i < number_of_contractions; i++) {
    uint32_t key = contraction_keys[i];

    if (key == contraction_key) {
      if (i > 0) {
        uint32_t next_contraction_key = contraction_keys[i - 1];
        store_contraction_for_key(next_contraction_key, next_contraction);
      }

      if (i < (number_of_contractions - 1)) {
        uint32_t previous_contraction_key = contraction_keys[i + 1];
        store_contraction_for_key(previous_contraction_key, previous_contraction);
      }

      break;
    }
  }

  return store_contraction_for_key(contraction_key, contraction);
}

uint32_t store_insert_contraction(time_t start_time, int seconds_elapsed) {
  Contraction contraction;
  contraction.start_time = start_time;
  contraction.seconds_elapsed = seconds_elapsed;

  uint32_t contraction_key = generate_key_from_time(start_time);
  status_t status = persist_write_data(contraction_key, &contraction, sizeof(Contraction));

  if (status == sizeof(Contraction)) {
    bool found = false;
    for (int i = 0; i < number_of_contractions; i++) {
      if (contraction_keys[i] == contraction_key) {
        found = true;
        break;
      }
    }

    if (!found) {
      if (number_of_contractions == MAX_NUMBER_OF_CONTRACTIONS) {
        contraction_keys[number_of_contractions - 1] = contraction_key;
      } else {
        contraction_keys[number_of_contractions] = contraction_key;
        number_of_contractions++;
      }
    }

    sort_contractions();
    rebuild_dates();
  }

  return contraction_key;
}

uint32_t store_replace_contraction(uint32_t old_contraction_key, time_t new_start_time, int seconds_elapsed) {
  store_remove_contraction(old_contraction_key);
  return store_insert_contraction(new_start_time, seconds_elapsed);
}

void store_remove_contraction(time_t start_time) {
  if (persist_exists(start_time)) {
    persist_delete(start_time);    
  }

  for (int i = 0; i < number_of_contractions; i++) {
    if (contraction_keys[i] == (uint32_t)start_time) {
      contraction_keys[i] = 0;
      break;
    }
  }

  sort_contractions();
  rebuild_dates();
}

void store_remove_all_contractions() {
  sort_contractions();
  for (int i = 0; i < number_of_contractions; ++i) {
    uint32_t start_time = contraction_keys[i];
    if (persist_exists(start_time)) {
      persist_delete(start_time);
    }
    contraction_keys[i] = 0;
  }

  sort_contractions();
  rebuild_dates();
}

SummaryResult store_calculate_summary(int minutes) {
  SummaryResult result;

  const time_t current_time = time(NULL);
  const time_t time_cutoff = current_time - 60 * minutes;

  int duration = 0;
  int interval = 0;
  int last_start_time = 0;

  for (int i = 0; i < number_of_contractions; i++) {
    Contraction contraction;
    store_contraction_for_key(contraction_keys[i], &contraction);
    if (contraction.start_time >= time_cutoff) {
      result.count++;

      duration += contraction.seconds_elapsed;

      if (last_start_time != 0) {
        interval += (last_start_time - contraction.start_time);
      }
      last_start_time = contraction.start_time;
    } else {
      break;
    }
  }

  result.average_duration_in_seconds = duration / result.count;
  if (result.count > 1) {
    result.average_interval_in_seconds = interval / (result.count - 1);
  } else {
    result.average_interval_in_seconds = interval;
  }

  return result;
}

bool store_should_show_disclaimer() {
  return persist_exists(DISCLAIMER_SHOWN_KEY) ? !persist_read_bool(DISCLAIMER_SHOWN_KEY) : true;
}

void store_set_disclaimer_shown(bool shown) {
  persist_write_bool(DISCLAIMER_SHOWN_KEY, shown);
}

void store_init() {
  if (persist_exists(CONTRACTIONS_KEY)) {
    status_t status = persist_read_data(CONTRACTIONS_KEY, contraction_keys, sizeof(contraction_keys));
    if (status == sizeof(contraction_keys)) {
      cleanup_contractions();
      sort_contractions();
      rebuild_dates();
    }
  }
}

void store_deinit() {
  sort_contractions();
  persist_write_data(CONTRACTIONS_KEY, contraction_keys, sizeof(contraction_keys));
}