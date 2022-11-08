# Para revisar las opciones de compilacion y ejecucion,
# ingrese en el terminal el comando: make
#
#

BIN=pbin-san
PROB=disk
TEST=test-$(PROB)

NTHHDR=nKernel/nthread.h nKernel/nthread-impl.h nKernel/pss.h

SRCS= $(PROB).c $(TEST).c priqueue.c spinlocks.c swap.s
HDRS= $(PROB).h priqueue.h spinlocks.h

SHELL=bash -o pipefail

INCLUDE=-InKernel
CFLAGS=-Wall -Werror -pedantic -std=c18 $(INCLUDE)
#CFLAGS=-Wall -Werror -pedantic -std=c18 $(INCLUDE)
LDLIBS=-lm -lpthread

ARCH=amd64
NTHSRC=nKernel/nthread.c nKernel/nutil.c nKernel/ntime.c nKernel/pss.c \
       nKernel/sched-fcfs.c nKernel/sched-rr.c nKernel/sched-lcfs1.c \
       nKernel/sched-pri-1core.c nKernel/sched-fcfs-1core.c \
       nKernel/sem.c nKernel/mutex-cond.c nKernel/nmsgs.c \
       nKernel/nStack-$(ARCH).s nKernel/nsystem.c

MAK=make --no-print-directory

readme:
	@less README.txt

run:
	@echo "Test con pthreads usando spin-locks que esperan con busy-waiting"
	@$(MAK) BIN=pbin disk.pbin
	./disk.pbin
	@echo "Test con nthreads con spin-locks basados en mutex/condiciones"
	@echo "El scheduling es non preemtive last come first served"
	@$(MAK) BIN=nbin disk.nbin
	./disk.nbin -lcfs1

run-g:
	@echo "Test con pthreads usando spin-locks que esperan con busy-waiting"
	@$(MAK) BIN=pbin-g disk.pbin-g
	./disk.pbin-g
	@echo "Test con nthreads con spin-locks basados en mutex/condiciones"
	@echo "El scheduling es non preemtive last come first served"
	@$(MAK) BIN=nbin-g disk.nbin-g
	./disk.nbin-g -lcfs1

run-san:
	@echo "Test con pthreads usando spin-locks que esperan con busy-waiting"
	@$(MAK) BIN=pbin-san disk.pbin-san
	./disk.pbin-san
	@echo "Test con nthreads con spin-locks basados en mutex/condiciones"
	@echo "El scheduling es non preemtive last come first served"
	@$(MAK) BIN=nbin-san disk.nbin-san
	./disk.nbin-san -lcfs1

ddd-pbin:
	@$(MAK) -B BIN=pbin-g disk.ddd-pbin

ddd-pbin-san:
	@$(MAK) -B BIN=pbin-san disk.ddd-pbin-san

ddd-nbin:
	@echo "*** Ejecute el programa en ddd con run -lcfs1 ***"
	@$(MAK) -B BIN=nbin-g disk.ddd-nbin

ddd-nbin-san:
	@echo "*** Ejecute el programa en ddd con run -lcfs1 ***"
	@$(MAK) -B BIN=nbin-san disk.ddd-nbin-san

zip:
	@if grep -P '\t' $(PROB).c ; then echo "Su archivo $(PROB).c contiene tabs.  Reemplacelos por espacios en blanco!" ; false ; else true; fi
	@rm -f resultados.txt $(PROB).zip
	@echo "Sistema operativo utilizado" > resultados.txt
	@uname -a >> resultados.txt
	@cat resultados.txt
	@echo ==== run-san ============================= | tee -a resultados.txt
	@$(MAK) -B run-san | tee -a resultados.txt
	@echo ==== run-g =============================== | tee -a resultados.txt
	@$(MAK) -B run-g | tee -a resultados.txt
	@echo ==== run ================================= | tee -a resultados.txt
	@$(MAK) -B run | tee -a resultados.txt
	@echo ==== zip =================================
	zip -r $(PROB).zip resultados.txt $(PROB).c
	@echo "Entregue por u-cursos el archivo $(PROB).zip"
	@echo "Descargue de u-cursos lo que entrego, descargue nuevamente los"
	@echo "archivos adjuntos y vuelva a probar la tarea tal cual como"
	@echo "la entrego.  Esto es para evitar que Ud. reciba un 1.0 en su"
	@echo "tarea porque entrego los archivos equivocados.  Creame, sucede"
	@echo "a menudo por ahorrarse esta verificacion."

#nbin-san:
#	@$(MAK) PROB=$(PROB) TEST=$(TEST) BIN=nbin-san $(PROB).nbin-san
#
#nbin-g:
#	@$(MAK) PROB=$(PROB) TEST=$(TEST) BIN=nbin-g $(PROB).nbin-g
#
#nbin:
#	@$(MAK) PROB=$(PROB) TEST=$(TEST) BIN=nbin $(PROB).nbin
#
#pbin-san:
#	@$(MAK) PROB=$(PROB) TEST=$(TEST) BIN=pbin-san $(PROB).pbin-san
#
#pbin-thr:
#	@$(MAK) PROB=$(PROB) TEST=$(TEST) BIN=pbin-thr $(PROB).pbin-thr
#
#pbin-g:
#	@$(MAK) PROB=$(PROB) TEST=$(TEST) BIN=pbin-g $(PROB).pbin-g
#
#pbin:
#	@$(MAK) PROB=$(PROB) TEST=$(TEST) BIN=pbin $(PROB).pbin

nKernel/libnth-g.a: $(NTHSRC) $(NTHHDR)
	@echo cd nKernel
	@cd nKernel ; $(MAK) libnth-g.a
	@echo cd ..
	
$(PROB).nbin-g: $(SRCS) $(HDRS) $(NTHHDR) nKernel/libnth-g.a
	gcc -g -DNTHREADS $(CFLAGS) $(SRCS) nKernel/libnth-g.a $(LDLIBS) -lrt -o $@
	@echo "Ejecute con: $(PROB).$(BIN) ... opciones ... (-h para help)"

$(PROB).pbin-g: $(SRCS) $(HDRS) $(NTHHDR) $(PSS)
	gcc -g $(CFLAGS) $(SRCS) $(LDLIBS) -o $@
	@echo "Ejecute con: $(PROB).$(BIN)"

nKernel/libnth-san.a: $(NTHSRC) $(NTHHDR)
	@echo cd nKernel
	@cd nKernel ; $(MAK) libnth-san.a
	@echo cd ..
	
$(PROB).nbin-san: $(SRCS) $(HDRS) $(NTHHDR) nKernel/libnth-san.a
	gcc -g -fsanitize=address -fsanitize=undefined -DSAN -DNTHREADS $(CFLAGS) $(SRCS) nKernel/libnth-san.a $(LDLIBS) -lrt -o $@
	@echo "Ejecute con: $(PROB).$(BIN) ... opciones ... (-h para help)"

$(PROB).pbin-san: $(SRCS) $(HDRS) $(NTHHDR) $(PSS)
	gcc -g -fsanitize=address -fsanitize=undefined -DSAN $(CFLAGS) $(SRCS) $(PSS) $(LDLIBS) -o $@
	@echo "Ejecute con: $(PROB).$(BIN)"

$(PROB).pbin-thr: $(SRCS) $(HDRS) $(NTHHDR) $(PSS)
	gcc -g -fsanitize=thread -DSAN $(CFLAGS) $(SRCS) $(PSS) $(LDLIBS) -o $@
	@echo "Ejecute con: $(PROB).$(BIN)"

nKernel/libnth.a: $(NTHSRC) $(NTHHDR)
	@echo cd nKernel
	@cd nKernel ; $(MAK) libnth.a
	@echo cd ..
	
$(PROB).nbin: $(SRCS) $(HDRS) $(NTHHDR) nKernel/libnth.a
	gcc -O -DNTHREADS $(CFLAGS) $(SRCS) nKernel/libnth.a $(LDLIBS) -lrt -o $@
	@echo "Ejecute con: $(PROB).$(BIN) ... opciones ... (-h para help)"

$(PROB).pbin: $(SRCS) $(HDRS) $(NTHHDR) $(PSS)
	gcc -g $(CFLAGS) $(SRCS) $(PSS) $(LDLIBS) -o $@
	@echo "Ejecute con: $(PROB).$(BIN)"

nKernel/libnth-san.a: $(NTHSRC) $(NTHHDR)

%.ddd-pbin:
	@$(MAK) PROB=$(*F) BIN=$(BIN) $(*F).pbin-g
	ddd $(*F).pbin-g &

%.ddd-pbin-san:
	@$(MAK) PROB=$(*F) BIN=$(BIN) $(*F).pbin-san
	ddd $(*F).pbin-san &

%.ddd-nbin:
	@echo $(*F)
	@$(MAK) PROB=$(*F) BIN=$(BIN) $(*F).nbin-g
	ddd $(*F).nbin-g &

%.ddd-nbin-san:
	@$(MAK) PROB=$(*F) BIN=$(BIN) $(*F).nbin-san
	ddd $(*F).nbin-san &

clean:
	rm -f nKernel/*.a
	rm -f *.o *.log *.bin* *.nbin* *.pbin* core
