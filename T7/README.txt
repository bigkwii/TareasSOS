Este ejemplo es una adaptacion del tutorial incluido
(archivo "device drivers tutorial.pdf") y bajado de:
http://www.freesoftwaremagazine.com/articles/drivers_linux

---

Guia rapida:

Lo siguiente se debe realizar parados en
el directorio en donde se encuentra este README.txt

+ Compilacion (puede ser en modo usuario):
% make
...
% ls
... prodcons.ko ...

+ Instalacion (en modo root)

# mknod /dev/prodcons c 61 0
# chmod a+rw /dev/prodcons
# insmod prodcons.ko
# dmesg | tail
...
[...........] Inserting prodcons module
#

+ Testing (en modo usuario preferentemente)

Ud. necesitara crear 4 shells independientes.  Luego
siga las instrucciones del enunciado de la tarea.

+ Desinstalar el modulo

# rmmod prodcons.ko
#
