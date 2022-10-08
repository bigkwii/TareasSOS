#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE 1

#include "nthread-impl.h"
#include <sys/resource.h>

typedef struct {
  int coreIn, coreOut, sig1, cnt, sig2, usr1;
  long long start, end, sliceIn, sliceOut, intr;
  nThread thIn, thOut;
} Entry;

#ifdef TRACERR
#ifndef TRACELEN
#define TRACELEN 10000
#endif
Entry log_e[TRACELEN];
int logIdx= 0;
__thread Entry *plog;

void nth_printLog(int n) {
  printf("core start  intr   end   elap slice sl out si1 cnt si2 us1 thread\n");  
  int end= logIdx;
  int ini= 0;
  if (n>0)
    end= n;
  else
    ini= logIdx+n;
  int k= ini;
  while (k<end) {
    Entry *plog= &log_e[k];
    printf(" %2d%c %5d %5d %5d  %5d %5d  %5d  %2d  %2d  %2d  %2d  %p%c\n",
           plog->coreIn,plog->coreIn==plog->coreOut?' ':'*',
           (int)(plog->start/1000000),
           plog->intr<0?-1:(int)(plog->intr/1000000),
           (int)(plog->end/1000000), (int)((plog->end-plog->start)/1000000),
           (int)(plog->sliceIn/1000000), (int)(plog->sliceOut/1000000),
           plog->sig1, plog->cnt, plog->sig2, plog->usr1,
           (void*)plog->thOut,plog->thIn==plog->thOut?' ':'*');
    k++;
  }
}
#endif

static void nth_setCoreTimerAlarm(long long sliceNanos, int coreId);
static long long nth_getCoreNanos();

long long nth_sliceNanos= 0; // Duration of the time slice before preempting
static timer_t *nth_timerSet;

static __thread clockid_t nth_clockid;

static NthQueue *nth_rrReadyQueue;

static void nth_VTimerHandler(int sig, siginfo_t *si, void *uc);


void nth_rrInit(void) {
  nth_timerSet= malloc(nth_totalCores*sizeof(*nth_timerSet));
}

void nth_rrThreadInit(void) {
  if (pthread_getcpuclockid(pthread_self(), &nth_clockid)!=0) {
    perror("pthread_getcpuclockid");
    nFatalError("nth_rrThreadInit", "Cannot continue\n");
  }
  
  struct sigaction sigact;
  sigact.sa_flags= SA_SIGINFO;
  sigact.sa_sigaction= nth_VTimerHandler;
  sigact.sa_mask= nth_sigsetCritical;

  if (sigaction(SIGVTALRM, &sigact, NULL)!=0) {
    perror("sigaction");
    nFatalError("nth_rrThreadInit", "Cannot continue\n");
  }
    
  struct sigevent sigev;
  sigev.sigev_notify= SIGEV_THREAD_ID;
  sigev._sigev_un._tid= gettid();
  sigev.sigev_signo= SIGVTALRM;
  sigev.sigev_value.sival_ptr= &nth_timerSet[nth_coreId()];
  if ( timer_create(CLOCK_THREAD_CPUTIME_ID,
                    &sigev, &nth_timerSet[nth_coreId()])!=0 ) {
    perror("timer_create");
    nFatalError("nth_rrThreadInit", "Cannot continue\n");
  }

#ifdef TRACERR
  plog= &log_e[logIdx++];
  plog->coreIn= nth_coreId(); plog->coreOut= -1;
  plog->start= nth_getCoreNanos(); plog->end= -1;
  plog->sliceIn= -1; plog->sliceOut= -1;
  plog->intr= -1;
#endif
}

static long long nth_getCoreNanos() {
  struct timespec ts;
  if (clock_gettime(nth_clockid, &ts)!=0) {
    perror("clock_gettime");
    nFatalError("nth_getCoreNanos", "Cannot continue\n");
  }
  return (long long)ts.tv_sec*1000000000+ts.tv_nsec;
}

static void nth_VTimerHandler(int sig, siginfo_t *si, void *uc) {
  // If this core is waiting in nth_rrSchedule, a return to nth_rrSchedule
  // is mandatory because calling recursively nth_rrSchedule is unsafe
  if (nth_coreIsIdle[nth_coreId()])
    return;
  nThread th= nth_selfCritical();
  if (th==NULL)
    return; // to avoid weird race conditions
  
  START_HANDLER
  
#ifdef TRACERR
    plog->intr= nth_getCoreNanos();
#endif
    
  th->sliceNanos= 0;
  if (!nth_emptyQueue(nth_rrReadyQueue))
    nth_implicitContextChanges++;

  // If this core is waiting in sigsuspend in nth_rrSchedule,
  // do not call recursively nth_rrSchedule
  if ( !nth_coreIsIdle[nth_coreId()] ) {
    schedule();
  }
  
  END_HANDLER
}

static void nth_rrSetReady(nThread th) {
  CHECK_CRITICAL("nth_rrSetReady")
  
  if (th->status==READY || th->status==RUN)
    nFatalError("nth_rrReady", "The thread was already in READY status\n");

  th->status= READY;
  if (nth_allocCoreId(th)<0) {
    if (th->sliceNanos>0)                 // it has some slice yet
      nth_putFront(nth_rrReadyQueue, th);
    else {                                // it exhausted its slice
       th->sliceNanos= nth_sliceNanos;    // give it a whole new slice and
       nth_putBack(nth_rrReadyQueue, th); // put it at the end of the queue
    }
  }
  else if (nth_allocCoreId(th)!=nth_coreId()) {
    // Thread th is allocated to nth_allocCoreId(th), wake it up
#if 0
    DBG(
      printf("wake core %d up from %d\n", nth_allocCoreId(th), nth_coreId());
    );
#endif
#ifdef TRACERR
    plog->sig1=nth_allocCoreId(th);
    plog->cnt++;
#endif
    nth_coreWakeUp(nth_allocCoreId(th));
  }
}

static void nth_rrSuspend(State waitState) {
  CHECK_CRITICAL("nth_rrSuspend")
  
  nThread th= nSelf();
  if (th->status!=RUN && th->status!=READY)
    nFatalError("nth_rrSuspend", "Thread was not ready or run\n");
  th->status= waitState;
}

static void nth_rrSchedule(void) {
  CHECK_CRITICAL("nth_rrSchedule")

  // int prevCoreId= coreId();

  nThread thisTh= nSelf();

#ifdef TRACERR  
  plog->thOut= thisTh;
  plog->coreOut= nth_coreId();
#endif

  if (thisTh!=NULL) {
    long long endNanos= nth_getCoreNanos();
    thisTh->sliceNanos -= endNanos-thisTh->startCoreNanos;
#ifdef TRACERR  
    plog->end= endNanos;
    plog->sliceOut= thisTh->sliceNanos;
#endif
  }
  // thisTh->sliceNanos can be negative if the slice expired when
  // signals were disabled

  for (;;) {
    if (thisTh!=NULL && (thisTh->status==READY || thisTh->status==RUN)) {
      if (thisTh->sliceNanos>0)
        break; // Continue running same allocated thread
      else {
        thisTh->sliceNanos= nth_sliceNanos;
        thisTh->status= READY;
        nth_putBack(nth_rrReadyQueue, thisTh);
      }
    }
      
    nThread nextTh= nth_getFront(nth_rrReadyQueue);
    if (nextTh!=NULL) {
      // The context change: give this core to nextTh
      // it will take a while to return from here
      // Meanwhile thread nextTh and others are being executed
      nth_changeContext(thisTh, nextTh);
      // Some time later, at return the scheduler gave back onother core
      // to thisTh Most probably coreId() != prevCoreId
      nth_setSelf(thisTh);
      if (thisTh->status==READY)
        break;
    }
    
    DBG(
      if (thisTh!=nSelf())
        nFatalError("nth_rrSchedule", "nSelf() inconsistency\n");
    );
    nth_coreIsIdle[nth_coreId()]= 1; // To prevent a signal handler to call
    CHECK_STACK          // recursively this scheduler
    if (nth_totalCores>1)
      llUnlock(&nth_schedMutex);
    sigsuspend(&nth_sigsetApp);
    if (nth_totalCores>1)
      llLock(&nth_schedMutex);
    nth_coreIsIdle[nth_coreId()]= 0;
  }

  DBG(
    if (thisTh!=nSelf())
      nFatalError("nth_rrSchedule", "nSelf() inconsistency\n");
  );
  CHECK_STACK
  thisTh->status= RUN;
  thisTh->startCoreNanos= nth_getCoreNanos();
#ifdef TRACERR
  if (logIdx>=TRACELEN) {
    nFatalError("nth_rrSchedule", "Trace length exceeded\n");
  }
  plog= &log_e[logIdx++];
  plog->coreIn= nth_coreId(); plog->coreOut= -1;
  plog->start= thisTh->startCoreNanos; plog->end= -1;
  plog->thIn= thisTh; plog->thOut= NULL;
  plog->intr= -1; plog->sliceIn= thisTh->sliceNanos; plog->sliceOut= -1;
  plog->sig1= plog->sig2= -1; plog->cnt= 0; plog->usr1= 0;
#endif
  nth_reviewCores(nth_peekFront(nth_rrReadyQueue));
  nth_setCoreTimerAlarm(thisTh->sliceNanos, nth_coreId());
}

static void nth_setCoreTimerAlarm(long long sliceNanos, int coreId) {
  struct itimerspec slicespec;
  slicespec.it_value.tv_sec= sliceNanos/1000000000;
  slicespec.it_value.tv_nsec= sliceNanos%1000000000;
  slicespec.it_interval.tv_sec= 0;
  slicespec.it_interval.tv_nsec= 0;
  int rc= timer_settime(nth_timerSet[coreId], 0, &slicespec, NULL);
  if (rc!=0) {
    if (rc==-1)
      perror("timer_settime");
    nFatalError("timer_settime", "Failed to set time alarm, cannot continue\n");
#if 0
    sigset_t sigcurr;
    pthread_sigmask(SIG_UNBLOCK, NULL, &sigcurr);
    if (sigismember(&sigcurr, SIGVTALRM))
      nFatalError("nth_setCoreTimerAlarm", "SIGVTALRM is enable in critical zone\n");
    if (sigismember(&sigcurr, SIGALRM))
      nFatalError("nth_setCoreTimerAlarm", "SIGALRM is enable in critical zone\n");
    if (sigismember(&sigcurr, SIGIO))
      nFatalError("nth_setCoreTimerAlarm", "SIGIO is enable in critical zone\n");
    if (sigismember(&sigcurr, SIGUSR1))
      nFatalError("nth_setCoreTimerAlarm", "SIGUSR1 is enable in critical zone\n");
    CHECK_CRITICAL("nth_setCoreTimerAlarm");
#endif
  }
}

static void nth_gettime(int coreId) {
  if (coreId<0)
    coreId= nth_coreId();
  struct itimerspec spec;
  if (timer_gettime(nth_timerSet[coreId], &spec)!=0)
    perror("timer_gettime");
  printf("next expiration in %lld nanos, interval is %lld nanos\n",
    (long long)spec.it_value.tv_sec*1000000000+spec.it_value.tv_nsec,
    (long long)spec.it_interval.tv_sec*1000000000+spec.it_interval.tv_nsec);
}

// Check the unblocking of the SIGVTALRM signal
static int nth_checkVtAlrm(void) {
  sigset_t empty, curr;
  sigemptyset(&empty);
  // nth_sigsetApp is the set of signals normally accepted in app mode
  pthread_sigmask(SIG_UNBLOCK, &empty, &curr);
  return sigismember(&curr, SIGVTALRM);
}

static void nth_rrStop(void) {
  CHECK_CRITICAL("nth_rrStop")
  
  // Disarm all timers
  int ncores= nth_totalCores;
  for (int i= 0; i<ncores; i++) {
    nth_setCoreTimerAlarm(0, i);
  }
  
  nth_destroyQueue(nth_rrReadyQueue);
}

Scheduler nth_rrScheduler= { .schedule = nth_rrSchedule,
                               .setReady = nth_rrSetReady,
                               .suspend = nth_rrSuspend,
                               .stop = nth_rrStop };
                               
void nSetTimeSlice(int sliceNanos) {
  START_CRITICAL
  
  if (nth_verbose)
    printf("Info: setting %d-core round robin scheduling\n", nth_totalCores);
  
  nth_sliceNanos= sliceNanos;

  if (!isRRScheduling()) {
    nth_rrReadyQueue= nth_makeQueue();
    nth_setScheduler(nth_rrScheduler);
    MapIterator *iter= getMapIterator(nth_threadSet);
    void *ptr;
    while (mapNext(iter, &ptr, &ptr)) {
      nThread th= ptr;
      if (th->status==READY) {
        th->sliceNanos= sliceNanos;
        nth_putBack(nth_rrReadyQueue, th);
      }
      else if (th->status==RUN) {
        int coreId= th->allocCoreId;
        th->sliceNanos= sliceNanos;
        // Arm timer for thread th
        nth_setCoreTimerAlarm(sliceNanos, coreId);
      }
    }
    destroyMapIterator(iter);
  }
  else {
    MapIterator *iter= getMapIterator(nth_threadSet);
    void *ptr;
    while (mapNext(iter, &ptr, &ptr)) {
      nThread th= ptr;
      if (th->status==READY) {
        th->sliceNanos= nth_sliceNanos;
      }
      else if (th->status==RUN) {
        int coreId= th->allocCoreId;
        // Arm timer for thread th
        nth_setCoreTimerAlarm(sliceNanos, coreId);
      }
    }
    destroyMapIterator(iter);
  }
  
  END_CRITICAL
}

void allocThreadRR(nThread th, int coreId) {
  START_CRITICAL
  
  if (! (0<=coreId && coreId<nth_totalCores))
    printf("The core %d does not exist\n", coreId);
  int id= nth_allocCoreId(th);
  if (id>=0)
    printf("The thread is already allocated at core %d\n", id);
  if (!nth_coreIsIdle[coreId])
    printf("Core %d is not idle\n", coreId);
  if (nth_queryThread(nth_rrReadyQueue, th))
    nth_delQueue(nth_rrReadyQueue, th);
  nth_putFront(nth_rrReadyQueue, th);
  nth_coreWakeUp(coreId);
  END_CRITICAL
}

int isRRScheduling(void) {
  return nth_scheduler.schedule==nth_rrScheduler.schedule;
}
