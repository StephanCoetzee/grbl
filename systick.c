/*
  Not part of Grbl. KeyMe specific.

  SysTick implementation. Global SysTick value is updated.
  Callbacks can be registed to be called after a certain time.

  SysTick uses Timer1. Timer1 should not be used anywhere else.

*/
#include "systick.h"
#include "nuts_bolts.h"

typedef struct {
  uint64_t callback_time;
  void (*cb_function)();
} callback_t;

callback_t systick_callbacks_array[MAX_CALLBACKS];
uint8_t systick_len;  // Length of callback array 

void systick_service_callbacks()
{
  // Assumes the callback array is sorted 
  // according to lowest callback time
  if (systick_callbacks_array[0].callback_time >= SysTick) {
    systick_callbacks_array[0].cb_function();

    // Shift all entries in the callback array to the left
    memmove(&systick_callbacks_array[0], &systick_callbacks_array[1], systick_len - 1);
    
    systick_len--;
  }
}

void systick_init()
{
  // Reset SysTick
  SysTick = 0;

  PRR0 &= ~(1<<PRTIM1); // Gives power to Timer 1
  TCCR1B |= (1<<CS11)|(1<<CS10); // Prescaler 64x - 16 MHz / 64 = 250 kHz

  // CTC mode - Clear Timer on Compare
  bit_true(TCCR1B, 1 << WGM12);
  
  // Inialize the counter
  TCNT1 = 0;
  
  // Initialize timer compare value
  OCR1A = 250; // 250 kHz/250 = 1 kHz, hence, SysTick will tick every ms 
  
  // Clear overflow flag - clear on write
  bit_true(TIFR1, 1 << TOV1);
  
  // Set Timer Overflow Interrupt Enable in Timer Interrupt Mask Register
  bit_true(TIMSK1, TOIE1);

  // Initialize length of the callback array
  systick_len = 0; 
}

// Every millisecond
ISR(TIMER1_COMPA_vect)
{
  SysTick++;
}

// Helper function for qsort. Used in systick_sort_callback_array
int cmp(const void * a, const void * b)
{
  callback_t *cb_a = (callback_t *)a;
  callback_t *cb_b = (callback_t *)b;
  
  if (cb_a->callback_time < cb_b->callback_time)
    return 1;

  if (cb_a->callback_time > cb_b->callback_time)
    return -1;

  return 0;
}

// Sort the callback array according to lowest callback time
void systick_sort_callback_array()
{
  qsort(systick_callbacks_array, systick_len, sizeof(callback_t), cmp); 
}


// Callback functions are called in protocol.c
void systick_register_callback(unsigned long ms_later, void (*func)())
{
  uint64_t callback_time = SysTick + ms_later;
  callback_t to_insert;
  to_insert.callback_time = callback_time;
  to_insert.cb_function = func;

  // Insert callback into callback array
  systick_callbacks_array[systick_len] = to_insert;

  // Increase the length of the array
  systick_len++; 
 
  // Sort the callback array
  systick_sort_callback_array();

}
