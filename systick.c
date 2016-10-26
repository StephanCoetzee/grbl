/*
  Not part of Grbl. KeyMe specific.

  SysTick implementation. Gloabl SysTick value is updated.
  Callbacks can be registed to be called after a certain time.

  SysTick uses Timer1. Timer1 should not be used anywhere else.

*/
#include "systick.h"
#include "system.h"
#include "nuts_bolts.h"

// SysTick is an unsigned long, which is 4 bytes on AVR.
#define MAX_SYS_TICK 0xFFFFFFFF

void systick_init()
{
  
  // Reset SysTick
  SysTick = 0;

  PRR0 &= ~(1<<PRTIM1); // Gives power to Timer 1
  //TCCR1B &= ~(1<<WGM13); // Normal operation / counter
  //TCCR1B &= ~(1<<WGM12);
  TCCR1B |= (1<<CS11)|(1<<CS10); // Prescaler 64x - 16 MHz / 64 = 250 kHz

  // CTC mode - Clear Timer on Compare
  bit_true(TCCR1B, 1 << WGM12);
  
  //TCCR1A &= ~((1<<WGM11) | (1<<WGM10));
  //TCCR1A &= ~((1<<COM1A1) | (1<<COM1A0) | (1<<COM1B1) | (1<<COM1B0)); // Disconnect OC4 output

  // Inialize the counter
  TCNT1 = 0;
  
  // Initialize timer compare value
  OCR1A = 250; // 250 kHz/250 = 1 kHz, hence, SysTick will tick every ms 
  
  // Clear overflow flag - clear on write
  bit_true(TIFR1, 1 << TOV1);
  
  // Set Timer Overflow Interrupt Enable in Timer Interrupt Mask Register
  bit_true(TIMSK1, TOIE1);
  
}

// Every millisecond
ISR(TIMER1_COMPA_vect)
{
  SysTick++;
}

// Callback functions are called in protocol.c
/*
void systick_register_callback(unsigned long ms_later, void (*f)(void *data), void *data)
{


}
*/
