typedef struct priqueue PriQueue;

PriQueue *makePriQueue(void);
void *priGet(PriQueue *pq);
void priPut(PriQueue *pq, void *ptr, int pri);
int priBest(PriQueue *pq);
int emptyPriQueue(PriQueue *pq);
