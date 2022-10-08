#define _POSIX_C_SOURCE 200809L

#include "nthread-impl.h"
#include "nSystem.h"

/*************************************************************
 * Creacion y manejo de tareas
 *************************************************************/

typedef struct InfoEmit {
  va_list ap;
  int (*proc)();
  nMutex m;
  nCond c;
  int  rdy;
}
  InfoEmit;

static void *TaskInit(void *info);

nTask nEmitTask( int (*proc)(), ... ) {
  /* (un procedimiento puede declarar mas argumentos que la cantidad
   * de argumentos con que es llamado)
   */
  nTask new_task, this_task;
  InfoEmit info;
  nMutexInit(&info.m, NULL);
  nCondInit(&info.c, NULL);
  info.rdy= 0;
  
  va_list ap;

  /* Los argumentos y el procedimiento se pasaran a TaskInit en info */
  va_start(ap, proc);

  // Modificacion para AMD64. Por Francisco Cifuentes
  va_copy(info.ap, ap);
  info.proc= proc;
  nLock(&info.m);

  nThread th;
  nThreadCreate(&th, NULL, TaskInit, &info);

  while (!info.rdy)
    nCondWait(&info.c, &info.m);
  nUnlock(&info.m);
  
  va_end(ap);

  return th;
}

__attribute__((no_sanitize_address))
static void *TaskInit( void *ptr ) {
  InfoEmit *pinfo= ptr;
  int (*proc)()= pinfo->proc;
  long long  a0= va_arg(pinfo->ap, long long);
  long long  a1= va_arg(pinfo->ap, long long);
  long long  a2= va_arg(pinfo->ap, long long);
  long long  a3= va_arg(pinfo->ap, long long);
  long long  a4= va_arg(pinfo->ap, long long);
  long long  a5= va_arg(pinfo->ap, long long);
  long long  a6= va_arg(pinfo->ap, long long);
  long long  a7= va_arg(pinfo->ap, long long);
  long long  a8= va_arg(pinfo->ap, long long);
  long long  a9= va_arg(pinfo->ap, long long);
  long long a10= va_arg(pinfo->ap, long long);
  long long a11= va_arg(pinfo->ap, long long);
  long long a12= va_arg(pinfo->ap, long long);
  long long a13= va_arg(pinfo->ap, long long);
  // soporta hasta 14 argumentos enteros (o 7 punteros de 64 bits)
  nLock(&pinfo->m);
  pinfo->rdy= 1;
  nCondSignal(&pinfo->c);
  nUnlock(&pinfo->m);

  // Llama el procedimiento raiz de la tarea
  intptr_t rc= (*proc)(a0, a1, a2, a3, a4, a5, a6, a7, a8
#if 0
                       , a9, a10, a11, a12, a13
#endif
                      );
  return (void*)rc;
}
                              // Crea una nueva tarea */
void nExitTask(int rc) {      // Termina la tarea que la invoca */
  nThreadExit((void*)(intptr_t)rc);
}

int nWaitTask(nTask task) {   // Espera el termino de otra tarea */
  void *ptr;
  nJoin(task, &ptr);
  return (intptr_t)ptr;
}


// void nExitSystem(int rc);     // Termina todas las tareas (el proceso Unix)

/*************************************************************
 * Definicion de parametros para las tareas
 *************************************************************/

// int  nSetStackSize(int size);  // Taman~o de stack para las nuevas tareas */
// void nSetTimeSlice(int slice); // Taman~o de la tajada (en ms)
void nSetTaskName(char *format, ... ); // Util para debugging

// nTask nCurrentTask(void);      // El identificador de la tarea actual
char* nGetTaskName(void );     // El nombre de esta tarea
int nGetContextSwitches(void) {// Cambos de contexto implicitos
  return nth_implicitContextChanges;
}

int nGetQueueLength() {        // Numero de procesos en ready queue
  START_CRITICAL
  fprintf(stderr, "Info: nGetQueueLength not implemented\n");
  return 0;
}

/*************************************************************
 * Mensajes
 *************************************************************/

// int nSend(nTask task, void *msg); // Envia un mensaje a una tarea
// void *nReceive(nTask *ptask, int max_delay);
                                  // Recepcion de un mensaje
// void nReply(nTask task, int rc);  // Responde un mensaje

// void nSleep(int delay);        // Suspende el proceso por delay milisecs
// int nGetTime(void);            // La hora en milisegundos y modulo 'maxint'

/*************************************************************
 * Semaforos
 *************************************************************/

struct sem *nMakeSem(int count) {     // Construye un semaforo
  struct sem *sem= nMalloc(sizeof(struct sem));
  nSemInit(sem, 0, count);
  return sem;
}

// int nWaitSem(struct sem *sem);     // Operacion Wait
// int nSignalSem(struct sem *sem);   // Operacion Signal

void nDestroySem(struct sem *sem) {   // Destruye un semaforo
  nSemDestroy(sem);
  nFree(sem);
}

/*************************************************************
 * Monitores
 * Cuidado! Estos monitores no son reentrantes
 *************************************************************/
 
nMonitor nMakeMonitor() {             // Construye un monitor
  nMutex *m= nMalloc(sizeof(nMutex));
  nMutexInit(m, NULL);
  m->cond= nMakeCondition(m);
  return m;
}

void nDestroyMonitor(nMonitor mon) {  // Destruye un monitor
  START_CRITICAL
  free(mon->cond);
  free(mon);
  END_CRITICAL
}

// void nEnter(nMonitor mon)          // Ingreso al monitor
// void nExit(nMonitor mon);          // Salida del monitor
void nWait(nMonitor mon) {            // Libera el monitor y suspende
  nCondWait(mon->cond, mon);
}

void nNotifyAll(nMonitor mon) {       // Retoma tareas suspendidas
  nCondBroadcast(mon->cond);
}
  
nCondition nMakeCondition(nMonitor mon) { // Construye una condicion
  nCondition cond= nMalloc(sizeof(*cond));
  nCondInit(cond, NULL);
  cond->m= mon;
  return cond;
}

void nDestroyCondition(nCondition cond) { // Destruye una condicion
  nFree(cond);
}

void nWaitCondition(nCondition cond) {    // operacion Wait
  nCondWait(cond, cond->m);
}

// void nSignalCondition(nCondition cond);  // operacion Signal

/*************************************************************
 * E/S basica
 *************************************************************/

/* Estas funciones son equivalentes a open, close, read y write en
 * Unix.  Las 'nano' funciones son no bloqueantes para el proceso Unix,
 * solo bloquean la tarea que las invoca.
 */

#if 0
int nOpen( char *path, int flags, ... );    // Abre un archivo
int nClose(int fd);                         // Cierra un archivo
int nRead(int fd, char *buf, int nbyte);    // Lee de un archivo
int nWrite(int fd, char *buf, int nbyte);   // Escribe en un archivo
#endif

/* 
 * Estas funciones se pueden usar en caso de necesitar que
 * la E/S estandar sea no bloqueante (ver examples1/iotest.c).
 * (En algunos casos como curses, es inevitable que sea bloqueante).
 */

#if 0
void nReopenStdio(void);         // Reabre la E/S estandar
void nSetNonBlockingStdio(void); // Coloca en modo no bloqueante la E/S std.
#endif

/*************************************************************
 * Los servicios
 *************************************************************/

int nFprintf( int fd, char *format, ... ) {

  START_CRITICAL

  va_list ap;
  va_start(ap, format);
  int rc= vdprintf(fd, format, ap);
  va_end(ap);

  END_CRITICAL
  
  return rc;
}
  
// int nPrintf( char *format, ... );
// void nFatalError( char *procname, char *format, ... );

// void *nMalloc(int size);
// void nFree(void *ptr);

/*************************************************************
 * Constantes varias
 *************************************************************/

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef EOF
#define EOF (-1)
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define nAssert(a, msg) if (!a) { nFatalError("Assertion failure", msg); } else ;
