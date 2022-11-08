#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

static NthQueue *nth_lcfs1ReadyQueue;
  
static void nth_lcfs1SetReady(nThread th) {
  CHECK_CRITICAL("nth_lcfs1SetReady")
  
  if (th->status==READY || th->status==RUN)
    nFatalError("nth_lcfs1Ready", "The thread was already in READY status\n");

  nThread thisTh= nSelf();
  if (thisTh->status==RUN) {
    thisTh->status=READY;
    nth_putFront(nth_lcfs1ReadyQueue, thisTh);
  }

  th->status= READY;
  nth_putFront(nth_lcfs1ReadyQueue, th);
}

static void nth_lcfs1Suspend(State waitState) {
  CHECK_CRITICAL("nth_lcfs1Suspend")

  nThread th= nSelf();
  if (th->status==READY)
    nth_delQueue(nth_lcfs1ReadyQueue, th);
  else if (th->status!=RUN)
    nFatalError("nth_lcfs1Suspend", "Thread was not ready or run\n");
  th->status= waitState;
}

static void nth_lcfs1Schedule(void) {
  CHECK_CRITICAL("nth_lcfs1Schedule")
  
  nThread thisTh= nSelf();
  if (thisTh!=NULL && thisTh->status==RUN) {
    return;
  }

  nThread nextTh= nth_getFront(nth_lcfs1ReadyQueue);
  while (nextTh==NULL) {
    // No thread to execute, only a signal can wake up a thread,
    // but signals are disabled because this is a critical section,
    // so while waiting, they must be enabled
    nth_coreIsIdle[0]= 1; // To prevent a signal handler to call
                                     // recursively this scheduler
    sigsuspend(&nth_sigsetApp);
    nth_coreIsIdle[0]= 0;
    nextTh= nth_getFront(nth_lcfs1ReadyQueue);
  }

  // The context change: give this core to nextTh
  // it will take a while to return from here
  // Meanwhile thread nextTh and others are being executed
  
  nth_changeContext(thisTh, nextTh);
  
  // Some time later, at return the scheduler gave back onother core
  // to thisTh, but most probably coreId() != prevCoreId

  nth_setSelf(thisTh); // Set current running thread:
                       // from now on, nSelf() gives thisTh

  thisTh->status= RUN;
}

static void nth_lcfs1Stop(void) {
  CHECK_CRITICAL("nth_lcfs1Stop")
  
  nth_destroyQueue(nth_lcfs1ReadyQueue);
}

static Scheduler nth_lcfs1Scheduler= { .schedule = nth_lcfs1Schedule,
                               .setReady = nth_lcfs1SetReady,
                               .suspend = nth_lcfs1Suspend,
                               .stop = nth_lcfs1Stop };
void setLcfs1Scheduling() {
  START_CRITICAL

  if (nth_verbose)
    printf("Info: setting single-core LCFS scheduling\n");

  if (nth_totalCores!=1)
    nFatalError("setFcfs1Scheduling",
                "This FCFS scheduler only accepts a single core\n");

  nth_lcfs1ReadyQueue= nth_makeQueue();
  nth_setScheduler(nth_lcfs1Scheduler);
  MapIterator *iter= getMapIterator(nth_threadSet);
  void *ptr;
  while (mapNext(iter, &ptr, &ptr)) {
    nThread th= ptr;
    if (th->status==READY)
      nth_putBack(nth_lcfs1ReadyQueue, th);
  }
  destroyMapIterator(iter);
  
  END_CRITICAL
}

int isLcfs1Scheduling(void) {
  return nth_scheduler.schedule==nth_lcfs1Scheduler.schedule;
}
