// BMP180_I2C.ino
//
// shows how to use the BMP180MI library with the sensor connected using I2C.
//
// Copyright (c) 2018 Gregor Christandl
//
// connect the BMP180 to the Arduino like this:
// Arduino - BMC180
// 5V ------ VCC
// GND ----- GND
// SDA ----- SDA
// SCL ----- SCL

#include <Arduino.h>
#include <Wire.h>

#include <BMP180I2C.h>

#define BMP_I2C_ADDRESS 0x77

//create an BMP180 object using the I2C interface
BMP180I2C bmp180(BMP_I2C_ADDRESS);

int bmp_mode = 0; // starr with temperature
int bmp_cnt;
float bmp_temp = 0.0;
float bmp_pres = 0.0;

void bmp_setup(void) {
  Wire.begin();

  //begin() initializes the interface, checks the sensor ID and reads the calibration parameters.  
  if(!bmp180.begin()) {
    display_error(F("BMP180 init fail"));
  }

  //reset sensor to default parameters.
  bmp180.resetToDefaults();

  //enable ultra high resolution mode for pressure measurements
  bmp180.setSamplingMode(BMP180MI::MODE_UHR);
}

int bmp_run(void) {
  if(bmp_mode == 0) {
    bmp_cnt = 0;
    if(bmp180.measureTemperature()) {
      bmp_mode = 1;
    } else {
#ifdef DEBUG
      Serial.println(F("Could not start temperature measurement?"));
#endif
    }
    return 1;
  }

  if(bmp_mode == 1 || bmp_mode == 4) {
    if(bmp180.hasValue()) {
      bmp_mode++;
    }
    return 0;
  }

  if(bmp_mode == 2) {
    bmp_temp = bmp180.getTemperature();
    bmp_mode = 3;
    return 1;
  }

  if(bmp_mode == 3) {
    if(bmp180.measurePressure()) {
      bmp_mode = 4;
    } else {
#ifdef DEBUG
      Serial.println(F("Could not start pressure measurement?"));
#endif
    }
    return 1;
  }

  if(bmp_mode == 5) {
    bmp_pres = bmp180.getPressure();
    bmp_cnt++;
    if(bmp_cnt >= 50) {
      bmp_mode = 0;
    } else {
      bmp_mode = 3;
    }
    return 1;
  }
}
