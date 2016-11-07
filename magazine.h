/*
  Not part of Grbl. Written by KeyMe.
*/

#ifndef MAGAZINE_H
#define MAGAZINE_H

// Magazine allignment pin initialization routine
void magazine_init();

#define magazine_get_state() (!(MAGAZINE_ALIGNMENT_PIN & MAGAZINE_ALIGNMENT_MASK))

// Monitors the gap in units between mags and throws an alarm if the gap is larger than a
// specified threshold.
void magazine_gap_monitor();

int32_t magazine_lash(void);
int32_t magazine_spacing(void);

#endif
