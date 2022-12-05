ccflags-y := -Wall -std=gnu99 -Wno-declaration-after-statement

obj-m := prodcons.o
prodcons-objs := kmutex.o prodcons-impl.o

KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

#include $(KDIR)/.config

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

prodcons-impl.o kmutex.o: kmutex.h

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
