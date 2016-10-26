/*
  Not part of Grbl. KeyMe specific
  
  Signal is one layer of abstraction above adc.h and adc.c.

  ADC values are read in specific time intervals, filtered and
  stored in the appropriate arrays.
*/

#ifndef signals_h
#define signals_h

#include "system.h"

#define N_FILTER 3

// Array of latest ADC readings
uint16_t analog_voltage_readings[VOLTAGE_SENSOR_COUNT];  // Filtered ADC readings
uint16_t analog_voltage_readings_x[VOLTAGE_SENSOR_COUNT][N_FILTER + 1];  // Unfiltered ADC readings

#define FORCE_VALUE_INDEX 4 // Index of analog_voltage_readings that holds force value
#define REV_VALUE_INDEX 5

void signals_update_motors();
void signals_update_force();
void signals_update_revision();

#endif

