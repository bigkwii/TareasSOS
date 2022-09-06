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
pthread_cond_t c = PTHREAD_COND_INITIALIZER;

int prices[]
char * sellerNames[];
int lowestPrice = MAX_INT;

int len(int arr[]){
    //returns the size of an int array
    return sizeof(arr) / sizeof(* arr);
}

int vendo(int precio, char *vendedor, char *comprador) {
  // retorna TRUE si le compran y guarda nombre de comprador en comprador
  // retorna FALSE si no tiene el precio mas bajo
  
}

int compro(char *comprador, char *vendedor) {
  // retorna precio del vendedor al que se le compro
  // y guarda el nombre del verdedor en vendedor
  // si no hay vendedores, retorna 0
  int precio;
  int lowestIdx;
  pthread_mutex_lock(&m);
  if(len(prices)>0) precio = MAX_INT;
  else{
    pthread_mutex_unlock(&m);
    return 0;
  }
  for(int i=0; i<len(prices);i++) {
    if(precio > prices[i]) {
        precio = prices[i];
        lowestIdx = i;
        lowestPrice = prices[i];
    }
  }
  vendedor = sellerNames[lowestIdx];
  pthread_mutex_unlock(&m);
  pthread_cond_broadcast(&c);
  return precio
}


