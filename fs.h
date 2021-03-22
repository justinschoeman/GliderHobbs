/*
 * flight state
 * 
 * maintain state and timers
 * speed must be above/below threshold continuously
 * for specified time, before changing state
 */

// state variables
bool fs_state = 0; // flight state - 0 = ground, 1 = flying
unsigned long fs_timer; // ms since state transition was good
// config opions
const float k_fs_asi = 8.0; // 8m/s ~ 30kph trigger speed
unsigned long k_fs_ms = 30000UL; // 30s to change state

// reset timer - called often, may help to make it a procedure
void fs_reset(void) {
  fs_timer = millis();
}

// set up state machine
void fs_setup(void) {
  fs_reset();
}

// run/update state engine
void fs_run(void) {
  if(fs_state) {
    // flying?
    if(asi_asi > k_fs_asi) {
      // still flying speed? reset timer
      fs_reset();
    } else {
      // below flying airspeed ? wait for timer to expire
      if(millis() - fs_timer > k_fs_ms) {
#ifdef DEBUG
        Serial.println("NO LONGER FLYING");
#endif
        ui_msg("  FLIGHT ENDED");
        fs_state = 0;
        fs_reset();
        airframe_hours.stop();
      }
    }
  } else {
    // ground
    if(asi_asi > k_fs_asi) {
      // flying speed? wait for timer to expire
      if(millis() - fs_timer > k_fs_ms) {
#ifdef DEBUG
        Serial.println("FLYING!!!");
#endif
        ui_msg("     FLYING");
        fs_state = 1;
        fs_reset();
        airframe_hours.start();
      }
    } else {
      // below flying airspeed ? reset timer
      fs_reset();
    }
  }
}
