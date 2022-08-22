#include "BigInt.h"

typedef struct {
  int n;
  BigInt_t *bits;
} BigNum;

BigNum *smallNum(BigInt_t x);
void freeBigNum(BigNum *bx);
BigNum *bigMul(BigNum *bx, BigNum *by);
BigNum *seqArrayProd(int a[], int i, int j);
BigNum *parArrayProd(int a[], int i, int j, int p);

int leer(int fd, void *vbuf, int n);
