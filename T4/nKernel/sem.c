#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

int nSemInit(nSem *psem, int unused, unsigned int initialTickets) {
  psem->count= initialTickets;
  psem->queue= nth_makeQueue();
  return 0;
}

int nSemDestroy(nSem *psem) {
  nth_destroyQueue(psem->queue);
  return 0;
}

int nSemWait(nSem *psem) {
  START_CRITICAL
  
  if (psem->count>0)
    psem->count--;
  else if (psem->count==0) {
    nThread thisTh= nSelf();
    nth_putBack(psem->queue, thisTh);
    suspend(WAIT_SEM);
    schedule();
  }
  else
    nFatalError("nSemWait", "The semaphore has a negative count\n");
  
  END_CRITICAL
  
  return 0;
}

int nSemPost(nSem *psem) {
  START_CRITICAL
  
  if (nth_emptyQueue(psem->queue))
    psem->count++;
  else {
    nThread w= nth_getFront(psem->queue);
    setReady(w);
    schedule();
  }
  
  END_CRITICAL
  
  return 0;
}
