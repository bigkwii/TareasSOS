#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "bolsa.h"

// Declare aca sus variables globales
// ...
#define TRUE 1
#define FALSE 0

// states
#define STANDBY 1 // "vendedor disponible, consulte uwu"

#define LOCK(m) (pthread_mutex_lock(&m))
#define UNLOCK(m) (pthread_mutex_unlock(&m))
#define WAIT(c,m) (pthread_cond_wait(&c,&m))
#define BROADCAST(c) (pthread_cond_broadcast(&c))
#define SIGNAL(c) (pthread_cond_signal(&c))

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;

// cool pointers B)
int * precioMin;
char ** vendedorMin;
char ** compradorActual;
int * state;
int * winGlobal;

int vendo(int precio, char *vendedor, char *comprador) {
  // retorna TRUE si le compran y guarda nombre de comprador en comprador
  // retorna FALSE si no tiene el precio mas bajo
  int stateLocal = STANDBY;
  int win = FALSE;
  LOCK(m);
  //printf("hola, soy %s, vengo a vender. a %d mi rey.\n",vendedor,precio);
  if (precioMin == NULL || *precioMin > precio){
    // :)
    //printf("hola, soy %s, ahora soy el bkn.\n",vendedor);
    precioMin = &precio;
    vendedorMin = &vendedor;
    compradorActual = &comprador;
    state = &stateLocal;
    winGlobal = &win;
    BROADCAST(c);
  }else{
    // :(
    //printf("hola, soy %s, me fue mal.\n",vendedor);
    UNLOCK(m);
    return win;
  }
  while(precioMin != NULL && precioMin != NULL && precio == *precioMin){
    //printf("*vendedor %s se fue a mimir*\n",vendedor);
    WAIT(c,m);
    //printf("*vendedor %s se desperto*\n",vendedor);

    if(state != NULL){
      // lo despertaron, pero no para comprar
      // D:
      //printf("hola, soy %s, me han derrocado. %d era muy caro. otro vende a menos.\n",vendedor, precio);
      break;
    }
    if(state == NULL){
      // lo despertaron para comprar
      // >:3
      //printf("SOLD. hola, soy %s, le vendi a %s a %d.\n",vendedor, comprador, precio);
      UNLOCK(m);
      return win;
    }
  }
  UNLOCK(m);
  return win;
}

int compro(char *comprador, char *vendedor) {
  // retorna precio del vendedor al que se le compro
  // y guarda el nombre del verdedor en vendedor
  // si no hay vendedores, retorna 0
  int precio = 0;
  LOCK(m);
  //printf("hola, soy %s, vengo a comprar.\n",comprador);
  if(state != NULL && *state == STANDBY){
    strcpy(vendedor, *vendedorMin);
    precio = *precioMin;
    strcpy(*compradorActual, comprador);
    precioMin = NULL;
    vendedorMin = NULL;
    compradorActual = NULL;
    state = NULL;
    *winGlobal = TRUE;
    winGlobal = NULL;
    //printf("BOUGHT. hola, soy %s, logre comprar a %s a %d.\n",comprador, vendedor, precio);
    BROADCAST(c);
  }
  //if(precio == 0) printf("hola, soy %s, no habia nadie.\n",comprador);
  UNLOCK(m);
  return precio;
}

