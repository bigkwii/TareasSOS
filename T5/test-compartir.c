#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <nthread.h>

#pragma GCC diagnostic ignored "-Wunused-function"

#define TRUE 1
#define FALSE 0

// ----------------------------------------------------
// Funcion que entrega el tiempo transcurrido desde el lanzamiento del
// programa en milisegundos

static int time0= 0;

void resetTime() {
  time0= nGetTime();
}

int getRelTime() {
  return nGetTime()-time0;
}

// ----------------------------------------------------
// Funciones para verificar la tarea

static char *p= "p";
static char *q= "q";
static char *r= "r";
static int terminar= 0;
static int llamadas= 0;

static int timeout_acceder= -1;

static void *accede_fun(void *ptr) {
  if (ptr==NULL)
    sleep(1);
  char *res= nAcceder(timeout_acceder);
  if (res!=r) {
    fprintf(stderr, "Se debio haber recibido %s\n", r);
    exit(1);
  }
  nDevolver();
  return NULL;
}

static void *t34fun(void *ptr) {
  char *nom= ptr;
  printf("%5d: %s solicita acceder\n", getRelTime(), nom);
  char *res= nAcceder(timeout_acceder);
  if (res!=p) {
    fprintf(stderr, "%s debio haber recibido %s\n", nom, p);
    exit(1);
  }
  printf("%5d: %s accede a %s\n", getRelTime(), nom, res);
  sleep(strcmp(nom, "T4")==0 ? 2 : 4);
  printf("%5d: %s devuelve\n", getRelTime(), nom);
  nDevolver();
  return NULL;
}

static void *t2fun(void *ptr) {
  char *nom= ptr;
  printf("%5d: %s solicita acceder\n", getRelTime(), nom);
  char *res= nAcceder(timeout_acceder);
  if (res!=p) {
    fprintf(stderr, "%s debio haber recibido %s\n", nom, p);
    exit(1);
  }
  printf("%5d: %s accede a %s\n", getRelTime(), nom, res);
  sleep(2);
  printf("%5d: %s devuelve\n", getRelTime(), nom);
  nDevolver();
  sleep(5);
  printf("%5d: %s solicita acceder\n", getRelTime(), nom);
  res= nAcceder(timeout_acceder);
  if (res!=q) {
    fprintf(stderr, "%s debio haber recibido %s\n", nom, q);
    exit(1);
  }
  printf("%5d: %s accede a %s\n", getRelTime(), nom, res);
  sleep(1);
  printf("%5d: %s devuelve\n", getRelTime(), nom);
  nDevolver();
  return NULL;
}

static void *t1fun(void *ptr) {
  printf("%5d: T1 solicita compartir %s\n", getRelTime(), p);
  nCompartir(p);
  printf("%5d: T1 termina de compartir %s\n", getRelTime(), p);
  sleep(2);
  printf("%5d: T1 solicita compartir %s\n", getRelTime(), q);
  nCompartir(q);
  printf("%5d: T1 termina de compartir %s\n", getRelTime(), q);
  return NULL;
}

static void *comparte_fun(void *ptr) {
  while (!terminar) {
    nCompartir(r);
    llamadas++;
  }
  return NULL;
}

static void *acceden_fun(void *ptr) {
  int m= *(int*)ptr;
  int i;
  for (i= 0; i<m; i++) {
    char *res= nAcceder(timeout_acceder);
    if (res!=r) {
      fprintf(stderr, "se debio haber recibido %s\n", r);
      exit(1);
    }
    nDevolver();
  }
  return NULL;
}

// ----------------------------------------------------
// Funciones para verificar los timeouts

int verbose= TRUE;

typedef struct {
  int idle, busy, timeout;
  int id, tAcceder, tCompartir, tDevolver;
} Arg;

/*  accedeProc:
 *  Espera idle milisegundos
 *  Invoca nAcceder(timeout)
 *  Espera busy milisegundos
 *  Invoca nDevolver()
 */
static void *accederProc(void *ptr) {
  Arg *parg= ptr;

  int idle= parg->idle;
  int timeout= parg->timeout;
  int busy= parg->busy;

  int id= parg->id;

  nSleepMillis(idle);

  parg->tAcceder= getRelTime();
  if (verbose)
    printf("T=%d llamada a nAcceder(%d) id=%d\n", parg->tAcceder, timeout, id);

  char *share_data= nAcceder(timeout);
  parg->tCompartir= getRelTime();
  if (verbose)
    printf("T=%d nAcceder id=%d retorna %s\n", parg->tCompartir, id,
           share_data==NULL ? "con timeout vencido" : "");
  if (share_data==NULL)
    return share_data;    /* Timeout vencido */

  nSleepMillis(busy);
  parg->tDevolver= getRelTime();
  if (verbose)
    printf("T=%d nDevolver id=%d\n", parg->tDevolver, id);

  nDevolver();

  return share_data;
}

void testCompartir(int tCompartir, Arg args[], int n) {
  int share_data;
  resetTime();
  nThread threads[n];
  if (verbose)
    printf("T=0 Creando %d threads\n", n);
  for (int i= 0; i<n; i++) {
    args[i].id= i;
    pthread_create(&threads[i], NULL, accederProc, &args[i]);
  }

  int termino= -1;
  int ultimoDevolver= 0;
  if (tCompartir>=0) {
    nSleepMillis(tCompartir);
    if (verbose)
      printf("T=%d llamando a nCompartir\n", getRelTime());
    nCompartir(&share_data);
    termino= getRelTime();
    ultimoDevolver= 0;
    if (verbose)
      printf("T=%d nCompartir retorna\n", termino);
  }

  for (int i= 0; i<n; i++) {
    void *res;
    if (verbose)
      printf("T=%d esperando thread %d\n", getRelTime(), i);
    pthread_join(threads[i], &res);
    if (verbose)
      printf("T=%d thread %d termina\n", getRelTime(), i);
    if (tCompartir>=0 && args[i].timeout+args[i].idle > tCompartir) {
      if (res != &share_data)
        nFatalError("testCompartir",
                    "Thread %d: nAcceder retorna valor erroneo\n", i);
      if (args[i].tCompartir<tCompartir)
        nFatalError("testCompartir",
                    "Thread %d: nAcceder retorna antes\n", i);
      if (args[i].tDevolver > ultimoDevolver)
        ultimoDevolver= args[i].tDevolver;
      if (args[i].tCompartir>tCompartir+50)
        nFatalError("testCompartir",
                    "Thread %d: nAcceder retorna demasiado tarde\n", i);
    }
    else {
      if (res != NULL)
        nFatalError("testCompartir",
                 "Thread %d: nAcceder reporta que no se vencio timeout\n", i);
      if (args[i].tCompartir < args[i].timeout+args[i].idle)
         nFatalError("testCompartir",
                     "Thread %d: nAcceder retorna antes\n", i);
      if (args[i].tCompartir > args[i].timeout+args[i].idle + 50)
         nFatalError("testCompartir",
                     "Thread %d: nAcceder retorna demasiado tarde\n", i);
    }
  }

  if (tCompartir>=0 && termino > ultimoDevolver+50)
    nFatalError("testCompartir",
                "nCompartir retorna demasiado tarde\n");
}

void testTarea4(void) {
  terminar= 0;
  llamadas= 0;
  /*** Un primer test **************************************************/

  {
    printf("Primer test: se invoca 1 nAcceder despues de nCompartir\n");
    pthread_t accede;
    pthread_create(&accede, 0, accede_fun, NULL);
    nCompartir(r);
    pthread_join(accede, NULL);
    printf("Primer test exitoso\n");
  }

  /*** Un segundo test *************************************************/

  {
    printf("2do. test: se invoca 1 nAcceder antes que nCompartir\n");
    pthread_t accede;
    pthread_create(&accede, 0, accede_fun, &accede);
    sleep(1);
    nCompartir(r);
    pthread_join(accede, NULL);
    printf("2do. test exitoso\n");
  }

  /*** El test del enunciado *******************************************/

  resetTime();
  printf("El test del enunciado\n");
  pthread_t t1, t2, t3, t4;
  printf("%5d: lanzando T4\n", getRelTime());
  pthread_create(&t4, 0, t34fun, "T4");
  sleep(1);
  printf("%5d: lanzando T3\n", getRelTime());
  pthread_create(&t3, 0, t34fun, "T3");
  sleep(2);
  printf("%5d: lanzando T1\n", getRelTime());
  pthread_create(&t1, 0, t1fun, "T1");
  sleep(1);
  printf("%5d: lanzando T2\n", getRelTime());
  pthread_create(&t2, 0, t2fun, "T2");
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  pthread_join(t3, NULL);
  pthread_join(t4, NULL);
  int dif= getRelTime()-12000;
  if (dif<0)
    dif= -dif;
  if (dif>500) {
    fprintf(stderr, "El tiempo total debio haber sido 12 segundos,\n");
    fprintf(stderr, "la diferencia es de %d milisegundos\n", dif);
    exit(1);
  }
  printf("Test del enunciado exitoso\n");
  fflush(stdout);

  /*** El test de esfuerzo *********************************************/

  #define N 200
  resetTime();
  int m= 30000; /* numero de solicitudes a acceder por cada thread */
  printf("\nEl test de esfuerzo: se crea 1 thread que comparte 'y'.\n");
  printf("%d threads que acceden %d veces.  No debe ocurrir un deadlock.\n",
          N, m);
  printf("Tomo ~ 4 segundos en mi ryzen 5 3550H\n");
  pthread_t comparte, acceden[N];
  pthread_create(&comparte, 0, comparte_fun, NULL);
  int k;
  for (k= 0; k<N; k++)
    pthread_create(&acceden[k], 0, acceden_fun, &m);
  for (k= 0; k<N; k++)
    pthread_join(acceden[k], NULL);
  nAcceder(-1);
  terminar= 1;
  nDevolver();
  pthread_join(comparte, NULL);
  printf("Test de esfuerzo exitoso en %d segundos\n", (getRelTime()+500)/1000);
  printf("Se hicieron %d llamadas a nCompartir\n", llamadas);
}

int main() {

#if 1
  printf("Los mismos testeos de la tarea 4, sin timeout\n");
  timeout_acceder= -1;
  testTarea4();
#endif
 
  printf("\nLos mismos testeos de la tarea 4, con timeout que no se vence\n");
  timeout_acceder= 1000000; // mil segundos
  // timeout_acceder= -1; // mil segundos
  testTarea4();
 
  printf("\nNuevos testeos para timeouts que si se vencen\n");
  verbose= TRUE;
  printf("nAcceder con timeout que se vence\n");
  Arg args1[]={ {0, 0, 100} };
  testCompartir(-1, args1, 1);
  printf("Ok\n");

  printf("2 nAcceder con timeout que se vencen\n");
  Arg args2[]={ {100, 0, 100}, {0, 0, 300} };
  testCompartir(-1, args2, 2);
  printf("Ok\n");

  printf("3 nAcceder con timeout, 2 se vencen, uno no\n");
  Arg args3[]={ {200, 0, 100}, {100, 0, 300}, {0, 100, 600} };
  testCompartir(500, args3, 3);
  printf("Ok\n");

  printf("5 nAcceder con timeout, 3 se vencen, 2 no\n");
  Arg args4[]={ {300, 0, 400},    // vencido, nAcceder retorna en T= 700
                {100, 100, 900},  // no vencido
                {200, 0, 400},    // vencido, nAcceder retorna en T= 600
                {0,   200, 1000}, // no vencido
                {400, 0, 400} };  // vencido, nAcceder retorna en T= 800
  testCompartir(900, args4, 5);   // llama a nCompartir en T=900
  printf("Ok\n");

  printf("\nFelicitaciones: su tarea funciona correctamente\n");

  return 0;
}
