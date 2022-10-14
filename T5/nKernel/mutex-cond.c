#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

int nMutexInit(nMutex *m, void *ptr) {
  m->wl= NULL;
  m->ownerTh= NULL;
  m->cond= NULL; // For compatibility with nSystem
  return 0;
}

int nMutexDestroy(nMutex *m) {
  return 0;
}

int nCondInit(nCond *c, void *ptr) {
  c->wl= NULL;
  c->m= NULL;
  return 0;
}

void nLock(nMutex *m) {
  START_CRITICAL
  
  nThread thisTh= nSelf();
  if (m->ownerTh==NULL) {
    m->ownerTh= thisTh;
  }
  else {
    DBG(
      if (thisTh->queue!=NULL)
        nFatalError("nLock", "The thread is inconsistently in a queue\n");
    );
    thisTh->queue= m;
    thisTh->nextTh= m->wl;
    m->wl= thisTh;
    suspend(WAIT_LOCK);
    schedule();
  }
  
  END_CRITICAL
}

void nUnlock(nMutex *m) {
  START_CRITICAL
  
  if (m->ownerTh!=nSelf())
    nFatalError("nUnlock", "This thread does not own this mutex\n");
  m->ownerTh= NULL;
  if (m->wl!=NULL) {
    nThread w= m->wl;
    m->wl= w->nextTh;
    m->ownerTh= w;
    DBG(
      if (w->queue!=m)
        nFatalError("nCondWait", "Thread inconsistently not in mutex queue\n");
    );
    w->nextTh= NULL;
    w->queue= NULL;
    setReady(w);
    schedule();
  }
  
  END_CRITICAL
}

void nCondWait(nCond *c, nMutex *m) {
  START_CRITICAL
  
  nThread thisTh= nSelf();
  DBG(
    if (m->ownerTh!=thisTh)
      nFatalError("nCondWait", "This thread does not own this mutex\n");
    if (c->m!=NULL && c->m!=m)
      nFatalError("nCondWait", "The mutex is not the same registered before\n");
  );
  c->m= m;
  DBG(
    if (thisTh->queue!=NULL)
      nFatalError("nCondWait", "The thread is inconsistently in a queue\n");
  );
  thisTh->nextTh= c->wl; // add thisTh to linked list headed by c->wl
  c->wl= thisTh;
  thisTh->queue= c;
  m->ownerTh= NULL;
  suspend(WAIT_COND);
  if (m->wl!=NULL) {
    nThread w= m->wl;
    m->wl= w->nextTh;
    m->ownerTh= w;
    DBG(
      if (w->queue!=m)
        nFatalError("nCondWait", "Thread inconsistently not in mutex queue\n");
    );
    w->queue= m;
    w->nextTh= NULL;
    w->queue= NULL;
    setReady(w);
  }
  schedule();
  
  END_CRITICAL
}

void nCondSignal(nCond *c) {
  START_CRITICAL
  
  nThread w= c->wl;
  if (w!=NULL) {
    DBG(
      if (w->queue!=c)
        nFatalError("nCondSignal", "Thread inconsistently not in cond queue\n");
    );
    c->wl= w->nextTh;
    nMutex *m= c->m;
    if (m!=NULL) {
      if (m->ownerTh!=nSelf())
        nFatalError("nCondSignal", "This thread does not own this mutex\n");
      w->nextTh= m->wl;
      w->queue= m;
      m->wl= w;
    }
  }
  
  END_CRITICAL
}

void nCondBroadcast(nCond *c) {
  START_CRITICAL
  
  // move all threads in c->wl to their mutex
  nThread w= c->wl;
  nMutex *m= c->m;
  if (m!=NULL) {
    if (m->ownerTh!=nSelf())
      nFatalError("nCondSignal", "Thread does not own this mutex\n");
    while (w!=NULL) {
      DBG(
        if (w->queue!=c)
          nFatalError("nCondSignal", "Thread inconsistently not in cond queue\n");
      );
      c->wl= w->nextTh;
      w->nextTh= m->wl;
      w->queue= m;
      m->wl= w;
      w= c->wl;
    }
  }
  END_CRITICAL
}

    
