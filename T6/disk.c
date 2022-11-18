#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "disk.h"
#include "priqueue.h"
#include "spinlocks.h"

// Le sera de ayuda la clase sobre semáforos:
// https://www.u-cursos.cl/ingenieria/2022/2/CC4302/1/novedades/detalle?id=431689
// Le serviran la solucion del productor/consumidor resuelto con el patron
// request y la solucion de los lectores/escritores, tambien resuelto con
// el patron request.  Puede substituir los semaforos de esas soluciones
// por spin-locks, porque esos semaforos almacenan a lo mas una sola ficha.

// usando c-scan o el elevator algorithm

// Declare los tipos que necesite
// ...
// estados 
#define TRUE 1
#define FALSE 0
enum {IDLE, BUSY}; 

// Declare aca las variables globales que necesite
// ...
int mutex;
static int state;
static int currentT;
static PriQueue *qUp;
static PriQueue *qDown;

// Agregue aca las funciones requestDisk y releaseDisk

void iniDisk(void) {
  // ...
  mutex = OPEN;
  state = IDLE;
  currentT = 0;
  qUp = makePriQueue();
  qDown = makePriQueue();
}

void requestDisk(int track) {
  // ...
  spinLock(&mutex);
  if (state != IDLE){
    int w = CLOSED; // wait spinlock
    if (track >= currentT){
      priPut(qUp, &w, track);
    } else {
      priPut(qDown, &w, track);
    }
    spinUnlock(&mutex);
    spinLock(&w);
    return;
  }
  state = BUSY;
  currentT = track;
  spinUnlock(&mutex);
  return;
}

void releaseDisk() {
  // ...
  spinLock(&mutex);
  if(!emptyPriQueue(qUp)){
    currentT = priBest(qUp);
    int *w = priGet(qUp);
    spinUnlock(w);
  } else if(!emptyPriQueue(qDown)){
    currentT = 0;
    // put them all in qUp
    while(!emptyPriQueue(qDown)){
      int w_track = priBest(qDown); // track of the node
      int *w_ = priGet(qDown); // wait spinlock of the node
      priPut(qUp, w_, w_track);
    }
    currentT = priBest(qUp);
    int *w = priGet(qUp);
    spinUnlock(w);
  } else {
    state = IDLE;
    currentT = 0;
  }
  spinUnlock(&mutex);
  return;
}
