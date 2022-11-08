
CFLAGS=-Wall -Werror -pedantic -std=c18 -Wno-unused-function -Wno-unused-variable -DNTHREADS $(INCLUDE)

ARCH=amd64
NTHSRC=nthread.c nutil.c ntime.c pss.c sched-fcfs.c sched-rr.c sched-lcfs1.c \
       sched-pri-1core.c sched-fcfs-1core.c sem.c mutex-cond.c nmsgs.c \
       nStack-$(ARCH).s nsystem.c
NTHOBJ=$(patsubst %.c,%.o,$(patsubst %.s,%.o,$(NTHSRC)))
NTHIMPLHDR=nthread.h nthread-impl.h nSystem.h pss.h

libnth-g.a: $(NTHSRC) $(NTHIMPLHDR)
	@echo NTHOBJ=$(NTHOBJ)
	gcc -g -c $(CFLAGS) $(NTHSRC)
	rm -f $@
	ar r $@ $(NTHOBJ)
	ranlib $@
	rm $(NTHOBJ)

libnth-san.a: $(NTHSRC) $(NTHIMPLHDR)
	@echo NTHOBJ=$(NTHOBJ)
	gcc -g -c -fsanitize=address -fsanitize=undefined -DSANITIZE $(CFLAGS) $(NTHSRC)
	rm -f $@
	ar r $@ $(NTHOBJ)
	ranlib $@
	rm $(NTHOBJ)

libnth.a: $(NTHSRC) $(NTHIMPLHDR)
	@echo NTHOBJ=$(NTHOBJ)
	gcc -O -c $(CFLAGS) $(NTHSRC)
	rm -f $@
	ar r $@ $(NTHOBJ)
	ranlib $@
	rm $(NTHOBJ)
