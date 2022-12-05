/* Necessary includes for device drivers */
#include <linux/init.h>
/* #include <linux/config.h> */
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/uaccess.h> /* copy_from/to_user */

#include "kmutex.h"

/* Para depurar el monitor cambie quite el comentario de la siguiente linea */
/* #define DEBUG 1 */

#ifdef DEBUG
#define LOG(x) do { x } while(0)
#else
#define LOG(x) do { ; } while(0)
#endif

static void queue_init(LinkQueue *queue);
static int empty(LinkQueue *queue);
static void append(LinkQueue *queue, Link *link);
static Link *extract(LinkQueue *queue);
static int remove(LinkQueue *queue, Link *link);
#ifdef DEBUG
static void show_queue(char *msg, LinkQueue *queue);
#endif

void m_init(KMutex *mutex) {
  sema_init(&mutex->mutex_sem, 1);
  queue_init(&mutex->queue);
}

void c_init(KCondition *cond) {
  queue_init(&cond->wait_queue);
}

void m_lock(KMutex *mutex) {
  LOG(printk("m_lock (%p): requesting\n", mutex););
  down(&mutex->mutex_sem);
  LOG(printk("m_lock (%p): acquired\n", mutex););
}

void m_unlock(KMutex *mutex) {
  Link *link= extract(&mutex->queue);
  if (link==NULL) {
    /* Ningun otro proceso esperaba este mutex.  Se libera depositando
     * un ticket en mutex->mutex_sem. */
    up(&mutex->mutex_sem);
    LOG(printk("m_unlock (%p): unlocked\n", mutex););
  }
  else {
    /* Si otro proceso esperaba este mutex, se cede directamente el
     * mutex a ese proceso, sin llamar a up(&mutex->mutex_sem).  Si mas
     * tarde ese proceso devuelve este mutex y no hay otro proceso en espera,
     * el llamara a up(&mutex->mutex_sem).  Esta es la razon por la
     * que mutex_sem no puede ser un struct mutex de Linux, ya que
     * en esta implementacion el proceso que devuelve mutex_sem podria
     * no ser el mismo que lo pidio.  Esto no es correcto para los struct
     * mutex.
     * Al declarar mutex_sem como struct_semaphore, cualquier proceso
     * puede depositar un ticket en el. */
    up(&link->wait_sem); /* Despierta al proceso en espera */
    LOG(printk("m_unlock (%p): giving to link %p\n", mutex, link););
  }
}

int c_wait(KCondition *cond, KMutex *mutex) {
  int rc= 0;
  Link link;
  link.mutex= mutex;
  sema_init(&link.wait_sem, 0);
  append(&cond->wait_queue, &link);
  LOG(printk("c_wait (%p,%p): waiting on link %p\n", cond, mutex, &link);
      show_queue("c_wait queue status", &cond->wait_queue);
  );
  m_unlock(mutex); /* libera el mutex */

  rc= down_interruptible(&link.wait_sem);
  if (rc) {
    /* Si down_interruptible retorno por un control-C, y no por
     * c_broadcast o c_signal, hay que borrar este link de
     * cond->wait_queue.
     */
    LOG(printk("c_wait (%p, %p): link %p interrupted\n", cond, mutex, &link););
    m_lock(mutex);
    if (remove(&cond->wait_queue, &link)<0)
      printk("<1>c_wait: cannot find link\n");
  }
  /* Si down_interruptible retorno porque se invoco c_broadcast o c_signal,
   * no hay que volver a solicitar el mutex: m_unlock cede directamente el
   * mutex a este proceso.
   */
  return rc; /* -EINTR si el proceso recibio una senal */
}

void c_broadcast(KCondition *cond) {
  /* Los procesos en espera ganaran la propiedad del mutex respetando
   * el orden de llegada.  Ademas tienen prioridad por sobre los procesos
   * que habian pedido previamente el mutex con m_lock. */
  while (!empty(&cond->wait_queue)) {
    Link *link= extract(&cond->wait_queue);
    append(&link->mutex->queue, link);
    LOG(printk("c_broadcast (%p): inserting link %p in mutex %p\n", cond,
               link, link->mutex););
  }
}

void c_signal(KCondition *cond) {
  Link *link= extract(&cond->wait_queue);
  if (link!=NULL) {
    /* Se mueve este link desde cond->wait_queue hacia link->mutex->queue */
    append(&link->mutex->queue, link);
    LOG(printk("c_signal (%p): inserting link %p in mutex %p\n", cond,
               link, link->mutex););
  }
  else {
    LOG(printk("c_signal (%p): queue empty\n", cond););
  }
}

/*** Manejo de colas **************************************/

static void queue_init(LinkQueue *queue) {
  queue->head= NULL;
  queue->last_next= &queue->head;
}

static int empty(LinkQueue *queue) {
  return queue->head==NULL;
}

static void append(LinkQueue *queue, Link *link) {
  link->next= NULL;
  *queue->last_next= link;
  queue->last_next= &link->next;
}

static Link *extract(LinkQueue *queue) {
  Link *head= queue->head;
  if (head!=NULL) {
    Link *next= head->next;
    queue->head= next;
    if (next==NULL)
      queue->last_next= &queue->head;
  }
  return head;
}

static int remove(LinkQueue *queue, Link *link) {
  Link **plink;
  plink= &queue->head;
  while (*plink!=NULL && *plink!=link)
    plink= &((*plink)->next);
  if (*plink!=link)
    return -1;
  else {
    *plink= link->next;
    if (link->next==NULL)
      queue->last_next= plink;
    return 0;
  }
}

#ifdef DEBUG
static void show_queue(char *msg, LinkQueue *queue) {
  Link *link= queue->head;
  while (link!=NULL) {
    printk("%s: %p\n", msg, link);
    link= link->next;
  }
}
#endif
