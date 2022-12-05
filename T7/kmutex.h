/* Mutex y condiciones para Linux implementados a partir de semaforos.
 * El siguiente trozo de codigo es una implementacion de mutex y condiciones
 * para ser usados dentro del nucleo de Linux.
 * La API es la siguiente:
 * void m_init(KMutex *m)     -> inicializa el mutex m
 * void c_init(KCondition *c) -> inicializa la condicion c 
 * void m_lock(KMutex *m)     -> solicita la propiedad del mutex
 * void m_unlock(KMutex *m)   -> devuelve el mutex
 * int c_wait(KCondition *c, KMutex *m) -> devuelve el mutex m y se bloquea
 *   hasta que otro proceso invoque c_broadcast(c) o c_signal(c), en cuyo
 *   caso c_wait retorna 0.  Si el proceso recibe una senal como control-C
 *   mientras espera, c_wait retorna -EINTR.  Antes de retornar, se solicita
 *   nuevamente la propiedad de m.
 * void c_broadcast(KCondition *c)  -> despierta todos los procesos que esperan
 *   en c_wait(c), que deben continuar esperando obtener la propiedad del
 *   mutex
 * void c_signal(KCondition *c) -> despierta un solo proceso que espera en
 *   c_wait(c), que debe continuar esperando obtener la propiedad del mutex
 */

typedef struct {
  struct Link *head;
  struct Link **last_next;
} LinkQueue;

typedef struct Link {
  struct semaphore wait_sem;
  struct kmutex *mutex;
  struct Link *next;
} Link;

typedef struct kmutex {
  struct semaphore mutex_sem;
  LinkQueue queue;
} KMutex;

typedef struct {
  LinkQueue wait_queue;
} KCondition;

void m_init(KMutex *mutex);
void c_init(KCondition *cond);
void m_lock(KMutex *mutex);
void m_unlock(KMutex *mutex);
int c_wait(KCondition *cond, KMutex *mutex);
void c_broadcast(KCondition *cond);
void c_signal(KCondition *cond);
