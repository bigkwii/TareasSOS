#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif

#ifndef NTHREAD_IMPL_H
#define NTHREAD_IMPL_H 1

#include <pthread.h>
#include <signal.h>


/*************************************************************
 * Headers for the API of nthreads are in nthread.h
 *************************************************************/

// This switch is to avoid definitions of standard pthread names in nthread.h
// (as for example pthread_create, pthread_mutex_lock, thread_t, etc.)
#define NTH_NO_ALT_PTHREAD_EQUIV

#include "nthread.h"

/*************************************************************
 * States of a thread
 *************************************************************/

typedef enum {
  RUN,              // Running in a core
  READY,            // Waiting for a core in nth_readyQueue
  ZOMBIE,           // Terminated but waiting nThreadJoin call
  BURIED,           // Buried but it is still allocated to a core, can't free it
  CREATED,          // In creation
  WAIT_RESUME,      // Waiting the creation of a thread in nThreadCreate
  WAIT_JOIN,        // Waiting the termination of a thread in nThreadJoin
  WAIT_SEM,         // Waiting for a ticket in a nSemWait
  WAIT_LOCK,        // Waiting to acquire a mutex in nLock or nWaitCond
  WAIT_COND,        // Waiting a signal or broadcast in nWaitCond
  WAIT_COND_TIMEOUT, // Waiting a signal or broadcast in nWaitCondTimeout
  WAIT_READ,        // Waiting in nRead
  WAIT_WRITE,       // Waiting in nWrite
  WAIT_SEND,        // For messages
  WAIT_SEND_TIMEOUT,
  WAIT_REPLY,
  WAIT_SLEEP        // Waiting in nSleep
} State;

#define STATE_NAMES \
  "RUN", "READY", "ZOMBIE", "BURIED", "CREATED", "WAIT_RESUME", "WAIT_JOIN", \
  "WAIT_SEM", "WAIT_LOCK", "WAIT_COND", "WAIT_COND_TIMEOUT", \
  "WAIT_READ", "WAIT_WRITE", "WAIT_SEND", "WAIT_SEND_TIMEOUT", "WAIT_REPLY", \
  "WAIT_SLEEP"

// *** CAUTION ***: states declared in the enum and in STATE_NAME must appear
// in the same order!

// For priority scheduling
  
#ifndef MAXPRI
#define MAXPRI 10
#endif

/*************************************************************
 * The thread descriptor
 *************************************************************/

// Declared in thread.h: typedef struct nthread *nThread;

struct nthread {
  State status;           // RUN, READY, ZOMBIE, WAIT_JOIN, WAIT_LOCK, etc.
  char *name;             // For debugging purposes
  int allocCoreId;
  int freeIt;

  // Scheduling
  void *queue;            // The queue where this thread is waiting
  nThread nextTh;         // Next node in a linked list of threads
  long long startCoreNanos; // To measure cpu burst duration
  long long sliceNanos;   // Nanos remaining for this cpu slice
  int pri;

  void **sp;              // Thread stack pointer when suspended
  void **stack;           // Pointer to the whole stack memory area
  unsigned long long stackSize;

  // For nThreadExit and nThreadJoin
  void *retPtr;
  nThread joinTh;
  
  // For timeouts
  void *timequeue;        // The timed queue where this thread is waiting
  nThread nextTimeTh;     // Next node in a time ordered linked list of threads
  long long wakeTime;
  void (*wakeUpFun)(nThread th);
  
  // For msgs
  struct nthqueue *sendQueue;
  union {
    void *msg;
    int rc;
  } send;
  void *ptr; // unused
};

void nth_setSelf(nThread th);

typedef struct nthqueue NthQueue;

NthQueue *nth_makeQueue();
void nth_putBack(NthQueue *queue, nThread th);
void nth_putFront(NthQueue *queue, nThread th);
nThread nth_peekFront(NthQueue *queue);
nThread nth_getFront(NthQueue *queue);
int nth_queryThread(NthQueue *queue, nThread query_th);
void nth_delQueue(NthQueue *queue, nThread th);
int nth_emptyQueue(NthQueue *queue);
int nth_queueLength(NthQueue *queue);
void nth_destroyQueue(NthQueue *queue);

typedef struct nthtimequeue NthTimeQueue;

NthTimeQueue *nth_makeTimeQueue();
void nth_putTimed(NthTimeQueue *timeq, nThread th, long long wakeTime);
nThread nth_getTimed(NthTimeQueue *timeq);
long long nth_nextTime(NthTimeQueue *timeq);
int nth_emptyTimeQueue(NthTimeQueue *timeq);
int nth_delTimed(NthTimeQueue *timeq, nThread th);
void nth_destroyTimeQueue(NthTimeQueue *timeq);

/*************************************************************
 * The schedulers
 *************************************************************/

// Low level mutex and condition

typedef pthread_mutex_t LLMutex;
typedef pthread_cond_t LLCond;

#define LLMutexInit(m) (pthread_mutex_init(m, NULL))
#define LLCondInit(c)  (pthread_cond_init(c, NULL))
#define llLock(m)      (pthread_mutex_lock(m))
#define llUnlock(m)    (pthread_mutex_unlock(m))
#define llWait(c,m)    (pthread_cond_wait(c,m))
#define llSignal(c)    (pthread_cond_signal(c))
#define llBroadcast(c) (pthread_cond_broadcast(c))

typedef struct {
  void (*schedule)(void);
  void (*setReady)(nThread th);
  void (*suspend)(State waitState);
  void (*stop)(void);
} Scheduler;

extern Scheduler nth_scheduler;

extern LLMutex nth_schedMutex;
extern LLCond nth_idleCoreCond;
extern LLCond nth_nthreadReadyCond;

extern int nth_totalCores;       // Number of total virtual cores
extern Map *nth_threadSet;       // A hash map keeping a set of all nthreads
extern int nth_threadCount;      // Number of living threads
extern nThread *nth_coreThreads; // Array of running pthreads w/virtual cores
extern int *nth_coreIsIdle;      // Prevent scheduler recursive calls
extern pthread_t *nth_nativeCores; // the pthreads virtualizing cores


// For Round Robin Scheduling
extern long long nth_sliceNanos;
extern int nth_implicitContextChanges;
extern int nth_contextChanges;
extern int nth_verbose;
void nth_rrInit(void);
void nth_rrThreadInit(void);
void nth_printLog(int n);

// Other schedulers
extern int nth_fcfs1;
extern int nth_pri1;

// Convenience macros

#define schedule()       (*nth_scheduler.schedule)()
#define setReady(th)     (*nth_scheduler.setReady)(th)
#define suspend(state)   (*nth_scheduler.suspend)(state)
#define stopScheduler()  (*nth_scheduler.stopScheduler)()

void nth_setScheduler(Scheduler scheduler);

/*************************************************************
 * Internal functions
 *************************************************************/

// Core wake up
void nth_coreWakeUp(int id);
void nth_reviewCores(nThread th);

// Context handling

extern __thread int nth_thisCoreId;

#define nth_allocCoreId(th) (th->allocCoreId)
#define nth_coreId() nth_thisCoreId
#define nth_selfCritical() (nth_coreThreads[nth_coreId()])

void nth_changeContext(nThread thisTh, nThread nextTh);
void nth_callInNewContext(nThread thisTh, nThread newTh,
                      void (*startFun)(), void *ptr);
void ***_ChangeToStack(void ***fromPsp, void ***toPsp);
void ***_CallInNewStack(void ***fromPsp, void **toPsp,
                     void (*proc)(), void *ptr);
void nth_markStack(void **sp);
void nth_checkStack(void **sp);

// Critical section management

extern __thread int nth_criticalLvl;
extern sigset_t nth_sigsetCritical;
extern sigset_t nth_sigsetApp;

void nth_startCritical(void);
void nth_endCritical(void);
void nth_startHandler(void);
void nth_endHandler(void);

// Time management
void nth_timeInit(void);
void nth_timeEnd(void);
void nth_programTimer(long long nanos, void (*wakeUpFun)(nThread th));
void nth_cancelThread(nThread th);
   
#define CHECK_STACK \
  DBG( nThread _th71= nth_selfCritical(); \
    if (_th71!=NULL && \
          (unsigned long long)((intptr_t)&_th71-(intptr_t)_th71->stack) >= \
          _th71->stackSize) \
        nFatalError("CHECK_STACK", "Stack pointer out of range\n"); \
  );
  
#define CHECK_CRITICAL(f) \
  DBG( \
    CHECK_STACK \
    if (nth_criticalLvl<=0) \
      nFatalError(f, "Inconsistency detected: not in critical section\n"); \
  );

#define START_HANDLER int _saveErrno= errno; nth_startHandler();

#define END_HANDLER nth_endHandler(); errno= _saveErrno;

#define START_CRITICAL nth_startCritical();

#define END_CRITICAL nth_endCritical();
  
#endif // ifndef NTHREAD_IMPL_H
