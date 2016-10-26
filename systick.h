/*
  Not part of Grbl. KeyMe specific.

  SysTick implementation. Gloabl SysTick value is updated.
  Callbacks can be registed to be called after a certain time.

  SysTick uses Timer1. Timer1 should not be used anywhere else.
*/

unsigned long SysTick;
void systick_init();
