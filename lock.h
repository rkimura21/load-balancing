#ifndef LOCK_H_
#define LOCK_H_

#include "utils/paddedprim.h"

typedef struct {
  volatile int flag;
} tas_t;

typedef struct {
  int size;
  volatile int tail;
  PaddedPrimBool_t *flag;
} anderson_t;

typedef union {
  tas_t *tas;
  anderson_t *anderson;
} locks_t;

tas_t * initTAS();
void acquireTAS(tas_t *lock);
void releaseTAS(tas_t *lock);
void freeTAS(tas_t *lock);

anderson_t * initAnderson(int capacity);
void acquireAnderson(anderson_t *lock, int *mySlot);
void releaseAnderson(anderson_t *lock, int mySlot);
void freeAnderson(anderson_t *lock);
int modL(int x, int y);

#endif

