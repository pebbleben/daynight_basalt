#include <pebble.h>
#include "config.h"

//go here to confirm image http://www.timeanddate.com/worldclock/sunearth.html
//http://codecorner.galanter.net/2015/04/03/simplify-access-to-framebuffer-on-pebble-time/
  
#define STR_SIZE 20
#define TIME_OFFSET_PERSIST 1
#define REDRAW_INTERVAL 15

static Window *window;
static TextLayer *time_text_layer;
static TextLayer *time_JPN_text_layer; //Ben Added
static TextLayer *time_SWE_text_layer; //Ben Added
static TextLayer *date_text_layer;
static GBitmap *world_bitmap;
static GBitmap *world_NIGHT_bitmap;
static Layer *canvas;
// this is a manually created bitmap, of the same size as world_bitmap
static GBitmap image;
static GBitmap *imageB;
static int redraw_counter;
// s is set to memory of size STR_SIZE, and temporarily stores strings
char *s;
// Local time is wall time, not UTC, so an offset is used to get UTC
int time_offset;

static void draw_earth() {
  // ##### calculate the time
#ifdef UTC_OFFSET
  int now = (int)time(NULL) + -3600 * UTC_OFFSET;
#else
  int now = (int)time(NULL) + time_offset;
#endif
  float day_of_year; // value from 0 to 1 of progress through a year
  float time_of_day; // value from 0 to 1 of progress through a day
  // approx number of leap years since epoch
  // = now / SECONDS_IN_YEAR * .24; (.24 = average rate of leap years)
  int leap_years = (int)((float)now / 131487192.0);
  // day_of_year is an estimate, but should be correct to within one day
  day_of_year = now - (((int)((float)now / 31556926.0) * 365 + leap_years) * 86400);
  day_of_year = day_of_year / 86400.0;
  time_of_day = day_of_year - (int)day_of_year;
  day_of_year = day_of_year / 365.0;
  // ##### calculate the position of the sun
  // left to right of world goes from 0 to 65536
  int sun_x = (int)((float)TRIG_MAX_ANGLE * (1.0 - time_of_day));
  // bottom to top of world goes from -32768 to 32768
  // 0.2164 is march 20, the 79th day of the year, the march equinox
  // Earth's inclination is 23.4 degrees, so sun should vary 23.4/90=.26 up and down
  int sun_y = -sin_lookup((day_of_year - 0.2164) * TRIG_MAX_ANGLE) * .26 * .25;
  // ##### draw the bitmap
  #ifdef PBL_PLATFORM_APLITE 
  //APPLITE
  int x, y;
  for(x = 0; x < image.bounds.size.w; x++) {
    int x_angle = (int)((float)TRIG_MAX_ANGLE * (float)x / (float)(image.bounds.size.w));
    for(y = 0; y < image.bounds.size.h; y++) {
      int byte = y * image.row_size_bytes + (int)(x / 8);
      int y_angle = (int)((float)TRIG_MAX_ANGLE * (float)y / (float)(image.bounds.size.h * 2)) - TRIG_MAX_ANGLE/4;
      // spherical law of cosines
      float angle = ((float)sin_lookup(sun_y)/(float)TRIG_MAX_RATIO) * ((float)sin_lookup(y_angle)/(float)TRIG_MAX_RATIO);
      angle = angle + ((float)cos_lookup(sun_y)/(float)TRIG_MAX_RATIO) * ((float)cos_lookup(y_angle)/(float)TRIG_MAX_RATIO) * ((float)cos_lookup(sun_x - x_angle)/(float)TRIG_MAX_RATIO);
      if ((angle < 0) ^ (0x1 & (((char *)world_bitmap->addr)[byte] >> (x % 8)))) {
        // white pixel
        ((char *)image.addr)[byte] = ((char *)image.addr)[byte] | (0x1 << (x % 8));
      } else {
        // black pixel
        ((char *)image.addr)[byte] = ((char *)image.addr)[byte] & ~(0x1 << (x % 8));
      }
    }
  }
  #else
  //BASALT
  #endif
  //
  layer_mark_dirty(canvas);
}

static void draw_watch(struct Layer *layer, GContext *ctx) {
  #ifdef PBL_PLATFORM_APLITE
    graphics_draw_bitmap_in_rect(ctx, &image, image.bounds);
  #else
    graphics_draw_bitmap_in_rect(ctx, world_bitmap, gbitmap_get_bounds(world_bitmap));
  //GPoint p0 = GPoint(25, 25);  
  //graphics_context_set_stroke_color(ctx, GColorRed );  
  //  graphics_draw_pixel(ctx, p0);
  //COPY FORMUAL FROM ABOVE
    // ##### calculate the time
//#ifdef UTC_OFFSET
//  int now2 = (int)time(NULL) + -3600 * -4;//UTC_OFFSET;
//#else
  int now2 = (int)time(NULL) + time_offset;
//#endif
  float day_of_year; // value from 0 to 1 of progress through a year
  float time_of_day; // value from 0 to 1 of progress through a day
  // approx number of leap years since epoch
  // = now / SECONDS_IN_YEAR * .24; (.24 = average rate of leap years)
  int leap_years = (int)((float)now2 / 131487192.0);
  // day_of_year is an estimate, but should be correct to within one day
  day_of_year = now2 - (((int)((float)now2 / 31556926.0) * 365 + leap_years) * 86400);
  day_of_year = day_of_year / 86400.0;
  time_of_day = day_of_year - (int)day_of_year;
  day_of_year = day_of_year / 365.0;
  // ##### calculate the position of the sun
  // left to right of world goes from 0 to 65536
  int sun_x = (int)((float)TRIG_MAX_ANGLE * (1.0 - time_of_day));
  // bottom to top of world goes from -32768 to 32768
  // 0.2164 is march 20, the 79th day of the year, the march equinox
  // Earth's inclination is 23.4 degrees, so sun should vary 23.4/90=.26 up and down
  int sun_y = -sin_lookup((day_of_year - 0.2164) * TRIG_MAX_ANGLE) * .26 * .25;
  // ##### draw the bitmap
  int x, y;
  //GSize image_size = gbitmap_get_bounds(imageB).size;

  static GBitmap *bb;
  //GColor *gbitmap_get_palette(bb);  
  bb = gbitmap_create_with_resource(RESOURCE_ID_NIGHT_PBLv2);//CLR64);//NIGHT_PBL);
  
  //static GBitmap *bbb;
  //bbb =  gbitmap_create_from_png_data(gbitmap_get_data(bb),GBitmapFormat8Bit);
  
  //put frame buffer here
  //GBitmap *fb = graphics_capture_frame_buffer_format(ctx, gbitmap_get_format(bb));//GBitmapFormat8Bit
  GBitmap *fb = graphics_capture_frame_buffer_format(ctx, GBitmapFormat8Bit);//
  uint8_t *fb_data = gbitmap_get_data(fb);  
  
  //#define PALETTE_SIZE GBitmapFormat8Bit //16
  //GColor *orig_palette = gbitmap_get_palette(bb);
  //GColor *new_palette = (GColor *)malloc(PALETTE_SIZE * sizeof(GColor));
  //for (int i = 0; i < PALETTE_SIZE; ++i) {
  //  new_palette[i].argb = orig_palette[i].argb ^ 0x3f;
  //}
  //gbitmap_set_palette(bb, new_palette, true);
    
  uint8_t *background_data = gbitmap_get_data(bb); 
  //int bpr;
  //bpr = gbitmap_get_bytes_per_row(bb);
  #define WINDOW_WIDTH 144 
  uint8_t (*fb_matrix)[WINDOW_WIDTH] = (uint8_t (*)[WINDOW_WIDTH]) fb_data;
  uint8_t (*background_matrix)[WINDOW_WIDTH] = (uint8_t (*)[WINDOW_WIDTH]) background_data;
  //put frame buffer here
  
  //GColor invisible = (Gcolor){
  //  .a = 0b01,
  //  .r = 0b11,
  //  .g = 0b10,
  //  .b = 0b00
  //};
  
  
  for(x = 0; x < 144; x++) {
    int x_angle = (int)((float)TRIG_MAX_ANGLE * (float)x / (float)(144));
    for(y = 0; y < 72; y++) {
      int y_angle = (int)((float)TRIG_MAX_ANGLE * (float)y / (float)(72 * 2)) - TRIG_MAX_ANGLE/4;
      // spherical law of cosines
      float angle = ((float)sin_lookup(sun_y)/(float)TRIG_MAX_RATIO) * ((float)sin_lookup(y_angle)/(float)TRIG_MAX_RATIO);
      angle = angle + ((float)cos_lookup(sun_y)/(float)TRIG_MAX_RATIO) * ((float)cos_lookup(y_angle)/(float)TRIG_MAX_RATIO) * ((float)cos_lookup(sun_x - x_angle)/(float)TRIG_MAX_RATIO);
      if (angle > 0) { //^ (0x1 & (((char *)world_bitmap->addr)[byte] >> (x % 8)))) {
        // white pixel
        //((char *)imageB.addr)[byte] = ((char *)imageB.addr)[byte] | (0x1 << (x % 8));
      } else {
        // black pixel
        //fb_matrix[y][x] = 0;
        //if (fb_matrix[y][x] == 0) {
        //  fb_matrix[y][x] = 255;
        //}
        //if (fb_matrix[y][x] == GColorIslamicGreen) {
        //  fb_matrix[y][x] = GColorDarkGreen ;
        //}
        //fb_matrix[y][x] = invisible;//background_matrix[y][x];
        //fb_matrix[y][x] = background_data[y*bpr+x];
        fb_matrix[y][x] = background_matrix[y][x];
          //GPoint p0 = GPoint(x, y);  
          //graphics_context_set_stroke_color(ctx, GColorWhite  );  
          //graphics_context_set_stroke_color(ctx, GColorBlack );  
        //graphics_draw_pixel(ctx, p0);
        //((char *)image.addr)[byte] = ((char *)image.addr)[byte] & ~(0x1 << (x % 8));
      }
    }
  }
  //NOW LETS DO CODE WHERE WE LOOP THROUGH IMAGE AND IF WE SEE COLOR PLACED ABOVE WE REPLACE IT WITH SOEMTHING ELSE
  //GBitmap *fb = graphics_capture_frame_buffer_format(ctx,GBitmapFormat8Bit);
  //uint8_t *fb_data = gbitmap_get_data(fb);
  //uint8_t *background_data = gbitmap_get_data(world_NIGHT_bitmap);
  //for (int i=0; i<144*168;i++){
  //  if (fb_data[i] == 255){
  //    fb_data[i] = background_data[i];
  //  }  
  //}
  graphics_release_frame_buffer(ctx,fb);
  gbitmap_destroy(bb);
    
  //
  #endif
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  //static char time_text[8] = "00:00 am";
  static char time_text[] = "00:00";
  static char time_JPN_text[] = "00:00xx"; //BEN ADDED
  static char time_SWE_text[] = "00:00xx"; //BEN ADDED
  static char date_text[] = "Xxx, Xxx 00";
  static struct tm *JPN_time; //BEN ADDED
  static struct tm *SWE_time; //BEN ADDED

  strftime(date_text, sizeof(date_text), "%a, %b %e", tick_time);
  text_layer_set_text(date_text_layer, date_text);

  if (tick_time->tm_hour > 12) {
  //strftime(time_text, 8, "%I:%M pm", tick_time);
    strftime(time_text, sizeof(time_text), "%I:%M", tick_time);
  }else {
  //strftime(time_text, 8, "%I:%M am", tick_time);
    strftime(time_text, sizeof(time_text), "%I:%M", tick_time);
  }
  //if (clock_is_24h_style()) {
    //strftime(time_text, sizeof(time_text), "%R", tick_time);
    //strftime(time_text, sizeof(time_text), "%I:%M", tick_time);
  //} else {
    //strftime(time_text, sizeof(time_text), "%I:%M", tick_time);
  //}
  //if (!clock_is_24h_style() && (time_text[0] == '0')) {
  if (time_text[0] == '0') {
  //20150608 edit force remove zero
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }
  text_layer_set_text(time_text_layer, time_text);
 
  //SWEDEN START//
  SWE_time = tick_time;
  SWE_time->tm_hour += 6;
  if (SWE_time->tm_hour > 23) {
   SWE_time->tm_hour -= 24;
  }  
  
  if (clock_is_24h_style()) {
    //strftime(time_SWE_text, sizeof(time_SWE_text), "%R",  SWE_time);
    strftime(time_SWE_text, sizeof(time_SWE_text), "%I:%M%p", SWE_time);
  } else {
    strftime(time_SWE_text, sizeof(time_SWE_text), "%I:%M%p", SWE_time);
  }
  if (time_SWE_text[0] == '0') {
    memmove(time_SWE_text, &time_SWE_text[1], sizeof(time_SWE_text) - 1);
  }
  text_layer_set_text(time_SWE_text_layer, time_SWE_text);  //Just added  
  //SWEDEN END//
  
  //JAPAN START//
  JPN_time = tick_time;
  JPN_time->tm_hour += 7;
  if (JPN_time->tm_hour > 23) {
   JPN_time->tm_hour -= 24;
  }  
  if (clock_is_24h_style()) {
    //strftime(time_JPN_text, sizeof(time_JPN_text), "%R",  JPN_time);
    strftime(time_JPN_text, sizeof(time_JPN_text), "%I:%M%p", JPN_time);
  } else {
    strftime(time_JPN_text, sizeof(time_JPN_text), "%I:%M%p", JPN_time);
  }
  if (time_JPN_text[0] == '0') {
    memmove(time_JPN_text, &time_JPN_text[1], sizeof(time_JPN_text) - 1);
  }
  text_layer_set_text(time_JPN_text_layer, time_JPN_text);  //Just added
  //JAPAN END//  
  
  redraw_counter++;
  if (redraw_counter >= REDRAW_INTERVAL) {
    draw_earth();
    redraw_counter = 0;
  }
}

// Get the time from the phone, which is probably UTC
// Calculate and store the offset when compared to the local clock
static void app_message_inbox_received(DictionaryIterator *iterator, void *context) {
  Tuple *t = dict_find(iterator, 0);
  int unixtime = t->value->int32;
  int now = (int)time(NULL);
  time_offset = unixtime - now;
  status_t s = persist_write_int(TIME_OFFSET_PERSIST, time_offset); 
  if (s) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved time offset %d with status %d", time_offset, (int) s);
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to save time offset with status %d", (int) s);
  }
  #ifdef PBL_PLATFORM_APLITE //20150607 Tried to fix applite offset
    draw_earth();
  #endif
}

static void window_load(Window *window) {
#ifdef BLACK_ON_WHITE
  window_set_background_color(window, GColorWhite);
#else
      //#ifdef PBL_COLOR
      //window_set_background_color(window, GColorDukeBlue);
      //#else
      window_set_background_color(window, GColorBlack );
      //#endif
#endif
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

 //Local
  time_text_layer = text_layer_create(GRect(0, 72, 144-0, 168-72));
#ifdef BLACK_ON_WHITE
  text_layer_set_background_color(time_text_layer, GColorWhite);
  text_layer_set_text_color(time_text_layer, GColorBlack);
#else
      //#ifdef PBL_COLOR
      //text_layer_set_background_color(time_text_layer, GColorDukeBlue);
      //#else
      text_layer_set_background_color(time_text_layer, GColorBlack );
      //#endif
  text_layer_set_text_color(time_text_layer, GColorWhite);
#endif
  text_layer_set_font(time_text_layer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
  text_layer_set_text(time_text_layer, "");
  text_layer_set_text_alignment(time_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_text_layer));

  //Date
  date_text_layer = text_layer_create(GRect(0, 110, 144-0, 168-110));
#ifdef BLACK_ON_WHITE
  text_layer_set_background_color(date_text_layer, GColorWhite);
  text_layer_set_text_color(date_text_layer, GColorBlack);
#else
      //#ifdef PBL_COLOR
      //text_layer_set_background_color(date_text_layer, GColorDukeBlue);
      //#else
      text_layer_set_background_color(date_text_layer, GColorBlack );
      //#endif     
  text_layer_set_text_color(date_text_layer, GColorWhite);
#endif
  text_layer_set_font(date_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text(date_text_layer, "");
  text_layer_set_text_alignment(date_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(date_text_layer));
  
//SWEDEN
  time_SWE_text_layer = text_layer_create(GRect(0, 139, 144-72, 168-139));
#ifdef BLACK_ON_WHITE
  text_layer_set_background_color(time_SWE_text_layer, GColorWhite);
  text_layer_set_text_color(time_SWE_text_layer, GColorBlack);
#else
      //#ifdef PBL_COLOR
      //text_layer_set_background_color(time_SWE_text_layer, GColorDukeBlue);
      //#else
      text_layer_set_background_color(time_SWE_text_layer, GColorBlack );
      //#endif    
  text_layer_set_text_color(time_SWE_text_layer, GColorWhite);
#endif
  text_layer_set_font(time_SWE_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(time_SWE_text_layer, "");
  text_layer_set_text_alignment(time_SWE_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_SWE_text_layer));  
  
  //JAPAN
  time_JPN_text_layer = text_layer_create(GRect(72, 139, 144-72, 168-139));
#ifdef BLACK_ON_WHITE
  text_layer_set_background_color(time_JPN_text_layer, GColorWhite);
  text_layer_set_text_color(time_JPN_text_layer, GColorBlack);
#else
      //#ifdef PBL_COLOR
      //text_layer_set_background_color(time_JPN_text_layer, GColorDukeBlue);
      //#else
      text_layer_set_background_color(time_JPN_text_layer, GColorBlack );
      //#endif  
  text_layer_set_text_color(time_JPN_text_layer, GColorWhite);
#endif
  text_layer_set_font(time_JPN_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(time_JPN_text_layer, "");
  text_layer_set_text_alignment(time_JPN_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_JPN_text_layer));


  
  //BOTH APPLITE AND BASALT
    canvas = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
    layer_set_update_proc(canvas, draw_watch);
    layer_add_child(window_layer, canvas);
  //APPLITE ONLY 
  #ifdef PBL_PLATFORM_APLITE 
    image = *world_bitmap;
    image.addr = malloc(image.row_size_bytes * image.bounds.size.h);
  //draw_earth();// 20150607 does this fix appligth?
  #else
    imageB = gbitmap_create_with_resource(RESOURCE_ID_WORLD);
  #endif     
  draw_earth(); ///20150607 does this fix appligth?
  
}

static void window_unload(Window *window) {
  text_layer_destroy(time_text_layer);
  text_layer_destroy(time_JPN_text_layer);
  text_layer_destroy(time_SWE_text_layer);
  text_layer_destroy(date_text_layer);
  layer_destroy(canvas);
  #ifdef PBL_PLATFORM_APLITE
    free(image.addr);
  #endif
}

static void init(void) {
  redraw_counter = 0;

  // Load the UTC offset, if it exists
  time_offset = 0;
  if (persist_exists(TIME_OFFSET_PERSIST)) {
    time_offset = persist_read_int(TIME_OFFSET_PERSIST);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "loaded offset %d", time_offset);
  }

  #ifdef PBL_PLATFORM_APLITE 
  world_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WORLD);
  #else 
  world_bitmap = gbitmap_create_with_resource(RESOURCE_ID_DAY_PBL);  //SIMPLE_DAY);//
  world_NIGHT_bitmap = gbitmap_create_with_resource(RESOURCE_ID_NIGHT_PBL);
  #endif

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  
  const bool animated = true;
  window_stack_push(window, animated);

  s = malloc(STR_SIZE);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

  app_message_register_inbox_received(app_message_inbox_received);
  app_message_open(30, 0);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  free(s);
  window_destroy(window);
  gbitmap_destroy(world_bitmap);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
