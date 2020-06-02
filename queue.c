#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

// returns x % y (y must be a positive power of 2)
int modQ(int x, int y) { return (x & (y - 1)); }

queue_t * initQueue(unsigned int depth)
{
  queue_t * queue = malloc(sizeof(queue_t));
  queue->buffer = malloc(depth * sizeof(volatile Packet_t *));
  queue->head = 0;
  queue->tail = 0;
  queue->depth = depth;
  return queue;
}


int enqueue(queue_t *queue, volatile Packet_t *packet)
{
  // i.e. throw full exception
  if (queue->tail - queue->head == queue->depth)
    return 0;

  int i = modQ(queue->tail, queue->depth);
  queue->buffer[i] = packet;
  __sync_synchronize();
  queue->tail++;
  return 1;
}

volatile Packet_t * dequeue(queue_t *queue)
{
  // i.e. throw empty exception
  if (queue->tail - queue->head == 0)
    return NULL;

  int i = modQ(queue->head, queue->depth);
  volatile Packet_t *packet = queue->buffer[i];
  __sync_synchronize();
  queue->head++;
  return packet;
}

void freeQueue(queue_t *queue)
{
  free(queue->buffer);
  free(queue);
}
