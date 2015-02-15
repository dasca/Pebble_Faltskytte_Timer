#include <pebble.h>
  
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_init_layer;
ActionBarLayer *action_bar;

const char *s_time_format = "%02d";
int i_init_time = 9;
int current_time = 0;
int init_time = 0;

GBitmap *ico_minus;
GBitmap *ico_play;
GBitmap *ico_plus;
GBitmap *ico_reset;
GBitmap *ico_stop;


enum State {
  START,
  INIT,
  PAUSE,
  STOP,
};

enum State current_state = STOP;

static void update_time() {
  // Create a long-lived buffer
  static char buf[] = "00";
  
  if(current_state == INIT)
    snprintf(buf, sizeof(buf), s_time_format, init_time);
  else
    snprintf(buf, sizeof(buf), s_time_format, current_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buf);
}

void inc_time(){
  if(i_init_time < 60) {
    i_init_time++;
    current_time = i_init_time;
    update_time();
  }
}

void dec_time(){
  if(i_init_time > 0) {
    i_init_time--;
    current_time = i_init_time;
    update_time();
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {

  if(current_state == INIT){
    init_time--;
    update_time();
    if(init_time == 3)
      vibes_long_pulse();
    else if(init_time == 0) {
      current_state = START;
      layer_set_hidden((Layer *) s_init_layer, true);
      vibes_long_pulse();
    }
  } else if(current_state == START) {
    current_time--;
    update_time();
    switch(current_time) {
      case 3:
      case 2:
      case 1:
        vibes_short_pulse();
      break;
      case 0:
        vibes_long_pulse();
        tick_timer_service_unsubscribe();
        current_state = PAUSE;
        return;
      break;
    }
  }
}
void up_click_handler(ClickRecognizerRef recognizer, void *context)
{
  if(current_state == STOP) {
    inc_time();
  }
}
 
void down_click_handler(ClickRecognizerRef recognizer, void *context)
{
  if(current_state == STOP) {
    dec_time();
  }
}
 
void select_click_handler(ClickRecognizerRef recognizer, void *context)
{
  if(current_state == STOP) {
    layer_set_hidden((Layer *) s_init_layer, false);
    init_time = 10;

    action_bar_layer_clear_icon(action_bar, BUTTON_ID_UP);
    action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, ico_stop);
    action_bar_layer_clear_icon(action_bar, BUTTON_ID_DOWN);
    
    current_state = INIT;
    update_time();
    // Register with TickTimerService
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  } else if(current_state == START || current_state == INIT) {
    tick_timer_service_unsubscribe();
    action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, ico_reset);
    current_state = PAUSE;
  } else if(current_state == PAUSE) {
    layer_set_hidden((Layer *) s_init_layer, true);
    current_time = i_init_time;
    update_time();
    action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, ico_plus);
    action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, ico_minus);
    action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, ico_play);
    current_state = STOP;
  }
}

void click_config_provider(void *context)
{
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void main_window_load(Window *window) {
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 40, 144, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  
  // Create init TextLayer
  s_init_layer = text_layer_create(GRect(25, 70, 15, 5));
  text_layer_set_background_color(s_init_layer, GColorBlack);
  text_layer_set_text_color(s_init_layer, GColorClear);
  
  // Improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  current_time = i_init_time;
  update_time();
  
  // Initialize the action bar:
  action_bar = action_bar_layer_create();
  // Associate the action bar with the window:
  action_bar_layer_add_to_window(action_bar, window);
  // Set the click config provider:
  action_bar_layer_set_click_config_provider(action_bar, click_config_provider);
  
  action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, ico_plus);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, ico_minus);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, ico_play);
  
  layer_set_hidden((Layer *) s_init_layer, true);
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_init_layer));
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_init_layer);
}


static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  ico_play = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PLAY_ICON_14);
  ico_plus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PLUS_ICON_14);
  ico_stop = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STOP_ICON_14);
  ico_minus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MINUS_ICON_14);
  ico_reset = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RESET_ICON_14);
  
  window_set_click_config_provider(s_main_window, click_config_provider);

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
  gbitmap_destroy(ico_play);
  gbitmap_destroy(ico_plus);
  gbitmap_destroy(ico_stop);
  gbitmap_destroy(ico_minus);
  gbitmap_destroy(ico_reset);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
