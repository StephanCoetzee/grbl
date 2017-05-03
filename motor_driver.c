#include <stdio.h>

#include "motor_driver.h"
#include "pin_map.h"
#include "nuts_bolts.h"

// For use with the SPI driver chip
void _motor_drv_write_reg(enum stepper_e stepper, enum address_e address uint8_t * data)
{
  uint8_t data_out[2];
  data_in[0] = (address << 4) | (data[1] & 0x0F);

  bit_true(SCS_PORT, 1 << scs_pin_lookup[stepper];
  spi_write(data_out, 2);
  bit_false(SCS_PORT, 1 << scs_pin_lookup[stepper];

}

void _motor_drv_read_reg(enum stepper_e stepper, enum address_e address, uint8_t * data)
{
  uint8_t data_in[2], data_out[2];
  data_in[0] = address << 5;
  data_in[1] = 0;
  bit_true(SCS_PORT, 1 << scs_pin_lookup[stepper]);
  spi_transact_array(data_out, data_in, 2);
  bit_false(SCS_PORT, 1 << scs_pin_lookup[stepper]);
}

void _motor_drv_set_val(enum stepper_e stepper,
                        enum address_e address,
                        uint8_t idx,
                        uint8_t mask,
                        uint8_t val)
{
  uint8_t data[] = {0, 0};
  /* Read the register */
  _motor_drv_read_reg(stepper, address, data);

  /* Clear the bits that needs to be set */
  data &= (mask << idx);

  /* Set the new value */
  data |= (new_val & mask) << idx;
 
  /* Write the updated value to the register */
  _motor_drv_write_reg(stepper, address, data);

}

void motor_drv_set_decay_mode(enum stepper_e stepper, enum decay_e decmod)
{
  _motor_drv_set_val(stepper, DECAY, 8, 0x0F, decmod);
}

void motor_drv_set_torque(enum stepper_e stepper, uint8_t torque)
{
  _motor_drv_set_val(stepper, TORQUE, 0, 0xFF, torque);
}

void motor_drv_set_micro_stepping(enum steps_e steps)
{
  _motor_drv_set_val(stepper, CTRL, 3, 0xF, steps);
}
