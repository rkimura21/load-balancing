#ifndef MAIN_H_
#define MAIN_H_

#include "queue.h"
#include "lock.h"
#include "utils/packetsource.h"
#include "utils/fingerprint.h"

#define MAX_THREAD 64

typedef struct {
  int tid;
  int numPackets;
  int processed;
  char *lockType;
  FILE *fp;
  long inc; // for counter-based tests
} metadata_t;

unsigned int power2Ceil(unsigned int n);
void parallelCounter(unsigned int n, char *L);
void executeSerial(PacketSource_t *packetSource, unsigned int T, unsigned int n,
		   volatile Packet_t* (*get)(PacketSource_t *, int));
void executeParallel(PacketSource_t *packetSource, unsigned int T, unsigned int n,
 		     unsigned int D, char *L, char *S,
		     volatile Packet_t* (*get)(PacketSource_t *, int));
void * routineLockFree(void *arg);
void * routineHomeQueue(void *arg);
void * routineAwesome(void *arg);
void * counterRoutine(void *arg);

#endif
