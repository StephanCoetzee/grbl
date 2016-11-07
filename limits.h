/*
  limits.h - code pertaining to limit-switches and performing the homing cycle
  Part of Grbl

  Copyright (c) 2013-2014 Sungeun K. Jeon  
  Copyright (c) 2009-2011 Simen Svale Skogsrud

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LIMITS_H
#define LIMITS_H 

// Largest possible distance to move during pulloff motion of homing.
// This is defined by the largest homing flag in the system. The Y flag is
// the largest with a length of just under 40 mm.
#define MAXFLAGLEN 40.0

// Travel distance for the gripper from home is around 12 mm.
// Travel distane for gripping key is usually around 2-3 mm.
#define MAXSERVODIST 12.0

typedef struct {
  uint8_t expected;
  uint8_t active;
  volatile uint8_t ishoming;
  volatile uint8_t isservoing;
  uint8_t mag_gap_check;  // KeyMe specific
  uint16_t bump_grip_force;  // KeyMe specific: Value must be 0-1023
} limit_t;

extern limit_t limits;

// Initialize the limits module
void limits_init();
// Configure the limit enable/disable state
void limits_configure();

void limits_enable(uint8_t axes,uint8_t expected);
void limits_disable();

// Perform one portion of the homing cycle based on the input settings.
void limits_go_home(uint8_t cycle_mask);

// Check for soft limit violations
void limits_soft_check(float *target);

// Perform force servo cycle
void limits_force_servo();
#endif
