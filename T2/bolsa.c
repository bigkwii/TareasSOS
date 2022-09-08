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
int * thereAreSellers;
int * lowestPrice;
char ** lowestPriceName;
// buyer stuff
int * thereAreBuyers;
char ** currentBuyerName;

int vendo(int precio, char *vendedor, char *comprador) {
  // retorna TRUE si le compran y guarda nombre de comprador en comprador
  // retorna FALSE si no tiene el precio mas bajo
  LOCK(m);
  if (lowestPrice == NULL || precio >= *lowestPrice) return FALSE;
  if (precio >= *lowestPrice) return FALSE;
  else {
  	*thereAreSellers = TRUE;
  	*lowestPrice = precio;
  	*lowestPriceName = vendedor;
  	BROADCAST(c);
  }
  while(precio < *lowestPrice){
  	WAIT(c,m);
  	LOCK(m);
  	if (precio >= *lowestPrice) break;
  	else {
  		comprador = *currentBuyerName;
  		*thereAreSellers = FALSE;
  		*lowestPrice = MAX_INT;
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
  if(thereAreSellers == NULL || !*thereAreSellers) return 0;
  LOCK(m);
  *currentBuyerName = comprador;
  vendedor = *lowestPriceName;
  int buyPrice = *lowestPrice;
  UNLOCK(m);
  BROADCAST(c);
  return buyPrice;
}


