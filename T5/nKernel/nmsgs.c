#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

/*************************************************************
 * nSend, nReceive y nReply
 *************************************************************/

int nSend(nThread th, void *msg) {
  START_CRITICAL
  
  if (th->status==WAIT_SEND || th->status==WAIT_SEND_TIMEOUT) {
    if (th->status==WAIT_SEND_TIMEOUT)
      nth_cancelThread(th);
    setReady(th);
  }
  else if (th->status==ZOMBIE)
    nFatalError("nSend", "The receiver is a ZOMBIE thread\n");
  
  nThread thisTh= nSelf();
  nth_putBack(th->sendQueue, thisTh);
  thisTh->send.msg= msg;
  suspend(WAIT_REPLY);
  
  schedule();

  int rc= thisTh->send.rc;

  END_CRITICAL
  
  return rc;
}

void *nReceiveNanos(nThread *pth, long long max_nanos) {
  START_CRITICAL
  
  nThread thisTh= nSelf();
  
  if (nth_emptyQueue(thisTh->sendQueue) && max_nanos!=0) {
    if (max_nanos<0)
      suspend(WAIT_SEND);
    else {
      suspend(WAIT_SEND_TIMEOUT);
      nth_programTimer(max_nanos, NULL);
    }
    schedule();
  }
  nThread sendTask= nth_getFront(thisTh->sendQueue);
  if (pth!=NULL)
    *pth= sendTask;
  void *msg= sendTask==NULL ? NULL : sendTask->send.msg;
  
  END_CRITICAL

  return msg;
}

void *nReceive(nThread *pth, int max_millis) {
  return nReceive(pth, max_millis*1000000LL);
}
  
void nReply(nThread th, int rc) {
  START_CRITICAL
  
  if (th->status!=WAIT_REPLY)
    nFatalError("nReply", "This thread does not wait for a nReply\n");
  
  th->send.rc= rc;
  setReady(th);
  schedule();
  
  END_CRITICAL
}
