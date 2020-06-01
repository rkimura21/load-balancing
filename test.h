#ifndef TEST_H_
#define TEST_H_

#include "queue.h"
#include "lock.h"
#include "utils/packetsource.h"

int floatcmp(const void *a, const void *b);
float getMedian(float arr[], int n);
float getVariance(long arr[], int n);
float getStdDev(long arr[], int n);

// unit tests
void parallelTest(unsigned int n, long W, unsigned int T, int seed);
void strategyTest(unsigned int n, long W, unsigned int T, int seed, char *S);
void timedParallelTest();
void timedStrategyTest();
void queueSequenceTest(long W, unsigned int D);
void countingTest(long B, int n, char *L);
void contiguityTest(long B, int n, char *L);
void orderingTest(long B, int n);

// experiments
void idleLockOverhead(unsigned int r);
void uniformSpeedup(unsigned int r);
void exponentialSpeedup(unsigned int r);
void awesomeSpeedup(unsigned int r);

#endif
