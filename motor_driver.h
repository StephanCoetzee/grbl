#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include "system.h"

enum address_e {
  CTRL = 0,
  TORQUE,
  OFF,
  BLANK,
  DECAY,
  STALL,
  DRIVE,
  STATUS

};

enum steps_e {
  FULL = 0,
  HALF,
  QUARTER,
  EIGHT,
  SIXTEEN,
  THREE_TWO,
  SIX_FOUR,
  ONE_TWO_EIGHT
};

enum stepper_e {
  XTABLE = 0,
  YTABLE,
  GRIPPER,
  CAROUSEL
};

/* The order of the entries in this enum is important */
enum decmod_e {
  SLOW = 0,
  SLOW_INCR_MIXED_DECR,
  FAST,
  MIXED,
  SLOW_INCR_AUTO_MIXED_DECR,
  AUTO_MIXED
};

static const uint8_t scs_pin_lookup[4] = {
  SCS_XTABLE_PIN,
  SCS_YTABLE_PIN,
  SCS_GRIPPER_PIN,
  SCS_CAROUSEL_PIN

};

#endif //MOTOR_DRIVER_H
