Mem: modulo que implementa el driver para un trozo de memoria
     de 80 bytes

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
... memory.ko ...

+ Instalacion (en modo root)

# mknod /dev/memory c 61 0
# chmod a+rw /dev/memory
# insmod memory.ko
# dmesg | tail
  ...
  [...........] Inserting memory module
#

+ Testing (puede ser en modo usuario)

% echo abcdef > /dev/memory
% cat /dev/memory
abcdef
% dmesg
...
[ 5964.936658] write 6 bytes at offset 0
[ 5967.449982] read 6 bytes at offset 0
[ 5967.449993] read 0 bytes at offset 6

+ Desinstalar el modulo

# rmmod memory.ko
#
