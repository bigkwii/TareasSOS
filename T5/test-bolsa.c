#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <nthread.h>

#include "bolsa.h"

// N: numero de compras que hara cada comprador
// NCOMP: numero de compradores
// NVEND: numero de vendedores

#define N 3000
#define NCOMP 10
#define NVEND 5

// Para probar su tarea por primera vez pruebe con:
//
// #define N 100
// #define NCOMP 1
// #define NVEND 2
//
// Con estos valores le sera mas facil diagnosticar los errores simples.

// Para depurar varios threads use los siguientes comandos de gdb:
// info threads : Muestra todos los threads en ejecucion.
// thread <id> : Cambia al thread <id>.  Esto significa que el comando
// backtrace muestra las funciones llamadas en ese thread.


// Para el test del enunciado

static void *pedroFun(void *ptr) {
  char c[100];
  printf("pedro: vendo a 10\n");
  if (vendo(10, "pedro", c)) {
    fprintf(stderr, "pedro no podia comprar a 10\n");
    exit(1);
  }
  printf("pedro: 1era. venta rechazada\n");
  sleep(1);
  printf("pedro: vendo a 7\n");
  if (vendo(7, "pedro", c)) {
    fprintf(stderr, "pedro no podia comprar a 7\n");
    exit(1);
  }
  printf("pedro: 2da. venta rechazada\n");
  return NULL;
}

static void *juanFun(void *ptr) {
  char c[100];
  printf("juan: vendo a 5\n");
  if (!vendo(5, "juan", c)) {
    fprintf(stderr, "juan tenia que comprar a 5\n");
    exit(1);
  }
  if (strcmp(c, "diego")!=0) {
    fprintf(stderr, "juan tenia que comprarle a diego\n");
    exit(1);
  }
  printf("juan: ok\n");
  return NULL;
}

static void *diegoFun(void *ptr) {
  char v[100];
  printf("diego: compro\n");
  if (compro("diego", v)!= 5) {
    fprintf(stderr, "diego tenia que comprar a 5\n");
    exit(1);
  }
  if (strcmp(v, "juan")!=0) {
    fprintf(stderr, "diego tenia que comprarle a juan\n");
    exit(1);
  }
  printf("diego: ok\n");
  return NULL;
}

// Para el test de robustez

typedef enum { CHECK, OK } Status;

typedef struct {
  char vendedor[10], comprador[10];
  int precio;
  pthread_mutex_t m;
  pthread_cond_t c;
  Status status;
  int compras;
} Comprador;

static Comprador compradores[NCOMP];

static int hay_comprador= 1;

static int start= 0;
static int compras= 0;
static pthread_mutex_t m= PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t c= PTHREAD_COND_INITIALIZER;

static void *vendedor(void *ptr) {
  char *nom= ptr;
  // Todos los compradores y vendedores parten al mismo tiempo
  pthread_mutex_lock(&m);
  while (!start)
    pthread_cond_wait(&c, &m);
  pthread_mutex_unlock(&m);
#if 0
  printf("Comienza %s\n", nom);
#endif
  // Se hace un numero no especificado de compras.
  // Antes de hacer su ultima compra, el comprador "comp0" colocara
  // hay_comprador en 0, esperara 1 segundo y hara su ultima compra
  while (hay_comprador) {
    int precio= random()%1000+1;
    char comprador[10];
    // Se ofrece la vender
    if (vendo(precio, nom, comprador)) {
#if 0
      printf("%s vendio a %s a %d\n", nom, comprador, precio);
#endif
      // Se logro la venta
      // Ahora verificamos que el comprador coincide con el
      // nombre del vendedor y el precio
      if (strncmp(comprador, "comp", 4)!=0) {
        fprintf(stderr, "El nombre del comprador debio comenzar con comp\n");
        exit(1);
      }
      int comp_id= atoi(&comprador[4]);
      Comprador *comp= &compradores[comp_id];
      pthread_mutex_lock(&comp->m);
      while (comp->status!=CHECK)
        pthread_cond_wait(&comp->c, &comp->m);
      if (strcmp(comp->vendedor, nom)!=0) {
        fprintf(stderr, "%s dice que le compro a %s, no a %s\n",
                comprador, comp->vendedor, nom);
        exit(1);
      }
      if (comp->precio!=precio) {
        fprintf(stderr, "%s dice que compro a %d, no a %d\n",
                comprador, comp->precio, precio);
        exit(1);
      }
      comp->status= OK;
      pthread_cond_broadcast(&comp->c);
      pthread_mutex_unlock(&comp->m);
    }
#if 0
    else
      printf("%s fracaso vendiendo a %d\n", nom, precio);
#endif
  }
#if 1
  printf("Termina %s\n", nom);
#endif
  return NULL;
}

static void *comprador(void *ptr) {
  char *nom= ptr;             // Nombre del comprador
  int comp_id= atoi(&nom[4]); // Identificacion del comprador
  Comprador *comp= &compradores[comp_id]; // Estructura del comprador
  // Inicializacion de la estructura del comprador
  comp->status= OK;
  sprintf(comp->comprador, "comp%d", comp_id);
  comp->compras= 0;
  pthread_mutex_init(&comp->m, NULL);
  pthread_cond_init(&comp->c, NULL);
  // Todos los compradores y vendedores parten al mismo tiempo
  pthread_mutex_lock(&m);
  while (!start)
    pthread_cond_wait(&c, &m);
  pthread_mutex_unlock(&m);
#if 0
  printf("Comienza %s\n", nom);
#endif
  // Hacemos N compras
  for (int i= 0; i<N; i++) {
    if (i==N-1 && comp_id==0) {
      // Si esta es la ultima compra de "comp0"
      printf("\ncomp0 hara una pausa de 3 segundos\n");
      hay_comprador= 0; // Paramos a todos los vendedores
      sleep(3);
      printf("comp0 hara la ultima compra\n");
      // Haremos una ultima compra por si hay un comprador pegado
    }
    char vendedor[10];
    int precio= compro(nom, vendedor);
    if (precio>0) {
#if 0
      printf("%s le compro a %s a %d\n", nom, vendedor, precio);
#else
      pthread_mutex_lock(&m);
      compras++;
      int mostrar= compras%1000==0;
      pthread_mutex_unlock(&m);
      if (mostrar) {
        printf(".");
        fflush(stdout);
      }
#endif
      pthread_mutex_lock(&comp->m);
      comp->precio= precio;
      strcpy(comp->vendedor, vendedor);
      comp->compras++;
      comp->status= CHECK;
      pthread_cond_broadcast(&comp->c);
      while (comp->status!=OK)
        pthread_cond_wait(&comp->c, &comp->m);
      pthread_mutex_unlock(&comp->m);
    }
    else
     usleep(1);
  }
#if 1
  printf("Termina %s\n", nom);
#endif
  return NULL;
}

int main() {
  if (1)
  {
    printf("El test del enunciado\n");
    pthread_t pedro, juan, diego;
    pthread_create(&pedro, 0, pedroFun, NULL);
    printf("Se lanzo pedro\n");
    sleep(1);
    pthread_create(&juan, 0, juanFun, NULL);
    printf("Se lanzo juan\n");
    sleep(2);
    pthread_create(&diego, 0, diegoFun, NULL);
    printf("Se lanzo diego\n");
    pthread_join(pedro, NULL);
    pthread_join(juan, NULL);
    pthread_join(diego, NULL);
    printf("Ok\n");
  }
  {
    printf("\n--------------------\n");
    printf("El test de robustez: se lanzaran %d compradores y %d vendedores\n",
           NCOMP, NVEND);
    printf("Cada comprador intentara %d compras\n", N);
    printf("Cada . son 1000 compraventas exitosas\n");
    pthread_t comp_tasks[NCOMP], vend_tasks[NVEND];
    char *vend_noms[NVEND], *comp_noms[NCOMP];
    for (int j= 0; j<NVEND; j++) {
      char *nom= malloc(10);
      sprintf(nom, "vend%d", j);
      vend_noms[j]= nom;
      pthread_create(&vend_tasks[j], NULL, vendedor, nom);
    }
    for (int i= 0; i<NCOMP; i++) {
      char *nom= malloc(10);
      sprintf(nom, "comp%d", i);
      comp_noms[i]= nom;
      pthread_create(&comp_tasks[i], NULL, comprador, nom);
    }
    printf("Partieron\n");
    pthread_mutex_lock(&m);
    start= 1;
    pthread_cond_broadcast(&c);
    pthread_mutex_unlock(&m);
    int total= 0;
    for (int i= 0; i<NCOMP; i++) {
      pthread_join(comp_tasks[i], NULL);
      printf("%s hizo %d compras\n",
             compradores[i].comprador, compradores[i].compras);
      total+= compradores[i].compras;
    }
    for (int j= 0; j<NVEND; j++) {
      pthread_join(vend_tasks[j], NULL);
    }
    for (int j= 0; j<NVEND; j++) {
      free(vend_noms[j]);
    }
    for (int i= 0; i<NCOMP; i++) {
      free(comp_noms[i]);
    }
    printf("Se hicieron en total %d compras\n", total);
    if (total<100) {
      printf("Son muy pocas compras. Algo anda mal.\n");
      exit(1);
    }
    printf("Ok\n");
  }
  printf("Felicitaciones: paso exitosamente los 2 tests de prueba\n");
  return 0;
}
