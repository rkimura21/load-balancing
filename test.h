#ifndef TEST_H_
#define TEST_H_

#include "lock.h"

typedef struct {
  int tid;
  char *lockType;
  FILE *fp;
  volatile long inc;
} metadata_t;

int floatcmp(const void *a, const void *b);
float getMedian(float arr[], int n);
float getVariance(long arr[], int n);
float getStdDev(long arr[], int n);
void packetCountTest(unsigned int r);
void queueSequenceTest(unsigned int r);
void countingTest(long B, int n, char *L, unsigned int r);
void contiguityTest(long B, int n, char *L, unsigned int r);
void orderingTest(long B, int n, char *L, unsigned int r);
void idleLockOverhead(unsigned int r);
void uniformSpeedup(unsigned int r);
void exponentialSpeedup(unsigned int r);
void awesomeSpeedup(unsigned int r);
void * counterRoutine(void *arg);

#endif
