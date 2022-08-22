#define _DEFAULT_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "prod.h"

#ifdef SAN

// Con ejecucion con valgrind/sanitize
#define N_INTENTOS 1
#define TOLERANCIA 0.1
#define BIGN 10000
#define SMALLN 100

#else
#ifdef OPT

// Con opciones de optimizacion
#define N_INTENTOS 4
#define TOLERANCIA 1.2
// #define TOLERANCIA 1.8
#define BIGN 400000
#define SMALLN 1000

#else

// Con opciones de debugging
#define N_INTENTOS 1
#define TOLERANCIA 0.1
#define BIGN 40000
#define SMALLN 1000

#endif
#endif

// ----------------------------------------------------
// Funcion que entrega el tiempo transcurrido desde el lanzamiento del
// programa en milisegundos

static long long getUSecsOfDay() {
  struct timeval Timeval;
  gettimeofday(&Timeval, NULL);
  return (long long)Timeval.tv_sec*1000000+Timeval.tv_usec;
}

static int time0= 0;

static int getTime0() {
    return getUSecsOfDay()/1000;
}

static void resetTime() {
  time0= getTime0();
}

static int getTime() {
  return getTime0()-time0;
}

// ----------------------------------------------------
// Funciones para trabajar con big numbers

BigNum *smallNum(BigInt_t x) {
  BigNum *bx= malloc(sizeof(BigNum));
  bx->n= 1;
  bx->bits= malloc(sizeof(BigInt_t));
  bx->bits[0]= x;
  return bx;
}

void freeBigNum(BigNum *bx) {
  free(bx->bits);
  free(bx);
}

static BigNum *newBigNum(int n, BigInt_t *bits) {
  BigNum *bx= malloc(sizeof(BigNum));
  int nadj= n;
  while (nadj>0 && bits[nadj-1]==0) {
    nadj--;
  }
  bx->n= nadj;
  if (n==nadj)
    bx->bits= bits;
  else {
    BigInt_t *bitsadj= malloc(nadj*sizeof(BigInt_t));
    for (int i= 0; i<nadj; i++) {
      bitsadj[i]= bits[i];
    }
    bx->bits= bitsadj;
    free(bits);
  }
  return bx;
}

int bigCmp(BigNum *bx, BigNum *by) {
  if (bx->n!=by->n)
    return bx->n - by->n;
  else
    return BigInt_cmp(bx->n, bx->bits, by->bits);
}

#if 1

// This uses the fast Karatsuba multiplication algorithm.  
// Below is my own more intuitive but slow algorithm.
// If you want to test how slow is, change the #if 1 by #if 0
// Compare with time ./test-prod 100000 for both algorithms.
BigNum *bigMul(BigNum *bx, BigNum *by) {
  int n= bx->n+by->n;
  BigInt_t *bits= malloc(n*sizeof(BigInt_t));
  BigInt_mul(bx->n, bx->bits, by->n, by->bits, n, bits);

  return newBigNum(n, bits);
}

#else

BigNum *bigMul(BigNum *bx, BigNum *by) {
  int n= bx->n+by->n;
  BigInt_t *bits= malloc(n*sizeof(BigInt_t));
  for (int i= 0; i<n; i++)
    bits[i]= 0;
  BigInt_t carry= 0;
  for (int i= 0; i<by->n; i++) {
    int pos= i;
    for (int j= 0; j<bx->n; j++) {
      BigInt_tmp_t prod= (BigInt_tmp_t)bits[pos] + (BigInt_tmp_t)carry;
      prod += (BigInt_tmp_t)bx->bits[j] * (BigInt_tmp_t)by->bits[i];
      // prod += (BigInt_tmp_t)bx->bits[j] * by->bits[i];
      bits[pos]= prod;
      carry= prod>>(8*sizeof(BigInt_t));
      pos++;
    }
    if (pos<n) {
      bits[pos]= carry;
      carry= 0;
    }
  }
  if (carry!=0) {
    fprintf(stderr, "Asercion fallida: ultimo carry no fue 0\n");
    exit(1);
  }
  
  return newBigNum(n, bits);
}

#endif

// ----------------------------------------------------
// Version secuencial del producto de los elementos de
// un arreglo

static int verbose= 0;

static BigNum *recArrayProd(int a[], int i, int j) {
  if (i>j) {
    fprintf(stderr, "Asercion fallida: i > j\n");
    exit(1);
  }
  if (i==j) {
    return smallNum(a[i]);
  }
  else {
    int h= (i+j)/2;
    BigNum *left= recArrayProd(a, i, h);
    BigNum *right= recArrayProd(a, h+1, j);
    BigNum *prod= bigMul(left, right);
    freeBigNum(left);
    freeBigNum(right);
    return prod;
  }
}

BigNum *seqArrayProd(int a[], int i, int j) {
  if (verbose) {
    printf("Llamada secuencial con i=%d j=%d\n", i, j);
    fflush(stdout);
  }
  return recArrayProd(a, i, j);
}

// ----------------------------------------------------
// Calculo del factorial

// This iterative algorithm to compute factorial is simple and intuitive,
// but it is slow for big numbers
BigNum *slowBigFact(int n) {
  BigNum *bp= smallNum(1);
  for (int i= 1; i<=n; i++) {
    BigNum *bi= smallNum(i);
    BigNum *newbp= bigMul(bp, bi);
    freeBigNum(bp);
    freeBigNum(bi);
    bp= newbp;
  }
  return bp;
}

// This is the recursive divide and conquer algorithm.  It happens to
// be way faster than the iterative algorithm.  Why?
BigNum *fastBigFact(int n, int p) {
  if (n==0)
    return smallNum(0);

  int *a= malloc(n*sizeof(int));

  // Fill a with 1, 2, 3, ..., n
  for (int i= 0; i<n; i++) {
    a[i]= i+1;
  }

#if 1
  // Filling a with 1, 2, 3, ..., n would work but the product of the first
  // half of the array would be much smaller than the second half,
  // making an unbalanced size of big numbers.
  // So the the array is filled with a permutation of 1, 2, 3, ..., n
  // to mix randomly small numbers with big numbers.
  // If you want to experiment without the permutation, change the
  // the #if 1 by #if 0
  // Try with ./test-prod 100000. It does segmentation fault because
  // of stack overflow.

  // Do a random permutation to balance the size of the numbers
  for (int i= 0; i<n; i++) {
    int r= random()%(n-i) + i;
    int tmp= a[i];
    a[i]= a[r];
    a[r]= tmp;
  }
#endif

  // Compute big product of array numbers
  BigNum *bf;
  if (p>1)
    bf= parArrayProd(a, 0, n-1, p); // parallel with p unix processes
  else
    bf= seqArrayProd(a, 0, n-1);    // sequential
  free(a);
  return bf;
}

// ----------------------------------------------------
// Conversion de un big number a un string hexadecimal
// (convertir a decimal tomaria demasiado tiempo)

char *bigNum2HexString(BigNum *bx) {
  char *str= malloc(bx->n*sizeof(BigInt_t)*2+1);
  char *res= str;
  int imax= sizeof(BigInt_t)*2;
  sprintf(str, "%lx", bx->bits[bx->n-1]);
  str += strlen(str);

  for (int k= bx->n-2; k>=0; k--) {
    BigInt_t w= bx->bits[k];
    for (int i= 1; i<=imax; i++) {
      int hex= w & 0x0f;
      str[imax-i]= hex>=10 ? 'a'+hex-10 : '0'+hex;
      w >>= 4;
    }
    str += imax;
  }
  *str= 0;
  return res;
}

int main(int argc, char *argv[]) {
  if (argc>3) {
    fprintf(stderr, "Usage: %s <number> <process number>\n", argv[0]);
    exit(1);
  }
  if (argc>=2) {
    int n= atoi(argv[1]);
#if 1
    int p= 1;
    if (argc==3)
      p= atoi(argv[2]);

    BigNum *bf= fastBigFact(n, p);
#else
    // If you want to experiment with the slow version
    // change the #if 1 by #if 0
    // Compare with time ./test-prod 100000
    if (argc==3)
      printf("No parallel version of slow big factorial\n");
    BigNum *bf= slowBigFact(n);
#endif
    char *bigstr= bigNum2HexString(bf);
    printf("%s\n", bigstr);
    freeBigNum(bf);
    free(bigstr);
  }
  else {
    printf("Test: factorial de numeros pequenos con 2 a 17 threads\n");
    fflush(stdout);

    {
      for (int i=1; i<=SMALLN; i++) {
        if (i%(SMALLN/10)==0) {
          printf("%d\n", i);
          fflush(stdout);
        }
        BigNum *ref= fastBigFact(i, 1);
        BigNum *bf= fastBigFact(i, (i%16)+2);
        if (bigCmp(ref, bf) != 0) {
          fprintf(stderr, "Resultado incorrecto para factorial de %d\n", i);
          exit(1);
        }
        freeBigNum(ref);
        freeBigNum(bf);
      }
      printf("\nAprobado\n");
    }
    printf("--------------\n");
    printf("Test: speed up\n");
    fflush(stdout);

    {
       int msg= 0;
       int i= 0;
       while (i<N_INTENTOS) {
         printf("Intento: %d\n", i+1);
         printf("Calculando factorial de %d secuencialmente\n", BIGN);
         printf("En mi computador Ryzen 3550H se demoro unos 20 segundos\n");
         resetTime();
         BigNum *ref= fastBigFact(BIGN, 1); // sequential version
         int seq_time= getTime();
         char *ref_str= bigNum2HexString(ref);
         printf("Tiempo=%d milisegundos, cifras hexadecimales=%d\n",
                seq_time, (int)strlen(ref_str));
         printf("Calculando en paralelo con 4 threads\n");
         printf("Deberia demorarse menos que la version secuencial\n");
         printf("En mi computador el speed up rondaba 1.4 con 2 cores\n");
	 fflush(stdout);
	 verbose= i==0 ? 1 : 0;
         resetTime();
         BigNum *bf= fastBigFact(BIGN, 4); // parallel version
         int par_time= getTime();
         verbose= 0;
         char *bf_str= bigNum2HexString(bf);
         if (strcmp(ref_str, bf_str)!=0) {
           fprintf(stderr, "*** Error *** Los resultados son distintos!\n");
           exit(1);
         }
         double speedUp= (double)seq_time/par_time;
         printf("Tiempo=%d milisegundos, speed up=%f\n", par_time, speedUp);
	 freeBigNum(ref);
	 freeBigNum(bf);
	 free(ref_str);
	 free(bf_str);
         if (speedUp>=TOLERANCIA)
           break;
         printf("No alcanza el speed up requerido, volveremos a intentar.\n");
         i++;
         printf("--------------\n");
         if (msg)
           continue;
         printf("Coloque su computador en modo alto rendimiento, porque el\n");
         printf("economizador de bateria puede alterar los resultados.\n");
         printf("No ejecute este programa junto a otros programas que hagan\n");
         printf("un uso intensivo de la CPU.  En windows puede lanzar el\n");
         printf("administrador de tareas para verificar que el uso de CPU\n");
         printf("sea bajo.\n");
         msg= 1;
       }
       if (i==N_INTENTOS) {
         fprintf(stderr, "Despues de %d intentos no alcanza el speed up "
                         "requerido de %f\n", N_INTENTOS, TOLERANCIA);
         exit(1);
       }
       else {
         printf("Felicitaciones: aprobo este modo de ejecucion\n");
       }
    }
  }

  return 0;
}
