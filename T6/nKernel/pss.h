//==========================================
// Funciones para el TDA HashMap
//==========================================

typedef struct map Map;
typedef unsigned (*HashFun)(void *ptr);
typedef int (*EqualsFun)(void *ptrX, void *ptrY);
typedef struct map_iterator MapIterator;

Map *makeHashMap(int size, HashFun hash, EqualsFun equals);
void destroyHashMap(Map *map);
int contains(Map *map, void *key);
void *query(Map *map, void *key);
int define(Map *map, void *key, void *val);
void *del(Map *map, void *key);
MapIterator *getMapIterator(Map *map);
void destroyMapIterator(MapIterator *iter);
int mapNext(MapIterator *iter, void **pkey, void **pval);
int mapHasNext(MapIterator *iter);
void resetMapIterator(MapIterator *iter);

// Funciones de hashing

unsigned hash_ptr(void *ptr);
int pointer_equals(void *ptr1, void* ptr2);
unsigned hash_string(void *str);
int equals_strings(void *ptrX, void *ptrY);

//==========================================
// Funciones para el TDA Queue
//==========================================

typedef struct queue Queue;

Queue *makeQueue();
void destroyQueue(Queue *q);
void put(Queue *q, void *ptr);
void *get(Queue *q);
void *peek(Queue *q);
int emptyQueue(Queue *q);
int queueLength(Queue *q);

//==========================================
// Funciones para el TDA sort generico
//==========================================

typedef int (*Comparator)(void *ptr, int i, int j);
typedef void (*Swapper)(void *ptr, int i, int j);
void sort(void *ptr, int left, int right, Comparator compare, Swapper swap);
