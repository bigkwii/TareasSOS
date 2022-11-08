#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

static NthQueue *nth_pri1ReadyQueue[MAXPRI];

void nSetPriority(nThread th, int pri) {
  START_CRITICAL
  
  th->pri= pri;
  schedule(); // The calling thread may loose the CPU if its priority is lower
  
  END_CRITICAL
}

static void nth_pri1SetReady(nThread th) {
  CHECK_CRITICAL("nth_fcfsSetReady")
  
  if (th->status==READY || th->status==RUN)
    nFatalError("nth_fcfsReady", "The thread was already in READY status\n");

  th->status= READY;
  nth_putBack(nth_pri1ReadyQueue[th->pri], th);
}

static void nth_pri1Suspend(State waitState) {
  CHECK_CRITICAL("nth_fcfsSuspend")
  
  nThread th= nSelf();
  if (th->status!=RUN && th->status!=READY)
    nFatalError("nth_fcfsSuspend", "Thread was not ready or run\n");
  th->status= waitState;
}

static void nth_pri1Schedule(void) {
  CHECK_CRITICAL("nth_fcfsSchedule")
  
  // int prevCoreId= coreId();
  nThread thisTh= nSelf();
  if (thisTh!=NULL && (thisTh->status==READY || thisTh->status==RUN)) {
    thisTh->status= READY;
    nth_putBack(nth_pri1ReadyQueue[thisTh->pri], thisTh);
  }

  nThread nextTh= NULL;
  while (nextTh==NULL) {
    int i=0;
    while (i<MAXPRI) {
      if (!nth_emptyQueue(nth_pri1ReadyQueue[i]))
        break;
      i++;
    }
    if (i<MAXPRI) {
      nextTh= nth_getFront(nth_pri1ReadyQueue[i]);
      break;
    }
    else {
      nth_coreIsIdle[0]= 1; // To prevent a signal handler to call
                                       // recursively this scheduler
      sigsuspend(&nth_sigsetApp);
      nth_coreIsIdle[0]= 0;
    }
  }

  // The context change: give this core to nextTh
  // it will take a while to return from here
  // Meanwhile thread nextTh and others are being executed
  
  nth_changeContext(thisTh, nextTh);
  
  // Some time later, at return the scheduler gave back onother core
  // to thisTh, but most probably coreId() != prevCoreId

  nth_setSelf(thisTh); // Set current running thread: from now on, nSelf()
                       // gives thisTh
  
  thisTh->status= RUN;
}

static void nth_pri1Stop(void) {
  CHECK_CRITICAL("nth_fcfsStop")
  
  for (int i= 0; i<MAXPRI; i++)
    nth_destroyQueue(nth_pri1ReadyQueue[i]);
}

Scheduler nth_pri1Scheduler= { .schedule = nth_pri1Schedule,
                               .setReady = nth_pri1SetReady,
                               .suspend = nth_pri1Suspend,
                               .stop = nth_pri1Stop };
void setPri1Scheduling() {
  START_CRITICAL

  if (nth_verbose)
    printf("Info: setting single-core priority scheduling\n");
  
  if (nth_totalCores!=1)
    nFatalError("setPri1Scheduling",
                "This priority scheduler only accepts a single core\n");
  for (int i= 0; i<MAXPRI; i++)
    nth_pri1ReadyQueue[i]= nth_makeQueue();
  nth_setScheduler(nth_pri1Scheduler);
  MapIterator *iter= getMapIterator(nth_threadSet);
  void *ptr;
  while (mapNext(iter, &ptr, &ptr)) {
    nThread th= ptr;
    if (th->status==READY)
      nth_putBack(nth_pri1ReadyQueue[th->pri], th);
  }
  destroyMapIterator(iter);
  
  END_CRITICAL
}

int isPri1Scheduling(void) {
  return nth_scheduler.schedule==nth_pri1Scheduler.schedule;
}
