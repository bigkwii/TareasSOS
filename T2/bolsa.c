#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "bolsa.h"

// Declare aca sus variables globales
// ...
#define TRUE 1
#define FALSE 0

#define MAX_INT 0x7fffffff

#define LOCK(m) (pthread_mutex_lock(&m))
#define UNLOCK(m) (pthread_mutex_unlock(&m))
#define WAIT(c,m) (pthread_cond_wait(&c,&m))
#define BROADCAST(c) (pthread_cond_broadcast(&c))

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;

// seller stuff
int precioMin[1];
char * vendedorMin[1];
int hayVendedor[1];
// buyer stuff
char * compradorActual[1];
int ready[1];


int vendo(int precio, char *vendedor, char *comprador) {
  // retorna TRUE si le compran y guarda nombre de comprador en comprador
  // retorna FALSE si no tiene el precio mas bajo
  LOCK(m);
  if(precio < precioMin[0]){
    precioMin[0] = precio;
    BROADCAST(c);
  }
  while(precio>=precioMin[0]){
    WAIT(c,m);
    if(precio >= precioMin[0]){
      break;
    }
    if(ready[0]){
      comprador = compradorActual[0];
      hayVendedor[0] = FALSE;
      precioMin[0] = MAX_INT;
      ready[0] = FALSE;
      UNLOCK(m);
      BROADCAST(c);
      return TRUE;
    }
  }
  UNLOCK(m);
  return FALSE;
}

int compro(char *comprador, char *vendedor) {
  // retorna precio del vendedor al que se le compro
  // y guarda el nombre del verdedor en vendedor
  // si no hay vendedores, retorna 0
  LOCK(m);
  int precioActual = 0;
  if(!hayVendedor[0]) {
    UNLOCK(m);
    return precioActual;
  }
  else {
    compradorActual[0] = comprador;
    vendedor = vendedorMin[0];
    ready[0] = TRUE;
    precioActual = precioMin[0];
    BROADCAST(c);
    UNLOCK(m);
  }
  return precioActual;
}


