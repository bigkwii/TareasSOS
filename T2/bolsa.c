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

int nSellers = 0; //ammount of sellers
int nBuyers = 0; // ammount of buyers
int prices[MAX_INT]; //keeping track of prices
char * sellerNames[MAX_INT]; //keeping track of names
int lowestPrice = MAX_INT;
int lowestPriceInx = 0; //index of the cheapest seller
char * currentBuyer; //name of the current buyer


int popInt(int arr[], int arrLen, int index){
	// pops the inedx-th element of an int array and returns it.
	int val = arr[index];
	for(int i = index; i<arrLen-1; i++)
		arr[i] = arr[i+1];
	arrLen--;
	return val;
}

void appendInt(int arr[], int arrLen, int val){
	//append at the end
	arr[arrLen] = val;
	arrLen++;
}

char * popString(char * arr[], int arrLen, int index){
	// pops the inedx-th element of a string array and returns it.
	char * val = arr[index];
	for(int i = index; i<arrLen-1; i++)
		arr[i] = arr[i+1];
	arrLen--;
	return val;
}

void appendString(char * arr[], int arrLen, char * val){
	//append at the end
	arr[arrLen] = val;
	arrLen++;
}

int vendo(int precio, char *vendedor, char *comprador) {
  // retorna TRUE si le compran y guarda nombre de comprador en comprador
  // retorna FALSE si no tiene el precio mas bajo
  pthread_mutex_lock(&m);
  appendInt(prices)
  pthread_mutex_unlock(&m);
  while(nBuyers==0){
  	pthread_mutex_lock(&m);
  	pthread_cond_wait(&w);
  	if(precio > lowestPrice)
  		return FALSE;
  	pthread_mutex_unlock(&m);
  }
  if(precio > lowestPrice)
  		return FALSE;
  
  
}

int compro(char *comprador, char *vendedor) {
  // retorna precio del vendedor al que se le compro
  // y guarda el nombre del verdedor en vendedor
  // si no hay vendedores, retorna 0
  
}


