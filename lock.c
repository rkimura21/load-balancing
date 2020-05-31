#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sched.h>

#include "lock.h"

// returns x % y (y must be a positive power of 2)
int mod(int x, int y) { return (x & (y - 1)); }

tas_t * initLock()
{
  tas_t *lock = malloc(sizeof(tas_t));
  lock->flag = 0;
  return lock;
}

void acquireLock(tas_t *lock)
{
  while (__sync_lock_test_and_set(&lock->flag, 1) == 1) { sched_yield(); }
}

void releaseLock(tas_t *lock) { lock->flag = 0; }

void freeLock(tas_t *lock) { free(lock); }

anderson_t * initAnderson(int capacity)
{
  anderson_t *lock = malloc(sizeof(anderson_t));
  lock->size = capacity;
  lock->tail = 0;
  lock->flag = malloc(capacity * sizeof(PaddedPrimBool_t));
  lock->flag[0].value = true;
  return lock;
}

void acquireAnderson(anderson_t *lock, int *mySlot)
{
  int slot = mod(__sync_fetch_and_add(&lock->tail, 1), lock->size);
  *mySlot = slot;
  while (! lock->flag[slot].value) { sched_yield(); }
}

void releaseAnderson(anderson_t *lock, int mySlot)
{
  int slot = mySlot;
  lock->flag[slot].value = false;
  lock->flag[mod(slot + 1, lock->size)].value = true;
}

void freeAnderson(anderson_t *lock)
{
  free(lock->flag);
  free(lock);
}
