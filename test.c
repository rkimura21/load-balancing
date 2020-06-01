#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "test.h"

static int verbose;
static char *prog;

int main(int argc, char *argv[])
{
  prog = argv[0];
  int c, err = 0, t = -1;
  return 0;
}

int floatcmp(const void *a, const void *b)
{
  float fa = *(float *)a;
  float fb = *(float *)b;
  return (fa > fb)  - (fa < fb);
}

float getMedian(float arr[], int n)
{
  float median;
  if (n % 2) median = arr[n/2];
  else median = (arr[(n-1)/2] + arr[n/2]) / 2.0;
  return median;
}

float getVariance(long arr[], int n)
{
  int i; long sum = 0;
  for (i = 0; i < n; i++) sum += arr[i];
  float mean = (float) sum / (float) n;

  float sqDiff = 0;
  for (i = 0; i < n; i++) sqDiff += (arr[i] - mean) * (arr[i] - mean);
  return sqDiff / n;
}

float getStdDev(long arr[], int n) { return sqrt(getVariance(arr, n)); }

void parallelTest(unsigned int n, long W, unsigned int T, int seed)
{
  int i, j, matching = 0;
  long pCount;
  FILE *fp;
  pid_t childPid;
  char fileStr[128];
  
  if (access("main", F_OK|X_OK)) {
    fprintf(stderr, "%s: 'main' file does not exist in current dir, or is not executable\n", prog);
    exit(1);
  } else {
    char nStr[10], char wStr[10]; char tStr[10]; char sStr[10];
    sprintf(nStr, "%u" n); sprintf(wStr, "%ld", W); sprintf(tStr, "%u", T); sprintf(sStr,"%d", seed);
    for (i = 0; i < 2; i++) { // uniform or exponential
      for (j = 0; j < 2; j++) { // serial or parallel
	childPid = fork();
	if (childPid == 0) {
	  execlp("./main", "main", "-n", nStr, "-W", wStr, "-T", tStr, "-s", sStr,
		 "-M", "2000", i == 0 ? "-u" : "", "-S", "lockFree", "-o", "1", j == 1 ? "-p" : "",
		 verbose ? "-v" : "", (char *) NULL);
	} else if (childPid < 0) {
	  fprintf(stderr, "%s: fork failed!\n", prog); exit(1);
	} else {
	  int rc;
	  waitpid(childPid, &rc, 0);
	  if (rc == 1) {
	    fprintf(stderr, "%s: child process terminated with an error!\n", prog); exit(1);
	  }
	}
      }
    }
  }

  // compare packet count per source (for every version x distribution combo)
  i = 0; long pCounts[2][2][n]; // arr[distr][version][n]
  if ((fp = fopen("out_parallelCheck.txt", "r")) == NULL) {
    fprintf(stderr, "%s: can't open out_parallelCheck.txt\n", prog);
    exit(1);
  } while (!feof(fp)) {
    if (fgets(fileStr, sizeof(fileStr), fp) != NULL) {
      sscanf(fileStr, "%ld", &pCount);
      pCounts[i / (2 * n)][i / n % 2][i % n] = pCount;
      i++;
    }
  }
  fclose(fp);
 
  long uniCounts[2][n] = pCounts[0];
  long expCounts[2][n] = pCounts[1];
  for (i = 0; i < n; i++) {
    if (uniCounts[0][i] != uniCounts[1][i]) break;
    if (expCounts[0][i] != expCounts[1][i]) break;
    matching++;
  }

  if (remove("out_parallelCheck.txt"))
    fprintf(stderr, "%s: unable to delete out_parallelCheck.txt\n", prog);
  printf("%s: parallelCheckTest(n = %u, T = %u, W = %ld, seed = %d) has %s!\n",
	 prog, n, T, W, seed, matching == n ? "PASSED" : "FAILED");
}

void strategyTest(unsigned int n, long W, unsigned int T, int seed, char *S)
{
  int i, j, k, matching = 0;
  long pCount;
  FILE *fp;
  pid_t childPid;
  char fileStr[128];
  
  if (strcmp(S, "lockFree") == 0) {
    fprintf(stderr, "%s: choose strategy other than 'lockFree' to avoid redundancy\n", prog);
    exit(1);
  }

  if (access("main", F_OK|X_OK)) {
    fprintf(stderr, "%s: 'main' file does not exist in current dir, or is not executable\n", prog);
    exit(1);
  } else {
    char nStr[10], char wStr[10]; char tStr[10]; char sStr[10];
    sprintf(nStr, "%u" n); sprintf(wStr, "%ld", W); sprintf(tStr, "%u", T); sprintf(sStr,"%d", seed);
    for (i = 0; i < 2; i++) { // strategy (lockFree or S)
      for (j = 0; j < 2; j++) { // distribution (uniform or exponential)
	for (k = 0; k < 2; k++) { // lock type (tas or anderson)
	  childPid = fork();
	  if (childPid == 0) {
	    execlp("./main", "main", "-n", nStr, "-W", wStr, "-T", tStr, "-s", sStr, "-M", "2000",
		   "-S", i == 1 ? S : "lockFree", i == 0 ? "-u" : "", "-L",
		   k == 0 ? "tas" : "anderson", "-p", verbose ? "-v" : "", "-o", "2", (char *) NULL);
	  } else if (childPid < 0) {
	    fprintf(stderr, "%s: fork failed!\n", prog); exit(1);
	  } else {
	    int rc;
	    waitpid(childPid, &rc, 0);
	    if (rc == 1) {
	      fprintf(stderr, "%s: child process has terminated with an error!\n", prog); exit(1);
	    }
	  }
	}
      }
    }
  }
    
  // compare packet count per source (for every S x L x distribution combo)
  i = 0; long pCounts[2][2][2][n]; // arr[S][distr][L][n]
  if ((fp = fopen("out_strategyCheck.txt", "r")) == NULL) {
    fprintf(stderr, "%s: can't open out_parallelCheck.txt\n", prog);
    exit(1);
  } while (!feof(fp)) {
    if (fgets(fileStr, sizeof(fileStr), fp) != NULL) {
      sscanf(fileStr, "%ld", &pCount);
      pCounts[i / (4 * n)][i / (2 * n) % 2][i / n % 2][i % n] = pCount;
      i++;
    }
  }
  fclose(fp);

  int broke = 0;
  long lFreeCounts[2][2][n] = pCounts[0];
  long otherCounts[2][2][n] = pCounts[1]; // for input S
  for (i = 0; i < 2; i++) { // distribution (uniform or exponential)
    long lFreeDistrCounts[2][n] = lFreeCounts[i];
    long otherDistrCounts[2][n] = otherCounts[i];
    for (j = 0; j < n; j++) {
      if (lFreeDistrCounts[0][j] != otherDistrCounts[0][j]) { break; broke = 1; }
      if (lFreeDistrCounts[1][j] != otherDistrCounts[1][j]) { break; broke = 1; }
      matching++;
    }
    if (broke) break;
  }  

  if (remove("out_strategyCheck.txt"))
    fprintf(stderr, "%s: unable to delete out_strategyCheck.txt\n", prog);
  printf("%s: strategyCheckTest(n = %u, T = %u, W = %ld, seed = %d, S = %s) has %s!\n",
	 prog, n, T, W, seed, S, matching == n * 2 ? "PASSED" : "FAILED");
}

void timedParallelTest()
{
}

void timedStrategyTest()
{
}

void queueSequenceTest(long W, unsigned int D)
{
  int i, matching = 0;
  Queue_t *queue = initQueue(D);
  volatile Packet_t *enqPackets[D];
  volatile Packet_t *deqPackets[D];

  // generate and enqueue all packets first
  for (i = 0; i < D; i++) {
    volatile Packet_t *packet = malloc(sizeof(volatile Packet_t));
    packet->iterations = i * (W / D); packet->seed = 1;
    enqPackets[i] = packet;
    enqueue(queue, packet);
  }

  // then dequeue all packets
  for (i = 0; i < D; i++)
    deqPackets[i] = dequeue(queue);

  // compare enqueue and dequeue array to test preservation of FIFO
  for (i = 0; i < D; i++) {
    if (verbose) {
      printf("in queueSequenceTest: enqueue packet %d: workload = %ld, addr = %p\n",
	     i, enqPackets[i]->iterations, enqPackets[i]);
      printf("in queueSequenceTest: dequeue packet %d: workload = %ld, addr = %p\n",
	     i, deqPackets[i]->iterations, deqPackets[i]);
    }

    if (! ((enqPackets[i]->iterations == deqPackets[i]->iterations) &&
	   (enqPackets[i] == deqPackets[i]))) break;
    else matching++;
  }
  
  for (i = 0; i < D; i++)
    free((void *)enqPackets[i]);
  freeQueue(queue);

  printf("%s: queueSequenceTest(W = %ld, D = %u) has %s!\n", prog, W, D,
	 matching == D ? "PASSED" : "FAILED");
}

void countingTest(long B, int n, char *L)
{
  long count;
  FILE *fp;
  pid_t childPid;
  char fileStr[128];

  if (access("main", F_OK|X_OK)) {
    fprintf(stderr, "%s: 'main' file does not exist in current dir, or is not executable\n", prog);
    exit(1);
  } else {
    char bStr[10]; char nStr[10];
    sprintf(bStr, "%ld", B); sprintf(nStr, "%d", n);
    childPid = fork();
    if (childPid == 0) {
      execlp("./main", "main", "-B", bStr, "-n", nStr, "-L", L, "-M", "2000", "-W", "1", "-T", "1",
	     "-S", "lockFree", "-o", "6", "-p", verbose ? "-v" : "", (char *) NULL);
    } else if (childPid < 0) {
      fprintf(stderr, "%s: fork failed!\n", prog); exit(1);
    } else {
      int rc;
      waitpid(childPid, &rc, 0);
      // if (rc == 0 && verbose) printf("%s: child process terminated successfully!\n", prog);
      if (rc == 1) {
	fprintf(stderr, "%s: child process terminated with an error!\n", prog); exit(1);
      }
    }
  }

  // compare value of count with B
  if ((fp = fopen("out_counter.txt", "r")) == NULL) {
    fprintf(stderr, "%s: can't open out_counter.txt\n", prog);
    exit(1);
  } while (!feof(fp)) {
    if (fgets(fileStr, sizeof(fileStr), fp) != NULL)
      sscanf(fileStr, "%ld", &count);
  }
  fclose(fp);

  if (remove("out_counter.txt"))
    fprintf(stderr, "%s: unable to delete out_counter.txt\n", prog);
  printf("%s: counterTest(B = %ld, n = %d, L = %s) has %s! count = %ld\n",
	 prog, B, n, L, B == count ? "PASSED" : "FAILED", count);
}

void contiguityTest(long B, int n, char *L)
{
  long curr = -1, pred = -1;
  FILE *fp;
  pid_t childPid;
  char fileStr[128];

  if (access("main", F_OK|X_OK)) {
    fprintf(stderr, "%s: 'main' file does not exist in current dir, or is not executable\n", prog);
    exit(1);
  } else {
    char bStr[10]; char nStr[10];
    sprintf(bStr, "%ld", B); sprintf(nStr, "%d", n);
    childPid = fork();
    if (childPid == 0) {
      execlp("./main", "main", "-B", bStr, "-n", nStr, "-L", L, "-M", "2000", "-W", "1", "-T", "1",
	     "-S", "lockFree", "-o", "7", "-p", verbose? "-v" : "", (char *) NULL);
    } else if (childPid < 0) {
      fprintf(stderr, "%s: fork failed!\n", prog); exit(-1);
    } else {
      int rc;
      waitpid(childPid, &rc, 0);
      // if (rc == 0 && verbose) printf("%s: child process terminated successfully!\n", prog);
      if (rc == 1) {
	fprintf(stderr, "%s: child process terminated with an error!\n", prog); exit(-1);
      }
    }
  }

  // verify monotonic increase of increment operations
  if ((fp = fopen("out_contiguity.txt", "r")) == NULL) {
    fprintf(stderr, "%s: can't open out_contiguity.txt\n", prog);
    exit(1);
  } while (!feof(fp)) {
    if (fgets(fileStr, sizeof(fileStr), fp) != NULL) {
      sscanf(fileStr, "%ld", &curr);
      if (curr < pred) break;
      else pred = curr;
    }
  }
  fclose(fp);

  if (remove("out_contiguity.txt"))
    fprintf(stderr, "%s: unable to delete out_contiguity.txt\n", prog);
  printf("%s: contiguityTest(B = %ld, n = %d, L = %s) has %s! curr = %ld, pred = %ld\n",
	 prog, B, n, L, B == curr ? "PASSED" : "FAILED", curr, pred);
  
}

void orderingTest(long B, int n)
{
  long inc;
  FILE *fp;
  pid_t c1, c2;
  char fileStr[128];

  if (access("main", F_OK|X_OK)) {
    fprintf(stderr, "%s: 'main' file does not exist in current dir, or is not executable\n", prog);
    exit(1);
  } else {
    char bStr[10]; char nStr[10];
    sprintf(bStr, "%ld", B); sprintf(nStr, "%d", n);

    c1 = fork();
    if (c1 == 0) {
      execlp("./main", "main", "-B", bStr, "-n", nStr, "-L", "anderson", "-M", "2000", "-W", "1",
	     "-T", "1", "-S", "lockFree", "-o", "8", "-p", verbose ? "-v" : "", (char *) NULL);
    } else if (c1 < 0) {
      fprintf(stderr, "%s: fork failed!\n", prog); exit(-1);
    } else {
      int rc;
      waitpid(c1, &rc, 0);
      // if (rc == 0 && verbose) printf("%s: child1 process terminated successfully!\n", prog);
      if (rc == 1) {
	fprintf(stderr, "%s: child1 process terminated with an error!\n", prog); exit(-1);
      }
    }

    c2 = fork();
    if (c2 == 0) {
      execlp("./main", "main", "-B", bStr, "-n", nStr, "-L", "tas", "-M", "2000", "-W", "1",
	     "-T", "1", "-S", "lockFree", "-o", "8", "-p", verbose ? "-v" : "", (char *) NULL);
    } else if (c2 < 0) {
      fprintf(stderr, "%s: fork failed!\n", prog); exit(-1);
    } else {
      int rc;
      waitpid(c2, &rc, 0);
      // if (rc == 0 && verbose) printf("%s: child2 process terminated successfully!\n", prog);
      if (rc == 1) {
	fprintf(stderr, "%s: child2 process terminated with an error!\n", prog); exit(-1);
      }
    }
  }

  // compare # of incs across threads between TAS and ABQ locks
  int i = 0; long incs[2][n];
  float vTAS, vABQ, sdTAS, sdABQ;
  if ((fp = fopen("out_ordering.txt", "r")) == NULL) {
    fprintf(stderr, "%s: can't open out_ordering.txt\n", prog);
    exit(1);
  } while (!feof(fp)) {
    if (fgets(fileStr, sizeof(fileStr), fp) != NULL) {
      sscanf(fileStr, "%ld", &inc);
      incs[i / n][i % n] = inc;
      i++;
    }
  }
  fclose(fp);

  vTAS = getVariance(incs[1], n);
  vABQ = getVariance(incs[0], n);
  sdTASr = getStdDev(incs[1], n);
  sdABQ = getStdDev(incs[0], n);

  if (remove("out_ordering.txt"))
    fprintf(stderr, "%s: unable to delete out_ordering.txt\n", prog);
  printf("%s: orderingTest(B = %ld, n = %d) results:\n", prog, B, n);
  printf("variance(tas) = %f, variance(anderson) = %f\nstdDev(tas) = %f, stdDev(anderson) = %f\n",
	 vTAS, vABQ, sdTAS, sdABQ);
}

void idleLockOverhead(unsigned int r)
{
}

void uniformSpeedup(unsigned int r)
{
}

void exponentialSpeedup(unsigned int r)
{
}

void awesomeSpeedup(unsigned int r)
{
}
