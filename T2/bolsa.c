#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "bolsa.h"

// Declare aca sus variables globales
// ...
#define TRUE 1
#define FALSE 0

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int lowestPrice; //price
    char * lowestSeller; //seller name
    pthread_cond_t w;
}Request ;


int vendo(int precio, char *vendedor, char *comprador) {
  // retorna TRUE si le compran u cuarda nombre de comprador en comprador
  // returna FALSE si no tiene el precio mas bajo
  
}

int compro(char *comprador, char *vendedor) {
  // retorna precio del vendedor al que se le compro
  // y guarda el nombre del verdedor en vendedor
  int precio = 0;
}


