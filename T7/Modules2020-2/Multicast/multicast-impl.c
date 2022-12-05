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

MODULE_LICENSE("Dual BSD/GPL");

/* Declaration of multicast.c functions */
static int multicast_open(struct inode *inode, struct file *filp);
static int multicast_release(struct inode *inode, struct file *filp);
static ssize_t multicast_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t multicast_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
void multicast_exit(void);
int multicast_init(void);

/* Structure that declares the usual file */
/* access functions */
struct file_operations multicast_fops = {
  read: multicast_read,
  write: multicast_write,
  open: multicast_open,
  release: multicast_release
};

/* Declaration of the init and exit functions */
module_init(multicast_init);
module_exit(multicast_exit);

/* Global variables of the driver */
/* Major number */
int multicast_major = 60;
/* Buffer to store data */
#define MAX_SIZE 8192
#define TRUE 1
#define FALSE 0
static char *multicast_buffer= NULL;
static size_t curr_size;
static size_t curr_pos;

/* El mutex y la condicion para multicast */
static KMutex mutex;
static KCondition cond;

int multicast_init(void) {
  int rc;

  /* Registering device */
  rc = register_chrdev(multicast_major, "multicast", &multicast_fops);
  if (rc < 0) {
    printk(
      "<1>multicast: cannot obtain major number %d\n", multicast_major);
    return rc;
  }

  /* Allocating multicast_buffer */
  multicast_buffer = kmalloc(MAX_SIZE, GFP_KERNEL); 
  if (!multicast_buffer) { 
    rc = -ENOMEM;
    goto fail; 
  } 
  memset(multicast_buffer, 0, MAX_SIZE);
  curr_size= 0;
  curr_pos= 0;
  m_init(&mutex);
  c_init(&cond);

  printk("<1>Inserting multicast module\n"); 
  return 0;

  fail: 
    multicast_exit(); 
    return rc;
}

void multicast_exit(void) {
  /* Freeing the major number */
  unregister_chrdev(multicast_major, "multicast");

  /* Freeing buffer multicast */
  if (multicast_buffer) {
    kfree(multicast_buffer);
  }

  printk("<1>Removing multicast module\n");
}

static int multicast_open(struct inode *inode, struct file *filp) {
  printk("<1>open succeeded (%p)\n", filp);
  return 0;
}

static int multicast_release(struct inode *inode, struct file *filp) {
  printk("<1>close succeeded (%p)\n", filp);
  return 0;
}

static ssize_t multicast_read(struct file *filp, char *buf, 
                    size_t count, loff_t *f_pos) { 
  ssize_t rc= 0;
  m_lock(&mutex); 
  if (c_wait(&cond, &mutex)) {
    printk("<1>read interrupted while waiting for data\n");
    rc= -EINTR;
    goto epilog;
  }

  if (count > curr_size) {
    count= curr_size;
  }

  printk("<1>read %d bytes at %d (%p)\n", (int)count, (int)*f_pos, filp);

  /* Transfering data to user space */ 
  if (copy_to_user(buf, multicast_buffer, count)!=0) {
    rc= -EFAULT;
    goto epilog;
  }
  *f_pos= curr_pos - (curr_size-count);
  rc= count;

epilog:
  m_unlock(&mutex);

  return rc;
}

static ssize_t multicast_write( struct file *filp, const char *buf,
                      size_t count, loff_t *f_pos) {
  ssize_t rc;
  m_lock(&mutex);
 
  if (count>MAX_SIZE) {
    count = MAX_SIZE;
  }
  printk("<1>write %d bytes at %d (%p)\n", (int)count, (int)curr_pos, filp);

  /* Transfering data from user space */ 
  if (copy_from_user(multicast_buffer, buf, count)!=0) {
    rc= -EFAULT;
    goto epilog;
  }
  curr_size = count;
  curr_pos += count;
  *f_pos= curr_pos;
  c_broadcast(&cond);
  rc= count;

epilog:
  m_unlock(&mutex);

  return rc;
}

