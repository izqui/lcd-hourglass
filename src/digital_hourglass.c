#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0x17, 0x86, 0x00, 0xB5, 0xDD, 0x2D, 0x45, 0xAA, 0x9B, 0x87, 0x82, 0x79, 0x3B, 0x0C, 0x8B, 0xE4 }
PBL_APP_INFO(MY_UUID,
             "Digital Hourglass ", "benmccanny.com",
             0, 1, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

#define SCREEN_HEIGHT   168
#define SCREEN_HEIGHT_F 168.0
#define SCREEN_WIDTH    144


Window window;
InverterLayer minute_layer;
BmpContainer hour_layer;
PblTm next_update;
bool first_update;

PropertyAnimation animation;
GRect from;
GRect to; 

PropertyAnimation animation_2;
GRect to_2;

int img_ids[12] = {
  RESOURCE_ID_IMAGE_1,
  RESOURCE_ID_IMAGE_2,
  RESOURCE_ID_IMAGE_3,
  RESOURCE_ID_IMAGE_4,
  RESOURCE_ID_IMAGE_5,
  RESOURCE_ID_IMAGE_6,
  RESOURCE_ID_IMAGE_7,
  RESOURCE_ID_IMAGE_8,
  RESOURCE_ID_IMAGE_9,
  RESOURCE_ID_IMAGE_10,
  RESOURCE_ID_IMAGE_11,
  RESOURCE_ID_IMAGE_12
};


/* This watchface only supports 12h format due to size of large numbers */
unsigned short get_formatted_hour(unsigned short hour) {
  unsigned short display_hour = hour % 12;
  return display_hour ? display_hour : 12;
}

/* Check if we've reached the next update time */
bool should_update(PblTm * t) {
  return first_update
    || (t->tm_sec >= next_update.tm_sec
    && t->tm_min >= next_update.tm_min
    && t->tm_hour >= next_update.tm_hour);
}

/* 
 * Takes an input minute bar height and hour and sets the next update to
 * occur at the corresponding time 
 */
void set_next_update(int height, int hour) {

  /* Rollover the hour if height param taller than screen height */
  if(height != height % SCREEN_HEIGHT) {
    next_update.tm_hour = (next_update.tm_hour + 1) % 24;
    height = height % SCREEN_HEIGHT;
  }

  float mins_f = 60.0 * ((height) / SCREEN_HEIGHT_F);

  next_update.tm_hour = hour;
  next_update.tm_min = (int)mins_f;
  next_update.tm_sec = 60.0 * (mins_f - next_update.tm_min);
}

/* Creates a rollout effect of the minute bar as the watchface inits */
void animate_minute_init(int height) {
  from = GRect(0, 0, SCREEN_WIDTH, 0);
  to = GRect(0, 0, SCREEN_WIDTH, height);

  property_animation_init_layer_frame (&animation,
    (Layer *)&minute_layer,
    &from,
    &to
  );

  animation_set_duration(&animation.animation, 500);
  animation_schedule(&animation.animation);
}

/* 
 * Used to transition the fully-extended minute bar at the end of an hour
 * into its collapsed new hour form 
 */
void animate_hour_change() {
  to_2 = GRect(0, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT);

  GRect frame = layer_get_frame((Layer *)&minute_layer);
  frame.size.h = SCREEN_HEIGHT;
  layer_set_frame((Layer *)&minute_layer, frame);

  property_animation_init_layer_frame (&animation_2,
      (Layer *)&minute_layer,
      NULL,
      &to_2
    );

  animation_set_duration(&animation_2.animation, 500);
  animation_set_delay(&animation_2.animation, 500);
  animation_schedule(&animation_2.animation);
}

void reset_layers() {
  layer_remove_from_parent(&hour_layer.layer.layer);
  layer_remove_from_parent(&minute_layer.layer);
  bmp_deinit_container(&hour_layer);
}

void display_hour(int hour) {
  int id = img_ids[get_formatted_hour(hour) - 1];
  bmp_init_container(id, &hour_layer);
  layer_add_child(&window.layer, &hour_layer.layer.layer);
  hour_layer.layer.layer.frame.origin.y = 27;
}

int display_minute(int minutes, int seconds) {
  int height = (int)((minutes + seconds / 60.0) / 60.0 * SCREEN_HEIGHT);
  inverter_layer_init(&minute_layer, GRect(0, 0, SCREEN_WIDTH, height));

  if(first_update) {
    animate_minute_init(height);
  }

  layer_add_child(&window.layer, &minute_layer.layer);
  return height;
}

void display_time(PblTm * t, bool do_reset) {

  if(should_update(t)) {

    if(t->tm_min == 59 && t->tm_sec == 59) {
      animate_hour_change();
    } else {

      if(do_reset) {
        reset_layers();
      }

      /* The meat of displaying the time */
      display_hour(t->tm_hour);
      int height = display_minute(t->tm_min, t->tm_sec);

      set_next_update(height + 1, t->tm_hour);
    }
  }
}

void handle_init(AppContextRef ctx) {
  window_init(&window, "Watchface");
  window_stack_push(&window, true);

  /*
   * Import the bitmap resource handles. 
   * The font is Samson, by Meir Sadan: http://meirsadan.com/
   */
  resource_init_current_app(&BIG_SQUARE_FONT_RESOURCES);

  first_update = true;

  /* Draw watchface for the first time */
  PblTm tick_time;
  get_time(&tick_time);
  display_time(&tick_time, false);

  first_update = false;
}


void handle_deinit(AppContextRef ctx) {
  bmp_deinit_container(&hour_layer);
}

void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t) {
  display_time(t->tick_time, true);
}

/* Everything gets set up here */
void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,

    .tick_info = {  /* Run update check every second */
      .tick_handler = &handle_second_tick,
      .tick_units = SECOND_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
