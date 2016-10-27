/*
  Not part of Grbl. KeyMe specifice
  
  Signal is one layer of abstraction above adc.h and adc.c.

  ADC values are read in specific time intervals, filtered and
  stored in the appropriate arrays.
*/

#include "signals.h"
#include "adc.h"
#include "systick.h"

uint16_t analog_voltage_readings_x[VOLTAGE_SENSOR_COUNT][N_FILTER + 1];  // Unfiltered ADC readings

// Updates motors ADC readings
void signals_update_motors()
{
  uint8_t idx;

  for (idx = 0; idx < N_AXIS; idx++) {
    // Assumes the motors are on ADC channels 0-3 and in the same
    // order in analog_voltage_readings If the pins are changed, a
    // map should be made to motors to ADC channels.
      analog_voltage_readings[idx] = adc_read_channel(idx);
  }
}

// Filter and update force ADC reading
void signals_update_force()
{
  #ifdef USE_LOAD_CELL
    analog_voltage_readings_x[FORCE_VALUE_INDEX][N_FILTER] = adc_read_channel(LC_ADC);
  #else
    analog_voltage_readings_x[FORCE_VALUE_INDEX][N_FILTER] = adc_read_channel(F_ADC); 
  #endif
    /*
    Filter:
      Moving average Hanning Filter:
      y[k] = 0.25 * (x[k] + 2x[k-1] + x[k-2])
    */
    analog_voltage_readings[FORCE_VALUE_INDEX] = (uint16_t)(
    0.25 * (analog_voltage_readings_x[FORCE_VALUE_INDEX][N_FILTER] 
    + (2 * analog_voltage_readings_x[FORCE_VALUE_INDEX][N_FILTER - 1]) 
    + analog_voltage_readings_x[FORCE_VALUE_INDEX][N_FILTER] ));

    // Advance all values in the unfiltered array
    uint8_t idx;
    for (idx = 0; idx < N_FILTER; idx++) {
      analog_voltage_readings_x[FORCE_VALUE_INDEX][idx] = analog_voltage_readings_x[FORCE_VALUE_INDEX][idx + 1];
    } 
}

void signals_callback()
{
  signals_update_motors();
  signals_update_force();
  // Register callback to this function in SIGNALS_CALLBACK_INTERVAL milliseconds
  systick_register_callback(SIGNALS_CALLBACK_INTERVAL, signals_callback);
}

// Read value from revision voltage divider
// No nead to filter since the value is constant.
// Only needs to be called once during initialization
void signals_update_revision()
{
  analog_voltage_readings[REV_VALUE_INDEX] = adc_read_channel(RD_ADC);
}

