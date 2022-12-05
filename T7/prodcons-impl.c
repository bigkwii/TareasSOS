/* Implemente aqui el driver para /dev/prodcons */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include "kmutex.h"

MODULE_LICENSE("Dual BSD/GPL"); // we're legal

// interface
int prodcons_open(struct inode *inode, struct file *filp);
int prodcons_release(struct inode *inode, struct file *filp);
ssize_t prodcons_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t prodcons_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
void prodcons_exit(void);
int prodcons_init(void);

// structure that declares the usual file
// access functions
struct file_operations prodcons_fops = {
  read: prodcons_read,
  write: prodcons_write,
  open: prodcons_open,
  release: prodcons_release
};

// declaration of the init and exit functions
module_init(prodcons_init);
module_exit(prodcons_exit);

// global vars
#define MAX_SIZE 1024

int prodcons_major = 61;

static char *prodcons_buffer;
static ssize_t curr_size; // current buffer size (ssize_t is a signed int)

static int writing;
static int readers;
static int pend_open_read; // how many readers are trying to access, but can't

static KMutex mutex;
static KCondition cond; // cond for readers

/**
 * return 0 if success. error code otherwise
 */
int prodcons_init(void) {
    int rc;
    // register device
    rc = register_chrdev(prodcons_major, "prodcons", &prodcons_fops);
    if (rc < 0) {
        printk("prodcons_init error");
        return rc;
    }
    // globals init
    writing = 0; // bool
    readers = 0;
    pend_open_read = 0;
    curr_size = 0;
    m_init(&mutex);
    c_init(&cond);
    // allocate buff
    prodcons_buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
    if (prodcons_buffer == NULL) {
        prodcons_exit();
        return -ENOMEM; // out of memory error code
    }
    return 0;
}

void prodcons_exit(void) {
    // unregister device
    unregister_chrdev(prodcons_major, "prodcons");
    
    // free buffer
    if (prodcons_buffer) kfree(prodcons_buffer);
}

int prodcons_open(struct inode *inode, struct file *filp) { return 0; }

int prodcons_release(struct inode *inode, struct file *filp) { return 0; }

/**
 * readers read the buffer by arrival order.
 * so, when a writer enters new text, we notify all readers,
 * and the oldest reader will read the new text.
 */
ssize_t prodcons_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
    ssize_t rc;
    m_lock(&mutex);

    // wait until there is something to read, if the buffer is empty
    while(curr_size == 0){
        if (c_wait(&cond, &mutex)) {
            printk("<1>Read interrupted!\n");
            rc= -EINTR; // system interrupt error code
            goto epilog; // jump to end
        }
    }
    
    if (count > curr_size - *f_pos) { 
        count = curr_size - *f_pos;
    }
    
    if (copy_to_user(buf, prodcons_buffer, count)) {
        rc = -EFAULT; // bad address error code
        goto epilog; // jump to end
    }

    curr_size -= count;
    rc = count;
    epilog:
    m_unlock(&mutex);
    return rc;
}

/**
 * we need to overwrite the buffer each time a writer enters new text.
 */
ssize_t prodcons_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    ssize_t rc;
    m_lock(&mutex);

    if (count > MAX_SIZE) { // max buffer size
        count = MAX_SIZE;
    }

    // copy user data
    if (copy_from_user(prodcons_buffer, buf, count)) {
        rc = -EFAULT; // bad address error code
        goto epilog; // jump to end
    }
    curr_size = count; // update buffer size
    rc = count; 
    c_broadcast(&cond);
    epilog:
        m_unlock(&mutex);
        return rc;
}

