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
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/uaccess.h> /* copy_from/to_user */

#include "kmutex.h"

MODULE_LICENSE("Dual BSD/GPL");

/* Declaration of pipe.c functions */
static int pipe_open(struct inode *inode, struct file *filp);
static int pipe_release(struct inode *inode, struct file *filp);
static ssize_t pipe_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t pipe_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

void pipe_exit(void);
int pipe_init(void);

/* Structure that declares the usual file */
/* access functions */
struct file_operations pipe_fops = {
  read: pipe_read,
  write: pipe_write,
  open: pipe_open,
  release: pipe_release
};

/* Declaration of the init and exit functions */
module_init(pipe_init);
module_exit(pipe_exit);

/*** El driver para lecturas sincronas *************************************/

#define TRUE 1
#define FALSE 0

/* Global variables of the driver */

int pipe_major = 61;     /* Major number */

/* Buffer to store data */
#define MAX_SIZE 10

static char *pipe_buffer;
static int in, out, size;

/* El mutex y la condicion para pipe */
static KMutex mutex;
static KCondition cond;

int pipe_init(void) {
  int rc;

  /* Registering device */
  rc = register_chrdev(pipe_major, "pipe", &pipe_fops);
  if (rc < 0) {
    printk(
      "<1>pipe: cannot obtain major number %d\n", pipe_major);
    return rc;
  }

  in= out= size= 0;
  m_init(&mutex);
  c_init(&cond);

  /* Allocating pipe_buffer */
  pipe_buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
  if (pipe_buffer==NULL) {
    pipe_exit();
    return -ENOMEM;
  }
  memset(pipe_buffer, 0, MAX_SIZE);

  printk("<1>Inserting pipe module\n");
  return 0;
}

void pipe_exit(void) {
  /* Freeing the major number */
  unregister_chrdev(pipe_major, "pipe");

  /* Freeing buffer pipe */
  if (pipe_buffer) {
    kfree(pipe_buffer);
  }

  printk("<1>Removing pipe module\n");
}

static int pipe_open(struct inode *inode, struct file *filp) {
  char *mode=   filp->f_mode & FMODE_WRITE ? "write" :
                filp->f_mode & FMODE_READ ? "read" :
                "unknown";
  printk("<1>open %p for %s\n", filp, mode);
  return 0;
}

static int pipe_release(struct inode *inode, struct file *filp) {
  printk("<1>release %p\n", filp);
  return 0;
}

static ssize_t pipe_read(struct file *filp, char *buf,
                    size_t ucount, loff_t *f_pos) {
  int count= ucount;

  printk("<1>read %p %d\n", filp, count);
  m_lock(&mutex);

  while (size==0) {
    /* si no hay nada en el buffer, el lector espera */
    if (c_wait(&cond, &mutex)) {
      printk("<1>read interrupted\n");
      count= -EINTR;
      goto epilog;
    }
  }

  if (count > size) {
    count= size;
  }

  /* Transfiriendo datos hacia el espacio del usuario */
  for (int k= 0; k<count; k++) {
    if (copy_to_user(buf+k, pipe_buffer+out, 1)!=0) {
      /* el valor de buf es una direccion invalida */
      count= -EFAULT;
      goto epilog;
    }
    printk("<1>read byte %c (%d) from %d\n",
            pipe_buffer[out], pipe_buffer[out], out);
    out= (out+1)%MAX_SIZE;
    size--;
  }

epilog:
  c_broadcast(&cond);
  m_unlock(&mutex);
  return count;
}

static ssize_t pipe_write( struct file *filp, const char *buf,
                      size_t ucount, loff_t *f_pos) {
  int count= ucount;

  printk("<1>write %p %d\n", filp, count);
  m_lock(&mutex);

  for (int k= 0; k<count; k++) {
    while (size==MAX_SIZE) {
      /* si el buffer esta lleno, el escritor espera */
      if (c_wait(&cond, &mutex)) {
        printk("<1>write interrupted\n");
        count= -EINTR;
        goto epilog;
      }
    }

    if (copy_from_user(pipe_buffer+in, buf+k, 1)!=0) {
      /* el valor de buf es una direccion invalida */
      count= -EFAULT;
      goto epilog;
    }
    printk("<1>write byte %c (%d) at %d\n",
           pipe_buffer[in], pipe_buffer[in], in);
    in= (in+1)%MAX_SIZE;
    size++;
    c_broadcast(&cond);
  }

epilog:
  m_unlock(&mutex);
  return count;
}

