/*
  Not part of Grbl. KeyMe specific.

  SysTick implementation. Global SysTick value is updated.
  Callbacks can be registed to be called after a certain time.

  SysTick uses Timer1. Timer1 should not be used anywhere else.
*/
#ifndef __SYSTICK_H
#define __SYSTICK_H

#include "system.h"
#define MAX_CALLBACKS 32

typedef struct {
  uint64_t callback_time;
  void (*cb_function)();
} callback_t;

uint64_t SysTick;

void systick_init();

void systick_register_callback(unsigned long, void(*)());

void systick_service_callbacks();

#endif
