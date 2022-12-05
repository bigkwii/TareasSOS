Este ejemplo es una adaptacion del tutorial incluido
(archivo "device drivers tutorial.pdf") y bajado de:
http://www.freesoftwaremagazine.com/articles/drivers_linux

---

Guia rapida:

Lo siguiente se debe realizar parados en
el directorio en donde se encuentra este README.txt

+ Compilacion (puede ser en modo usuario):
$ make
...
$ ls
... syncread.ko ...

+ Instalacion (en modo root)

# mknod /dev/syncread c 61 0
# chmod a+rw /dev/syncread
# insmod syncread.ko
# dmesg | tail
...
[...........] Inserting syncread module
#

+ Testing (en modo usuario preferentemente)

Ud. necesitara crear 4 shells independientes.  Luego
siga las instrucciones del enunciado de la tarea 3 de 2013-1

+ Desinstalar el modulo

# rmmod syncread.ko
#
