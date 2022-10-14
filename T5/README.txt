==========================================================
Esta es la documentación para compilar y ejecutar su tarea
==========================================================

Se está ejecutando el comando: less README.txt

***************************
*** Para salir: tecla q ***
***************************

Para avanzar a una nueva página: tecla <page down>
Para retroceder a la página anterior: tecla <page up>
Para avanzar una sola línea: tecla <enter>
Para buscar un texto: tecla / seguido del texto (/...texto...)
         por ejemplo: /ddd

-----------------------------------------------

Ud. debe crear el archivo nKernel/ncompartir.c y programar ahí las funciones
solicitadas.  Ya hay una plantilla en nKernel/ncompartir.c.plantilla. En el
contenido de la plantilla hay algunas indicaciones adicionales que le servirán.

Pruebe su tarea bajo Debian 11 de 64 bits.  Estos son los requerimientos
para aprobar su tarea:

+ make run-g debe felicitarlo.

+ make run debe felicitarlo.

+ make run-san debe felicitarlo y sanitize no debe reportar ningún tipo de
  problema de manejo de memoria.

Ignore específicamente este mensaje:
==26307==WARNING: ASan is ignoring requested __asan_handle_no_return: stack top: 0x7fed024fef00; bottom 0x7ffc5cf25000; size: 0xfffffff0a55d9f00 (-65945100544)
False positive error reports may follow

Se ejecutará el programa de prueba con varios tipos de scheduling.

Lamentablemente en esta tarea no se puede usar sanitize para detectar
dataraces (make run-thr).

Invoque el comando make zip para ejecutar todos los tests y generar un
archivo ncompartir.zip que contiene nKernel/ncompartir.c, con su solución,
y resultados.txt, con la salida de make run-san, make run-g y make run.

Para depurar use: make ddd

Video con ejemplos de uso de ddd: https://youtu.be/FtHZy7UkTT4
Archivos con los ejemplos: https://www.u-cursos.cl/ingenieria/2020/2/CC3301/1/novedades/r/demo-ddd.zip

-----------------------------------------------

Entrega de la tarea

Ejecute: make zip

Entregue por U-cursos el archivo ncompartir.zip

A continuación es muy importante que descargue de U-cursos el mismo
archivo que subió, luego descargue nuevamente los archivos adjuntos y
vuelva a probar la tarea tal cual como la entregó.  Esto es para
evitar que Ud. reciba un 1.0 en su tarea porque entregó los archivos
equivocados.  Creame, sucede a menudo por ahorrarse esta verificación.

-----------------------------------------------

Breve introducción a nThreads

Este es nThreads (nano threads).  Es un sistema operativo de
juguete que implementa threads ultra livianos a partir de un numero fijo
de p-threads (no tan livianos).

Para compilar sus tareas previas requiere cambiar:

#include <pthread.h>

por:

#define _XOPEN_SOURCE 500
#include <nthread.h>

No todas las funciones de Unix/Debian se pueden ejecutar.  Además las
funciones tienen nombres distintos.  Al final del archivo nKernel/nthread.h
hay macros del tipo #define pthread_create nThreadCreate que traducen
los nombres oficiales de la API de Unix/Debian a los nombres de nThreads.
Por eso Ud. sí puede usar los nombres tradicionales en sus programas.
Pero considere que el debugger le mostrará los nombres traducidos.
Para saber qué funciones puede invocar, vea si hay un #define para su
función al final del archivo nKernel/nthread.h.  Si no lo hay, lo más
probable es que no puede usar esa función.

Para compilar una aplicación que consista por ejemplo de los archivos
disco.c y test-disco.c, ejecute alguno de estos comandos:

make PROB=disco nbin-san
make PROB=disco nbin-g
make PROB=disco nbin

El primero genera el binario disco.nbin-san con operaciones de memoria o
indefinidas verificadas por sanitize.  El segundo es para hacer debugging con
el comando ddd disco.nbin-g.  El tercero genera disco.nbin con todas las
optimizaciones.

*** No olvide que debe incluir nthread.h, no pthread.h ***
Además al inicio de disco.c debe definir _XOPEN_SOURCE.

Ejecute el binario con la opción -h para recibir explicaciones sobre
las opciones para ejecutar con distintos schedulings.  Por ejemplo:

./disco-nbin -h

Para compilar su solución bolsa.c:

make PROB=bolsa nbin-san
make PROB=bolsa nbin-g
make PROB=bolsa nbin

-----------------------------------------------

Limpieza de archivos

make clean

Hace limpieza borrando todos los archivos que se pueden volver
a reconstruir a partir de los fuentes: *.o binarios etc.

-----------------------------------------------

Acerca del comando make

El comando make sirve para automatizar el proceso de compilación asegurando
recompilar el archivo binario ejecutable cuando cambió uno de los archivos
fuentes de los cuales depende.

A veces es útil usar make con la opción -n para que solo muestre
exactamente qué comandos va a ejecutar, sin ejecutarlos de verdad.
Por ejemplo:

   make -n sort-rv-nbits.ddd

También es útil usar make con la opción -B para forzar la recompilación
de los fuentes a pesar de que no han cambiado desde la última compilación.
Por ejemplo:

   make -B sort-rv-nbits

Recompilará para generar el archivo sort-rv-nbits desde cero
