#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>

#include "main.h"

StopWatch_t *timer;
static unsigned int M;
static int verbose;
static int output;
static char *prog;
static char fileStr[128];
queue_t *queueArr;
locks_t *locksArr;
volatile int active;

int main(int argc, char *argv[])
{
  prog = argv[0];
  verbose = 0, output = 0, M = 2000;
  int c, err = 0;
  unsigned int n = 1, T = 1, D = 8;
  long W = 1;
  int seed = 1, uniform = 0, parallel = 0;
  char *L = NULL, *S = NULL;
  int mFlag = 0, nFlag = 0, wFlag = 0, lFlag = 0, sFlag = 0, tFlag = 0;
  char usage[400] = "usage %s: -M <numMilliseconds> -n <numSources> -W <mean> -L <lockType> ";
  strcat(usage, "-S <strategy> -T <numPackets> [-D <queueDepth>] [-s <seed>] [-u] [-p] [-v]\n");
  char defaults[200] = "defaults: D = 8, seed = 1, uniform = false, parallel = false\n";
  char okString[200] = "lockType = { 'tas', 'anderson' } ";
  stract(okString, "strategy = { 'lockFree', 'homeQueue', 'awesome' }\n");
  timer = malloc(sizeof(StopWatch_t));
  
  // retrieve command-line arguments
  while ((c = getopt(argc, argv "M:n:W:us:D:L:S:T:pvo:")) != -1)
    switch(c) {
    case 'M':
      mFlag = 1; sscanf(optarg, "%u", &M); break;
    case 'n':
      nFlag = 1; sscanf(optarg, "%u", &n); break;
    case 'W':
      wFlag = 1; sscanf(optarg, "%ld", &W); break;
    case 'u':
      uniform = 1; break;
    case 's':
      sscanf(optarg, "%d", &seed); break;
    case 'D':
      sscanf(optarg, "%u", &D); break;
    case 'L':
      lFlag = 1; sscanf(optarg, "%s", &L); break;
    case 'S':
      sFlag = 1; sscanf(optarg, "%s", &S); break;
    case 'T':
      tFlag = 1; sscanf(optarg, "%ld", &T); break;
    case 'p':
      parallel = 1; break;
    case 'v':
      verbose = 1; break;
    case 'o':
      sscanf(optarg, "%d", &output); break;
    case '?':
	err = 1; break;
      }

  // handle malformed or incorrect command-line arguments
  if (nFlag == 0 || mFlag == 0 || wFlag == 0 || tFlag == 0) {
    fprintf(stderr, "%s: missing -n, -M, -W, and/or -T option(s)\n", prog);
    fprintf(stderr, usage, prog);
    fprintf(stderr, defaults, prog);
    exit(1);
  } else if (parallel && (lFlag == 0 || sFlag == 0)) {
    fprintf(stderr, "%s: missing -L and/or -S option(s) needed for parallel version\n", prog);
    fprintf(stderr, usage, prog);
    fprintf(stderr, defaults, prog);
    exit(1);
  } else if (n < 1 || n > MAX_THREAD) {
    fprintf(stderr, "%s: n (numSources) must be greater than 0 and less than/equal to 64\n", prog);
    exit(1);
  } else if (W < 1 || T < 1) {
    fprintf(stderr, "%s: W (mean) and T (numPackets) values must be greater than 0\n", prog);
    exit(1);
  } else if (M < 2000) {
    fprintf(stderr, "%s: M (numMilliseconds) value must be greater than 2000\n", prog);
    exit(1);
  } else if (parallel && (D < 1 || D % 2)) {
    fprintf(stderr, "%s: D (queueDepth) must be a positive power of 2\n", prog);
    exit(1);
  } else if (parallel && (L == NULL || S == NULL)) {
    fprintf(stderr, "%s: -L (lockType) and/or -S (strategy) values are missing\n", prog);
    fprintf(stderr, okString, prog);
    exit(1);
  } else if (parallel && (strcmp(L, "tas") != 0 && strcmp(L, "anderson") != 0)) {
    fprintf(stderr, "%s: L (lockType) value must be 'tas' or 'anderson'\n", prog);
    exit(1);
  } else if (parallel && (strcmp(S, "lockFree") != 0 &&
			  strcmp(L, "homeQueue") != 0 && strcmp(s, "awesome") != 0)) {
    fprintf(stderr, "%s: S (strategy) value must be 'lockFree', 'homeQueue', or 'awesome'\n", prog);
    exit(1);
  } else if (err) {
    fprintf(stderr, usage, prog);
    exit(1);
  }

  // specify packet distribution method
  PacketSource_t *packetSource = createPacketSource(W, n, seed);
  volatile Packet_t* (*get)(PacketSource_t *, int);
  if (uniform) get = &getUniformPacket;
  else get = &getExponentialPacket;

  // route to parallel/serial code
  if (parallel) executeParallel(packetSource, T, n, D, L, S, get);
  else executeSerial(packetSource, T, n, get);

  // output final packet counts

  deletePacketSource(packetSource);
  free(timer);
  return 0;
}

unsigned int power2Ceil(unsigned int n)
{
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n++;
  return n;
}

void executeSerial(PacketSource_t *packetSource, unsigned int T, unsigned int n,
		   volatile Packet_t* (*get)(PacketSource_t *, int))
{
  int i, j;

  startTimer(timer);
  for (i = 0; i < T; i++) {
    for (j = 0; j < n; j++) {
      volatile Packet_t *packet = (*get)(packetSource, j);
      long checksum = getFingerprint(packet->iterations, packet->seed);
      if (verbose) printf("source %d's checksum[%d] = %ld\n", j, i, checksum);
      free((void *)packet);
    }
  }
  stopTimer(timer);

  if (verbose) printf("serial execution complete (time = %f msec)\n", getElapsedTime(timer));
  char *tests[7] = { "counter", "contiguity", "ordering", "overhead", "uniformSpeedup",
		     "exponentialSpeedup", "awesomeSpeedup" };

  if (output >= 4 || output <= 7) {
    FILE *fp = NULL;
    snprintf(fileStr, sizeof(fileStr), "out_%s.txt", tests[output-1]);
    fp = fopen(fileStr, "a");
    sprintf(fuleStr, "%f\n", getElapsedTime(timer));
    fputs(fileStr, fp);
    fclose(fp);
  }
}

void executeParallel(PacketSource_t *packetSource, unsigned int T, unsigned int n,
		     unsigned int D, char *L, char *S,
		     volatile Packet_t* (*get)(PacketSource_t *, int))
{
  int i, j, rc;
  FILE *fp = NULL;
  active = n;
  queueArr = malloc(sizeof(queue_t) * n);
  locksArr = malloc(sizeof(locks_t) * n);
  pthread_t *threads = malloc(sizeof(pthread_t) * n);
  metadata_t *args = malloc(sizeof(metadata_t) * n);
  char *tests[7] = { "counter", "contiguity", "ordering", "overhead", "uniformSpeedup",
		     "exponentialSpeedup", "awesomeSpeedup" };

  // populate global queue and lock arrays
  for (i = 0; i < n; i++)
    queueArr[i] = initQueue(D);
  for (i = 0; i < n; i++) {
    if (strcmp(L, "tas") == 0)
      locksArr[i]->tas = initTAS(); // need to allocate space for union itself too?
    else
      locksArr[i]->anderson = initAnderson(power2Ceil(n));
  }

  // specify strategy for picking a queue
  void* (*workerRoutine)(void*);
  if (strcmp(S, "lockFree") == 0) 
    workerRoutine = &routineLockFree;
  else if (strcmp(S, "homeQueue") == 0)
    workerRoutine = &routineHomeQueue;
  else
    workerRoutine = &routineAwesome;
						
  // spawn threads
  startTimer(timer);
  for (i = 0; i < n; i++) {
    args[i].tid = i;
    args[i].numPackets = T;
    args[i].lockType = L;
    args[i].fp = fp;
    rc = pthread_create(&threads[i], NULL, workerRoutine, (void *)(args+i));
    if (rc) {
      fprintf(stderr, "error: return code from pthread_create() is %d\n", rc);
      exit(1);
    } if (pthread_detach(threads[i])) {
      fprintf(stderr, "error: from pthread_detach()\n");
      exit(1);
    } if (verbose) printf("in executeParallel: spawning thread %d\n", i);
  }

  // enqueue all threads
  for (i = 0; i < T; i++) {
    for (j = 0; j < n; j++) {
      volatile Packet_t *nextPacket = (*get)(packetSource, j);
      while (! enqueue(queueArr[j], nextPacket)) { /* spin */ }
      if (verbose) printf("in executeParallel: just enqueue'd packet %d for worker %d\n", i, j);
    }
  }
  if (verbose) printf("in executeParallel: enqueue-ing done, waiting for workers to finish...\n");

  // wait for all threads to terminate
  do {
    __sync_synchronize();
  } while (active);
  stopTimer(timer);
  if (verbose) printf("in executeParallel: all threads finished, preparing to free memory...\n");

  // output relevant information for specified test

  // free stuff
  for (i = 0; i < n; i++)
    freeQueue(queueArr[i]);
  for (i = 0; i < n; i++) {
    if (strcmp(L, "tas") == 0) freeTAS(locksArr[i]->tas);
    if (strcmp(L, "anderson") == 0) freeAnderson(locksArr[i]->anderson);
  }
  
  free(queueArr);
  free(locksArr);
  if (fp != NULL) fclose(fp);
  free(args);
  free(threads);
}

void * routineLockFree(void *arg)
{
  int i;
  metadata_t *m = (metadata_t *)arg;
  queue_t *queue = queueArr[m->tid];
  volatile Packet_t *nextPacket;
  if (verbose) printf("in lockFree routine: hello, this is worker %d\n", m->tid);

  // dequeue and process all packets
  for (i = 0; i < m->numPackets; i++) {
    while (! (nextPacket = dequeue(queue))) { sched_yield(); }
    long checksum = getFingerprint(nextPacket->iterations, nextPacket->seed);
    if (verbose) printf("in lockFree routine: worker %d's checksum[%d] = %ld\n",
			m->tid, i, checksum);
    free((void *)nextPacket);
  }

  if (verbose) printf("in lockFree routine: worker %d exiting...\n", m->tid);
  __sync_fetch_and_sub(&active, 1);
  return NULL;
}

void * routineHomeQueue(void *arg)
{
  int i, mySlot; // thread local
  metadata_t *m = (metadata_t *)arg;
  queue_t *queue = queueArr[m->tid];
  char *lockType = m->lockType;
  volatile Packet_t *nextPacket;
  if (verbose) printf("in homeQueue routine: hello, this is worker %d\n", m->tid);

  // dequeue and process all packets
  for (i = 0; i < m->numPackets; i++) {
    // acquire lock
    if (strcmp(lockType, "tas") == 0) acquireTAS(locksArr[m->tid]->tas);
    if (strcmp(lockType, "anderson") == 0) acquireAnderson(locksArr[m->tid]->anderson, &mySlot);

    // critcal section: dequeue
    while (! (nextPacket = dequeue(queue))) { sched_yield(); }

    // release lock
    if (strcmp(lockType, "tas") == 0) acquireTAS(locksArr[m->tid]->tas);
    if (strcmp(lockType, "anderson") == 0) releaseAnderson(locksArr[m->tid]->anderson, mySlot);

    long checksum = getFingerprint(nextPacket->iterations, nextPacket->seed);
    if (verbose) printf(" in homeQueue routine: worker %d's checksum[%d] = %ld\n",
			m->tid, i, checksum);
    free((void *)nextPacket);
  }
  
  if (verbose) printf("in homeQueue routine: worker %d exiting...\n", m->tid);
  __sync_fetch_and_sub(&active, 1);
  return NULL;
}

void * routineAwesome(void *arg)
{
  if (verbose) printf("in awesome: worker %d exiting...\n", m->tid);
  __sync_fetch_and_sub(&active, 1);
  return NULL;
}
