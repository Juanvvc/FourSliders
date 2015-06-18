/*
A Pebble color wahtface that shows Time, weekday and days as sliders.

This project does not use bitmaps, and the font is a customized LilGrotest (under the GPL)

(c) 2015, Juan Vera
*/

#include <pebble.h>

// TODO: unfortuntaly, you cannot changed this constants without changing lots of thigs in update_time()
// number of segments to show of hours
#define HOUR_SEGMENTS 3
// width of a complete, half and quarter segments (=144/HOUR_SEGMENTS)
#define HOUR_WIDTH 48
#define HOUR_HALF_WIDTH 24
#define HOUR_QUARTER_WIDTH 12
#define WEEK_SEGMENTS 3
#define WEEK_WIDTH 48
#define DAY_SEGMENTS 3
#define DAY_WIDTH 48
#define MONTH_SEGMENTS 2
#define MONTH_WIDTH 72

// Resources and layers
Window *s_main_window;
static Layer *s_layer_hour;
static GFont s_time_font;
static GFont s_time_small_font;
static GPath *s_mark;
  
static char *hours[] = { "12", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"};
static char *weekdays[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static char *months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
// numbers of days for each month
static int maxdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// The central mark uses colors to show BT connection and battery status
static GColor color_border;
static GColor color_lines;
static GColor color_back;
static GColor color_text;
static GColor color_bt;
static GColor color_warning;
static bool hide_status = false;

// the mark
static const GPathInfo MARK_INFO = {
  .num_points = 3,
  .points = (GPoint []) {{0,0}, {-12, -14}, {12, -14}}
};

// configuration received from the remote server
enum CommunicationKey {
  KEY_STATUS = 0x0,
  KEY_THEME = 0x1
};

// returns the number of days of a month (0-11), in the range 1-31
static int get_max_days(int month, int year) {
  if ( month == 1) {
    if ( year % 400 == 0 ) {
      return 29;
    } else if ( year % 100 == 0 ) {
      return 28;
    } else if ( year % 4 == 0 ) {
      return 29;
    } else {
      return 28;
    }
  }
  return maxdays[month];
}

// loads a theme
static void load_theme(int theme) {
#ifdef PBL_COLOR
  switch(theme) {
  case 1: // green theme
  color_border = GColorDarkGreen;
  color_text = color_lines = GColorBrass;
  color_back = GColorPastelYellow;
  color_bt = GColorVividCerulean;
  color_warning = GColorRed;
  break;
  
  case 2: // blue theme
  color_border = GColorOxfordBlue;
  color_text = color_lines = GColorWhite;
  color_back = GColorCobaltBlue;
  color_bt = GColorVividCerulean;
  color_warning = GColorRed;
  break;
  
  case 3: // red
  color_border = GColorDarkCandyAppleRed;
  color_text = color_lines = GColorBlack;
  color_back = GColorMelon;
  color_bt = GColorVividCerulean;
  color_warning = GColorRed;
  break;  
  
  default:
  color_border = color_text = color_lines = GColorBlack;
  color_back = GColorWhite;
  color_bt = GColorVividCerulean;
  color_warning = GColorRed;
  }
  persist_write_int(KEY_THEME, theme);
#else
  color_border = color_text = color_lines = color_bt = GColorBlack;
  color_back = color_warning = GColorWhite;
#endif
}

// given a day (0-30), a month (0-11), a year (0-5000) and an offset, returns (0-30) the number of day of the offseted day.
// For example, if today is January, 1st yesterday (offset=-1) was 31th. If it is April, 30th, tomorrow (offset=1) is 1st.
static int this_day_is(int today, int month, int year, int offset) {
  int this_month_days = get_max_days(month, year);
  int last_month_days = get_max_days((month + 11) % 12, year);
  APP_LOG(APP_LOG_LEVEL_INFO, "%d %d %d %d %d", today, month, this_month_days, last_month_days, offset);
  if ( today + offset >= this_month_days) {
    return (today - this_month_days) + offset;
  }
  if ( today + offset < 0 ) {
    return last_month_days + offset;
  }
  return today + offset;
}

// updates the layer
static void update_layer_hour(struct Layer *layer, GContext *ctx) {
  int x;
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  int hour = tick_time->tm_hour;
  int min = tick_time->tm_min;
  int weekday = tick_time->tm_wday;
  int day = tick_time->tm_mday -1;  // the rest of the code expects this starting at 0
  int month = tick_time->tm_mon;
  int year = tick_time->tm_year + 1900;
  
  // draw borders
  graphics_context_set_fill_color(ctx, color_border);
  graphics_fill_rect(ctx, GRect(0,0,144,168), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, color_back);
  graphics_fill_rect(ctx, GRect(4,4,136,86), 5, GCornersAll);
  graphics_fill_rect(ctx, GRect(4,95,136,30), 5, GCornersAll);
  graphics_fill_rect(ctx, GRect(4,130,136,34), 5, GCornersAll);
  graphics_draw_line(ctx, GPoint(0, 147), GPoint(144, 147));
  
  graphics_context_set_stroke_color(ctx, color_lines);
  graphics_context_set_text_color(ctx, color_text);
      
  // TODO: from here, most of the offsets are not calculated but just "values that work"
  
  // Hour
  int offset = (HOUR_WIDTH * min) / 60;
  for(int i=0; i<HOUR_SEGMENTS + 1; i++) {
    // hour line
    x = HOUR_HALF_WIDTH - offset + i * HOUR_WIDTH;
    if ( x > 4 && x < 140 ) {
      graphics_draw_line(ctx, GPoint(x,5), GPoint(x,25));
      graphics_draw_line(ctx, GPoint(x+1,5), GPoint(x+1,25)); // double line
      graphics_draw_line(ctx, GPoint(x,67), GPoint(x,87));
      graphics_draw_line(ctx, GPoint(x+1,67), GPoint(x+1,87)); // double line
    }
    // text
    graphics_draw_text(
      ctx,
      hours[abs((hour + i + 11) % 12)],
      s_time_font,
      GRect(x - HOUR_HALF_WIDTH, 16, HOUR_WIDTH, 55),
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    // half hour line
    x = -offset + i * HOUR_WIDTH;
    if ( x > 4 && x < 140 ) {
      graphics_draw_line(ctx, GPoint(x,5), GPoint(x,20));
      graphics_draw_line(ctx, GPoint(x,71), GPoint(x,86));
    }
    // quarter hour lines
    x = HOUR_HALF_WIDTH - offset + i * HOUR_WIDTH - HOUR_QUARTER_WIDTH; 
    if ( x > 4 && x < 140 ) {
      graphics_draw_line(ctx, GPoint(x,5), GPoint(x,15));
      graphics_draw_line(ctx, GPoint(x,76), GPoint(x,86));
    }
    x = HOUR_HALF_WIDTH - offset + i * HOUR_WIDTH + HOUR_QUARTER_WIDTH; 
    if ( x > 4 && x < 140 ) {
      graphics_draw_line(ctx, GPoint(x,5), GPoint(x,15));
      graphics_draw_line(ctx, GPoint(x,76), GPoint(x,86));
    }
  }
  
  // weekday
  offset = (WEEK_WIDTH * hour) / 24 - WEEK_WIDTH / 2;
  for(int i=-1; i<WEEK_SEGMENTS + 1; i++) {
    x = -offset + i * WEEK_WIDTH;
    if ( x > 4 && x < 140 ) {
      graphics_draw_line(ctx, GPoint(x,105), GPoint(x,112));
    }
    // text
    graphics_draw_text(
      ctx,
      weekdays[abs((weekday - WEEK_SEGMENTS / 2 + i + 7) % 7)],
      s_time_small_font,
      GRect(x, 100, WEEK_WIDTH, 20),
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }
  
  // day
  offset = (DAY_WIDTH * hour) / 24;
  char *buffer = "XXX";
  int show_day;
  for(int i=-2; i<DAY_SEGMENTS + 1; i++) {
    x = -offset + i * DAY_WIDTH + 72;
    show_day = this_day_is(day, month, year, i);
    if ( x > 4 && x < 140 ) {
      graphics_draw_line(ctx, GPoint(x,135), GPoint(x,140));
    }
    // text
    snprintf(buffer, 3, "%d", show_day + 1);
    graphics_draw_text(
      ctx,
      buffer,
      s_time_small_font,
      GRect(x, 130, DAY_WIDTH, 20),
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }
  // month
  offset = (MONTH_WIDTH * day ) / get_max_days(month, year);
  for(int i=-1; i<DAY_SEGMENTS + 1; i++) {    
    x = -offset + i * MONTH_WIDTH + 72;
    if ( x > 4 && x < 140 ) {
      graphics_draw_line(ctx, GPoint(x,153), GPoint(x,158));
    }
    // text
    graphics_draw_text(
      ctx,
      months[(month + i + 12) % 12],
      s_time_small_font,
      GRect(x, 146, MONTH_WIDTH, 20),
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }
  
  // draw borders, again
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_round_rect(ctx, GRect(4,4,136,86), 5);
  graphics_draw_round_rect(ctx, GRect(4,95,136,30), 5);
  graphics_draw_round_rect(ctx, GRect(4,130,136,34), 5);
  
  // draw marks
  gpath_rotate_to(s_mark, 0);
  // use colors to show information about bt and battery
  if ( hide_status) {
    graphics_context_set_fill_color(ctx, color_border);
  } else {
    if ( bluetooth_connection_service_peek() ) {
      graphics_context_set_fill_color(ctx, color_bt);
    } else {
      graphics_context_set_fill_color(ctx, color_border);
    }
    if ( battery_state_service_peek().charge_percent < 20 ) {
      graphics_context_set_fill_color(ctx, color_warning);
    }
  }
  gpath_move_to(s_mark, GPoint(72,14));
  gpath_draw_filled(ctx, s_mark);
  graphics_context_set_fill_color(ctx, color_border);
  gpath_move_to(s_mark, GPoint(72,14));
  gpath_draw_outline(ctx, s_mark);
  //gpath_move_to(s_mark, GPoint(72,140));
  //gpath_draw_filled(ctx, s_mark);
  gpath_move_to(s_mark, GPoint(72,103));
  gpath_draw_filled(ctx, s_mark);
  gpath_draw_outline(ctx, s_mark);
  gpath_rotate_to(s_mark, TRIG_MAX_ANGLE / 2);
  gpath_move_to(s_mark, GPoint(72,79));
  gpath_draw_filled(ctx, s_mark);
  gpath_draw_outline(ctx, s_mark);
  gpath_move_to(s_mark, GPoint(72,157));
  gpath_draw_filled(ctx, s_mark);
  gpath_draw_outline(ctx, s_mark);
  // redraw borders, to overwrite numbers on the border
  graphics_context_set_fill_color(ctx, color_border);
  graphics_fill_rect(ctx, GRect(0,0,4,168), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(140,0,4,168), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(0,90,144,5), 0, GCornerNone);
}

static void update_time() {
  layer_mark_dirty(s_layer_hour);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

// Manage communication
static void in_recv_handler(DictionaryIterator *iterator, void *context) {
  //Get Tuple
  Tuple *t = dict_read_first(iterator);
  while(t != NULL) {
    switch(t->key) {
      case KEY_THEME:
      if (strcmp(t->value->cstring, "Green") == 0) {
        load_theme(1);
      } else if (strcmp(t->value->cstring, "Blue") == 0) {
        load_theme(2);
      } else if (strcmp(t->value->cstring, "Red") == 0) {
        load_theme(3);
      } else {
        load_theme(0);
      }
      break;
      
      case KEY_STATUS:
      hide_status = (strcmp(t->value->cstring, "hide") == 0);
      persist_write_int(KEY_STATUS, hide_status);
      break;
    }
    t = dict_read_next(iterator);
  }
  update_time();
}

static void main_window_load(Window *w) {
  load_theme(persist_read_int(KEY_THEME));
  hide_status = persist_read_bool(KEY_STATUS);
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TIME_48));
  s_time_small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TIME_14));
  s_mark = gpath_create(&MARK_INFO);
  
  s_layer_hour = layer_create(GRect(0,0,144,168));
  layer_set_update_proc(s_layer_hour, update_layer_hour);
  layer_add_child(window_get_root_layer(w), s_layer_hour);
}

static void main_window_unload(Window *w) {
  layer_destroy(s_layer_hour);
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_time_small_font);
  gpath_destroy(s_mark);
}

void init(void) {
  // create the window and register handlers
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload    
  });
    
  // show the window on the watch, with animated==true
  window_stack_push(s_main_window, true);
  
  // register services
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  app_message_register_inbox_received((AppMessageInboxReceived) in_recv_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  update_time();
}

void deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
