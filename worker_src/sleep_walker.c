#include <pebble_worker.h>
void health_handler(HealthEventType e, void *ud) {
  HealthActivityMask act = health_service_peek_current_activities();
  if ((act&HealthActivitySleep) || (act&HealthActivityRestfulSleep))
    //Still sleeping
    return;
  worker_launch_app();
}
int main() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Just waiting for user to wake up");
  health_service_events_subscribe(health_handler, NULL);
  worker_event_loop();
}