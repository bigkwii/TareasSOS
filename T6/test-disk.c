#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <nthread.h>

#include "disk.h"

#pragma GCC diagnostic ignored "-Wunused-function"

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

#ifndef NTHREADS

void nFatalError( char *procname, char *format, ... ) {
  va_list ap;

  fprintf(stderr,"Error Fatal en la rutina %s\n", procname);
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);

  exit(1); /* shutdown */
}

#endif

static pthread_cond_t *makeCondition() {
  pthread_cond_t *c= malloc(sizeof(pthread_cond_t));
  pthread_cond_init(c, NULL);
  return c;
}

static void destroyCondition(pthread_cond_t *c) {
  pthread_cond_destroy(c);
  free(c);
}

typedef struct {
  int entrada, salida;
} Tiempos;

static int verbose= FALSE;

// ==================================================
// Para los tests unitarios en modo non preemptive
//

static int tiempo_actual= 1; /* en centesimas de segundo */

static pthread_mutex_t t_mutex= PTHREAD_MUTEX_INITIALIZER;

static void sleep_millis(int millis) {
  usleep((long long)millis*1000);
}

static void iniciar() {
  pthread_mutex_lock(&t_mutex);
  tiempo_actual= 1;
  pthread_mutex_unlock(&t_mutex);
}

static int tiempoActual() {
  pthread_mutex_lock(&t_mutex);
  int t= tiempo_actual;
  pthread_mutex_unlock(&t_mutex);
  return t;
}

static void pausa(int tiempo_espera) { // tiempo_espera en centesimas de segundo
  pthread_mutex_lock(&t_mutex);
  int tiempo_inicio= tiempo_actual;
  pthread_mutex_unlock(&t_mutex);
  sleep_millis(tiempo_espera*300);
  pthread_mutex_lock(&t_mutex);
  tiempo_actual= tiempo_inicio+tiempo_espera;
  pthread_mutex_unlock(&t_mutex);
}

static int leyendo= FALSE;

typedef struct {
  char *nom;
  int track, tiempo_estadia;
  Tiempos *pt;
  sem_t sem;
} ArgLector;

static void *lector(void *ptr);

static pthread_t crearLector(char *nom, int track, int tiempo_estadia,
                             Tiempos *pt) {
  ArgLector args= {nom, track, tiempo_estadia, pt};
  sem_init(&args.sem, 0, 0);
  pthread_t t;
  pthread_create(&t, NULL, lector, &args);
  sem_wait(&args.sem);
  sem_destroy(&args.sem);
  return t;
}

static void *lector(void *ptr) {
  ArgLector *args= ptr;
  
  char *nom= args->nom;
  int track= args->track;
  int tiempo_estadia= args->tiempo_estadia;
  Tiempos *pt= args->pt;
  sem_post(&args->sem);

  if (verbose)
    printf("%d: %s solicita pista %d\n", tiempoActual(), nom, track);

  requestDisk(track);
  pt->entrada= tiempoActual();
  if (verbose)
    printf("%d: %s obtiene pista %d\n", pt->entrada, nom, track);

  if (leyendo)
    nFatalError("lector",
                "no se respeta exclusion mutua al acceder al disco\n");
  leyendo= TRUE;
  if (tiempo_estadia!=0)
    pausa(tiempo_estadia);
  if (verbose)
    printf("%d: %s libera disco\n", tiempoActual(), nom);
  leyendo=FALSE;

  releaseDisk(); // <-- Ud. debe programar esta funcion en nsystem/nsrc/nDisk.c
  pt->salida= tiempoActual();

  return 0;
}

#define RECUERDO "\nEste test puede fallar con una solucion correcta si\n" \
                 "el proceso no recibe la CPU durante 0.3 segundos.\n" \
                 "Intente terminar procesos que puedan gastar CPU como\n" \
                 "por ejemplo el navegador.  Si con esto no se resuelve el\n" \
                 "problema, es porque su solucion es incorrecta.\n"

static void verificar(Tiempos *pt, int entrada, int salida, char *nom) {
  if (pt->entrada!=entrada) {
    nFatalError("verificar",
         "Tiempo de entrada incorrecto de %s.  Es %d y debio ser %d.\n"
         RECUERDO,
         nom, pt->entrada, entrada);
  }
  if (pt->salida!=salida)
    nFatalError("verificar",
         "Tiempo de salida incorrecto de %s.  Es %d y debio ser %d.\n",
         RECUERDO,
         nom, pt->salida, salida);
}

static void epilogo() {
  pthread_mutex_destroy(&t_mutex);
}

static void unitTest() {
  printf("\n");
  printf("===============================\n");
  printf("Test unitarios\n");
  printf("===============================\n\n");
  // nSetTimeSlice(0);
  verbose= TRUE;

  pthread_t pedro, juan, diego, paco, maria, ana, silvia, sonia;
  Tiempos t_pedro, t_juan, t_diego, t_paco, t_maria, t_ana, t_silvia, t_sonia;

  printf("--- Primer test: un solo lector de disco ---\n");
  printf("Secuencia esperada:\n");
  printf("1: pedro solicita pista 1\n");
  printf("1: pedro obtiene pista 1\n");
  printf("2: pedro libera disco\n");
  printf("Secuencia obtenida =========>\n");
  pedro= crearLector("pedro", 1, 1, &t_pedro);
  iniciar();
  pthread_join(pedro, NULL);
  verificar(&t_pedro, 1, 2, "pedro");

  printf("--- 2do. test: 2 lectores de disco ---\n");
  printf("Secuencia esperada:\n");
  printf("1: pedro solicita pista 1\n");
  printf("1: pedro obtiene pista 1\n");
  printf("2: juan solicita pista 2\n");
  printf("3: pedro libera disco\n");
  printf("3: juan obtiene pista 2\n");
  printf("4: pedro libera disco\n");
  printf("Secuencia obtenida =========>\n");
  iniciar();
  pedro= crearLector("pedro", 1, 2, &t_pedro);
  pausa(1);
  juan= crearLector("juan", 2, 1, &t_juan);
  pthread_join(pedro, NULL);
  pthread_join(juan, NULL);
  verificar(&t_pedro, 1, 3, "pedro");
  verificar(&t_juan, 3, 4, "juan");

  printf("--- 3er. test: 3 lectores de disco ordenados ---\n");
  printf("Secuencia esperada:\n");
  printf("1: pedro solicita pista 1\n");
  printf("1: pedro obtiene pista 1\n");
  printf("2: juan solicita pista 2\n");
  printf("3: diego solicita pista 3\n");
  printf("4: pedro libera disco\n");
  printf("4: juan obtiene pista 2\n");
  printf("5: juan libera disco\n");
  printf("5: diego obtiene pista 3\n");
  printf("6: diego libera disco\n");
  printf("Secuencia obtenida =========>\n");
  iniciar();
  pedro= crearLector("pedro", 1, 3, &t_pedro);
  pausa(1);
  juan= crearLector("juan", 2, 1, &t_juan);
  pausa(1);
  diego= crearLector("diego", 3, 1, &t_diego);
  pthread_join(pedro, NULL);
  pthread_join(juan, NULL);
  pthread_join(diego, NULL);
  verificar(&t_pedro, 1, 4, "pedro");
  verificar(&t_juan, 4, 5, "juan");
  verificar(&t_diego, 5, 6, "diego");

  printf("--- 4to. test: 3 lectores de disco desordenados ---\n");
  printf("Secuencia esperada:\n");
  printf("1: pedro solicita pista 2\n");
  printf("1: pedro obtiene pista 2\n");
  printf("2: juan solicita pista 1\n");
  printf("3: diego solicita pista 3\n");
  printf("4: pedro libera disco\n");
  printf("4: diego obtiene pista 3\n");
  printf("5: diego libera disco\n");
  printf("5: juan obtiene pista 1\n");
  printf("6: juan libera disco\n");
  printf("Secuencia obtenida =========>\n");
  iniciar();
  pedro= crearLector("pedro", 2, 3, &t_pedro);
  pausa(1);
  juan= crearLector("juan", 1, 1, &t_juan);
  pausa(1);
  diego= crearLector("diego", 3, 1, &t_diego);
  pthread_join(pedro, NULL);
  pthread_join(juan, NULL);
  pthread_join(diego, NULL);
  verificar(&t_pedro, 1, 4, "pedro");
  verificar(&t_diego, 4, 5, "diego");
  verificar(&t_juan, 5, 6, "juan");

  printf("--- 5to. test: test del enunciado ---\n");
  printf("Secuencia esperada:\n");
  printf("1: pedro solicita pista 4\n");
  printf("1: pedro obtiene pista 4\n");
  printf("2: juan solicita pista 3\n");
  printf("3: ana solicita pista 6\n");
  printf("4: paco solicita pista 2\n");
  printf("5: maria solicita pista 4\n");
  printf("6: silvia solicita pista 1\n");
  printf("7: diego solicita pista 10\n");
  printf("8: pedro libera disco\n");
  printf("8: maria obtiene pista 4\n");
  printf("9: maria libera disco\n");
  printf("9: ana obtiene pista 6\n");
  printf("10: ana libera disco\n");
  printf("10: diego obtiene pista 10\n");
  printf("11: diego libera disco\n");
  printf("11: silvia obtiene pista 1\n");
  printf("12: silvia libera disco\n");
  printf("12: paco obtiene pista 2\n");
  printf("13: paco libera disco\n");
  printf("13: juan obtiene pista 3\n");
  printf("14: juan libera disco\n");
  printf("Secuencia obtenida =========>\n");
  iniciar();
  pedro= crearLector("pedro", 4, 7, &t_pedro);
  pausa(1);
  juan= crearLector("juan", 3, 1, &t_juan);
  pausa(1);
  ana= crearLector("ana", 6, 1, &t_ana);
  pausa(1);
  paco= crearLector("paco", 2, 1, &t_paco);
  pausa(1);
  maria= crearLector("maria", 4, 1, &t_maria);
  pausa(1);
  silvia= crearLector("silvia", 1, 1, &t_silvia);
  pausa(1);
  diego= crearLector("diego", 10, 1, &t_diego);
  pthread_join(pedro, NULL);
  pthread_join(juan, NULL);
  pthread_join(ana, NULL);
  pthread_join(paco, NULL);
  pthread_join(maria, NULL);
  pthread_join(silvia, NULL);
  pthread_join(diego, NULL);
  verificar(&t_pedro, 1, 8, "pedro");
  verificar(&t_maria, 8, 9, "maria");
  verificar(&t_ana, 9, 10, "ana");
  verificar(&t_diego, 10, 11, "diego");
  verificar(&t_silvia, 11, 12, "silvia");
  verificar(&t_paco, 12, 13, "paco");
  verificar(&t_juan, 13, 14, "juan");
 
  printf("--- 6to. test: 8 lectores  ---\n");
  printf("Secuencia esperada:\n");
  printf("1: pedro solicita pista 5\n");
  printf("1: pedro obtiene pista 5    (Trk=4)\n");
  printf("2: juan solicita pista 3    (Trk=4 Q=juan:3)\n");
  printf("3: ana solicita pista 7     (Trk=4 Q=ana:7 juan:3)\n");
  printf("4: paco solicita pista 1    (Trk=4 Q=ana:7 paco:1 juan:3)\n");
  printf("5: pedro libera disco\n");
  printf("5: ana obtiene pista 7      (Trk=7 Q=paco:1 juan:3)\n");
  printf("6: maria solicita pista 8   (Trk=7 Q=maria:8 paco:1 juan:3)\n");
  printf("7: silvia solicita pista 2  (Trk=7 Q=maria:8 paco:1 silvia:2 juan:3)\n");
  printf("8: ana libera disco\n");
  printf("8: maria obtiene pista 8    (Trk=8 Q=paco:1 silvia:2 j:3)\n");
  printf("9: maria libera disco\n");
  printf("9: paco obtiene pista 1\n");
  printf("10: diego solicita pista 4  (Trk=1 Q=silvia:2 juan:3 diego:4)\n");
  printf("11: sonia solicita pista 6  (Trk=1 Q=silvia:2 juan:3 diego:4 sonia:6)\n");
  printf("12: paco libera disco\n");
  printf("12: silvia obtiene pista 2  (Trk=2 Q=juan:3 diego:4 sonia:6)\n");
  printf("13: silvia libera disco\n");
  printf("13: juan obtiene pista 3    (Trk=3 diego:4 sonia:6)\n");
  printf("14: juan libera disco\n");
  printf("14: diego obtiene pista 4   (Trk=4 sonia:6)\n");
  printf("15: diego libera disco\n");
  printf("15: sonia obtiene pista 6   (Trk=6 Q=)\n");
  printf("16: sonia libera disco\n");
  printf("Secuencia obtenida =========>\n");
  iniciar();
  pedro= crearLector("pedro", 5, 4, &t_pedro);
  pausa(1);
  juan= crearLector("juan", 3, 1, &t_juan);
  pausa(1);
  ana= crearLector("ana", 7, 3, &t_ana);
  pausa(1);
  paco= crearLector("paco", 1, 3, &t_paco);
  pausa(2);
  maria= crearLector("maria", 8, 1, &t_maria);
  pausa(1);
  silvia= crearLector("silvia", 2, 1, &t_silvia);
  pausa(3);
  diego= crearLector("diego", 4, 1, &t_diego);
  pausa(1);
  sonia= crearLector("sonia", 6, 1, &t_sonia);
  pthread_join(pedro, NULL);
  pthread_join(juan, NULL);
  pthread_join(ana, NULL);
  pthread_join(paco, NULL);
  pthread_join(maria, NULL);
  pthread_join(silvia, NULL);
  pthread_join(diego, NULL);
  pthread_join(sonia, NULL);
  verificar(&t_pedro, 1, 5, "pedro");
  verificar(&t_ana, 5, 8, "ana");
  verificar(&t_maria, 8, 9, "maria");
  verificar(&t_paco, 9, 12, "paco");
  verificar(&t_silvia, 12, 13, "silvia");
  verificar(&t_juan, 13, 14, "juan");
  verificar(&t_diego, 14, 15, "diego");
  verificar(&t_sonia, 15, 16, "sonia");

  epilogo();
  printf("Aprobado\n");
}

// ==================================================
// Para el test de robustez en modo non preemptive
//

#define NTASKS 20
#define CICLOS 100
#define PBBRELEASE 0.52

static pthread_mutex_t mutex= PTHREAD_MUTEX_INITIALIZER;
static int pos= 0;
static int solicitados[NTASKS];
static int ocupado= FALSE;

static pthread_t tasks[NTASKS];
static char *nombres[NTASKS];
static pthread_cond_t *pends[NTASKS];
static int suspend[NTASKS];
static int noterminados= NTASKS, corriendo= 0, solicitudes= 0;

static int largoCola() {
  int len= 0;
  for (int track=0; track<NTASKS; track++) {
    if (solicitados[track] && pends[track]!=NULL)
      len++;
  }
  return len;
}

static int solicitando(int track) {
  if (verbose)
    printf("Solicitud de %s, largo de la cola: %d\n",
            nombres[track], largoCola());
  // pthread_mutex_lock(&mutex);
  int ocup= ocupado;
  solicitados[track]= TRUE;
  solicitudes++;
  corriendo--;
  // pthread_mutex_unlock(&mutex);
  return ocup;
}

static void otorgado(int track) {
  if (verbose)
    printf("Otorgado a %s\n", nombres[track]);
  // pthread_mutex_lock(&mutex);
  if (ocupado) 
    nFatalError("otorgado", "Se otorga a %d y a %d simultaneamente\n",
                track, pos);
  ocupado= TRUE;
  int i= pos;
  while (i!=track) {
    if (solicitados[i])
      nFatalError("otorgado", "Estando en %d se otorgo erradamente a %d, "
           "debio ser %d\n", pos, track, i);
    i= (i+1)%NTASKS;
  }
  pos= track;
  solicitados[track]= FALSE;
  corriendo++;
  // pthread_mutex_unlock(&mutex);
}

static void liberando(int track) {
  if (verbose)
    printf("Liberado por %s\n", nombres[track]);
  // pthread_mutex_lock(&mutex);
  ocupado= FALSE;
  // pthread_mutex_unlock(&mutex);
}

static int nticks= 0;
static long long sumlen= 0;

static void tick(int track) {
  pthread_mutex_lock(&mutex);
  nticks++;
  int len= largoCola();
  sumlen+= len;
  if (track>=0 && pends[track]!=NULL) {
    suspend[track]= TRUE;
  }
  if (corriendo<=1) {
    int r= random();
    double p= (double)(r>>10) / (double)((1<<21));
    int activo, prim;
    if (ocupado && suspend[pos] && p<PBBRELEASE) {
      prim= activo= pos;
    }
    else {
      activo= (r>>1)%NTASKS;
      prim= activo;
      while (activo<NTASKS && (pends[activo]==NULL || !suspend[activo]))
        activo++;
      if (activo==NTASKS)
        activo= 0;
      while (activo<prim && (pends[activo]==NULL || !suspend[activo]))
        activo++;
    }
    if (pends[activo]!=NULL && suspend[activo]) {
      if (verbose)
        printf("tick prim= %d, activo= %d, len= %d\n", prim, activo, len);
      suspend[activo]= FALSE;
      corriendo++;
      pthread_cond_signal(pends[activo]);
    }
    else if (noterminados!=0 && verbose)
      printf("tick prim= %d, sin activo, len= %d, threads=%d\n",
              prim, len, noterminados);
  }
  if (track>=0 && pends[track]!=NULL) {
    corriendo--;
    while (suspend[track]) {
      if (verbose)
        printf("Suspendiendo %d\n", track);
      pthread_cond_wait(pends[track], &mutex);
    }
  }
  pthread_mutex_unlock(&mutex);
}

typedef struct {
  char *nom;
  int track, ciclos;
  sem_t sem;
} ArgLector23;

static void *lector2(void *ptr);

static pthread_t crearLector2(char *nom, int track, int ciclos) {
  ArgLector23 args= {nom, track, ciclos};
  sem_init(&args.sem, 0, 0);
  pthread_t t;
  pthread_create(&t, NULL, lector2, &args);
  sem_wait(&args.sem);
  sem_destroy(&args.sem);
  return t;
}

static void *lector2(void *ptr) {
  ArgLector23 *args= ptr;
  char *nom= args->nom;
  int track= args->track;
  int ciclos= args->ciclos;
  sem_post(&args->sem);

  // nSetTaskName(nom);
  pends[track]= makeCondition();

  pthread_mutex_lock(&mutex);
  suspend[track]= TRUE;
  while (suspend[track]) {
    pthread_cond_wait(pends[track], &mutex);
  }
  pthread_mutex_unlock(&mutex);

  int i= 0;
  for (;;) {
    int msg= FALSE;
    i++;
    solicitando(track);
    requestDisk(track);
    otorgado(track);
    tick(track);
    if (strcmp(nom, "lector 0")==0 && i%(ciclos/10)==0)
      msg= TRUE;
    liberando(track);
    releaseDisk();
    if (i >= ciclos)
      break;
    tick(track);
    if (msg)
      printf("%d iteraciones completadas en lector 0 de %d\n", i, ciclos);
  }

  pthread_mutex_lock(&mutex);
  destroyCondition(pends[track]);
  pends[track]= NULL;
  noterminados--;
  corriendo--;
  pthread_mutex_unlock(&mutex);

  tick(-1);

  return 0;
}

#if 0
static int espia() {
  while (noterminados>0) {
    sleep_millis(500);
    // pthread_mutex_lock(&mutex);
    int len= largoCola();
    // pthread_mutex_unlock(&mutex);
    printf("Largo de la cola de espera del disco: %d\n", len);
  }
  return 0;
}
#endif

static void nonPreemptiveTest() {
  printf("\n");
  printf("===============================\n");
  printf("Test de robustez non preemptive\n");
  printf("Verifica exclusion mutua y orden\n");
  printf("%d lectores de disco\n", NTASKS);
  printf("===============================\n\n");
  // nSetTimeSlice(0);
  verbose= FALSE;
  // srandom(1000);
  for (int track= 0; track<NTASKS; track++) {
    char *nom= malloc(20);
    sprintf(nom, "lector %d", track);
    nombres[track]= nom;
    tasks[track]= crearLector2(nom, track, CICLOS);
  }
  sleep(1);
  // pthread_t lenTask= crearEspia();
  while (noterminados>0)
    tick(-1);
  for (int track= 0; track<NTASKS; track++) {
    pthread_join(tasks[track], NULL);
    free(nombres[track]);
  }
  // pthread_join(lenTask, NULL);
  printf("Nro. de threads: %d\n", NTASKS);
  printf("Total solicitudes: %d\n", solicitudes);
  printf("Largo promedio de la cola del disco: %f\n", (double)sumlen/nticks);
  // mutex= NULL;
  printf("Aprobado\n");
}

// ==================================================
// Para el test de robustez en modo preemptive
//

static void *lector3(void *ptr);

static pthread_t crearLector3(char *nom, int track, int ciclos) {
  ArgLector23 args= {nom, track, ciclos};
  sem_init(&args.sem, 0, 0);
  pthread_t t;
  pthread_create(&t, NULL, lector3, &args);
  sem_wait(&args.sem);
  sem_destroy(&args.sem);
  return t;
}

static void *lector3(void *ptr) {
  ArgLector23 *args= ptr;
  char *nom= args->nom;
  int track= args->track;
  int ciclos= args->ciclos;
  sem_post(&args->sem);

  // nSetTaskName(nom);
  long long sum= 0;

  for (int i= 1; i<=ciclos; i++) {
    int msg= FALSE;

    requestDisk(track);

    if (leyendo)
      nFatalError("lector",
                  "no se respeta exclusion mutua al acceder al disco\n");
    leyendo= TRUE;
    if (strcmp(nom, "lector 0")==0 && i%(ciclos/10)==0)
      msg= TRUE;
    int r= random()%100000;
    for (int j= 0; j<r; j++)
      sum += j;

    leyendo=FALSE;

    releaseDisk(); // <-- Ud. debe programar esta funcion
    if (msg)
      printf("%d iteraciones completadas en lector 0 de %d\n", i, ciclos);
  }

  return (void*)(intptr_t)sum;
}

static void preemptiveTest() {
  printf("\n");
  printf("===============================\n");
  printf("Test de robustez preemptive\n");
  printf("Solo verifica la exclusion mutua\n");
  printf("%d lectores de disco\n", NTASKS);
  printf("===============================\n\n");
  // nSetTimeSlice(1);
  verbose= FALSE;
  int ciclos= CICLOS;
  for (int track= 0; track<NTASKS; track++) {
    char *nom= malloc(20);
    sprintf(nom, "lector %d", track);
    nombres[track]= nom;
    tasks[track]= crearLector3(nom, track, ciclos);
  }
  for (int track= 0; track<NTASKS; track++) {
    pthread_join(tasks[track], NULL);
    free(nombres[track]);
  }
  printf("Aprobado\n");
}

int main() {
  iniDisk();
#if 1
  unitTest();
  preemptiveTest();
#endif

#ifdef NTHREADS
  nonPreemptiveTest();
#endif

  printf("Felicitaciones: paso todos los tests.\n");

  return 0;
}
