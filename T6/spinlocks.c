#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <nthread.h>

#include "spinlocks.h"

#ifndef NTHREADS

// Implementacion de verdaderos spin-locks que esperan con busy-waiting

int swapInt(volatile int *psl, int status); // en archivo swap.s

void spinLock(volatile int *psl) {
  do {
    while (*psl==CLOSED)
      ;
  } while (swapInt(psl, CLOSED)==CLOSED);
}

void spinUnlock(int *psl) {
  *psl= OPEN;
}

#else

// Estos spin-locks son para probar la tarea con scheduling LCFS monocore.
// No tiene sentido usar busy-waiting en esta estrategia de scheduling.

static pthread_mutex_t mtx= PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond= PTHREAD_COND_INITIALIZER;

void spinLock(volatile int *psl) {
  pthread_mutex_lock(&mtx);
  while (*psl!=OPEN)
    pthread_cond_wait(&cond, &mtx);
  *psl= CLOSED;
  pthread_mutex_unlock(&mtx);
}

void spinUnlock(int *psl) {
  pthread_mutex_lock(&mtx);
  *psl= OPEN;
  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&mtx);
}

#endif
