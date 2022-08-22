# Para revisar las opciones de compilacion y ejecucion,
# ingrese en el terminal el comando: make
#
#

PROB=prod

TEST=test-$(PROB)
HEADER=$(PROB)

SHELL=bash -o pipefail

INCLUDE=
CFLAGS=-Wall -Werror -std=c18 $(INCLUDE)
#CFLAGS=-Wall -Werror -pedantic -std=c18 $(INCLUDE)
LDLIBS=-lm -lpthread

MAK=make --no-print-directory

readme:
	@less README.txt

$(PROB).bin $(PROB).bin-g $(PROB).bin-san: $(PROB).c $(TEST).c BigInt.c $(HEADER).h BigInt.h

run-san: $(PROB).bin-san
	@if grep -P '\t' $(PROB).c ; then echo "Su archivo $(PROB).c contiene tabs.  Reemplacelos por espacios en blanco!" ; false ; else true; fi
	./$(PROB).bin-san

run-g: $(PROB).bin-g
	./$(PROB).bin-g | tee run.log
	@grep "Llamada secuencial " run.log | wc -l > sec.log
	@echo "Verificando que se crearon 4 procesos pesados"
	@diff sec.log 4.Txt || ( echo "No aprueba: no se crearon 4 procesos" ; false )
	@echo Aprobado

run: $(PROB).bin
	./$(PROB).bin

ddd: $(PROB).ddd

ddd-san: $(PROB).ddd-san

zip:
	@if grep -P '\t' $(PROB).c ; then echo "Su archivo $(PROB).c contiene tabs.  Reemplacelos por espacios en blanco!" ; false ; else true; fi
	@rm -f resultados.txt $(PROB).zip
	@echo "Sistema operativo utilizado" > resultados.txt
	@uname -a >> resultados.txt
	@cat resultados.txt
	@echo ==== run-san ==== | tee -a resultados.txt
	@$(MAK) -B run-san | tee -a resultados.txt
	@echo ==== run-g ==== | tee -a resultados.txt
	@$(MAK) -B run-g | tee -a resultados.txt
	@echo ==== run ==== | tee -a resultados.txt
	@$(MAK) -B run | tee -a resultados.txt
	@echo ==== zip ====
	zip -r $(PROB).zip resultados.txt $(PROB).c
	@echo "Entregue por u-cursos el archivo $(PROB).zip"
	@echo "Descargue de u-cursos lo que entrego, descargue nuevamente los"
	@echo "archivos adjuntos y vuelva a probar la tarea tal cual como"
	@echo "la entrego.  Esto es para evitar que Ud. reciba un 1.0 en su"
	@echo "tarea porque entrego los archivos equivocados.  Creame, sucede"
	@echo "a menudo por ahorrarse esta verificacion."

%.bin: %.c
	gcc -O -DOPT=1 $(CFLAGS) $^ $(LDLIBS) -o $@

%.bin-g: %.c
	gcc -g $(CFLAGS) $^ $(LDLIBS) -o $@

%.bin-san: %.c
	gcc -g -DSAN=1 -fsanitize=address -fsanitize=undefined $(CFLAGS) $^ $(LDLIBS) -o $@

%.ddd: %.bin-g
	ddd $(*F).bin-g &

%.ddd-san: %.bin-san
	ddd $(*F).bin-san &

clean:
	rm -f *.o *.log *.bin* core
