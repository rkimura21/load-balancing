#ifndef TEST_H_
#define TEST_H_

#include "queue.h"
#include "lock.h"
#include "utils/packetsource.h"

#define MAX_THREAD 64

int longcmp(const void *a, const void *b);
int floatcmp(const void *a, const void *b);
long getMedianL(long arr[], int n);
float getMedianF(float arr[], int n);
float getVariance(long arr[], int n);
float getStdDev(long arr[], int n);

// unit tests
void parallelTest(unsigned int n, long W, unsigned int T, int seed);
void strategyTest(unsigned int n, long W, unsigned int T, int seed, char *S);
void timedParallelTest(unsigned int n, long W, unsigned int T, int seed, unsigned int M);
void timedStrategyTest(unsigned int n, long W, unsigned int T, int seed, char *S, unsigned int M);
void queueSequenceTest(long W, unsigned int D);
void countingTest(long B, int n, char *L);
void contiguityTest(long B, int n, char *L);
void orderingTest(long B, int n);

// experiments
void idleLockOverhead(unsigned int r);
void uniexpSpeedup(unsigned int r, int uniform);
void awesomeSpeedup(unsigned int r);

#endif
