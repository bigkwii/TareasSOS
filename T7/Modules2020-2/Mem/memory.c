/* Driver memory.c: una memoria residente en el nucleo.
 *
 * Un write suministra datos que se almacenan en una memoria residente
 * en el nucleo.
 *
 * Cada write sobreescribe los datos del write previo.
 *
 * La operacion read nunca se bloquea.
 * Cada read entrega los datos sumnistrados en el ultimo write y
 * borra los datos.  Si se lee cuando no hay datos, read entrega
 * 0 bytes (lo que usualmente se interpreta como fin de archivo).
 */

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

MODULE_LICENSE("Dual BSD/GPL");

/* Declaration of memory.c functions */
static int memory_open(struct inode *inode, struct file *filp);
static int memory_release(struct inode *inode, struct file *filp);
static ssize_t memory_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t memory_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
void memory_exit(void);
int memory_init(void);

/* Structure that declares the usual file */
/* access functions */
static struct file_operations memory_fops = {
  read: memory_read,
  write: memory_write,
  open: memory_open,
  release: memory_release
};

/* Declaration of the init and exit functions */
module_init(memory_init);
module_exit(memory_exit);

/* Global variables of the driver */
/* Major number */
static int memory_major = 61;
/* Buffer to store data */
#define MAX_SIZE 8192
static char *memory_buffer;
static ssize_t curr_size;
static struct semaphore mutex;
static struct semaphore write_mutex;

int memory_init(void) {
  int result;

  /* Registering device */
  result = register_chrdev(memory_major, "memory", &memory_fops);
  if (result < 0) {
    printk(
      "<1>memory: cannot obtain major number %d\n", memory_major);
    return result;
  }

  sema_init(&mutex, 1);
  sema_init(&write_mutex, 1);

  /* Allocating memory for the buffer */
  memory_buffer = kmalloc(MAX_SIZE, GFP_KERNEL); 
  if (!memory_buffer) { 
    result = -ENOMEM;
    goto fail; 
  } 
  memset(memory_buffer, 0, MAX_SIZE);
  curr_size= 0;

  printk("<1>Inserting memory module\n"); 
  return 0;

  fail: 
    memory_exit(); 
    return result;
}

void memory_exit(void) {
  /* Freeing the major number */
  unregister_chrdev(memory_major, "memory");

  /* Freeing buffer memory */
  if (memory_buffer) {
    kfree(memory_buffer);
  }

  printk("<1>Removing memory module\n");

}

static int memory_open(struct inode *inode, struct file *filp) {
  char *mode=   filp->f_mode & FMODE_WRITE ? "write" :
                filp->f_mode & FMODE_READ ? "read" :
                "unknown";
  if (filp->f_mode & FMODE_WRITE) {
    int rc= down_interruptible(&write_mutex);
    if (rc) {
      printk("<1> down interrupted, rc=%d\n", rc);
      return rc;
    }
    curr_size= 0;
  }
  printk("<1>open for %s\n", mode);
  /* Success */
  return 0;
}

static int memory_release(struct inode *inode, struct file *filp) {
 
  if (filp->f_mode & FMODE_WRITE) {
    up(&write_mutex);
  }
  printk("<1>close\n");
  /* Success */
  return 0;
}

static ssize_t memory_read(struct file *filp, char *buf, 
                    size_t count, loff_t *f_pos) { 
  ssize_t rc;
  down(&mutex);

  if (count > curr_size-*f_pos) {
    count= curr_size-*f_pos;
  }

  printk("<1>read %d bytes at %d\n", (int)count, (int)*f_pos);

  /* Transfering data to user space */
  if (copy_to_user(buf, memory_buffer+*f_pos, count)!=0) {
    /* el valor de buf es una direccion invalida */
    rc= -EFAULT;
    goto epilog;
  }

  *f_pos+= count;
  rc= count;

epilog:
  up(&mutex);
  return rc;
}

static ssize_t memory_write( struct file *filp, const char *buf,
                      size_t count, loff_t *f_pos) {

  ssize_t rc;
  loff_t last;

  down(&mutex);

  last= *f_pos + count;
  if (last>MAX_SIZE) {
    count -= last-MAX_SIZE;
  }
  printk("<1>write %d bytes at %d\n", (int)count, (int)*f_pos);

  /* Transfering data from user space */
  if (copy_from_user(memory_buffer+*f_pos, buf, count)!=0) {
    /* el valor de buf es una direccion invalida */
    rc= -EFAULT;
    goto epilog;
  }

  *f_pos += count;
  curr_size= *f_pos;
  rc= count;

epilog:
  up(&mutex);
  return rc;
}

