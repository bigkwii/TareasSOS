#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

// Use los estados predefinidos WAIT_ACCEDER y WAIT_COMPARTIR
// El descriptor de thread incluye el campo ptr para que Ud. lo use
// a su antojo.

#define MILI_TO_NANO 1000000 // 1 millon de nanosegundos = 1 milisegundo

//... defina aca sus variables globales con el atributo static ...
static int nAcc; // numero de threads accediendo al recurso compartido
static NthQueue *q; // cola de threads esperando para acceder al recurso compartido
static nThread gTh; // puntero al thread que tiene el recurso compartido

// nth_compartirInit se invoca al partir nThreads para Ud. inicialize
// sus variables globales

void nth_compartirInit(void) {
  START_CRITICAL
  q = nth_makeQueue();
  nAcc = 0;
  END_CRITICAL
}

void nCompartir(void *ptr) {
  START_CRITICAL
  nThread thisTh= nSelf();
  thisTh->ptr = ptr;
  gTh = thisTh;
  // wake up all threads in queue
  while(!nth_emptyQueue(q)){
    nThread w = nth_getFront(q);
    if(w->status == WAIT_COMPARTIR || w->status == WAIT_ACCEDER_TIMEOUT){
      if(w->status == WAIT_ACCEDER_TIMEOUT) nth_cancelThread(w);
      setReady(w);
      schedule();
    }
  }
  suspend(WAIT_ACCEDER);
  schedule();
  gTh->ptr = NULL;
  gTh = NULL;
  END_CRITICAL
}

static void f(nThread th) {
  // programe aca la funcion que usa nth_queryThread para consultar si
  // th esta en la cola de espera de nCompartir.  Si esta presente
  // eliminela con nth_delQueue.
  // Ver funciones en nKernel/nthread-impl.h y nKernel/pss.h
  if(nth_queryThread(q, th)){
    nth_delQueue(q, th);
  }
}

void *nAcceder(int max_millis) {
  // ...  use nth_programTimer(nanos, f);  f es la funcion de mas arriba
  START_CRITICAL
  long long max_nanos = (long long) max_millis * MILI_TO_NANO;
  nAcc++;
  nThread thisTh= nSelf();

  if(gTh == NULL){
    nth_putBack(q, thisTh);
    if(max_nanos > 0){
      suspend(WAIT_ACCEDER_TIMEOUT);
      nth_programTimer(max_nanos, f);
    }else{
      suspend(WAIT_COMPARTIR);
    }
    schedule();
    if(gTh == NULL){
      nAcc--;
        END_CRITICAL
    return NULL;
    }
  }
  void * ptr = gTh->ptr;
  END_CRITICAL
  return ptr;
}

void nDevolver(void) {
  START_CRITICAL
  nAcc--;
  nThread thisTh= nSelf();
  if(gTh != NULL && gTh->status == WAIT_ACCEDER && nAcc == 0){
    setReady(gTh);
    schedule();
  }
  END_CRITICAL
}
