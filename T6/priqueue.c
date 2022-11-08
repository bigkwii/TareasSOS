#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "priqueue.h"

typedef struct node {
  void *ptr;
  int pri;
  struct node *next;
} Node;

struct priqueue {
  Node *best;
};

PriQueue *makePriQueue(void) {
  PriQueue *pq= malloc(sizeof(*pq));
  pq->best= NULL;
  return pq;
}

void *priGet(PriQueue *pq) {
  if (pq->best==NULL)
    return NULL;
  Node *pnode= pq->best;
  void *ptr= pnode->ptr;
  pq->best= pnode->next;
  free(pnode);
  return ptr;
}

void priPut(PriQueue *pq, void *ptr, int pri) {
  Node **ppnode= &pq->best;
  Node *pnode= *ppnode;
  while (pnode!=NULL && pnode->pri<pri) {
    ppnode= &pnode->next;
    pnode= *ppnode;
  }
    
  Node *pNewNode= malloc(sizeof(Node));
  pNewNode->ptr= ptr;
  pNewNode->pri= pri;
  pNewNode->next= pnode;

  *ppnode= pNewNode;
}

int priBest(PriQueue *pq) {
  return pq->best->pri;
}

int emptyPriQueue(PriQueue *pq) {
  return pq->best==NULL;
}
