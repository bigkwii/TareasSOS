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

Ud. debe crear el archivo disk.c y programar ahí las funciones
solicitadas.  Ya hay una plantilla en disk.c.plantilla. En el
contenido de la plantilla hay algunas indicaciones adicionales que le servirán.

Pruebe su tarea bajo Debian 11 de 64 bits.  Estos son los requerimientos
para aprobar su tarea:

+ make run-g debe felicitarlo.

+ make run debe felicitarlo.

+ make run-san debe felicitarlo y sanitize no debe reportar ningún tipo de
  problema de manejo de memoria.

Cada una de estas ejecuciones se prueba primero con pthreads con spin-locks
verdaderos y luego se prueba con nThreads con spin-locks implementados con
mutex/condiciones.

Lamentablemente en esta tarea no se puede usar sanitize para detectar
dataraces (make run-thr).

Invoque el comando make zip para ejecutar todos los tests y generar un
archivo disk.zip que contiene disk.c, con su solución,
y resultados.txt, con la salida de make run-san, make run-g y make run.

Si falla la prueba con pthreads depure con: make ddd-pbin
Si falla la prueba con nThreads depure con: make ddd-nbin

Video con ejemplos de uso de ddd: https://youtu.be/FtHZy7UkTT4
Archivos con los ejemplos: https://www.u-cursos.cl/ingenieria/2020/2/CC3301/1/novedades/r/demo-ddd.zip

-----------------------------------------------

Entrega de la tarea

Ejecute: make zip

Entregue por U-cursos el archivo disk.zip

A continuación es muy importante que descargue de U-cursos el mismo
archivo que subió, luego descargue nuevamente los archivos adjuntos y
vuelva a probar la tarea tal cual como la entregó.  Esto es para
evitar que Ud. reciba un 1.0 en su tarea porque entregó los archivos
equivocados.  Creame, sucede a menudo por ahorrarse esta verificación.

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
