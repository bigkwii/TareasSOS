#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <nthread.h>

// ----------------------------------------------------
// Funcion que entrega el tiempo transcurrido desde el lanzamiento del
// programa en milisegundos

static int time0= 0;

static int getTime0() {
    struct timeval Timeval;
    gettimeofday(&Timeval, NULL);
    return Timeval.tv_sec*1000+Timeval.tv_usec/1000;
}

void resetTime() {
  time0= getTime0();
}

int getTime() {
  return getTime0()-time0;
}

// ----------------------------------------------------
// Funciones para verificar la tarea

static char *p= "p";
static char *q= "q";
static char *r= "r";
static int terminar= 0;
static int llamadas= 0;

static void *accede_fun(void *ptr) {
  if (ptr==NULL)
    sleep(1);
  char *res= nAcceder(-1);
  if (res!=r) {
    fprintf(stderr, "Se debio haber recibido %s\n", r);
    exit(1);
  }
  nDevolver();
  return NULL;
}

static void *t34fun(void *ptr) {
  char *nom= ptr;
  printf("%5d: %s solicita acceder\n", getTime(), nom);
  char *res= nAcceder(-1);
  if (res!=p) {
    fprintf(stderr, "%s debio haber recibido %s\n", nom, p);
    exit(1);
  }
  printf("%5d: %s accede a %s\n", getTime(), nom, res);
  sleep(strcmp(nom, "T4")==0 ? 2 : 4);
  printf("%5d: %s devuelve\n", getTime(), nom);
  nDevolver();
  return NULL;
}

static void *t2fun(void *ptr) {
  char *nom= ptr;
  printf("%5d: %s solicita acceder\n", getTime(), nom);
  char *res= nAcceder(-1);
  if (res!=p) {
    fprintf(stderr, "%s debio haber recibido %s\n", nom, p);
    exit(1);
  }
  printf("%5d: %s accede a %s\n", getTime(), nom, res);
  sleep(2);
  printf("%5d: %s devuelve\n", getTime(), nom);
  nDevolver();
  sleep(5);
  printf("%5d: %s solicita acceder\n", getTime(), nom);
  res= nAcceder(-1);
  if (res!=q) {
    fprintf(stderr, "%s debio haber recibido %s\n", nom, q);
    exit(1);
  }
  printf("%5d: %s accede a %s\n", getTime(), nom, res);
  sleep(1);
  printf("%5d: %s devuelve\n", getTime(), nom);
  nDevolver();
  return NULL;
}

static void *t1fun(void *ptr) {
  printf("%5d: T1 solicita compartir %s\n", getTime(), p);
  nCompartir(p);
  printf("%5d: T1 termina de compartir %s\n", getTime(), p);
  sleep(2);
  printf("%5d: T1 solicita compartir %s\n", getTime(), q);
  nCompartir(q);
  printf("%5d: T1 termina de compartir %s\n", getTime(), q);
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
    char *res= nAcceder(-1);
    if (res!=r) {
      fprintf(stderr, "se debio haber recibido %s\n", r);
      exit(1);
    }
    nDevolver();
  }
  return NULL;
}

#if 0
static int n_true_f_big_sec;
static int tiempo_sec;

static double compare_f_big() {
  printf("Calculando recuento paralelo para f_big (~10 segundos en mi PC)\n");
  resetTime();
  int n_true_f_big= recuento(29, f_big);
  int tiempo_par= getTime();
  printf("recuento paralelo para f_big: %d (%d milisegs.)\n",
         n_true_f_big, tiempo_par);
  if (n_true_f_big!=n_true_f_big_sec) {
    fprintf(stderr, "recuento incorrecto para f_big: %d!=%d\n",
            n_true_f_big, n_true_f_big_sec);
    exit(1);
  }
  double speedUp= (double)tiempo_sec / tiempo_par;
  return speedUp;
}
#endif

int main() {

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
  printf("%5d: lanzando T4\n", getTime());
  pthread_create(&t4, 0, t34fun, "T4");
  sleep(1);
  printf("%5d: lanzando T3\n", getTime());
  pthread_create(&t3, 0, t34fun, "T3");
  sleep(2);
  printf("%5d: lanzando T1\n", getTime());
  pthread_create(&t1, 0, t1fun, "T1");
  sleep(1);
  printf("%5d: lanzando T2\n", getTime());
  pthread_create(&t2, 0, t2fun, "T2");
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  pthread_join(t3, NULL);
  pthread_join(t4, NULL);
  int dif= getTime()-12000;
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
  printf("Test de esfuerzo exitoso en %d segundos\n", (getTime()+500)/1000);
  printf("Se hicieron %d llamadas a nCompartir\n", llamadas);
 
  printf("\nFelicitaciones: su tarea funciona correctamente\n");

  return 0;
}
