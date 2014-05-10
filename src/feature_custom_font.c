/*

   How to use a custom non-system font.

 */

#include "pebble.h"

static bool primary_view = true;
static bool blink_view = false;
static bool alerts_suppressed = false;
static bool just_changed = false;
static Window *window;
static Layer *primary_layer;
static Layer *secondary_layer;
static TextLayer *location_layer;
static TextLayer *date_layer;
static TextLayer *time_layer;
static TextLayer *weather_temp_layer;
static TextLayer *next_layer;
static TextLayer *manual_layer;
static TextLayer *manual_code_layer;
static bool refreshing = false;
static char location_buffer[24] = "HELLO PEBBLE";
static char date_buffer[24] = "Mon 01.01.14";
static char time_buffer[8] = "00:00";
static char weather_temp_buffer[] = "---";
static int weather_temp = -1;
static int weather_rain = -1;
static GBitmap* img_00d_bitmap;
static GBitmap* img_01d_bitmap;
static GBitmap* img_02d_bitmap;
static GBitmap* img_03d_bitmap;
static GBitmap* img_04d_bitmap;
static GBitmap* img_09d_bitmap;
static GBitmap* img_10d_bitmap;
static GBitmap* img_11d_bitmap;
static GBitmap* img_13d_bitmap;
static GBitmap* img_50d_bitmap;
static GBitmap* img_umbrella_bitmap;
static GBitmap* tick_bitmap;
static BitmapLayer* img_layer;
static BitmapLayer* img_blink_layer;
static BitmapLayer* tick_layer;
static BitmapLayer* tick2_layer;
static BitmapLayer* tick3_layer;
static BitmapLayer* tick4_layer;

static void redraw() {
  if (refreshing)
    text_layer_set_text(date_layer, "Refreshing...");
  else
    text_layer_set_text(date_layer, date_buffer);
  text_layer_set_text(time_layer, time_buffer);
  text_layer_set_text(location_layer, location_buffer);
  text_layer_set_text(weather_temp_layer, weather_temp_buffer);
  text_layer_set_text(next_layer, "45M");
  text_layer_set_text(manual_layer, "GO TO: \nGOOGLE.COM/DEVICE\n\n");
  text_layer_set_text(manual_code_layer, "C3G97R");
  layer_set_hidden(bitmap_layer_get_layer(img_layer), blink_view);

  if (!just_changed)
    layer_set_hidden(bitmap_layer_get_layer(img_blink_layer), !blink_view);

  layer_set_hidden(bitmap_layer_get_layer(tick_layer), !alerts_suppressed);
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Received!");
  Tuple *main_tuple = dict_find(iter, 1);
  Tuple *icon_tuple = dict_find(iter, 2);
  Tuple *temp_tuple = dict_find(iter, 3);
  Tuple *rain_tuple = dict_find(iter, 4);
  Tuple *location_tuple = dict_find(iter, 6);

  weather_temp = temp_tuple->value->int8;
  snprintf(weather_temp_buffer, sizeof(weather_temp_buffer),
           "%dC", weather_temp);

  int icon = icon_tuple->value->int8;
  switch (icon) {
    case 1:
      bitmap_layer_set_bitmap(img_layer, img_01d_bitmap);
      break;
    case 2:
      bitmap_layer_set_bitmap(img_layer, img_02d_bitmap);
      break;
    case 3:
      bitmap_layer_set_bitmap(img_layer, img_03d_bitmap);
      break;
    case 4:
      bitmap_layer_set_bitmap(img_layer, img_04d_bitmap);
      break;
    case 9:
      bitmap_layer_set_bitmap(img_layer, img_09d_bitmap);
      break;
    case 10:
      bitmap_layer_set_bitmap(img_layer, img_10d_bitmap);
      break;
    case 11:
      bitmap_layer_set_bitmap(img_layer, img_11d_bitmap);
      break;
    case 13:
      bitmap_layer_set_bitmap(img_layer, img_13d_bitmap);
      break;
    case 50:
      bitmap_layer_set_bitmap(img_layer, img_50d_bitmap);
      break;
    default:
      bitmap_layer_set_bitmap(img_layer, img_00d_bitmap);
      break;
  }
  
  weather_rain = rain_tuple->value->int32;
  if (weather_rain <= 0)
    blink_view = false;

  snprintf(location_buffer, sizeof(location_buffer), location_tuple->value->cstring);

  refreshing = false;
  redraw();
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped!");
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Failed to Send!");
}

static void request_update() {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (!iter)
    return;
//  Tuplet value = TupletInteger(0, 1);
//  dict_write_tuplet(iter, &value);
  dict_write_end(iter);
  app_message_outbox_send();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (!alerts_suppressed && weather_rain > 0)
    blink_view = !blink_view;
  just_changed = false;

  strftime(date_buffer, sizeof(date_buffer), "%a  %d.%m.%y", tick_time);
  strftime(time_buffer, sizeof(time_buffer), "%H:%M", tick_time);
  
  if (units_changed == MINUTE_UNIT && tick_time->tm_min % 10 == 0)
    request_update();

  redraw();
}

static void on_animation_stopped(Animation *anim, bool finished, void *context) {
    property_animation_destroy((PropertyAnimation*) anim);
}
 
static void animate_layer(Layer *layer, GRect *start, GRect *finish, int duration, int delay) {
    PropertyAnimation *anim = property_animation_create_layer_frame(layer, start, finish);
    animation_set_duration((Animation*) anim, duration);
    animation_set_delay((Animation*) anim, delay);
    AnimationHandlers handlers = {
        .stopped = (AnimationStoppedHandler) on_animation_stopped
    };
    animation_set_handlers((Animation*) anim, handlers, NULL);
    animation_schedule((Animation*) anim);
}

static void slide_in_layer(Layer *layer) {
  GRect bounds = layer_get_bounds(layer);
  GRect start = GRect(bounds.size.w, 0, bounds.size.w, bounds.size.h);
  GRect finish = GRect(0, 0, bounds.size.w, bounds.size.h);
  animate_layer(layer, &start, &finish, 300, 0);
}

static void slide_out_layer(Layer *layer) {
  GRect bounds = layer_get_bounds(layer);
  GRect start = GRect(0, 0, bounds.size.w, bounds.size.h);
  GRect finish = GRect(-bounds.size.w, 0, bounds.size.w, bounds.size.h);
  animate_layer(layer, &start, &finish, 300, 0);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (primary_view) {
    slide_out_layer(primary_layer);
    slide_in_layer(secondary_layer);
  }
  else {
    slide_out_layer(secondary_layer);
    slide_in_layer(primary_layer);
  }
  primary_view = !primary_view;
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  alerts_suppressed = !alerts_suppressed;
  if (alerts_suppressed) {
    blink_view = false;
    just_changed = true;
  }
  redraw();
}

static void dbl_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  request_update();
  refreshing = true;
  redraw();
}

static void config_provider(Window *window) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
//  window_multi_click_subscribe(BUTTON_ID_UP, 2, 2, 200, false, dbl_down_click_handler);
  window_multi_click_subscribe(BUTTON_ID_SELECT, 2, 2, 200, false, dbl_select_click_handler);
}

static void init() {
  window = window_create();
  window_set_fullscreen(window, true);
  window_set_background_color(window, GColorBlack);
  img_00d_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_00D);
  img_01d_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_01D);
  img_02d_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_02D);
  img_03d_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_03D);
  img_04d_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_04D);
  img_09d_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_09D);
  img_10d_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_10D);
  img_11d_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_11D);
  img_13d_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_13D);
  img_50d_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_50D);
  img_umbrella_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UMBRELLA);
  tick_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TICK);
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  primary_layer = layer_create(GRect(0, 0, bounds.size.w, 100));
  secondary_layer = layer_create(GRect(-bounds.size.w, 0, bounds.size.w, 100));

  img_layer = bitmap_layer_create(GRect(0, 10, bounds.size.w, 64));
  bitmap_layer_set_bitmap(img_layer, img_00d_bitmap);
  layer_add_child(primary_layer, bitmap_layer_get_layer(img_layer));

  img_blink_layer = bitmap_layer_create(GRect(0, 10, bounds.size.w, 64));
  bitmap_layer_set_bitmap(img_blink_layer, img_umbrella_bitmap);
  layer_add_child(primary_layer, bitmap_layer_get_layer(img_blink_layer));

  GFont date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TEXT_14));
  GFont time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGI_66));

  manual_layer = text_layer_create(GRect(0, 10, bounds.size.w, bounds.size.h));
  text_layer_set_background_color(manual_layer, GColorClear);
  text_layer_set_text_color(manual_layer, GColorWhite);
  text_layer_set_text_alignment(manual_layer, GTextAlignmentCenter);
  layer_add_child(secondary_layer, text_layer_get_layer(manual_layer));

  manual_code_layer = text_layer_create(GRect(0, 40, bounds.size.w, bounds.size.h));
  text_layer_set_font(manual_code_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_background_color(manual_code_layer, GColorClear);
  text_layer_set_text_color(manual_code_layer, GColorWhite);
  text_layer_set_text_alignment(manual_code_layer, GTextAlignmentCenter);
  layer_add_child(secondary_layer, text_layer_get_layer(manual_code_layer));
  
  weather_temp_layer = text_layer_create(GRect(0, 33, bounds.size.w, bounds.size.h));
  text_layer_set_font(weather_temp_layer, date_font);
  text_layer_set_background_color(weather_temp_layer, GColorClear);
  text_layer_set_text_color(weather_temp_layer, GColorWhite);
  text_layer_set_text_alignment(weather_temp_layer, GTextAlignmentRight);
  layer_add_child(primary_layer, text_layer_get_layer(weather_temp_layer));

  next_layer = text_layer_create(GRect(0, 33, bounds.size.w, bounds.size.h));
  text_layer_set_font(next_layer, date_font);
  text_layer_set_background_color(next_layer, GColorClear);
  text_layer_set_text_color(next_layer, GColorWhite);
  text_layer_set_text_alignment(next_layer, GTextAlignmentLeft);
  layer_add_child(primary_layer, text_layer_get_layer(next_layer));

  layer_add_child(window_layer, primary_layer);
  layer_add_child(window_layer, secondary_layer);

  location_layer = text_layer_create(GRect(20, 82, bounds.size.w - 20 * 2, 18));
  text_layer_set_font(location_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_background_color(location_layer, GColorWhite);
  text_layer_set_text_color(location_layer, GColorBlack);
  text_layer_set_text_alignment(location_layer, GTextAlignmentCenter);
  layer_add_child(primary_layer, text_layer_get_layer(location_layer));
  
  date_layer = text_layer_create(GRect(0, 100, bounds.size.w, bounds.size.h));
  text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
//  text_layer_set_font(date_layer, date_font);
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(date_layer));
  
  tick_layer = bitmap_layer_create(GRect(0, 105, bounds.size.w, bounds.size.h));
  bitmap_layer_set_bitmap(tick_layer, tick_bitmap);
  bitmap_layer_set_alignment(tick_layer, GAlignTopRight);
  layer_add_child(window_layer, bitmap_layer_get_layer(tick_layer));

  tick2_layer = bitmap_layer_create(GRect(0, 50, bounds.size.w, bounds.size.h));
  bitmap_layer_set_bitmap(tick2_layer, tick_bitmap);
  bitmap_layer_set_alignment(tick2_layer, GAlignTopLeft);
  layer_add_child(primary_layer, bitmap_layer_get_layer(tick2_layer));

  tick3_layer = bitmap_layer_create(GRect(0, 60, bounds.size.w, bounds.size.h));
  bitmap_layer_set_bitmap(tick3_layer, tick_bitmap);
  bitmap_layer_set_alignment(tick3_layer, GAlignTopLeft);
  layer_add_child(primary_layer, bitmap_layer_get_layer(tick3_layer));

  tick4_layer = bitmap_layer_create(GRect(0, 70, bounds.size.w, bounds.size.h));
  bitmap_layer_set_bitmap(tick4_layer, tick_bitmap);
  bitmap_layer_set_alignment(tick4_layer, GAlignTopLeft);
  layer_add_child(primary_layer, bitmap_layer_get_layer(tick4_layer));

  
  time_layer = text_layer_create(GRect(0, 100, bounds.size.w, bounds.size.h));
  text_layer_set_font(time_layer, time_font);
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_layer));

  tick_timer_service_subscribe(SECOND_UNIT, (TickHandler) tick_handler);

  window_set_click_config_provider(window, (ClickConfigProvider) config_provider);
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_failed(out_failed_handler);
  app_message_open(512, 512);

  window_stack_push(window, true /* Animated */);
}

static void deinit() {
  tick_timer_service_unsubscribe();
  text_layer_destroy(time_layer);
  text_layer_destroy(date_layer);
  text_layer_destroy(location_layer);
  text_layer_destroy(weather_temp_layer);
  text_layer_destroy(next_layer);
  text_layer_destroy(manual_layer);
  text_layer_destroy(manual_code_layer);
  layer_destroy(primary_layer);
  layer_destroy(secondary_layer);
  bitmap_layer_destroy(img_layer);
  gbitmap_destroy(img_00d_bitmap);
  gbitmap_destroy(img_01d_bitmap);
  gbitmap_destroy(img_02d_bitmap);
  gbitmap_destroy(img_03d_bitmap);
  gbitmap_destroy(img_04d_bitmap);
  gbitmap_destroy(img_09d_bitmap);
  gbitmap_destroy(img_10d_bitmap);
  gbitmap_destroy(img_11d_bitmap);
  gbitmap_destroy(img_13d_bitmap);
  gbitmap_destroy(img_50d_bitmap);
  window_destroy(window);
}

int main(void) {
  init();
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  tick_handler(&tm, SECOND_UNIT);
  app_event_loop();
  deinit();
}
