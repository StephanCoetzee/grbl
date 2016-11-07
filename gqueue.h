#ifndef QUEUE_H
#define QUEUE_H

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>

struct generic_queue {
  volatile void *head;
  volatile void *tail;
  int32_t item_size;
  int32_t len;
  int32_t max_capacity;
  volatile uint8_t memory[0];
};

#define DECLARE_QUEUE(name, element_type, max_size)	\
  static struct {					\
    struct generic_queue gq;				\
    element_type __elements[max_size];			\
  } name;


static inline void queue_init(volatile void *q, int elt_size, int capacity)
{
  volatile struct generic_queue *gq = q;
  const size_t q_size = (sizeof(struct generic_queue) +
			 (elt_size * capacity));
  memset((void*)q, 0x00, q_size);
  gq->item_size = elt_size;
  gq->max_capacity = capacity;
}

static inline bool queue_is_empty(volatile void *q)
{
  volatile struct generic_queue *gq = q;

  return (gq->len == 0);
}
static inline int queue_get_len(volatile void *q)
{
  volatile struct generic_queue *gq = q;
  return gq->len;
}
static inline bool queue_is_full(volatile void *q)
{
  volatile struct generic_queue *gq = q;
  return (gq->len >= gq->max_capacity);
}
void queue_enqueue(volatile void *q, const void *elt) __attribute__((nonnull));
void queue_dequeue(volatile void *q, void *elt) __attribute__((nonnull));

#endif
