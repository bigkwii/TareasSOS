#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "pss.h"
#include "disco.h"

// Defina aca sus variables globales
#define LOCK(m) pthread_mutex_lock(&m)
#define UNLOCK(m) pthread_mutex_unlock(&m)
#define BROADCAST(c) pthread_cond_broadcast(&c)
#define WAIT(c,m) pthread_cond_wait(&c,&m)
#define SIGNAL(c) pthread_cond_signal(&c)

#define TRUE 1
#define FALSE 0

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
  int ready;
  char * nom;
  char ** parejaNom;
  pthread_cond_t w;
} Request;

Queue *dq;
Queue *vq;

void DiscoInit(void) {
  // ... inicialice aca las variables globales ...
  dq = makeQueue();
  vq = makeQueue();
}

void DiscoDestroy(void) {
  // ... destruya las colas para liberar la memoria requerida ...
  destroyQueue(dq);
  destroyQueue(vq);
}

char *dama(char *nom) {
  // Llega
  Request * pololo;
  char * pololoNom;
  LOCK(m);
  if(!emptyQueue(vq)){
    pololo = get(vq);
    pololo->ready = TRUE;
    *pololo->parejaNom = nom;
    pololoNom = pololo->nom;
    SIGNAL(pololo->w);
    UNLOCK(m);
    return pololoNom;
  }
  Request req = {FALSE, nom, &pololoNom, PTHREAD_COND_INITIALIZER};
  put(dq, &req);
  while(!req.ready) WAIT(req.w, m);
  UNLOCK(m);
  return pololoNom;
}

char *varon(char *nom) {
  // Llega
  Request * polola;
  char * pololaNom;
  LOCK(m);
  if(!emptyQueue(dq)){
    polola = get(dq);
    polola->ready = TRUE;
    *polola->parejaNom = nom;
    pololaNom = polola->nom;
    SIGNAL(polola->w);
    UNLOCK(m);
    return pololaNom;
  }
  Request req = {FALSE, nom, &pololaNom, PTHREAD_COND_INITIALIZER};
  put(vq, &req);
  while(!req.ready) WAIT(req.w, m);
  UNLOCK(m);
  return pololaNom;
}
