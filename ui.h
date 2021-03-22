// ui algorithm
uint8_t ui_state;
unsigned long ui_ms;
const unsigned long ui_interval = 500UL; // 0.5s display update rate
const unsigned long ui_msg_interval = 5000UL; // 5s display message time
int ui_msg_state; // state tracker for message display
char ui_msg_buf[17];

void ui_msg(const char * msg) {
  snprintf(ui_msg_buf, sizeof(ui_msg_buf), "%s", msg);
  ui_msg_state = 0;
}

void ui_setup(void) {
  ui_state = 0;
  ui_msg_state = -1;
}

// run display message state, if required
void ui_run_msg(void) {
  if(display_c >= 0) return; // wait for last row to be complete
  display_style = 0;
  display_buf[16] = 0; // terminate for when we populate spaces
  switch(ui_msg_state) {
    case 0:
      display_y = 3;
      memcpy(display_buf, ui_msg_buf, strlen(ui_msg_buf)+1);
      break;
    case 1:
      memset(display_buf, ' ', 16);
    case 2:
    case 3:
      display_y = ui_msg_state - 1;
      break;
    case 4:
    case 5:
    case 6:
    case 7:
      display_y = ui_msg_state;
      break;
    default:
      // no more states to update - wait for next interval and restart
      if(millis() - ui_ms < ui_msg_interval) return; // wait for next interval
      ui_ms = millis();
      ui_msg_state = -1;
      return;
  }
  //trigger next line output
  display_c = 0;
  // advance to next state
  ui_msg_state++;
}

void ui_run(void) {
  if(display_c >= 0) return; // wait for last row to be complete
  if(ui_msg_state >= 0) return ui_run_msg(); // jump to message display, if required
  display_style = 0; // default to 1x1 chars
  switch(ui_state) {
    case 0:
      display_y = 0;
      display_style = 0x82;
      if(fs_state) {
        snprintf(display_buf, sizeof(display_buf), "*");
        display_append_float(airframe_hours.get_current());
      } else {
        display_buf[0] = 0;
        display_append_float(airframe_hours.get());
      }
      break;
    case 1:
      display_y = 3;
      display_style = 0x80;
      if(!fs_state) {
        snprintf(display_buf, sizeof(display_buf), "*");
        display_append_float(airframe_hours.get_current());
      } else {
        display_buf[0] = 0;
        display_append_float(airframe_hours.get());
      }
      break;
    case 2:
      display_y = 5;
      //snprintf(display_buf, sizeof(display_buf), "A %d.%d   ", (int)asi_asi, (int)((asi_asi-(int)asi_asi)*10.0));
      snprintf(display_buf, sizeof(display_buf), "A ");
      display_append_float(asi_asi * 1.94384); // convert to kts
      display_append_string("   "); // pad till we are sure to overlap H
      snprintf(display_buf+8, sizeof(display_buf)-8, "H %d", (int)(alt_alt*3.28084)); // convert to ft
      break;
    case 3:
      display_y = 6;
      snprintf(display_buf, sizeof(display_buf), "V %d     ", (int)(alt_vsi_m * 100.0)); // vsi in cm/s
      snprintf(display_buf+8, sizeof(display_buf)-8, "M %d", (int)(alt_vsi * 1000.0)); // model vsi in cm/s
      break;
    case 4:
      display_y = 7;
      snprintf(display_buf, sizeof(display_buf), "T %d     ", (int)((alt_vsi_m+asi_de) * 100.0)); // vsi in cm/s
      snprintf(display_buf+8, sizeof(display_buf)-8, "T %d", (int)((alt_vsi+asi_de) * 1000.0)); // model vsi in cm/s
      break;
    default:
      // no more states to update - wait for next interval and restart
      if(millis() - ui_ms < ui_interval) return; // wait for next interval
      ui_ms = millis();
      ui_state = 0;
      return;
  }
  //trigger next line output
  display_c = 0;
  // advance to next state
  ui_state++;
}
