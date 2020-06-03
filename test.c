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
static unsigned int M;

int main(int argc, char *argv[])
{
  prog = argv[0];
  verbose = 0;
  int c, err = 0, t = -1, seed = 1;
  unsigned int r = 1, n = 1, D = 8, T = 1; M = 2000;
  long W = 1, B = 1;
  char *L = NULL, *S = NULL;
  char usage[400] = "usage %s: -t <testNo> [-r <trialRuns>] [-M <numMillisecs>] [-n <numSources>] ";
  strcat(usage, "[-W <mean>] [-L <lockType>] [-S <strategy>] [-T <numPackets>] [-D <queueDepth>] ");
  strcat(usage, "[-s <seed>] [-B <big>] [-v]\n");
  char okString[200] = "lockType = { 'tas', 'anderson' } ";
  strcat(okString, "strategy = { 'lockFree', 'homeQueue', 'awesome' }\n");
  
  // retrieve command-line arguments
  while ((c = getopt(argc, argv, "M:n:W:s:D:L:S:T:B:r:t:v")) != -1)
    switch(c) {
    case 'M':
      sscanf(optarg, "%u", &M); break;
    case 'n':
      sscanf(optarg, "%u", &n); break;
    case 'W':
      sscanf(optarg, "%ld", &W); break;
    case 's':
      sscanf(optarg, "%d", &seed); break;
    case 'D':
      sscanf(optarg, "%u", &D); break;
    case 'L':
      L = optarg; break;
    case 'S':
      S = optarg; break;
    case 'T':
      sscanf(optarg, "%u", &T); break;
    case 'B':
      sscanf(optarg, "%ld", &B); break;
    case 'r':
      sscanf(optarg, "%u", &r); break;
    case 't':
      sscanf(optarg, "%d", &t); break; 
    case 'v':
      verbose = 1; break;
    case '?':
      err = 1; break;
    }

  // handle malformed or incorrect command-line arguments
  if (r < 1) {
    fprintf(stderr, "%s: r (trialRuns) must be greater than 0\n", prog);
    fprintf(stderr, usage, prog);
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
  } else if (D < 1 || D % 2) {
    fprintf(stderr, "%s: D (queueDepth) must be a positive power of 2\n", prog);
    exit(1);
  } else if ((t == 2 || t == 4) && S == NULL) {
    fprintf(stderr, "%s: -S (strategy) value is missing\n", prog);
    fprintf(stderr, okString, prog);
    exit(1);
  } else if ((t == 6 || t == 7) && L == NULL) {
    fprintf(stderr, "%s: -L (lockType) value is missing\n", prog);
    fprintf(stderr, okString, prog);
    exit(1);
  } else if ((t == 2 || t== 4) && (strcmp(S, "lockFree") != 0 &&
				   strcmp(S, "homeQueue") != 0 && strcmp(S, "awesome") != 0)) {
    fprintf(stderr, "%s: S (strategy) value must be 'lockFree', 'homeQueue', or 'awesome'\n", prog);
    exit(1);
  } else if ((t == 6 || t == 7) && (strcmp(L, "tas") != 0 && strcmp(L, "anderson") != 0)) {
    fprintf(stderr, "%s: L (lockType) value must be 'tas' or 'anderson'\n", prog);
    exit(1);
  } else if ((t >= 6 && t <= 8) && B < 1) {
    fprintf(stderr, "%s: -B (big) must be greater than 0\n", prog);
    exit(1);
  } else if (err) {
    fprintf(stderr, usage, prog);
    exit(1);
  }

  switch(t) {
  case 1:
    parallelTest(n, W, T, seed); break;
  case 2:
    strategyTest(n, W, T, seed, S); break;
  case 3:
    timedParallelTest(n, W, T, seed, M); break;
  case 4:
    timedStrategyTest(n, W, T, seed, S, M); break;
  case 5:
    queueSequenceTest(W, D); break;
  case 6:
    countingTest(B, n, L); break;
  case 7:
    contiguityTest(B, n, L); break;
  case 8:
    orderingTest(B, n); break;
  case 9:
    idleLockOverhead(r); break;
  case 10:
    uniexpSpeedup(r, 1); break;
  case 11:
    uniexpSpeedup(r, 0); break;
  case 12:
    awesomeSpeedup(r); break;
  case 13:
    idleLockOverhead(r);
    uniexpSpeedup(r, 1);
    uniexpSpeedup(11, 0);
    // awesomeSpeedup(r);
    break;
  default:
    fprintf(stderr, "%s: -t (testNo) must be between 1 and 13, inclusive\n", prog);
    fprintf(stderr, "1 = parallelTest(n, W, T, seed)\n2 = strategyTest(n, W, T, seed, S)\n");
    fprintf(stderr, "3 = timedParallelTest(n, W, T, seed, M)\n");
    fprintf(stderr, "4 = timedStrategyTest(n, W, T, seed, S, M)\n5 = queueSequenceTest(W, D)\n");
    fprintf(stderr, "6 = countingTest(B, n, L)\n7 = contiguityTest(B, n, L)\n");
    fprintf(stderr, "8 = orderingTest(B, n)\n9 = idleLockOverhead(r)\n");
    fprintf(stderr, "10 = uniformSpeedup(r)\n11 = exponentialSpeedup(r)\n");
    fprintf(stderr, "12 = awesomeSpeedup(r)\n13 = run all experiments\n");
    exit(1);
  }
    
  return 0;
}

int longcmp(const void *a, const void *b)
{
  long la = *(long *)a;
  long lb = *(long *)b;
  return (la > lb) - (la < lb);
}

int floatcmp(const void *a, const void *b)
{
  float fa = *(float *)a;
  float fb = *(float *)b;
  return (fa > fb)  - (fa < fb);
}

long getMedianL(long arr[], int n)
{
  long median;
  if (n %2) median = arr[n/2];
  else median = (arr[(n-1)/2] + arr[n/2]) / 2;
  return median;
}

float getMedianF(float arr[], int n)
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
  int i, j, k, matching = 0;
  FILE *fp;
  pid_t childPid;
  char fileStr[128];
  
  if (access("main", F_OK|X_OK)) {
    fprintf(stderr, "%s: 'main' file does not exist in current dir, or is not executable\n", prog);
    exit(1);
  } else {
    char nStr[10]; char wStr[10]; char tStr[10]; char sStr[10]; char mStr[10];
    sprintf(nStr, "%u", n); sprintf(wStr, "%ld", W); sprintf(tStr, "%u", T);
    sprintf(sStr,"%d", seed); sprintf(mStr, "%u", M);
    for (i = 0; i < 2; i++) { // uniform or exponential
      for (j = 0; j < 2; j++) { // serial or parallel
	childPid = fork();
	if (childPid == 0) {
	  execlp("./main", "main", "-n", nStr, "-W", wStr, "-T", tStr, "-s", sStr,
		 "-M", mStr, i == 0 ? "-u" : "", "-S", "lockFree", "-o", "1", j == 1 ? "-p" : "",
		 "-L", "tas", verbose ? "-v" : "", (char *) NULL);
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
  int offset = 0; i = 0;
  long pCount;
  long *valsD = malloc(4 * n * sizeof(long));
  long valsS[2][2][n]; // arr[distr][version][n]
  if ((fp = fopen("out_parallelCheck.txt", "r")) == NULL) {
    fprintf(stderr, "%s: can't open out_parallelCheck.txt\n", prog);
    exit(1);
  } while (!feof(fp)) {
    if (fgets(fileStr, sizeof(fileStr), fp) != NULL) {
      sscanf(fileStr, "%ld", &pCount);
      offset = (i / (2 * n)) * 2 * n + (i / n % 2) * n + (i % n); // = arr[i/(2*n)][i/n%2][i%n]
      valsD[offset] = pCount;
      i++;
    }
  }
  fclose(fp);

  for (i = 0; i < 2; i++) // distr
    for (j = 0; j < 2; j++) // version
      for (k = 0; k < n; k++) // n
	valsS[i][j][k] = valsD[i*2*n+j*n+k];

  free(valsD);
  for (i = 0; i < n; i++) {
    if (valsS[0][0][i] != valsS[0][1][i]) break; // compare versions (uniform)
    if (valsS[1][0][i] != valsS[1][1][i]) break; // compare versions (exponential)
    matching++;
  }

  if (remove("out_parallelCheck.txt"))
    fprintf(stderr, "%s: unable to delete out_parallelCheck.txt\n", prog);
  printf("%s: parallelCheckTest(n = %u, T = %u, W = %ld, seed = %d) has %s!\n",
	 prog, n, T, W, seed, matching == n ? "PASSED" : "FAILED");
}

void strategyTest(unsigned int n, long W, unsigned int T, int seed, char *S)
{
  int i, j, k, l, matching = 0;
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
    char nStr[10]; char wStr[10]; char tStr[10]; char sStr[10]; char mStr[10];
    sprintf(nStr, "%u", n); sprintf(wStr, "%ld", W);
    sprintf(tStr, "%u", T); sprintf(sStr,"%d", seed); sprintf(mStr, "%d", M);
    for (i = 0; i < 2; i++) { // strategy (lockFree or S)
      for (j = 0; j < 2; j++) { // distribution (uniform or exponential)
	for (k = 0; k < 2; k++) { // lock type (tas or anderson)
	  childPid = fork();
	  if (childPid == 0) {
	    execlp("./main", "main", "-n", nStr, "-W", wStr, "-T", tStr, "-s", sStr, "-M", mStr,
		   "-S", i == 1 ? S : "lockFree", j == 0 ? "-u" : "", "-L", k == 0 ? "tas" :
		   "anderson", "-p", verbose ? "-v" : "", "-o", "2", (char *) NULL);
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
  int offset = 0; i = 0;
  long pCount;
  long *valsD = malloc(8 * n * sizeof(long));
  long valsS[2][2][2][n]; // arr[S][distr][L][n]
  if ((fp = fopen("out_strategyCheck.txt", "r")) == NULL) {
    fprintf(stderr, "%s: can't open out_parallelCheck.txt\n", prog);
    exit(1);
  } while (!feof(fp)) {
    if (fgets(fileStr, sizeof(fileStr), fp) != NULL) {
      sscanf(fileStr, "%ld", &pCount);
      // = arr[i/(4n)][i/(2n)%2]][i/n%2][i%n]
      offset = ((i / (4 * n)) * 4 * n) + ((i / (2 * n) % 2) * 2 * n) + ((i / n % 2) * n) + (i % n);
      valsD[offset] = pCount;
      i++;
    }
  }
  fclose(fp);

  for (i = 0; i < 2; i++) // S
    for (j = 0; j < 2; j++) // distr
      for (k = 0; k < 2; k++) // L
	for (l = 0; l < n; l++) // n
	  valsS[i][j][k][l] = valsD[i*4*n+j*2*n+k*n+l];

  free(valsD);
  for (i = 0; i < n; i++) {
    if (valsS[0][0][0][i] != valsS[1][0][0][i]) break; // compare S's (uniform, tas)
    if (valsS[0][0][1][i] != valsS[1][0][1][i]) break; // compare S's (uniform, anderson)
    if (valsS[0][1][0][i] != valsS[1][1][0][i]) break; // compare S's (exponential, tas)
    if (valsS[0][1][1][i] != valsS[1][1][1][i]) break; // compare S's (exponential, anderson)
    matching++;
  }

  if (remove("out_strategyCheck.txt"))
    fprintf(stderr, "%s: unable to delete out_strategyCheck.txt\n", prog);
  printf("%s: strategyCheckTest(n = %u, T = %u, W = %ld, seed = %d, S = %s) has %s!\n",
	 prog, n, T, W, seed, S, matching == n ? "PASSED" : "FAILED");
}

void timedParallelTest(unsigned int n, long W, unsigned int T, int seed, unsigned int M)
{
}

void timedStrategyTest(unsigned int n, long W, unsigned int T, int seed, char *S, unsigned int M)
{
}

void queueSequenceTest(long W, unsigned int D)
{
  int i, matching = 0;
  queue_t *queue = initQueue(D);
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
  sdTAS = getStdDev(incs[1], n);
  sdABQ = getStdDev(incs[0], n);

  if (remove("out_ordering.txt"))
    fprintf(stderr, "%s: unable to delete out_ordering.txt\n", prog);
  printf("%s: orderingTest(B = %ld, n = %d) results:\n", prog, B, n);
  printf("variance(tas) = %f, variance(anderson) = %f\nstdDev(tas) = %f, stdDev(anderson) = %f\n",
	 vTAS, vABQ, sdTAS, sdABQ);
}

void idleLockOverhead(unsigned int r)
{
  int i, j, k, l;
  FILE *fp;
  pid_t childPid;
  char fileStr[128], trialNo[10];
  char *Ws[6] = { "25", "50", "100", "200", "400", "800" };
  unsigned int T = 100000000; 
  
  if (access("main", F_OK|X_OK)) {
    fprintf(stderr, "%s: 'main' file does not exist in current dir, or is not executable\n", prog);
  } else {
    if (verbose) printf("%s: Running %u trials of 'Idle Lock Overhead'...\n", prog, r);
    char mStr[10]; sprintf(mStr, "%u", M);
    char tStr[10]; sprintf(tStr, "%u", T);
    for (i = 0; i < r; i++) { // per trial
      sprintf(trialNo, "%d", r);
      for (j = 0; j < 6; j++) { // per W
	for (k = 0; k < 2; k++) { // per strategy
	  for (l = 0; l < 2; l++) { // per lock
	    childPid = fork();
	    if (childPid == 0) {
	      execlp("./main", "main", "-M", mStr, "-n", "1", "-L", l == 0 ? "tas" : "anderson",
		     "-S", k == 0 ? "lockFree" : "homeQueue", "-T", tStr, "-s", trialNo, "-u", "-p",
		     "-W", Ws[j], "-o", "9", (char *) NULL);
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
	}
      }
    }
  }

  // compute median parallel throughputs
  int offset = 0; i = 0;
  long throughput;
  long *valsD = malloc(24 * r * sizeof(long));
  long valsS[6][2][2][r]; // arr[W][L][S][r]
  if ((fp = fopen("out_overhead.txt", "r")) == NULL) {
    fprintf(stderr, "%s: can't open out_overhead.txt\n", prog);
    exit(1);
  } while (!feof(fp)) {
    if (fgets(fileStr, sizeof(fileStr), fp) != NULL) {
      sscanf(fileStr, "%ld", &throughput);
      offset = ((i / 4 % 6) * 4 * r) + ((i % 2) * 2 * r) + ((i / 2 % 2) * r) + (i / 24); 
      // = arr[i/4%6][i%2][i/2%2][i/24]
      valsD[offset] = throughput;
      i++;
    }
  }

  for (i = 0; i < 6; i++) // W
    for (j = 0; j < 2; j++) // L
      for (k = 0; k < 2; k++) // S
	for (l = 0; l < r; l++) // r
      valsS[i][j][k][l] = valsD[i*4*r+j*2*r+k*r+l];
  free(valsD);
  fclose(fp);
  
  long median[6][2][2];
  float speedup[6][2]; 

  for (i = 0; i < 6; i++) { // W
    for (j = 0; j < 2; j++) { // L
      for (k = 0; k < 2; k++) { // S
	qsort(valsS[i][j][k], r, sizeof(long), longcmp);
	median[i][j][k] = getMedianL(valsS[i][j][k], r);
      }
    }
  }

  for (i = 0; i < 6; i++) // W
    for (j = 0; j < 2; j++) // L
      speedup[i][j] = median[i][j][1] / (float) median[i][j][0];

  if (remove("out_overhead.txt"))
    fprintf(stderr, "%s: unable to delete out_overhead.txt\n", prog);
  printf("%s: Lock Scaling results (M = %u, T = %u, %u trials):\n", prog, M, T, r);	 
  for (i = 0; i < 6; i++)
    for (j = 0; j < 2; j++)
      printf("%s lock speedup (W = %s): homeQueue / lockFree = %ld / %ld = %f\n",
	     j == 0 ? "tas" : "anderson", Ws[i], median[i][j][1], median[i][j][0], speedup[i][j]);  
}

void uniexpSpeedup(unsigned int r, int uniform)
{
  int i, j, k, l, h;
  FILE *fp;
  pid_t childPid;
  char fileStr[128], trialNo[10];
  char *Ws[4] = { "1000", "2000", "4000", "8000" };
  char *ns[6] = { "1", "2", "3", "7", "13", "27" };
  unsigned int T = 100000000;
  char *fileName = uniform ? "out_speedupU.txt" : "out_speedupE.txt";

  if (access("main", F_OK|X_OK)) {
    fprintf(stderr, "%s: 'main' file does not exist in current dir, or is not executable\n", prog);
  } else {
    if (verbose) printf("%s: Running %u trials of '%s Speedup'...\n",
			prog, r, uniform ? "Uniform" : "Exponential");
    char mStr[10]; sprintf(mStr, "%u", M);
    char tStr[10]; sprintf(tStr, "%u", T);
    for (i = 0; i < r; i++) { // per trial
      sprintf(trialNo, "%d", r);
      for (j = 0; j < 4; j++) { // per W
	for (k = 0; k < 6; k++) { // per n
	  for (l = 0; l < 2; l++) { // per strategy
	    for (h = 0; h < 2; h++) { // per lock
	      childPid = fork();
	      if (childPid == 0) {
		execlp("./main", "main", "-M", mStr, "-n", ns[k], "-L", l == 0 ? "tas" : "anderson",
		       "-S", h == 0 ? "lockFree" : "homeQueue", "-T", tStr, "-s", trialNo,
		       uniform ? "-u" : "", "-p", "-W", Ws[j], "-o", uniform ? "10" : "11", (char *) NULL);
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
	  }
	}
      }
    }
  }

  // compute median parallel throughputs
  int offset = 0; i = 0;
  long throughput;
  long *valsD = malloc(96 * r * sizeof(long));
  long valsS[4][6][2][2][r]; // arr[W][n][L][S][r]
  if ((fp = fopen(fileName, "r")) == NULL) {
    fprintf(stderr, "%s: can't open out_speedupU.txt/out_speedupE.txt\n", prog);
    exit(1);
  } while (!feof(fp)) {
    if (fgets(fileStr, sizeof(fileStr), fp) != NULL) {
      sscanf(fileStr, "%ld", &throughput);
      offset = ((i / 24 % 4) * r * 24) + ((i / 4 % 6) * r * 4) + ((i / 2 % 2) * r * 2) + ((i % 2) * r) + (i / 96);
      // = arr[i/24%4][i/4%6][i/2%2][i%2][i/96]
      valsD[offset] = throughput;
      i++;
    }
  }

  for (i = 0; i < 4; i++) // W
    for (j = 0; j < 6; j++) // n
      for (k = 0; k < 2; k++) // L
	for (l = 0; l < 2; l++) // S
	  for (h = 0; h < r; h++) // r
	    valsS[i][j][k][l][h] = valsD[i*r*24+j*r*4+k*r*2+l*r+h];    
  free(valsD);
  
  long medianP[4][6][2][2];
  float speedup[4][6][2][2];

  for (i = 0; i < 4; i++) { // W
    for (j = 0; j < 6; j++) { // n
      for (k = 0; k < 2; k++) { // L
	for (l = 0; l < 2; l++) { // S 
	  qsort(valsS[i][j][k][l], r, sizeof(long), longcmp);
	  medianP[i][j][k][l] = getMedianL(valsS[i][j][k][l], r);
	}
      }
    }
  }

  if (remove(fileName))
    fprintf(stderr, "%s: unable to delete out_speedupU.txt/speedupE.txt\n", prog);

  if (access("main", F_OK|X_OK)) {
    fprintf(stderr, "%s: 'main' file does not exist in current dir, or is not executable\n", prog);
  } else {
    if (verbose) printf("%s: Running %u trials of '%s Speedup'...\n",
			prog, r, uniform ? "Uniform" : "Exponential");
    char mStr[10]; sprintf(mStr, "%u", M);
    char tStr[10]; sprintf(tStr, "%u", T);
    for (i = 0; i < r; i++) { // per trial
      sprintf(trialNo, "%d", r);
      for (j = 0; j < 4; j++) { // per W
	for (k = 0; k < 6; k++) { // per n
	  childPid = fork();
	  if (childPid == 0) {
	    execlp("./main", "main", "-M", mStr, "-n", ns[k], "-L", "tas", "-S", "lockFree",
		   "-T", tStr, "-s", trialNo, uniform ? "-u" : "", "-W", Ws[j],
		   "-o", uniform ? "10" : "11", (char *) NULL);
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
      }
    }
  }

  // compute median serial throughputs
  offset = 0; i = 0;
  long *valsD2 = malloc(24 * r * sizeof(long));
  long valsS2[4][6][r]; // arr[W][n][r]
  if ((fp = fopen(fileName, "r")) == NULL) {
    fprintf(stderr, "%s: can't open out_speedupU.txt/out_speedupE.txt\n", prog);
    exit(1);
  } while (!feof(fp)) {
    if (fgets(fileStr, sizeof(fileStr), fp) != NULL) {
      sscanf(fileStr, "%ld", &throughput);
      offset = ((i / 6 % 4) * 6 * r) + ((i % 6) * r) + (i / 24); // = arr[i/6%4][i%6][i/24]
      valsD[offset] = throughput;
      i++;
    }
  }
    
  for (i = 0; i < 4; i++) // W
    for (j = 0; j < 6; j++) // n
      for (k = 0; k < r; k++) // r
	valsS2[i][j][k] = valsD2[i*6*r+j*r+k];
  free(valsD2);
  
  long medianS[4][6];
  
  for (i = 0; i < 4; i++) {
    for (j = 0; j < 6; j++) {
      qsort(valsS2[i][j], r, sizeof(long), longcmp);
      medianS[i][j] = getMedianL(valsS2[i][j], r);
    }
  }

  // compute speedups
  for (i = 0; i < 4; i++) // W
    for (j = 0; j < 6; j++) // n
      for (k = 0; k < 2; k++)  // L
	for (l = 0; l < 2; l++) // S
	  speedup[i][j][k][l] = medianP[i][j][k][l] / (float) medianS[i][j];
  
  if (remove(fileName))
    fprintf(stderr, "%s: unable to delete out_speedupU.txt/speedupE.txt\n", prog);
  printf("%s: %s Speedup results (M = %u, T = %u, %u trials):\n",
	 prog, uniform ? "Uniform" : "Exponential", M, T, r);
  for (i = 0; i < 4; i++)
    for (j = 0; j < 6; j++)
      for (k = 0; k < 2; k++)
	for (l = 0; l < 2; l++)
	  printf("[W = %s, n = %s, L = %s, S = %s] speedup: parallel / serial = %ld / %ld = %f\n",
		 Ws[i], ns[j], k == 0 ? "tas" : "anderson", l == 0 ? "lockFree" : "homeQueue",
		 medianP[i][j][k][l], medianS[i][j], speedup[i][j][k][l]);
}


void awesomeSpeedup(unsigned int r)
{
}
