#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <signal.h>

#include "main.h"

volatile int doStop = 0;
static unsigned int M;
static long B;
static int verbose;
static int output;
static char *prog;
static char fileStr[128];
queue_t **queueArr;
locks_t **locksArr;
static locks_t *counterLock;
volatile int active;
volatile long count;
volatile long remaining;
char *tests[12] = { "parallelCheck", "strategyCheck", "timedParallel", "timedStrategy", "queue",
		    "counter", "contiguity", "ordering", "overhead", "speedupU", "speedupE",
		    "speedupA" };

int main(int argc, char *argv[])
{
  prog = argv[0];
  verbose = 0, output = 0;
  M = 2000, B = 0, count = 0;
  int i, c, err = 0;
  unsigned int n = 1, D = 8, T = 1;
  long W = 1;
  int seed = 1, uniform = 0, parallel = 0;
  char *L = NULL, *S = NULL;
  FILE *fp = NULL;
  int mFlag = 0, nFlag = 0, wFlag = 0, lFlag = 0, sFlag = 0, tFlag = 0;
  char usage[400] = "usage %s: -M <numMilliseconds> -n <numSources> -W <mean> -L <lockType> ";
  strcat(usage, "-S <strategy> -T <numPackets> [-D <queueDepth>] [-s <seed>] [-u] [-p] [-v]\n");
  char defaults[200] = "defaults: D = 8, seed = 1, uniform = false, parallel = false\n";
  char okString[200] = "lockType = { 'tas', 'anderson' } ";
  strcat(okString, "strategy = { 'lockFree', 'homeQueue', 'awesome' }\n");
    
  // retrieve command-line arguments
  while ((c = getopt(argc, argv, "M:n:W:us:D:L:S:T:B:pvo:")) != -1)
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
      lFlag = 1; L = optarg; break;
    case 'S':
      sFlag = 1; S = optarg; break;
    case 'T':
      tFlag = 1; sscanf(optarg, "%u", &T); break;
    case 'B':
      sscanf(optarg, "%ld", &B); break;
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
  } else if (parallel && (strcmp(S, "lockFree") != 0 &&
			  strcmp(S, "homeQueue") != 0 && strcmp(S, "awesome") != 0)) {
    fprintf(stderr, "%s: S (strategy) value must be 'lockFree', 'homeQueue', or 'awesome'\n", prog);
    exit(1);
  } else if (parallel && (strcmp(S, "homeQueue") == 0 || strcmp(S, "awesome") == 0) &&
	     (strcmp(L, "tas") != 0 && strcmp(L, "anderson") != 0)) {
    fprintf(stderr, "%s: L (lockType) value must be 'tas' or 'anderson'\n", prog);
    exit(1);
  } else if ((output >= 6 && output <= 8) && B < 1) {
    fprintf(stderr, "%s: -B (big) must be greater than 0\n", prog);
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
  if (output >= 6 && output <= 8) parallelCounter(n, L);
  else if (parallel) executeParallel(packetSource, T, n, M, D, L, S, get);
  else executeSerial(packetSource, T, n, M, get);

  // output final packet counts
  if (output == 1 || output == 2) {
    snprintf(fileStr, sizeof(fileStr), "out_%s.txt", tests[output-1]);
    fp = fopen(fileStr, "a");
    for (i = 0; i < n; i++) {
      if (uniform) sprintf(fileStr, "%ld\n", getUniformCount(packetSource, i));
      else sprintf(fileStr, "%ld\n", getExponentialCount(packetSource, i));
      fputs(fileStr, fp);
    }
  }

  if (fp != NULL) fclose(fp);
  deletePacketSource(packetSource);
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

void notifyStop(union sigval s) { doStop = 1; }

void parallelCounter(unsigned int n, char *L)
{
  int i, rc;
  active = n;
  FILE *fp = NULL;
  pthread_t *threads = malloc(n * sizeof(pthread_t));
  metadata_t *args = malloc(n * sizeof(metadata_t));
  counterLock = malloc(sizeof(locks_t));
  
  // set up appropriate lock
  if (strcmp(L, "tas") == 0) counterLock->tas = initTAS();
  else counterLock->anderson = initAnderson(power2Ceil(n));    

  if (output == 7) { // for ordering test
    snprintf(fileStr, sizeof(fileStr), "out_%s.txt", tests[output-1]);
    fp = fopen(fileStr, "a");
  }

  // spawn threads
  for (i = 0; i < n; i++) {
    args[i].tid = i;
    args[i].inc = 0;
    args[i].fp = fp;
    args[i].lockType = L;
    rc = pthread_create(&threads[i], NULL, counterRoutine, (void *)(args+i));
    if (rc) {
      fprintf(stderr, "%s: return code from pthread_create() is %d\n", prog, rc);
      exit(-1);
    } if ((rc = pthread_detach(threads[i]))) {
      fprintf(stderr, "%s: return code from pthread_detach() is %d\n", prog, rc);
      exit(-1);
    } if (verbose) printf("in parallelCounter: spawning thread %d\n", i);
  }

  // wait for all threads to terminate
  do {
    __sync_synchronize();
  } while (active);
  if (verbose) printf("in parallelCounter: all threads finished! preparing to free memory...\n");

  // free lock
  if (strcmp(L, "tas") == 0) freeTAS(counterLock->tas);
  else freeAnderson(counterLock->anderson);
  free(counterLock);

  // output relevant information for specified counter-based test
  if (output != -1) {
    snprintf(fileStr, sizeof(fileStr), "out_%s.txt", tests[output-1]);
    fp = fopen(fileStr, "a");
  } switch(output) {
  case 6:
    sprintf(fileStr, "%ld\n", count);
    fputs(fileStr, fp);
    break;
  case 8:
    for (i = 0; i < n; i++) {
      sprintf(fileStr, "%ld\n", args[i].inc);
      fputs(fileStr, fp);
    }
    break;
  }

  if (verbose) {;
    for (i= 0; i < n; i++)
      printf("%s: thread %d's increment count = %ld\n", prog, i, args[i].inc);
  }

  if (fp != NULL) fclose(fp);
  free(args);
  free(threads);
}

void executeSerial(PacketSource_t *packetSource, unsigned int T, unsigned int n,
		   unsigned int M, volatile Packet_t* (*get)(PacketSource_t *, int))
{
  int i, j;
  FILE *fp = NULL;
  
  // create and set-up timer
  timer_t timer;
  struct sigevent event;
  struct itimerspec tspec = { 0 };
  memset(&event, 0, sizeof(event)); // to get valgrind to stop complaining
  event.sigev_notify = SIGEV_THREAD;
  event.sigev_notify_function = &notifyStop;
  event.sigev_notify_attributes = NULL;
  timer_create(CLOCK_MONOTONIC, &event, &timer);
  tspec.it_value.tv_sec = M / 1000; 
  tspec.it_value.tv_nsec = M % 1000 * 1000000;
  tspec.it_interval.tv_sec = 0;
  tspec.it_interval.tv_nsec = 0;

  if (output == 3) { // for timed parallel test
    snprintf(fileStr, sizeof(fileStr), "out_%s_s.txt", tests[output-1]);
    fp = fopen(fileStr, "a");
  }
  
  if (! (output == 1 || output == 2))
    timer_settime(timer, 0, &tspec, NULL); // start timer

  long processed = 0;
  for (i = 0; i < T && !doStop; i++) {
    for (j = 0; j < n && !doStop; j++) {
      volatile Packet_t *packet = (*get)(packetSource, j);
      long checksum = getFingerprint(packet->iterations, packet->seed);
      processed++;
      if (verbose) printf("source %d's checksum[%d] = %ld\n", j, i, checksum);
      free((void *)packet);
    }
  }

  if (output >= 9 && output <= 12) {
    snprintf(fileStr, sizeof(fileStr), "out_%s_s.txt", tests[output-1]);
    fp = fopen(fileStr, "a");
    sprintf(fileStr, "%ld\n", processed);
    fputs(fileStr, fp);
  }

  if (fp != NULL) fclose(fp);
  if (timer_delete(timer)) fprintf(stderr, "%s: timer unsuccessfully deleted\n", prog);
  if (verbose)
    printf("%s: serial execution done, %ld of %u packets processed in %u msec\n",
	   prog, processed, n * T, M);
}

void executeParallel(PacketSource_t *packetSource, unsigned int T, unsigned int n,
		     unsigned int M, unsigned int D, char *L, char *S,
		     volatile Packet_t* (*get)(PacketSource_t *, int))
{
  int i, rc;
  FILE *fp = NULL;
  active = n;
  remaining = n * T;
  volatile int *packetProgress = malloc(sizeof(volatile int) * n);
  queueArr = malloc(sizeof(queue_t *) * n);
  locksArr = malloc(sizeof(locks_t *) * n);
  pthread_t *threads = malloc(sizeof(pthread_t) * n);
  metadata_t *args = malloc(sizeof(metadata_t) * n);

  // create and set-up timer
  timer_t timer;
  struct sigevent event;
  struct itimerspec tspec = { 0 };
  memset(&event, 0, sizeof(event)); // to get valgrind to stop complaining
  event.sigev_notify = SIGEV_THREAD;
  event.sigev_notify_function = &notifyStop;
  event.sigev_notify_attributes = NULL;
  timer_create(CLOCK_MONOTONIC, &event, &timer);
  tspec.it_value.tv_sec = M / 1000;
  tspec.it_value.tv_nsec = M % 1000 * 1000000;
  tspec.it_interval.tv_sec = 0;
  tspec.it_interval.tv_nsec = 0;  
    
  // populate global queue and lock arrays
  for (i = 0; i < n; i++)
    queueArr[i] = initQueue(D);
  for (i = 0; i < n; i++) {
    locksArr[i] = malloc(sizeof(locks_t));
    if (strcmp(L, "tas") == 0) locksArr[i]->tas = initTAS();
    else locksArr[i]->anderson = initAnderson(power2Ceil(n));
  }
  for (i = 0; i < n; i++)
    packetProgress[i] = 0;

  // specify strategy for picking a queue
  void* (*workerRoutine)(void*);
  if (strcmp(S, "lockFree") == 0) 
    workerRoutine = &routineLockFree;
  else if (strcmp(S, "homeQueue") == 0)
    workerRoutine = &routineHomeQueue;
  else
    workerRoutine = &routineAwesome;

  if (output == 3) { // for timed parallel test
    snprintf(fileStr, sizeof(fileStr), "out_%s_p.txt", tests[output-1]);
    fp = fopen(fileStr, "a");
  } else if (output == 4) { // for timed strategy test
    snprintf(fileStr, sizeof(fileStr), "out_%s_%s.txt", tests[output-1], S);
    fp = fopen(fileStr, "a");
  }
						
  if (! (output == 1 || output == 2))
    timer_settime(timer, 0, &tspec, NULL); // start timer  

  // spawn threads
  for (i = 0; i < n; i++) {
    args[i].tid = i;
    args[i].numPackets = T;
    args[i].processed = 0;
    args[i].lockType = L;
    args[i].fp = fp;
    args[i].numSources = n;
    args[i].packetProgress = packetProgress;
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
  for (i = 0; i < n * T; i++) {
    volatile Packet_t *nextPacket = (*get)(packetSource, i % n);
    while (! enqueue(queueArr[i % n], nextPacket)) { /* spin */ }
    if (verbose)
      printf("in executeParallel: just enqueue'd packet %d for worker %d\n", i / n, i % n);
  }
  
  if (verbose) printf("in executeParallel: enqueue-ing done, waiting for workers to finish...\n");

  // wait for all threads to terminate
  do {
    __sync_synchronize();
  } while (active);
  if (verbose) printf("in executeParallel: all threads finished, preparing to free memory...\n");

  // harvest worker data for total throughput
  long processed = 0;
  for (i = 0; i < n; i++) processed += args[i].processed;
  if (output >= 9 && output <= 12) {
    snprintf(fileStr, sizeof(fileStr), "out_%s_p.txt", tests[output-1]);
    fp = fopen(fileStr, "a");
    sprintf(fileStr, "%ld\n", processed);
    fputs(fileStr, fp);
  }
  
  // free queues and locks
  for (i = 0; i < n; i++)
    freeQueue(queueArr[i]);
  for (i = 0; i < n; i++) {
    if (strcmp(L, "tas") == 0) freeTAS(locksArr[i]->tas);
    if (strcmp(L, "anderson") == 0) freeAnderson(locksArr[i]->anderson);
    free(locksArr[i]);
  }
  free(queueArr);
  free(locksArr);
  free((void *)packetProgress);

  if (fp != NULL) fclose(fp);
  free(args);
  free(threads);
  if (timer_delete(timer)) fprintf(stderr, "%s: timer unsuccessfully deleted\n", prog);
  if (1)
    printf("%s: parallel execution (L = %s, S = %s) done, %ld of %u packets processed in %u msec\n",
	   prog, L, S, processed, n * T, M);
}

void * routineLockFree(void *arg)
{
  int i;
  metadata_t *m = (metadata_t *)arg;
  queue_t *queue = queueArr[m->tid];
  volatile Packet_t *nextPacket;
  if (verbose) printf("in lockFree routine: hello, this is worker %d\n", m->tid);
  
  // dequeue and process packets
  for (i = 0; i < m->numPackets && !doStop; i++) {
    if (doStop) { // free any remaining packets
      while ((nextPacket = dequeue(queue))) free((void *)nextPacket);
      break;
    }
    while (! (nextPacket = dequeue(queue))) { sched_yield(); }
    long checksum = getFingerprint(nextPacket->iterations, nextPacket->seed);
    m->processed++;
    if (output == 3 || output == 4) {
      sprintf(fileStr, "%d:%ld\n", m->tid, checksum);
      fputs(fileStr, m->fp);
    }
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

  // dequeue and process packets
  for (i = 0; i < m->numPackets && !doStop; i++) {
    // acquire lock
    if (strcmp(lockType, "tas") == 0) acquireTAS(locksArr[m->tid]->tas);
    if (strcmp(lockType, "anderson") == 0) acquireAnderson(locksArr[m->tid]->anderson, &mySlot);

    // critcal section: dequeue
    while (! (nextPacket = dequeue(queue))) { sched_yield(); }

    // release lock
    if (strcmp(lockType, "tas") == 0) releaseTAS(locksArr[m->tid]->tas);
    if (strcmp(lockType, "anderson") == 0) releaseAnderson(locksArr[m->tid]->anderson, mySlot);

    long checksum = getFingerprint(nextPacket->iterations, nextPacket->seed);
    m->processed++;

    if (output == 3 || output == 4) {
      sprintf(fileStr, "%d:%ld\n", m->tid, checksum);
      fputs(fileStr, m->fp);
    }
    
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
  metadata_t *m = (metadata_t *)arg;
  int mru = (m->tid + 1) % m->numSources; // used for targeting victim queues
  int mySlots[m->numSources]; // thread local; keep track of position in all anderson queues
  queue_t *myQueue = queueArr[m->tid];
  char *lockType = m->lockType;
  volatile Packet_t *nextPacket;
  printf("in awesome routine: hello, this is worker %d\n", m->tid);

  while (remaining) {
    while (!isEmpty(myQueue)) {
      printf("remaining = %ld\n", remaining);
      // acquire my lock
      if (strcmp(lockType, "tas") == 0) acquireTAS(locksArr[m->tid]->tas);
      if (strcmp(lockType, "anderson") == 0) acquireAnderson(locksArr[m->tid]->anderson, &mySlots[m->tid]);

      // my critcal section: dequeue
      while (! (nextPacket = dequeue(myQueue))) { sched_yield(); }
      __sync_fetch_and_sub(&remaining, 1);
      m->processed++;
      m->packetProgress[m->tid]++;
      
      // release my lock
      if (strcmp(lockType, "tas") == 0) releaseTAS(locksArr[m->tid]->tas);
      if (strcmp(lockType, "anderson") == 0) releaseAnderson(locksArr[m->tid]->anderson, mySlots[m->tid]);

      long checksum = getFingerprint(nextPacket->iterations, nextPacket->seed);
      
      if (output == 3 || output == 4) {
	sprintf(fileStr, "%d:%ld\n", m->tid, checksum);
	fputs(fileStr, m->fp);
      }

      printf(" in awesome routine: worker %d computed own checksum[%d] = %ld\n",
			  m->tid, m->packetProgress[m->tid]-1, checksum);
      free((void *)nextPacket);    
    }
    
    while (isEmpty(myQueue) && remaining) { // attempt to steal from another queue
      printf("remaining = %ld\n", remaining);
      printf("in awesome routine: worker %d attempting to steal from worker %d\n",
	     m->tid, mru);
      sched_yield();
      if (!isEmpty(queueArr[mru])) {
	// acquire their lock
	if (strcmp(lockType, "tas") == 0) acquireTAS(locksArr[mru]->tas);
	if (strcmp(lockType, "anderson") == 0) acquireAnderson(locksArr[mru]->anderson, &mySlots[mru]);

	// their critcal section: dequeue
	while (! (nextPacket = dequeue(queueArr[mru]))) { sched_yield(); }
	__sync_fetch_and_sub(&remaining, 1);
	m->processed++;
	m->packetProgress[mru]++;

	// release their lock
	if (strcmp(lockType, "tas") == 0) releaseTAS(locksArr[mru]->tas);
	if (strcmp(lockType, "anderson") == 0) releaseAnderson(locksArr[mru]->anderson, mySlots[mru]);

	long checksum = getFingerprint(nextPacket->iterations, nextPacket->seed);

	if (output == 3 || output == 4) {
	  sprintf(fileStr, "%d:%ld\n", mru, checksum);
	  fputs(fileStr, m->fp);
	}

	printf(" in awesome routine: worker %d computed worker %d's checksum[%d] = %ld\n",
			    m->tid, mru, m->packetProgress[mru]-1, checksum);
	free((void *)nextPacket);
      }
      else { do { mru = rand() % m->numSources; } while (mru == m->tid); } // assign new target queue
    }
  } 
  
  if (verbose) printf("in awesome routine: worker %d exiting...\n", m->tid);
  __sync_fetch_and_sub(&active, 1);
  return NULL;
}

void * counterRoutine(void *arg)
{
  metadata_t *m = (metadata_t *)arg;
  if (verbose) printf("in counterRoutine: hello, this is thread %u\n", m->tid);

  if (strcmp(m->lockType, "tas") == 0) {
    while (count < B) {
      acquireTAS(counterLock->tas);
      if (count < B) {
	count++; m->inc++;
	if (output == 7) {
	  sprintf(fileStr, "%ld\n", count);
	  fputs(fileStr, m->fp);
	}
      }
      releaseTAS(counterLock->tas);
    }
  } else {
      int mySlot; // thread local
      while (count < B) {
	acquireAnderson(counterLock->anderson, &mySlot);
	if (count < B) {
	  count++; m->inc++;
	  if (output == 7) {
	    sprintf(fileStr, "%ld\n", count);
	    fputs(fileStr, m->fp);
	  }
	}
	releaseAnderson(counterLock->anderson, mySlot);
      }
    }
  
  if (verbose) printf("in counterRoutine: worker %d exiting...\n", m->tid);
  __sync_fetch_and_sub(&active, 1);
  return NULL;
}
