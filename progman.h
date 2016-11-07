#ifndef PROGMAN_H
#define PROGMAN_H

#include <stdint.h>

void progman_init(void);
void progman_execute(void);
bool progman_read(uint8_t *dst);

#endif  /* PROGMAN_H */
