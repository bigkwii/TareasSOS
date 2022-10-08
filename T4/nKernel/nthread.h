#ifndef NTHREAD_H
#define NTHREAD_H 1

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdarg.h>

#include "pss.h"

#ifdef OPT
#define DBG(stmt) do { } while (0)
#define PC(x)
#else
extern __thread int _pc;
#define PC(x) (_pc=(x))
#define DBG(stmt) do { stmt } while (0)
#endif

#ifndef NTHREADS

#include <pthread.h>
#include <semaphore.h>

#else

/*************************************************************
 * nCompartir
 *************************************************************/

void nCompartir(void *ptr);
void *nAcceder(int max_millis);
void nDevolver(void);

/*************************************************************
 * Thread management
 *************************************************************/

typedef struct nthread *nThread; // Actual definition in nthread-impl.h
typedef int nAttr;

int  nThreadCreate(nThread *pth, void *attr,
                  void *(*startFun)(void*), void *ptr);
void nThreadExit(void *ptr);
int  nJoin(nThread th, void **pRetPtr);
void nShutdown(int rc);
void setPri1Scheduling(void);
void setFcfsScheduling(void);
void setFcfs1Scheduling(void);
void nSetTimeSlice(int sliceNanos);
void nSetPriority(nThread th, int pri);
int  isFcfsScheduling(void);
int isRRScheduling(void);
void allocThreadFcfs(nThread th, int coreId);
void allocThreadRR(nThread th, int coreId);
void dumpThreads(void);
nThread findSp(void **sp);

/*************************************************************
 * Thread attributes
 *************************************************************/

int  nMain();                  // The main function user defined
int  nSetStackSize(int size);  // Stack size for new threads
void nSetThreadName(char *format, ... ); // Thread name for debugging purposes

nThread nSelf(void);           // Thread id of the calling thread
char* nGetThreadName(void);    // Name of the calling thread
int nGetImplicitContextSwitches(void);
                               // Total number of implicit context switches
int nGetQueueLength();         // Number of threads waiting in the ready queue

/*************************************************************
 * Utilities
 *************************************************************/

int nPrintf(char *format, ...);
int nFPrintf(FILE *file, char *format, ...);
int nFFlush(FILE *file);
int nVFPrintf(FILE *stream, const char *format, va_list ap);
int nSPrintf(char *str, char *format, ...);
int nSNPrintf(char *str, size_t size, char *format, ...);
void nFatalError(char *procname, char *format, ...);
void *nMalloc(size_t size);
void nFree(void *ptr);
int nGettimeofday(struct timeval *tv, void *tz);
int nGetrusage(int who, struct rusage *usage);
long nRandom(void);
void nSrandom(unsigned seed);

/*************************************************************
 * Time management
 *************************************************************/

long long nGetTimeNanos(void);
int nGetTime(void);
int nSleepMicros(useconds_t usec);
int nSleepNanos(long long nanos);
int nSleepMillis(long long millis);
int nSleepSeconds(unsigned int seconds);

/*************************************************************
 * Semaphores
 *************************************************************/

typedef struct sem {
  int count;
  void *queue;
} nSem;

int nSemInit(nSem *psem, int unused, unsigned int initialTickets);
int nSemDestroy(nSem *psem);

int nSemPost(nSem *psem);
int nSemWait(nSem *psem);

/*************************************************************
 * Mutex and conditions
 *************************************************************/

// Actual definitions in mutex.c

typedef struct mutex nMutex;
typedef struct cond nCond;

typedef struct cond {
  struct mutex *m;
  nThread wl;  // Linked list of threads waiting for signal or broadcast
} nCond;

typedef struct mutex {
  nThread ownerTh;
  nThread wl;       // Linked list of threads waiting for the mutex
  nCond *cond;      // For compatibility with nSystem
} nMutex;

int nMutexInit(nMutex *pmutex, void *unused);
int nMutexDestroy(nMutex *pmutex);
int nCondInit(nCond *pcond, void *unused);
int nCondDestroy(nCond *pcond);

void nLock(nMutex *pmutex);
void nUnlock(nMutex *pmutex);

void nCondWait(nCond *pcond, nMutex *pmutex);
void nCondSignal(nCond *pcond);
void nCondBroadcast(nCond *pcond);

/*************************************************************
 * Messages
 *************************************************************/

int nSend(nThread th, void *msg); // Send message msg to thread th
void *nReceiveNanos(nThread *pth, long long max_nanos); // Receive from anyone
void *nReceive(nThread *pth, int max_millis); // Receive from anyone millis
void nReply(nThread th, int rc);  // Reply pending message of th


/*************************************************************
 * Equivalences between nthreads and pthreads API
 *************************************************************/

#ifndef NTH_NO_ALT_PTHREAD_EQUIV

#define pthread_t nThread
#define pthread_create nThreadCreate
#define pthread_join nJoin
#define pthread_exit nThreadExit
#define pthread_sel nSelf

#define sem_t nSem
#define sem_init nSemInit
#define sem_destroy nSemDestroy
#define sem_post nSemPost
#define sem_wait nSemWait

#define pthread_mutex_t nMutex
#define pthread_cond_t nCond
#define PTHREAD_MUTEX_INITIALIZER {NULL, NULL}
#define PTHREAD_COND_INITIALIZER {NULL, NULL}
#define pthread_mutex_init nMutexInit
#define pthread_cond_init nCondInit
#define pthread_mutex_lock nLock
#define pthread_mutex_unlock nUnlock
#define pthread_cond_wait nCondWait
#define pthread_cond_signal nCondSignal
#define pthread_cond_broadcast nCondBroadcast

#define malloc(s) nMalloc(s)
#define free(p) nFree(p)

#define usleep nSleepMicros
#define sleep nSleepSeconds
#define printf nPrintf
#define fprintf nFPrintf
#define vfprintf nVFPrintf
#define sprintf nSPrintf
#define snprintf nSNPrintf
#define fflush nFFlush
#define main nMain
#define exit nShutdown
#define gettimeofday nGettimeofday
#define getrusage nGetrusage
#define random nRandom
#define srandom nSrandom

#endif // ifndef NTH_NO_ALT_PTHREAD_EQUIV

#endif // ifndef NTHREADS

#endif // ifndef NTHREAD_H
