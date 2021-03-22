#ifndef _ADC_H_
#define _ADC_H_

#define ADC_MUX(ref, apin) ((ref << 6) | apin)

typedef struct {
  uint8_t mux;
  void (*handler)(uint8_t cfg, uint16_t value);
} adc_config_t;

adc_config_t * adc_config;
uint8_t adc_config_count;
uint8_t adc_act;

ISR(ADC_vect) {
  uint16_t val = ADC;
  uint8_t bak = adc_act; // back it up

  // next handler
  adc_act++;
  if(adc_act >= adc_config_count) adc_act = 0;
  
  // set next mux
  _SFR_BYTE(ADMUX) = adc_config[adc_act].mux;
  
  // start next sample
  _SFR_BYTE(ADCSRA) |= _BV(ADSC);

  // fire off handler
  adc_config[bak].handler(bak, val);
}

void adc_setup(adc_config_t * cfg, uint8_t cnt) {
  // record config in globals
  adc_config = cfg;
  adc_config_count = cnt;
  // start at first config item
  adc_act = 0;
  // everything else is really configured by wiring -> just turn on interrupts
  _SFR_BYTE(DIDR0) |= 0x3f;
  _SFR_BYTE(ADCSRA) |= _BV(ADIE);
  // fire off the first read
  _SFR_BYTE(ADMUX) = adc_config[0].mux;
  _SFR_BYTE(ADCSRA) |= _BV(ADSC);
}

#endif
