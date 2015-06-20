#include "num2words.h"

static const char* const ONES[] = {
  "",
  "en",
  "to",
  "tre",
  "fire",
  "fem",
  "seks",
  "syv",
  "otte",
  "ni"
};

static const char* const TEENS[] ={
  "",
  "elleve",
  "tolv",
  "tretten",
  "fjorten",
  "femten",
  "seksten",
  "sytten",
  "atten",
  "nitten"
};

static const char* const TENS[] = {
  "",
  "ti",
  "tyve",
  "trevide",
  "fyrre",
  "halvtreds",
  "treds",
  "halvfjers",
  "firs",
  "halvfems"
};

static const char* const MONTHS[] = {
  "jan ",
  "feb ",
  "mar ",
  "apr ",
  "maj ",
  "jun ",
  "jul ",
  "aug ",
  "sep ",
  "okt ",
  "nov ",
  "dec "
};

static const char* STR_OH_CLOCK = "o'clock";
static const char* STR_CLOCK = "clock";
static const char* STR_NOON = "noon";
static const char* STR_MID = "mid";
static const char* STR_NIGHT = "night";
static const char* STR_OH = "oh";
static const char* STR_O = "o'";
static const char* STR_TEEN = "teen";

static size_t append_number(char* words, int num) {
  int tens_val = num / 10;
  int ones_val = num % 10;

  size_t len = 0;

  if (tens_val == 1 && num != 10) {
    strcat(words, TEENS[ones_val]);
    return strlen(TEENS[ones_val]);
  }
  strcat(words, TENS[tens_val]);
  len += strlen(TENS[tens_val]);
  if (tens_val < 1) {
    strcat(words, ONES[ones_val]);
    return strlen(ONES[ones_val]);
  }
return len;
}

static size_t append_minutes_number(char* words, int num) {
  int ones_val = num % 10;

  size_t len = 0;
  strcat(words, ONES[ones_val]);
  len += strlen(ONES[ones_val]);
  return len;
}

static size_t append_string(char* buffer, const size_t length, const char* str) {
  strncat(buffer, str, length);

  size_t written = strlen(str);
  return (length > written) ? written : length;
}

void fuzzy_minutes_to_words(struct tm *t, char* words) {
  int fuzzy_hours = t->tm_hour;
  int fuzzy_minutes = t->tm_min;

  size_t remaining = BUFFER_SIZE;
  memset(words, 0, BUFFER_SIZE);

  //Is it midnight? or noon
  if (fuzzy_minutes != 0 || (fuzzy_hours != 12 && fuzzy_hours != 0)) {
    //is it the top of the hour?
    if(fuzzy_minutes == 0){
      const char *top_hour = persist_exists(112) && persist_read_int(112) ? STR_O : STR_OH_CLOCK;
      remaining -= append_string(words, remaining, top_hour);
    } else if(fuzzy_minutes < 10){
      //is it before ten minutes into the hour
      const char *oh = persist_exists(112) && persist_read_int(112) ? STR_O : STR_OH;
      remaining -= append_string(words, remaining, oh);
    } else {
      remaining -= append_number(words, fuzzy_minutes);
    }
  } else if (fuzzy_hours == 0) {
    remaining -= append_string(words, remaining, STR_NIGHT);
  }
}

void fuzzy_sminutes_to_words(struct tm *t, char* words) {
  int fuzzy_hours = t->tm_hour;
  int fuzzy_minutes = t->tm_min;

  size_t remaining = BUFFER_SIZE;
  memset(words, 0, BUFFER_SIZE);

  if(fuzzy_minutes == 0 && persist_exists(112) && persist_read_int(112))
    remaining -= append_string(words, remaining, STR_CLOCK);

  if (10 < fuzzy_minutes && fuzzy_minutes < 20) {
    if (fuzzy_minutes > 13 && 15 != fuzzy_minutes) {
        strcat(words, STR_TEEN);
      }
  } else if (fuzzy_minutes != 0 || (fuzzy_hours != 12 && fuzzy_hours != 0)) {
      remaining -= append_minutes_number(words, fuzzy_minutes);
  }
}

void fuzzy_hours_to_words(struct tm *t, char* words) {
  int fuzzy_hours = t->tm_hour;
  int fuzzy_minutes = t->tm_min;

  size_t remaining = BUFFER_SIZE;
  memset(words, 0, BUFFER_SIZE);

  //Is it midnight?
  if (fuzzy_hours == 0 && fuzzy_minutes == 0) {
    remaining -= append_string(words, remaining, STR_MID);
  //is it noon?
  } else if (fuzzy_hours == 12 && fuzzy_minutes == 0) {
    remaining -= append_string(words, remaining, STR_NOON);
  } else if (fuzzy_hours == 0  || fuzzy_hours == 12){
    remaining -= append_number(words, 12);
  } else {
    //get hour
    remaining -= append_number(words, fuzzy_hours % 12);
  }
}

void fuzzy_dates_to_words(struct tm *t, char* words) {
  if (persist_exists(789) && persist_read_int(789)) {
    int fuzzy_months = t->tm_mon;
    size_t remaining = BUFFER_SIZE;
    memset(words, 0, BUFFER_SIZE);
    remaining -= append_string(words, remaining, MONTHS[fuzzy_months]);
    char mday[15];
    snprintf(mday, BUFFER_SIZE, "%d", t->tm_mday);
    remaining -= append_string(words, remaining, mday);
  } else {
    strftime(words, 15, DATE_FORMAT, t);
  }
}
