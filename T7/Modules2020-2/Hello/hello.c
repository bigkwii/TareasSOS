#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

int init_module(void) {
  printk(KERN_ALERT "Hello world\n");
  return 0;
}
void cleanup_module(void) {
  printk(KERN_ALERT "Goodbye world\n");
}
