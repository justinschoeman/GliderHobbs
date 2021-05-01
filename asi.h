
const float asi_cal_zero = 0.22835;
const float asi_cal_scale = 0.45;

#if 0

// IIR filter
// approx pole fs/100

#define ASI_AVG_BITS 6

uint16_t asi_val;
void asi_isr(uint8_t cfg, uint16_t val) {
  asi_val -= asi_val >> ASI_AVG_BITS;
  asi_val += val;
}

float asi_read(void) {
  // grab isr val under interrupt lock
  noInterrupts();
  float t = asi_val;
  interrupts();

  // convert to voltage
  t /= (float)(1024ULL << ASI_AVG_BITS);
  t *= 5.0;
  Serial.print("rawasi: ");
  Serial.println(t * 100.0);

  // convert to pressure
  t -= asi_cal_zero; // offset
  t /= asi_cal_scale; // scale
  
  // clamp zero
  if(t < 0.0) t = 0.0;

  // convert to airspeed
  //https://tghaviation.com/aircraft-instrument-services/pitot-static-system-airspeed-calculation/
  t /= 101.325;
  t += 1.0;
  t = pow(t, 2.0/7.0);
  t -= 1.0;
  t *= 5.0;
  t = sqrt(t);
  t *= 343.0; // m/s
  return t;
}

#else

// simple mean over measurement period

uint32_t asi_val = 0;
uint16_t asi_cnt = 0;
void asi_isr(uint8_t cfg, uint16_t val) {
  asi_val += val;
  asi_cnt++;
}

// return current airspeed in m/s
float asi_read(void) {
  // grab isr val under interrupt lock
  uint32_t t_val;
  uint16_t t_cnt;
  noInterrupts();
  t_val = asi_val;
  t_cnt = asi_cnt;
  asi_val = 0;
  asi_cnt = 0;
  interrupts();

  // no reading? return 0
  if(t_cnt == 0) return 0.0;

  // convert to voltage
  float t = (float)t_val;
  t /= (float)t_cnt;
  t *= 3.3/1024.0;
#ifdef DEBUG
  Serial.print("rawasi: ");
  Serial.println(t * 1000.0);
#endif

  // convert to pressure
  t -= asi_cal_zero; // offset
#ifdef DEBUG
  Serial.print("rawasi2: ");
  Serial.println(t * 1000.0);
#endif
  t /= asi_cal_scale; // scale
#ifdef DEBUG
  Serial.print("rawasi3: ");
  Serial.println(t * 1000.0);
#endif

  // clamp zero
  if(t < 0.0) t = 0.0;
#ifdef DEBUG
  Serial.print("rawasi4: ");
  Serial.println(t * 1000.0);
#endif

  // convert to airspeed
  //https://tghaviation.com/aircraft-instrument-services/pitot-static-system-airspeed-calculation/
  t /= 101.325;
  t += 1.0;
  t = pow(t, 2.0/7.0);
#ifdef DEBUG
  Serial.print("rawasi5: ");
  Serial.println(t * 1000.0);
#endif
  t -= 1.0;
  t *= 5.0;
  t = sqrt(t);
  t *= 343.0; // m/s
  return t;
}
#endif
