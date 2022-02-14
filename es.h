/*
 * engine state
 * 
 * maintain state and timers
 */

// state variables
bool es_state = 0; // engine state - 0 = off, 1 = runing
unsigned long es_timer; // ms since state transition was good
// config opions
unsigned long k_es_ms = 5000UL; // 5s to change state

// reset timer - called often, may help to make it a procedure
void es_reset(void) {
  es_timer = millis();
}

// set up state machine
void es_setup(void) {
  es_reset();
}

// run/update state engine
void es_run(void) {
  if(es_state) {
    // running?
    if(digitalRead(DE_PIN)) {
      // still running? reset timer
      es_reset();
    } else {
      // below flying airspeed ? wait for timer to expire
      if(millis() - es_timer > k_es_ms) {
#ifdef DEBUG
        Serial.println("NO LONGER RUNING");
#endif
        ui_msg(" ENGINE STOPPED");
        es_state = 0;
        es_reset();
        engine_hours.stop();
      }
    }
  } else {
    // stopped
    if(digitalRead(DE_PIN)) {
      // engine started? wait for timer to expire
      if(millis() - es_timer > k_es_ms) {
#ifdef DEBUG
        Serial.println("ENGINE START!!!");
#endif
        ui_msg(" ENGINE RUNNING");
        es_state = 1;
        es_reset();
        engine_hours.start();
      }
    } else {
      // not running ? reset timer
      es_reset();
    }
  }
}
