#ifndef QUEUE_H_
#define QUEUE_H_

#include "utils/packetsource.h"

typedef struct {
  volatile int depth;
  volatile Packet_t **buffer;
  volatile int head;
  volatile int tail;
} queue_t;

int mod(int x, int y);
queue_t * initQueue(int depth);
int enqueue(queue_t *queue, volatile Packet_t *packet);
volatile Packet_t * dequeue(queue_t *queue);
void freeQueue(queue_t *queue);

#endif
