/*
 * General purpose display module.
 * 
 * Uses u8x8 from U8g2 for broad compatibility.
 * 
 * Choose your display and connect as per mfg/library instructions.
 * 
 * Code is configured for 128x64 display, but can be modified as required.
 */
//#define ALTERNATIVE_I2C_BYTE_PROCEDURE
#include <U8x8lib.h>

U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);

/*
 * Display library uses single byt I2C writes - no queues or buffers.
 * Each character takes about 2ms and a full line 32ms...
 * This will completely break DSP algortihms.
 * 
 * Instead app sets line to be displayed in following variables, and
 * display_run() outputs one character at a time (2ms per char, 8ms for 2x2 chars)
 * 
 */
uint8_t display_y; // y postion - 0 = top .. 7 = bottom
uint8_t display_style; // 0 = normal, 1 = double height, 2 = double width, double height
int8_t display_c; // character counter - initialise to x offest to start, reset to -1 when complete
char display_buf[17]; // row buffer
bool display_eol; // internal tracking of eol (remaineder of row is blanked after eol
char display_char[2]; // internal string for display function

// display_buf helpers
void display_append_float(float f) {
  int t = (int) f;
  f -= t;
  f *= 10;
  snprintf(display_buf + strlen(display_buf), sizeof(display_buf) - strlen(display_buf), "%d.%d", t, (int)f);
}

void display_append_string(const char * s) {
  snprintf(display_buf + strlen(display_buf), sizeof(display_buf) - strlen(display_buf), "%s", s);
}


// setup
void display_setup(void) {
    //u8x8.setI2CAddress(0x3C);
    u8x8.begin();
    u8x8.setFont(u8x8_font_pressstart2p_r);
    //u8x8.drawString(0, 0, "01234567890123456789");
    display_c = -1;
    display_eol = 0;
    display_char[1] = 0;
}

// display an error and 'hang' the CPU
// will hang until watchdog resets CPU and we can try again
void display_error(const __FlashStringHelper  * t) {
  u8x8.clearDisplay();
#ifdef DEBUG
  Serial.println(t);
#endif
  u8x8.setCursor(0, 1);
  u8x8.print(t);
  while(1) {};
}

// run display state engine
// triggered by setting display_c >= 0
// done when display_c < 0
bool display_run(void) {
  if(display_c < 0) return 0; // wait for ui to request new draw
#ifdef DEBUG
  unsigned long ms = millis();
#endif
  // pre-calculate line length
  uint8_t len = display_style & 2 ? 8 : 16;
  // pre-process style at start
  if(display_c == 0) {
    // center?
    if(display_style & 0x80) {
      uint8_t i = strlen(display_buf);
      uint8_t j = (len-i)/2;
      memmove(display_buf+j, display_buf, i+1);
      memset(display_buf, ' ', j);
    }
    display_style &= 3;
  }
  // eol?
  if(display_buf[display_c] == 0) {
    display_eol = 1;
  }
  // pick char to render (spaces when clearing to eol)
  if(display_eol) {
    display_char[0] = ' ';
  } else {
    display_char[0] = display_buf[display_c];
  }
  // render char and calculate line end length
  switch(display_style) {
    case 1:
      u8x8.draw1x2String(display_c, display_y, display_char);
      break;
    case 2:
      u8x8.draw2x2String(display_c*2, display_y, display_char);
      break;
    default:
      u8x8.drawString(display_c, display_y, display_char);
  }
  // update state
  display_c++;
  // reset when line complete
  if(display_c >= len) {
    display_c = -1;
    display_eol = 0;
  }
#ifdef DEBUG
  //Serial.print("dt: ");
  //Serial.println(millis() - ms);
#endif
  return 1;
}
