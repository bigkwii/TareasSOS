Multicast: implementacion de la tarea 3 de Sistemas Operativos'2009
usando monitores (kmutex) para Linux

---

Guia rapida:

Lo siguiente se debe realizar parados en
el directorio en donde se encuentra este README.txt

+ Compilacion (puede ser en modo usuario):
% make
...
% ls
... multicast.ko ...

+ Instalacion (en modo root)

# mknod /dev/multicast c 60 0
# chmod a+rw /dev/multicast
# insmod multicast.ko
# dmesg | tail
...
[...........] Inserting multicast module
#

+ Testing (en modo usuario preferentemente)

Ud. necesitara crear 4 shells independientes.  Luego
siga las instrucciones del enunciado de la tarea 3 de 2009
incluido en este mismo directorio (t3.pdf).

Bug conocido:
si un proceso lee de /dev/multicast y 2 procesos escriben al mismo
tiempo, podria ser que el lector vea una sola escritura.
La correccion de este bug sera tarea en el futuro!

+ Desinstalar el modulo

# rmmod multicast.ko
#
