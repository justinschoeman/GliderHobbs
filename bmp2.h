// BMx280_I2C.ino
//
// shows how to use the BMP280 / BMx280 library with the sensor connected using I2C.
//
// Copyright (c) 2018 Gregor Christandl
//
// connect the AS3935 to the Arduino like this:
//
// Arduino - BMP280 / BME280
// 3.3V ---- VCC
// GND ----- GND
// SDA ----- SDA
// SCL ----- SCL
// some BMP280/BME280 modules break out the CSB and SDO pins as well: 
// 5V ------ CSB (enables the I2C interface)
// GND ----- SDO (I2C Address 0x76)
// 5V ------ SDO (I2C Address 0x77)
// other pins can be left unconnected.

#include <Arduino.h>
#include <Wire.h>

//BMx280MI
#include <BMx280I2C.h>

#define BMP_I2C_ADDRESS 0x77

//create an BMX280 object using the I2C interface
BMx280I2C bmx280(BMP_I2C_ADDRESS);

uint8_t bmp_mode;
uint8_t bmp_cnt;
float bmp_temp = 0.0;
float bmp_pres = 0.0;

void bmp_setup(void) {
  Wire.begin();

  //begin() checks the Interface, reads the sensor ID (to differentiate between BMP280 and BME280)
  //and reads compensation parameters.
  if(!bmx280.begin()) {
    display_error(F("BMx280 init fail"));
  }

  //reset sensor to default parameters.
  bmx280.resetToDefaults();

#ifdef DEBUG
  if(bmx280.isBME280()) {
    Serial.println("sensor is a BME280");
  } else {
    Serial.println("sensor is a BMP280");
  }
#endif

  //by default sensing is disabled and must be enabled by setting a non-zero
  //oversampling setting.
  //set an oversampling setting for pressure and temperature measurements. 
  // max pressure
  bmx280.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
  // 2x temp
  bmx280.writeOversamplingTemperature(BMx280MI::OSRS_T_x02);
  // disable humidity, if present
  if(bmx280.isBME280()) {
    bmx280.writeOversamplingHumidity(BMx280MI::OSRS_H_x00);
  }
  // set up filter (max)
  //bmx280.writeFilterSetting(BMx280MI::FILTER_x16);
  
  // start free running conversions
  bmx280.writePowerMode(BMx280MI::BMx280_MODE_NORMAL);

  bmp_mode = 0;
  bmp_cnt = 255;
}

int bmp_run(void) {
  if(bmp_mode == 0) {
    if(bmx280.hasValue()) {
      bmp_mode++;
    }
    return 0;
  }

//  if(bmp_mode == 1) {
    if(bmp_cnt >= 50) {
      bmp_temp = bmx280.getTemperature();
      bmp_cnt = 0;
      return 1;
    }
    bmp_pres = bmx280.getPressure64();
    bmp_cnt++;
    bmp_mode = 0;
    return 1;
//  }
}
