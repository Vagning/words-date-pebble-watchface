#include "pebble.h"
#include "num2words.h"

#define TIME_SLOT_ANIMATION_DURATION 700
#define NUM_LAYERS 4
#define UNUSED(x) (void)(x)

static bool TAPPED = false;

enum layer_names {
  MINUTES,
  TENS,
  HOURS,
  DATE
};

enum batt_stats {
  CHARGE = MINUTES,
  CHAR_STAT = TENS,
  PERCENT = HOURS
};

AppSync *sync;
uint8_t *sync_buffer;

enum SettingsKey {
  WATCHFACE_BATT_KEY = 0,
  WATCHFACE_BLUE_KEY = 1,
  WATCHFACE_LOW_DATE = 2,
  WATCHFACE_INVERT = 3
};

Window *window;

typedef struct CommonWordsData {
  TextLayer *label;
  PropertyAnimation in_animation;
  PropertyAnimation out_animation;
  void (*update) (struct tm *t, char *words);
  char buffer[BUFFER_SIZE];
} CommonWordsData;

CommonWordsData *layers;

void handle_init();
static void do_deinit();

void slide_out(PropertyAnimation *animation, CommonWordsData *layer) {
  Layer* text_layer = text_layer_get_layer(layer->label);
  GRect from_frame = layer_get_frame(text_layer);

  Layer *root_layer = window_get_root_layer(window);
  GRect frame = layer_get_frame(root_layer);

  GRect to_frame = GRect(-frame.size.w, from_frame.origin.y,
                          frame.size.w, from_frame.size.h);

  *animation = *property_animation_create_layer_frame(text_layer, NULL, &to_frame);
  animation_set_duration(&animation->animation, TIME_SLOT_ANIMATION_DURATION);
  animation_set_curve(&animation->animation, AnimationCurveEaseIn);
}

void slide_in(PropertyAnimation *animation, CommonWordsData *layer) {
  Layer* text_layer = text_layer_get_layer(layer->label);
  GRect origin_frame = layer_get_frame(text_layer);

  Layer *root_layer = window_get_root_layer(window);
  GRect frame = layer_get_frame(root_layer);
  GRect to_frame   = GRect(0, origin_frame.origin.y,
                            frame.size.w, origin_frame.size.h);
  GRect from_frame = GRect(2*frame.size.w, origin_frame.origin.y,
                            frame.size.w, origin_frame.size.h);

  layer_set_frame(text_layer, from_frame);
  text_layer_set_text(layer->label, layer->buffer);

  *animation = *property_animation_create_layer_frame(text_layer, &from_frame, &to_frame);
  animation_set_duration(&animation->animation, TIME_SLOT_ANIMATION_DURATION);
  animation_set_curve(&animation->animation, AnimationCurveEaseOut);
}

void slide_out_animation_stopped(Animation *slide_out_animation, bool finished,
                                  void *context) {
  CommonWordsData *layer = (CommonWordsData *)context;
  Layer* text_layer = text_layer_get_layer(layer->label);
  GRect frame = layer_get_frame(text_layer);
  frame.origin.x = 0;
  frame = frame;
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  layer->update(t, layer->buffer);
  slide_in(&layer->in_animation, layer);
  animation_schedule(&layer->in_animation.animation);
}

void update_layer(CommonWordsData *layer) {
  slide_out(&layer->out_animation, layer);
  animation_set_handlers(&layer->out_animation.animation, (AnimationHandlers){
    .stopped = (AnimationStoppedHandler)slide_out_animation_stopped
  }, (void *) layer);
  animation_schedule(&layer->out_animation.animation);
}

static void handle_minute_tick(struct tm *t, TimeUnits units_changed) {
  if((units_changed & MINUTE_UNIT) == MINUTE_UNIT) {
    if ((17 > t->tm_min || t->tm_min > 19)
          && (11 > t->tm_min || t->tm_min > 13)) {
      update_layer(&layers[MINUTES]);
    }
    if (t->tm_min % 10 == 0 || (t->tm_min > 10 && t->tm_min < 20)
          || t->tm_min == 1) {
      update_layer(&layers[TENS]);
    }
  }
  if ((units_changed & HOUR_UNIT) == HOUR_UNIT ||
        ((t->tm_hour == 00 || t->tm_hour == 12) && t->tm_min == 01)) {
    update_layer(&layers[HOURS]);
  }
  if ((units_changed & DAY_UNIT) == DAY_UNIT) {
    update_layer(&layers[DATE]);
  }
}

void init_layer(CommonWordsData *layer, GRect rect, GFont font) {
  GColor color = persist_exists(101) && persist_read_int(101) ? GColorBlack : GColorWhite;
  layer->label = text_layer_create(rect);
  text_layer_set_background_color(layer->label, GColorClear);
  text_layer_set_text_color(layer->label, color);
  text_layer_set_font(layer->label, font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(layer->label));
}

void time_slide_in(Animation *slide_out_animation, bool finished,
                                  void *context)  {
  for (int i = 0; i < NUM_LAYERS; ++i)
  {
    slide_out_animation_stopped(NULL, true, &layers[i]);
  }
  TAPPED = false;
}

void clear_batt() {
  for (int i = 0; i < NUM_LAYERS - 1; ++i)
  {
    slide_out(&layers[i].out_animation, &layers[i]);
    animation_set_handlers(&layers[PERCENT].out_animation.animation,
      (AnimationHandlers){
        .stopped = (AnimationStoppedHandler)time_slide_in
      }, (void *) NULL);
    animation_schedule(&layers[i].out_animation.animation);
  }
}

void batt_slide_in(Animation *slide_out_animation, bool finished,
                                  void *context) {
  UNUSED(slide_out_animation);
  UNUSED(finished);
  UNUSED(context);
  
  BatteryChargeState curr_batt = battery_state_service_peek();
  char *charging = "";
  char *status = "";
  if (curr_batt.is_charging || curr_batt.is_plugged) {
    charging = "on";
    status = "charge";
  }

  snprintf(layers[PERCENT].buffer, BUFFER_SIZE, "%d%%", curr_batt.charge_percent);
  snprintf(layers[CHAR_STAT].buffer, BUFFER_SIZE, "%s", charging);
  snprintf(layers[CHARGE].buffer, BUFFER_SIZE, "%s", status);
  
  for (int i = 0; i < NUM_LAYERS - 1; ++i)
  {
    slide_in(&layers[i].in_animation, &layers[i]);
    animation_schedule(&layers[i].in_animation.animation);
  }
}

void run_notification() {
  if(TAPPED) return;
  TAPPED = true;
  for (int i = 0; i < NUM_LAYERS; ++i) {
    slide_out(&layers[i].out_animation, &layers[i]);
    animation_set_handlers(&layers[DATE].out_animation.animation,
      (AnimationHandlers){
        .stopped = (AnimationStoppedHandler)batt_slide_in
      }, (void *) NULL);
    animation_schedule(&layers[i].out_animation.animation);
  }
  app_timer_register (1750, &clear_batt ,NULL);
}

void accel_tap_handler(AccelAxisType axis, int32_t blah) {
  UNUSED(blah);
  if (!persist_exists(123) || !persist_read_int(123))
    return;
  run_notification();
}

void bluetooth_handler(bool connected) {
  if (!persist_exists(456) || !persist_read_int(456))
    return;
  if (connected) {
    vibes_double_pulse();
  } else {
    uint32_t durations[8] = {200,200,200,200,200,200,200,200};
    VibePattern pattern = {.durations = durations, .num_segments = 8};
    vibes_enqueue_custom_pattern(pattern);
  }
}

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  app_log(APP_LOG_LEVEL_WARNING, "words-date-pebble-watchface", 189, "Appsync failed: %d", app_message_error);
  run_notification();
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
    case WATCHFACE_BATT_KEY:
      persist_write_int(123, new_tuple->value->uint8);
      if(new_tuple->value->uint8) {
        accel_tap_service_subscribe(&accel_tap_handler);
      } else {
        accel_tap_service_unsubscribe();
      }
      break;
    case WATCHFACE_BLUE_KEY:
      persist_write_int(456, new_tuple->value->uint8);
      if(new_tuple->value->uint8) {
        bluetooth_connection_service_subscribe (&bluetooth_handler);
      } else {
        bluetooth_connection_service_unsubscribe();
      }
      break;
    case WATCHFACE_LOW_DATE:
      persist_write_int(789, new_tuple->value->uint8);
      time_t now = time(NULL);
      struct tm *t = localtime(&now);
      layers[DATE].update(t, layers[DATE].buffer);
      slide_in(&layers[DATE].in_animation, &layers[DATE]);
      animation_schedule(&layers[DATE].in_animation.animation);
      break;
    case WATCHFACE_INVERT:
      persist_write_int(101, new_tuple->value->uint8);
      GColor color = new_tuple->value->uint8 ? GColorWhite : GColorBlack;
      window_set_background_color(window, color);
      color = color == GColorBlack ? GColorWhite : GColorBlack;
      for (int i = 0; i < NUM_LAYERS; ++i)
    //so we aren't animating the date layer twice.
      {
        text_layer_set_text_color(layers[i].label, color);
      }
      break;
  }
}

void handle_init() {
  TAPPED = false;
  window = window_create();
  const bool animated = false;
  window_stack_push(window, animated);
  GColor color = persist_exists(101) && persist_read_int(101) ? GColorWhite : GColorBlack;
  window_set_background_color(window, color);

  Layer *root_layer = window_get_root_layer(window);
  GRect frame = layer_get_frame(root_layer);

  layers = malloc(sizeof(CommonWordsData) * NUM_LAYERS);
  layers[0].update = &fuzzy_sminutes_to_words;
  layers[1].update = &fuzzy_minutes_to_words;
  layers[2].update = &fuzzy_hours_to_words;
  layers[3].update = &fuzzy_dates_to_words;

// single digits
  init_layer(&layers[MINUTES],GRect(0, 76, frame.size.w, 50),
                    fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));

// 00 minutes
  init_layer(&layers[TENS], GRect(0, 38, frame.size.w, 50),
                    fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));

//hours
  init_layer(&layers[HOURS], GRect(0, 0, frame.size.w, 50),
                    fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));

//Date
  init_layer(&layers[DATE], GRect(0, 114, frame.size.w, 50),
                    fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));

//show your face
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  for (int i = 0; i < NUM_LAYERS - 1; ++i)
  //so we aren't animating the date layer twice.
  {
    layers[i].update(t, layers[i].buffer);
    slide_in(&layers[i].in_animation, &layers[i]);
    animation_schedule(&layers[i].in_animation.animation);
  }
  tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);

  app_message_open(64, 64);

  int batt = 0, blue = 0, low = 0, invert = 0;
  if (persist_exists(123) && persist_read_int(123))
    batt = 1;
  if (persist_exists(456) && persist_read_int(456))
    blue = 1;
  if (persist_exists(789) && persist_read_int(789))
    low  = 1;
  if (persist_exists(101) && persist_read_int(101))
    invert = 1;

  Tuplet initial_values[] = {
    TupletInteger(WATCHFACE_BATT_KEY, batt),
    TupletInteger(WATCHFACE_BLUE_KEY, blue),
    TupletInteger(WATCHFACE_LOW_DATE, low),
    TupletInteger(WATCHFACE_INVERT, invert)
  };

  sync_buffer = calloc(sizeof(uint8_t), 64);
  sync = malloc(sizeof(AppSync));

  app_sync_init(sync, sync_buffer, 64, initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);
}

static void do_deinit(void) {
  if (persist_exists(123) && persist_read_int(123))
    accel_tap_service_unsubscribe();
  if (persist_exists(456) && persist_read_int(456))
    bluetooth_connection_service_unsubscribe();
  
  app_sync_deinit(sync);
  free(sync);
  free(sync_buffer);
  
  for (int i = 0; i < NUM_LAYERS; ++i) {
    text_layer_destroy(layers[i].label);
  }
  
  free(layers);
  window_destroy(window);
}

int main(void) {
  handle_init();
  app_event_loop();
  do_deinit();
}
