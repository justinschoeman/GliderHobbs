// shared state
float asi_asi; // current airspeed
float asi_last; // airspeed 1s ago
float asi_de; // estimate energy change in airspeed, in terms of height
float alt_last; // altitude 1s ago
float alt_vsi_m; // altitude calculated by change over 1s

// ugly forward declarations
void ui_msg(const char * msg);

// load modules
#include <avr/wdt.h>
#include "display.h"
#include "alt.h"
#include "bmp.h"
#include "adc.h"
#include "asi.h"
#include "hour.h"

// configure free-running ADC ISR to read ASI
// Analog pin for ASI
#define A_ASI 0

adc_config_t adc_conf[] = {
  {ADC_MUX(DEFAULT, A_ASI), asi_isr}
};

// instantiate airframe hour meter with eeprom storage
// 32 blocks starting at address 16
// 32 .. 159
Hour airframe_hours = Hour(16, 32);

// requires airframe_hours to be declared...
#include "fs.h"

// load ui (requires everything else to be defined)
#include "ui.h"

// set up all modules
void setup() {
  // start watchdog (8s timeout)
  wdt_enable(WDTO_8S);

#ifdef DEBUG
	//wait for serial connection to open (only necessary on some boards)
  Serial.begin(9600);
	while (!Serial);
#endif
  display_setup();

  alt_setup();
  bmp_setup();
  adc_setup(adc_conf, 1);
  //airframe_hours.set(85400LL);
  //airframe_hours.set(49998LL);
  //while(1) {}
  airframe_hours.load();
  fs_setup();
  ui_setup();
}

// loop timer
unsigned long ltime = 0;

void loop() {
  // run the pressure sensor
  if(bmp_run()) return;
  // run the altitude algorithm, if a new pressure reading is available
  if(bmp_pres) {
    alt_run(bmp_pres);
    bmp_pres = 0.0;
    return;
  }
  // run the airframe hobbs
  if(airframe_hours.update()) return;
  // display update
  if(display_run()) return;
  // ui update
  ui_run();

  if(millis() - ltime > 1000) {
    // update non-freerunning state every 1s

    // read asi
    asi_asi = asi_read();

    // update flight state (requires new asi reading)
    fs_run();

    // estimate vsi as current alt - last alt (runs every second, so result should be m/s)
    alt_vsi_m = alt_alt - alt_last;
    alt_last = alt_alt;

    // convert asi change into equivant height change
    asi_de = asi_asi * asi_asi;
    asi_de -= asi_last * asi_last;
    asi_de /= 2.0 * 9.8;
    asi_last = asi_asi;

#ifdef DEBUG
    Serial.print("Temperature: "); 
    Serial.print(bmp_temp); 
    Serial.println(" degC");
    Serial.print("Pressure: "); 
    Serial.print(bmp_pres); 
    Serial.println(" Pa");
    Serial.print("Altitude: "); 
    Serial.print(alt_alt); 
    Serial.println(" m");
    Serial.println(bmp_cnt);
    Serial.println(asi_read());
    Serial.println(airframe_hours.get());
    Serial.println(airframe_hours.get_current());
    Serial.print("ASI: ");
    Serial.println(asi_asi);
    Serial.print("FS: ");
    Serial.println(fs_state);
    Serial.print("DE: ");
    Serial.println(asi_de);
    Serial.print("DH: ");
    Serial.println(alt_vsi_m);
    Serial.print("DH2: ");
    Serial.println(alt_vsi_m+asi_de);
    Serial.print("VSI: "); 
    Serial.print(alt_vsi); 
    Serial.println(" m/s");
    Serial.print("VSI2: "); 
    Serial.println(alt_vsi+asi_de); 
#endif
    // energy
    //float e = asi_asi * asi_asi * 0.5;
    //e += alt_alt * 9.8;
    //Serial.println(e);

    // next run in 1000ms
    ltime = millis();
    // successful end of main look + 1 sec timer, then kick wdt
    wdt_reset();
  }
}
