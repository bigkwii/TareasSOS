#include "pss.h"

/*************************************************************
 * Manejo de colas FIFO
 *
 * Atencion: Estas colas no pueden ser manipuladas concurrentemente.
 *
 *************************************************************/

typedef Queue *FifoQueue;               // Atencion, una cola es un puntero

// FifoQueue MakeFifoQueue();           // El constructor
#define MakeFifoQueue makeQueue
// void PutObj(FifoQueue q, void* o);    // Agrega un objeto al final
#define PutObj put
// void* GetObj(FifoQueue q);           // Retorna y extrae el primer objeto
#define GetObj get
// void *PeekObj(FifoQueue queue);      // Retorna primer objeto sin extraerlo
#define PeekObj peek
// int  EmptyFifoQueue(FifoQueue q);    // Verdadero si la cola esta vacia
#define EmptyFifoQueue emptyQueue
// void DestroyFifoQueue(FifoQueue q);  // Elimina la cola
#define DestroyFifoQueue destroyQueue

// Procedimientos adicionales

// int LengthFifoQueue(FifoQueue q);    // Entrega el largo de la cola
#define LengthFifoQueue queueLength
int QueryObj(FifoQueue q, void* o);     // Verdadero si o esta en la cola
void DeleteObj(FifoQueue q, void* o);   // Elimina o si esta en la cola
void PushObj(FifoQueue q, void* o);     // Agrega un objeto al principio
