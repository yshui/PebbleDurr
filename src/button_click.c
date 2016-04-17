#include <pebble.h>

static Window *window;
static TextLayer *text_layer, *interval_text, *last_text = NULL, *next_text = NULL;
int wakeupid, interval;
bool changed;
static void text_update();
static void interval_update();
static void update_record();
static void setup_wakeup() {
  time_t curr = time(NULL);
  int tmpint = persist_read_int(1);
  curr += tmpint*60;
  do {
    wakeupid = wakeup_schedule(curr, 0, false);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%d", wakeupid);
    curr += 60;
  }while(wakeupid == E_RANGE);
  if (wakeupid >= 0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%d %d", wakeupid, wakeup_query(wakeupid, NULL));
    persist_write_int(0, wakeupid);
    update_record();
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to setup wakeup, bye");
  }
}
static void fmt_time(int sec, char *res) {
  int u = sec > 0 ? sec : -sec;
  int s = u%60;
  u /= 60;
  int m = u%60;
  u /= 60;
  int h = u%24;
  u /= 24;
  int d = u;
  if (d > 999) {
    strcpy(res, sec < 0 ? "Long long ago" : "Far far away");
    return;
  }
  if (d)
    snprintf(res, 20, "%s%dd%dh%dm%ds%s", sec > 0 ? "in " : "", d, h, m, s, sec < 0 ? " ago" : "");
  else if (h)
    snprintf(res, 20, "%s%dh%dm%ds%s", sec > 0 ? "in " : "", h, m, s, sec < 0 ? " ago" : "");
  else if (m)
    snprintf(res, 20, "%s%dm%ds%s", sec > 0 ? "in " : "", m, s, sec < 0 ? " ago" : "");
  else if (s)
    snprintf(res, 20, "%s%ds%s", sec > 0 ? "in " : "", s, sec < 0 ? " ago" : "");
  else
    snprintf(res, 20, "Just now");
}
static void update_record() {
  static char lstr[30], nstr[30];
  if (!persist_exists(2)) {
    text_layer_set_text(last_text, "LAST: None");
  } else {
    int diff = persist_read_int(2)-time(NULL);
    strcpy(lstr, "LAST: ");
    fmt_time(diff, lstr+6);
    text_layer_set_text(last_text, lstr);
  }
  
  time_t next;
  if (!wakeup_query(wakeupid, &next))
    text_layer_set_text(next_text, "NEXT: Never");
  else {
    int diff = next-time(NULL);
    strcpy(nstr, "NEXT: ");
    fmt_time(diff, nstr+6);
    text_layer_set_text(next_text, nstr);
  }
}
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (changed) {
    wakeup_cancel_all();
    persist_write_int(1, interval);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "written %d", (int)persist_read_int(1));
    changed = false;
    interval_update();
  }
  if (!wakeup_query(wakeupid, NULL)) {
    setup_wakeup();
  } else {
    wakeup_cancel(wakeupid);
  }
  text_update();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  interval++;
  if (interval >= 1000)
    interval = 999;
  changed = true;
  interval_update();
  text_update();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  interval--;
  if (interval < 1)
    interval = 1;
  changed = true;
  interval_update();
  text_update();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}
static void text_update() {
  if (!wakeup_query(wakeupid, NULL))
    text_layer_set_text(text_layer, "Press select to start");
  else {
    if (changed)
      text_layer_set_text(text_layer, "Press select to apply");
    else
      text_layer_set_text(text_layer, "Press select to stop");
  }
}
static void interval_update() {
  static char str[4];
  int len = 0;
  if (interval >= 1000)
    strcpy(str, "999");
  else {
    int tmpi = interval;
    if (tmpi >= 100) {
      str[len++] = tmpi/100+'0';
      tmpi %= 100;
    }
    if (tmpi >= 10) {
      str[len++] = tmpi/10+'0';
      tmpi %= 10;
    } else if (len)
      str[len++] = '0';
    str[len++] = tmpi+'0';
    str[len] = '\0';
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%d %s", interval, str);
  text_layer_set_text(interval_text, str);
}
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create(GRect(0, bounds.size.h/2+20, bounds.size.w, 20));
  text_update();
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  
  interval_text = text_layer_create(GRect(0, bounds.size.h/2-40, bounds.size.w, 42));
  text_layer_set_text_alignment(interval_text, GTextAlignmentCenter);
  text_layer_set_font(interval_text, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  layer_add_child(window_layer, text_layer_get_layer(interval_text));
  interval_update();
  
  last_text = text_layer_create(GRect(0, 0, bounds.size.w, 20));
  text_layer_set_text_alignment(last_text, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(last_text));
  
  next_text = text_layer_create(GRect(0, bounds.size.h-20, bounds.size.w, 20));
  text_layer_set_text_alignment(next_text, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(next_text));
  
  update_record();
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
  text_layer_destroy(interval_text);
  text_layer_destroy(last_text);
  text_layer_destroy(next_text);
  text_layer = interval_text = last_text = next_text = NULL;
}

static void wakeup_handler(WakeupId wid, int32_t reason) {
  //Should I vibrate?
  HealthActivityMask act = health_service_peek_current_activities();
  if ((act&HealthActivitySleep) || (act&HealthActivityRestfulSleep)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Not vibrating due to sleep.");
    //Since user is asleep, we might just as well quit
    window_stack_pop_all(false);
    //See if someone else is using worker
    //If not, good, otherwise we are going to drain slightly more power
    if (!app_worker_is_running()) {
      if (app_worker_kill() != APP_WORKER_RESULT_DIFFERENT_APP) {
        app_worker_launch();
        if (!app_worker_is_running()) {
          APP_LOG(APP_LOG_LEVEL_DEBUG, "Worker is still being started, setup wakeup to be safe.");
          setup_wakeup();
        }
      } else {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Another worker is running, using wakeup instead.");
        setup_wakeup();
      }
    }
    return;
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Hey!!");
  //User has awaken, kill app worker
  app_worker_kill();
  vibes_double_pulse();
  persist_write_int(2, time(NULL)); 
  setup_wakeup();
  if (last_text && next_text)
    update_record();
}

static void init(void) {
  if (!persist_exists(0))
    wakeupid = 0;
  else
    wakeupid = persist_read_int(0);
  if (!persist_exists(1))
    persist_write_int(1, 30);
  
  if (app_worker_is_running()) {
    //User launched this app, but we think
    //the user is asleep.
    app_worker_kill();
    setup_wakeup();
  }
  wakeup_service_subscribe(wakeup_handler);
  interval = persist_read_int(1);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "interval = %d", interval);
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}


int main(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
  if (launch_reason() == APP_LAUNCH_WAKEUP) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Launched by wakeup.");
    WakeupId wid;
    int32_t reason;
    wakeup_get_launch_event(&wid, &reason);
    wakeup_handler(wid, reason);
  } else if (launch_reason() == APP_LAUNCH_WORKER) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Launched by worker, user might be awake.");
    wakeup_handler(0, -1);
  } else {
    init();
    app_event_loop();
    deinit();
  }
}