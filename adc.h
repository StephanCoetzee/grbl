/*
  Not part of Grbl, written by KeyMe
*/

#ifndef adc_h
#define adc_h

// Array of latest ADC readings
uint16_t analog_voltage_readings[VOLTAGE_SENSOR_COUNT];

#define FORCE_VALUE_INDEX 4 // Index of analog_voltage_readings that holds force value
#define REV_VALUE_INDEX 5



void adc_init();

uint16_t adc_read_channel(uint8_t channel);



#endif
