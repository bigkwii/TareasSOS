#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

#include <stdio.h>

/*************************************************************
 * Printing
 *************************************************************/

int nPrintf(char *format, ...) {
  START_CRITICAL
  
  va_list ap;
  va_start(ap, format);
  int rc= vfprintf(stdout, format, ap);
  va_end(ap);
  if (rc>=0) {
    rc= fflush(stdout);
    if (rc<0)
      perror("fflush");
  }
  else
    perror("nPrintf");

  END_CRITICAL
  
  return rc;
}

int nFPrintf(FILE *file, char *format, ...) {
  START_CRITICAL
  
  va_list ap;
  va_start(ap, format);
  int rc= vfprintf(file, format, ap);
  va_end(ap);
  if (rc>=0) {
    rc= fflush(file);
    if (rc<0)
      perror("fflush");
  }
  else
    perror("nPrintf");

  END_CRITICAL
  
  return rc;
}

int nFFlush(FILE *file) {
  START_CRITICAL
  
  int rc= fflush(file);
  if (rc<0)
    perror("nFFlush");

  END_CRITICAL
  
  return rc;
}

int nVFPrintf(FILE *stream, const char *format, va_list ap) {
  START_CRITICAL
  
  int rc= vfprintf(stream, format, ap);
  if (rc<0)
    perror("nVFPrint");

  END_CRITICAL
  
  return rc;
}

int nSPrintf(char *str, char *format, ...) {
  START_CRITICAL
  
  va_list ap;
  va_start(ap, format);
  int rc= vsprintf(str, format, ap);
  if (rc<0)
    perror("nSPrintf");
  va_end(ap);

  END_CRITICAL
  
  return rc;
}

int nSNPrintf(char *str, size_t size, char *format, ...) {
  START_CRITICAL
  
  va_list ap;
  va_start(ap, format);
  int rc= vsnprintf(str, size, format, ap);
  if (rc<0)
    perror("nSNPrintf");
  va_end(ap);

  END_CRITICAL
  
  return rc;
}

/*************************************************************
 * Fatal error stopping
 *************************************************************/

static int nth_lock= 0;

void nFatalError(char *procname, char *format, ...) {
  // Avoid recursive calls
  if (nth_lock==1) return; // Whatever
  nth_lock=1;

  va_list ap;
  va_start(ap, format);
  fprintf(stderr,"Fatal error in function %s\n", procname);
  vfprintf(stderr, format, ap);
  va_end(ap);

  START_CRITICAL
  nShutdown(1); // shutdown!
  // Does not came back here
}

/*************************************************************
 * Random numer generation
 *************************************************************/

long nRandom(void) {
  START_CRITICAL

  long r= random();
  
  END_CRITICAL

  return r;
}

void nSrandom(unsigned seed) {
  START_CRITICAL

  srandom(seed);
  
  END_CRITICAL

}

/*************************************************************
 * Getting time statistics
 *************************************************************/

int nGettimeofday(struct timeval *tv, void *tz) {
  START_CRITICAL

  int rc= gettimeofday(tv, tz);
  
  END_CRITICAL
  
  return rc;
}

int nGetrusage(int who, struct rusage *usage) {
  START_CRITICAL

  int rc= getrusage(who, usage);
  
  END_CRITICAL
  
  return rc;
}

/*************************************************************
 * Thread safe memory allocator
 *************************************************************/

void *nMalloc(size_t size) {
  START_CRITICAL

  void *ptr= malloc(size);
  
  END_CRITICAL
  return ptr;
}

void nFree(void *ptr) {
  START_CRITICAL

  free(ptr);
  
  END_CRITICAL
}
  
/*************************************************************
 * Scheduling FIFO queues
 *************************************************************/

struct nthqueue {
  nThread first, *last;
};

NthQueue *nth_makeQueue() { /* Puede ser llamada de cualquier parte */
  NthQueue *queue= nMalloc(sizeof(*queue));
  queue->first= NULL;
  queue->last= &queue->first;

  return queue;
}

void nth_putBack(NthQueue *queue, nThread th) {
  CHECK_CRITICAL("nth_putBack")
  
  if (th->queue!=NULL)
    nFatalError("PutTask", "The thread was already in a queue\n");
  th->queue= queue;

  *(queue->last)= th;
  th->nextTh= NULL;
  queue->last= &th->nextTh;
}

void nth_putFront(NthQueue *queue, nThread th) {
  CHECK_CRITICAL("nth_putFront");
  
  if (th->queue!=NULL)
    nFatalError("PushTask", "The thread was already in a queue\n");
  th->queue= queue;

  th->nextTh= queue->first;
  queue->first= th;
  if (th->nextTh==NULL) queue->last= &th->nextTh;
}

nThread nth_peekFront(NthQueue *queue) {
  return queue->first;
}

nThread nth_getFront(NthQueue *queue) {
  CHECK_CRITICAL("nth_getFront")

  nThread th= queue->first;
  if (th==NULL) return NULL;

  queue->first= th->nextTh;
  if (queue->first==NULL) queue->last= &queue->first;

  th->queue= NULL;
  th->nextTh= NULL;

  return th;
}

int nth_queryThread(NthQueue *queue, nThread query_th) {
  CHECK_CRITICAL("nth_queryThread")
  
  nThread th= queue->first;
  while (th!=NULL && th!=query_th)
    th= th->nextTh;
  return th!=NULL;
}

void nth_delQueue(NthQueue *queue, nThread th) {
  CHECK_CRITICAL("delQueue")

  nThread *pth= &queue->first;
  while (*pth!=th && *pth!=NULL)
    pth= &(*pth)->nextTh;

  if (*pth==NULL)
    nFatalError("DeleteTaskQueue",
                "Did not find the request thread\n");

  *pth= th->nextTh;
  if (queue->last==&th->nextTh) queue->last= pth;
  th->queue= NULL;
  th->nextTh= NULL;
}

int nth_emptyQueue(NthQueue *queue) {
  return queue->first==NULL;
}

int nth_queueLength(NthQueue *queue) {
  CHECK_CRITICAL("nth_queueLength")
  
  int length=0;
  nThread th= queue->first;

  while (th!=NULL) {
    th= th->nextTh;
    length++;
  }

  return length;
}

void nth_destroyQueue(NthQueue *queue) {
  if (!nth_emptyQueue(queue))
    nFatalError("DelQueue","Destroying a queue with pending threads\n");

  nFree(queue);  /* Se supone que no hay procesos colgando */
}


/*************************************************************
 * NthTimeQueue: time ordered queue
 * 
 * Note: for simplicity, this time queue is implemented
 * with a linked list, so they are very inefficient.
 * They should be implemented with a heap.
 *************************************************************/

struct nthtimequeue {
  nThread first;
};

NthTimeQueue *nth_makeTimeQueue() {
  NthTimeQueue *timeq= nMalloc(sizeof(*timeq));
  timeq->first= NULL;

  return timeq;
}

void nth_putTimed(NthTimeQueue *timeq, nThread th, long long wakeTime) {
  CHECK_CRITICAL("nth_putTimed")
  
  if (th->timequeue!=NULL)
    nFatalError("nth_putTimed", "Thread already in a queue\n");
  
  th->timequeue= timeq;

  nThread *pth= &timeq->first;
  while (*pth!=NULL && (*pth)->wakeTime-wakeTime<0)
    pth= &(*pth)->nextTimeTh;

  th->wakeTime= wakeTime;
  th->nextTimeTh= *pth;
  *pth= th;
}

nThread nth_getTimed(NthTimeQueue *timeq) {
  CHECK_CRITICAL("nth_getTimed")
  
  nThread th= timeq->first;
  if (th==NULL) return NULL;

  timeq->first= th->nextTimeTh;

  th->timequeue= NULL;

  return th;
}

long long nth_nextTime(NthTimeQueue *timeq) {
  CHECK_CRITICAL("nth_nextTime")
  
  return timeq->first==NULL ? 0 : timeq->first->wakeTime ;
}

int nth_emptyTimeQueue(NthTimeQueue *timeq) {  
  return timeq->first==NULL;
}

int nth_delTimed(NthTimeQueue *timeq, nThread th) {
  CHECK_CRITICAL("nth_delTimed")

  nThread *pth= &timeq->first;
  while (*pth!=th && *pth!=NULL &&
         (*pth)->wakeTime-th->wakeTime<=0)
    pth= &(*pth)->nextTimeTh;

  if (*pth!=th)
    return 0;

  *pth= th->nextTimeTh;

  th->timequeue= NULL;
  return 1;
}

  
void nth_destroyTimeQueue(NthTimeQueue *timeq) {
  if (!nth_emptyTimeQueue(timeq))
    nFatalError("nth_destroyTimeQueue",
                "Destroying Time queue with pending threads\n");
  nFree(timeq);  /* No hay procesos colgando */
}

/*************************************************************
 * Stack overflow check
 *************************************************************/

// MarkStack places magic numbers at the stack base, where the program
// is not supposed to modify.  During context changes, the scheduler
// verifies that those numbers have not changed.  If they did, it is
// because there was a stack overflow.  This check is not fail proof
// because some stack overflow won't be detected, but it is better
// than nothing.

#define MAGIC1 ((void **)0x184f72a5)
#define ADDRMAGIC1 0
#define MAGIC2 ((void **)0xc923fa03)
#define ADDRMAGIC2 32

void nth_markStack(void **sp) {
  sp[ADDRMAGIC1]= MAGIC1;
  sp[ADDRMAGIC2]= MAGIC2;
}

void nth_checkStack(void **sp) {
    if (sp!=NULL) // El nMain nunca hace overflow
      if (sp[ADDRMAGIC1] != MAGIC1 || sp[ADDRMAGIC2] != MAGIC2 )
        nFatalError("nth_CheckStack",
                    "Stack overflow did happen sometime ago\n");
}
