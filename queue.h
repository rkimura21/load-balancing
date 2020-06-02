#ifndef QUEUE_H_
#define QUEUE_H_

#include "utils/packetsource.h"

typedef struct {
  volatile Packet_t **buffer;
  volatile unsigned int depth;
  volatile int head;
  volatile int tail;
} queue_t;

int modQ(int x, int y);
queue_t * initQueue(unsigned int depth);
int enqueue(queue_t *queue, volatile Packet_t *packet);
volatile Packet_t * dequeue(queue_t *queue);
void freeQueue(queue_t *queue);
int isEmpty(queue_t *queue);

#endif
