#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

static NthQueue *nth_fcfsReadyQueue;

static void nth_fcfsSetReady(nThread th) {
  CHECK_CRITICAL("nth_fcfsSetReady")
  
  if (th->status==READY || th->status==RUN)
    nFatalError("nth_fcfsReady", "The thread was already in READY status\n");

  th->status= READY;
  if (nth_allocCoreId(th)<0)
    nth_putBack(nth_fcfsReadyQueue, th);
  else if (nth_allocCoreId(th)!=nth_coreId()) {
#if 0
    DBG(
      printf("wake core %d up from %d\n", nth_allocCoreId(th), nth_coreId());
    );
#endif
    nth_coreWakeUp(nth_allocCoreId(th));
  }
}

static void nth_fcfsSuspend(State waitState) {
  CHECK_CRITICAL("nth_fcfsSuspend")

  nThread th= nSelf();
  if (th->status==READY)
    nth_delQueue(nth_fcfsReadyQueue, th);
  else if (th->status!=RUN)
    nFatalError("nth_fcfsSuspend", "Thread was not ready or run\n");

  th->status= waitState;
}

static void nth_fcfsSchedule(void) {
  CHECK_CRITICAL("nth_fcfsSchedule")
  
  // int prevCoreId= coreId();
  nThread thisTh= nSelf();

  for (;;) {
    if (thisTh!=NULL && (thisTh->status==READY || thisTh->status==RUN)) {
      if (nth_allocCoreId(thisTh)<0 && thisTh->status==READY)
        nth_delQueue(nth_fcfsReadyQueue, thisTh);
      break;              // Continue running same allocated thread
    }

    nThread nextTh= nth_getFront(nth_fcfsReadyQueue);
    if (nextTh!=NULL) {
      // The context change: give this core to nextTh
      // it will take a while to return from here
      // Meanwhile thread nextTh and others are being executed
      nth_changeContext(thisTh, nextTh);
      // Some time later, at return the scheduler gave back onother core
      // to thisTh, but most probably coreId() != prevCoreId
      nth_setSelf(thisTh);
      if (thisTh->status==READY)
        break;
    }
    nth_coreIsIdle[nth_coreId()]= 1; // To prevent a signal handler to call
                                     // recursively this scheduler
    CHECK_STACK
    llUnlock(&nth_schedMutex);
    sigsuspend(&nth_sigsetApp);
    llLock(&nth_schedMutex);
    nth_coreIsIdle[nth_coreId()]= 0;
  } 
  DBG(
    if (thisTh!=nSelf())
      nFatalError("nth_fcfsSchedule", "nSelf() inconsistency\n");
  );
  CHECK_STACK
  thisTh->status= RUN;
  nth_reviewCores(nth_peekFront(nth_fcfsReadyQueue));
}

static void nth_fcfsStop(void) {
  CHECK_CRITICAL("nth_fcfsStop")
  
  nth_destroyQueue(nth_fcfsReadyQueue);
}

static Scheduler nth_fcfsScheduler= { .schedule = nth_fcfsSchedule,
                               .setReady = nth_fcfsSetReady,
                               .suspend = nth_fcfsSuspend,
                               .stop = nth_fcfsStop };
void setFcfsScheduling() {
  START_CRITICAL

  if (nth_verbose)
    printf( "Info: setting %d-core FCFS scheduling\n", nth_totalCores);

  nth_fcfsReadyQueue= nth_makeQueue();
  nth_setScheduler(nth_fcfsScheduler);
  MapIterator *iter= getMapIterator(nth_threadSet);
  void *ptr;
  while (mapNext(iter, &ptr, &ptr)) {
    nThread th= ptr;
    if (th->status==READY)
      nth_putBack(nth_fcfsReadyQueue, th);
  }
  destroyMapIterator(iter);
  
  END_CRITICAL
}

void allocThreadFcfs(nThread th, int coreId) {
  START_CRITICAL
  
  if (! (0<=coreId && coreId<nth_totalCores))
    printf("The core %d does not exist\n", coreId);
  int id= nth_allocCoreId(th);
  if (id>=0)
    printf("The thread is already allocated at core %d\n", id);
  if (!nth_coreIsIdle[coreId])
    printf("Core %d is not idle\n", coreId);
  if (nth_queryThread(nth_fcfsReadyQueue, th))
    nth_delQueue(nth_fcfsReadyQueue, th);
  nth_putFront(nth_fcfsReadyQueue, th);
  nth_coreWakeUp(coreId);
  END_CRITICAL
}

int isFcfsScheduling(void) {
  return nth_scheduler.schedule==nth_fcfsScheduler.schedule;
}
