# Para revisar las opciones de compilacion y ejecucion,
# ingrese en el terminal el comando: make
#
#

BIN=nbin-san
PROB=disco
TEST=test-$(PROB)

NTHHDR=nKernel/nthread.h nKernel/nthread-impl.h nKernel/pss.h

SRCS= $(PROB).c $(TEST).c
HDRS= $(PROB).h

SHELL=bash -o pipefail

INCLUDE=-InKernel
CFLAGS=-Wall -Werror -pedantic -std=c18 $(INCLUDE)
#CFLAGS=-Wall -Werror -pedantic -std=c18 $(INCLUDE)
LDLIBS=-lm -lpthread

ARCH=amd64
NTHSRC=nKernel/nthread.c nKernel/nutil.c nKernel/ntime.c nKernel/pss.c \
       nKernel/sched-fcfs.c nKernel/sched-rr.c nKernel/ncompartir.c \
       nKernel/sched-pri-1core.c nKernel/sched-fcfs-1core.c \
       nKernel/sem.c nKernel/mutex-cond.c nKernel/nmsgs.c \
       nKernel/nStack-$(ARCH).s nKernel/nsystem.c

MAK=make --no-print-directory

readme:
	@less README.txt

run:
	@$(MAK) PROB=test-compartir TEST=none nbin
	@echo --------------------------------------------
	./test-compartir.nbin
	@echo --------------------------------------------
	./test-compartir.nbin -ncores 4
	@echo --------------------------------------------
	./test-compartir.nbin -slice 1

run-g:
	@$(MAK) PROB=test-compartir TEST=none nbin-g
	@echo --------------------------------------------
	./test-compartir.nbin-g
	@echo --------------------------------------------
	./test-compartir.nbin-g -ncores 4

run-san:
	@$(MAK) PROB=test-compartir TEST=none nbin-san
	@echo --------------------------------------------
	./test-compartir.nbin-san
	@echo --------------------------------------------
	./test-compartir.nbin-san -slice 1

ddd:
	@$(MAK) PROB=test-compartir TEST=none test-compartir.ddd

ddd-san:
	@$(MAK) PROB=test-compartir TEST=none test-compartir.ddd-san

zip:
	@if grep -P '\t' nKernel/ncompartir.c ; then echo "Su archivo nKernel/ncompartir.c contiene tabs.  Reemplacelos por espacios en blanco!" ; false ; else true; fi
	@rm -f resultados.txt ncompartir.zip
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
	zip -r ncompartir.zip resultados.txt nKernel/ncompartir.c
	@echo "Entregue por u-cursos el archivo ncompartir.zip"
	@echo "Descargue de u-cursos lo que entrego, descargue nuevamente los"
	@echo "archivos adjuntos y vuelva a probar la tarea tal cual como"
	@echo "la entrego.  Esto es para evitar que Ud. reciba un 1.0 en su"
	@echo "tarea porque entrego los archivos equivocados.  Creame, sucede"
	@echo "a menudo por ahorrarse esta verificacion."

nbin-san:
	@$(MAK) PROB=$(PROB) TEST=$(TEST) BIN=nbin-san $(PROB).nbin-san

nbin-g:
	@$(MAK) PROB=$(PROB) TEST=$(TEST) BIN=nbin-g $(PROB).nbin-g

nbin:
	@$(MAK) PROB=$(PROB) TEST=$(TEST) BIN=nbin $(PROB).nbin

pbin-san:
	@$(MAK) PROB=$(PROB) TEST=$(TEST) BIN=pbin-san $(PROB).pbin-san

pbin-thr:
	@$(MAK) PROB=$(PROB) TEST=$(TEST) BIN=pbin-thr $(PROB).pbin-thr

pbin-g:
	@$(MAK) PROB=$(PROB) TEST=$(TEST) BIN=pbin-g $(PROB).pbin-g

pbin:
	@$(MAK) PROB=$(PROB) TEST=$(TEST) BIN=pbin $(PROB).pbin

nKernel/libnth-g.a: $(NTHSRC) $(NTHHDR)
	@cd nKernel ; $(MAK) libnth-g.a
	
$(PROB).nbin-g: $(SRCS) $(HDRS) $(NTHHDR) nKernel/libnth-g.a
	gcc -g -DNTHREADS $(CFLAGS) $(SRCS) nKernel/libnth-g.a $(LDLIBS) -lrt -o $@
	@echo "Ejecute con: $(PROB).$(BIN) ... opciones ... (-h para help)"

$(PROB).pbin-g: $(SRCS) $(HDRS) $(NTHHDR) $(PSS)
	gcc -g $(CFLAGS) $(SRCS) $(LDLIBS) -o $@
	@echo "Ejecute con: $(PROB).$(BIN)"

nKernel/libnth-san.a: $(NTHSRC) $(NTHHDR)
	@cd nKernel ; $(MAK) libnth-san.a
	
$(PROB).nbin-san: $(SRCS) $(HDRS) $(NTHHDR) nKernel/libnth-san.a
	gcc -g -fsanitize=address -fsanitize=undefined -DSAN -DNTHREADS $(CFLAGS) $(SRCS) nKernel/libnth-san.a $(LDLIBS) -lrt -o $@
	@echo "Ejecute con: $(PROB).$(BIN) ... opciones ... (-h para help)"

$(PROB).pbin-san: $(SRCS) $(HDRS) $(NTHHDR) $(PSS)
	gcc -g -fsanitize=address -fsanitize=undefined -DSAN $(CFLAGS) $(SRCS) nKernel/pss.c $(LDLIBS) -o $@
	@echo "Ejecute con: $(PROB).$(BIN)"

$(PROB).pbin-thr: $(SRCS) $(HDRS) $(NTHHDR) $(PSS)
	gcc -g -fsanitize=thread -DSAN $(CFLAGS) $(SRCS) nKernel/pss.c $(LDLIBS) -o $@
	@echo "Ejecute con: $(PROB).$(BIN)"

nKernel/libnth.a: $(NTHSRC) $(NTHHDR)
	@cd nKernel ; $(MAK) libnth.a
	
$(PROB).nbin: $(SRCS) $(HDRS) $(NTHHDR) nKernel/libnth.a
	gcc -O -DNTHREADS $(CFLAGS) $(SRCS) nKernel/libnth.a $(LDLIBS) -lrt -o $@
	@echo "Ejecute con: $(PROB).$(BIN) ... opciones ... (-h para help)"

$(PROB).pbin: $(SRCS) $(HDRS) $(NTHHDR) $(PSS)
	gcc -g $(CFLAGS) $(SRCS) nKernel/pss.c $(LDLIBS) -o $@
	@echo "Ejecute con: $(PROB).$(BIN)"

nKernel/libnth-san.a: $(NTHSRC) $(NTHHDR)

%.ddd:
	@$(MAK) PROB=$(*F) $(*F).nbin-g
	ddd $(*F).nbin-g &

%.ddd-san:
	@$(MAK) PROB=$(*F) $(*F).nbin-san
	ddd $(*F).nbin-san &

clean:
	rm -f nKernel/*.a
	rm -f *.o *.log *.bin* *.nbin* *.pbin* core
