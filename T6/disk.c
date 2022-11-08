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


// Declare los tipos que necesite
// ...
// Buffer
typedef struct {
  int * lastT;
  PriQueue *qUp;
  PriQueue *qDown;
  int * mutex;
} Buffer;

// Declare aca las variables globales que necesite
// ...
int mutex; // spinLock = wait, spinUnlock = post
PriQueue * qUp;
PriQueue * qDown;
static int lastT;
Buffer * buffer;

// Agregue aca las funciones requestDisk y releaseDisk

void iniDisk(void) {
  // ...
  buffer = malloc(sizeof(Buffer));
  buffer->lastT = &lastT;
  buffer->qUp = qUp;
  buffer->qDown = qDown;
  buffer->mutex = &mutex;

  qUp = makePriQueue();
  qDown = makePriQueue();
  mutex = OPEN;
}

void requestDisk(int track) {
  // ...
  spinLock(buffer->mutex);
  if (track > *(buffer->lastT)) {
    priPut(buffer->qUp, (void *)&track, track);
  } else {
    priPut(buffer->qDown, (void *)&track, track);
  }
  spinUnlock(buffer->mutex);
}

void releaseDisk() {
  // ...
  spinLock(buffer->mutex);
  if (emptyPriQueue(buffer->qUp)) {
    *(buffer->lastT) = priBest(buffer->qDown);
    priGet(buffer->qDown);
  } else {
    *(buffer->lastT) = priBest(buffer->qUp);
    priGet(buffer->qUp);
  }
  spinUnlock(buffer->mutex);
}
