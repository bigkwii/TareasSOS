#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <nthread.h>

#include "pss.h"
#include "disco.h"

#define TICK 1000

static void pausa(int milisegs) {
  usleep(milisegs*1000);
}

// Test Simple

typedef struct {
  char *nom, **ppareja;
} Args;

static void *fdama(void *ptr) {
  Args *pargs= ptr;
  char *nom= pargs->nom, **ppareja= pargs->ppareja;
  free(pargs);
  printf("%s espera pareja\n", nom);
  *ppareja= dama(nom);
  if (*ppareja==NULL) {
    fprintf(stderr, "fdama: La pareja de %s es NULL\n", nom);
    exit(1);
  }
  printf("La pareja de %s es %s\n", nom, *ppareja);
  return 0;
}

static void *fvaron(void *ptr) {
  Args *pargs= ptr;
  char *nom= pargs->nom, **ppareja= pargs->ppareja;
  free(pargs);
  printf("%s espera pareja\n", nom);
  *ppareja= varon(nom);
  if (*ppareja==NULL) {
    fprintf(stderr, "fvaron: La pareja de %s es NULL\n", nom);
    exit(1);
  }
  printf("La pareja de %s es %s\n", nom, *ppareja);
  return 0;
}

static void verificar_pareja(char *nom_dama, char *nom_varon,
                      char *pareja_dama, char *pareja_varon) {
  if (strcmp(pareja_dama, nom_varon)!=0) {
    fprintf(stderr, "verificar_pareja: La pareja de %s debio ser %s, "
                "pero es %s\n", nom_dama, nom_varon, pareja_dama);
    exit(1);
  }
  if (strcmp(pareja_varon, nom_dama)!=0) {
    fprintf(stderr, "verificar_pareja: La pareja de %s debio ser %s, "
                "pero es %s\n", nom_varon, nom_dama, pareja_varon);
    exit(1);
  }
}

static void lanzar(pthread_t *pth, void *(fun)(void *),
                   char *nom, char **ppareja) {
  Args *pargs= malloc(sizeof(Args));
  pargs->nom= nom;
  pargs->ppareja= ppareja;
  if (pthread_create(pth, NULL, fun, pargs)!=0) {
    perror("pthread_create");
    exit(2);
  }
}

static void testSimple() {
  printf("Test: una sola pareja, adan y eva\n");
  char *pareja_eva= NULL, *pareja_adan= NULL;
  pthread_t eva, adan, ana, sara, pedro, juan, diego, alba;
  lanzar(&eva, fdama, "eva", &pareja_eva);
  pausa(TICK);
  if (pareja_eva!=NULL) {
    fprintf(stderr, "testSimple: eva tenia que esperar a adan\n");
    exit(1);
  }
  lanzar(&adan, fvaron, "adan", &pareja_adan);
  pthread_join(eva, NULL);
  pthread_join(adan, NULL);
  verificar_pareja("eva", "adan", pareja_eva, pareja_adan);
  printf("Aprobado\n");

  printf("Test: el ejemplo del enunciado\n");
  char *pareja_ana= NULL, *pareja_sara= NULL, *pareja_alba= NULL;
  char *pareja_pedro= NULL, *pareja_juan= NULL, *pareja_diego= NULL;
  lanzar(&ana, fdama, "ana", &pareja_ana);
  pausa(TICK);
  lanzar(&sara, fdama, "sara", &pareja_sara);
  pausa(TICK);
  lanzar(&pedro, fvaron, "pedro", &pareja_pedro);
  pausa(TICK);
  lanzar(&juan, fvaron, "juan", &pareja_juan);
  pausa(TICK);
  lanzar(&diego, fvaron, "diego", &pareja_diego);
  pausa(TICK);
  lanzar(&alba, fdama, "alba", &pareja_alba);
  pthread_join(sara, NULL);
  pthread_join(juan, NULL);
  pthread_join(ana, NULL);
  pthread_join(pedro, NULL);
  pthread_join(alba, NULL);
  pthread_join(diego, NULL);
  if (strcmp(pareja_ana,"pedro")==0 && strcmp(pareja_pedro, "ana")!=0) {
    fprintf(stderr, "simpleTest: Inconsitencia de pareja en ana y pedro\n");
    exit(1);
  }
  if (strcmp(pareja_ana,"juan")==0 && strcmp(pareja_juan, "ana")!=0) {
    fprintf(stderr, "simpleTest: Inconsitencia de pareja en ana y juan\n");
    exit(1);
  }
  if (strcmp(pareja_ana,"diego")==0 && strcmp(pareja_diego, "ana")!=0) {
    fprintf(stderr, "simpleTest: Inconsitencia de pareja en ana y diego\n");
    exit(1);
  }

  if (strcmp(pareja_sara,"pedro")==0 && strcmp(pareja_pedro, "sara")!=0) {
    fprintf(stderr, "simpleTest: Inconsitencia de pareja en sara y pedro\n");
    exit(1);
  }
  if (strcmp(pareja_sara,"juan")==0 && strcmp(pareja_juan, "sara")!=0) {
    fprintf(stderr, "simpleTest: Inconsitencia de pareja en sara y juan\n");
    exit(1);
  }
  if (strcmp(pareja_sara,"diego")==0 && strcmp(pareja_diego, "sara")!=0) {
    fprintf(stderr, "simpleTest: Inconsitencia de pareja en sara y diego\n");
    exit(1);
  }

  if (strcmp(pareja_alba,"pedro")==0 && strcmp(pareja_pedro, "alba")!=0) {
    fprintf(stderr, "simpleTest: Inconsitencia de pareja en alba y pedro\n");
    exit(1);
  }
  if (strcmp(pareja_alba,"juan")==0 && strcmp(pareja_juan, "alba")!=0) {
    fprintf(stderr, "simpleTest: Inconsitencia de pareja en alba y juan\n");
    exit(1);
  }
  if (strcmp(pareja_alba,"diego")==0 && strcmp(pareja_diego, "alba")!=0) {
    fprintf(stderr, "simpleTest: Inconsitencia de pareja en alba y diego\n");
    exit(1);
  }

  printf("Aprobado\n");
}

// Test de robustez

#ifdef VALGRIND
#define NPAREJAS 25
#define NBAILES 50
#else
#ifdef DRD
#define NPAREJAS 20
#define NBAILES 25
#else
#define NPAREJAS 50
#define NBAILES 2000
#endif
#endif

static sem_t anotadas[NPAREJAS], verificadas[NPAREJAS];
static int pareja_damas[NPAREJAS];
static pthread_t tasks_damas[NPAREJAS], tasks_varones[NPAREJAS];
static char *noms[NPAREJAS];

void *loop_dama(void *ptr) {
  char *nom= ptr;
  int id= atoi(nom);
  int iter= NBAILES;
  while (iter--) {
    char *pareja= dama(nom);
    int id_pareja= atoi(pareja);
    pareja_damas[id]= id_pareja;
    sem_post(&anotadas[id]);
    sem_wait(&verificadas[id]);
    pareja_damas[id]= -1;
  }
  return NULL;
}

static void *loop_varon(void *ptr) {
  char *nom= ptr;
  int id= atoi(nom);
  int iter= NBAILES;
  while (iter--) {
    char *pareja= varon(nom);
    int id_pareja= atoi(pareja);
    sem_wait(&anotadas[id_pareja]);
    if (pareja_damas[id_pareja]!=id) {
      fprintf(stderr, "loop_varon: Varon %s dice que su pareja es dama %s, "
                                "pero dama %s dice que su pareja es varon %d\n",
                  nom, pareja, pareja, pareja_damas[id_pareja]); 
      exit(1);
    }
    sem_post(&verificadas[id_pareja]);
  }
  return NULL;
}

static void testRobustez() {
  printf("Test: robustez\n");
  for (int i= 0; i<NPAREJAS; i++) {
    sem_init(&anotadas[i], 0, 0);
    sem_init(&verificadas[i], 0, 0);
    pareja_damas[i]= -1;
    noms[i]=malloc(16);
    sprintf(noms[i], "%d", i);
    int check= atoi(noms[i]);
    if (check!=i) {
      fprintf(stderr, "testRobustez: check");
      exit(1);
    }
    if (pthread_create(&tasks_damas[i], NULL, loop_dama, noms[i])!=0) {
      perror("pthread_create");
      exit(2);
    }
    if (pthread_create(&tasks_varones[i], NULL, loop_varon, noms[i])!=0) {
      perror("pthread_create");
      exit(2);
    }
  }
  for (int i= 0; i<NPAREJAS; i++) {
    pthread_join(tasks_damas[i], NULL);
    pthread_join(tasks_varones[i], NULL);
  }
  for (int i= 0; i<NPAREJAS; i++) {
    free(noms[i]);
    sem_destroy(&anotadas[i]);
    sem_destroy(&verificadas[i]);
  }
  printf("Aprobado\n");
}

int main() {
  DiscoInit();
  testSimple();
  testRobustez();
  DiscoDestroy();
  printf("Felicitaciones: aprobo todos los tests\n");
  return 0;
}
