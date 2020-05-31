#ifndef TEST_H_
#define TEST_H_

#include "queue.h"
#include "lock.h"
#include "utils/packetsource.h"

int floatcmp(const void *a, const void *b);
float getMedian(float arr[], int n);
float getVariance(long arr[], int n);
float getStdDev(long arr[], int n);
void parallelTest();
void strategyTest();
void timedParallelTest();
void timedStrategyTest();
void queueSequenceTest(long W, unsigned int D);
void countingTest(long B, int n, char *L);
void contiguityTest(long B, int n, char *L);
void orderingTest(long B, int n);
void idleLockOverhead(unsigned int r);
void uniformSpeedup(unsigned int r);
void exponentialSpeedup(unsigned int r);
void awesomeSpeedup(unsigned int r);

#endif
