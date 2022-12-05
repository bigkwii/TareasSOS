Hello: modulo trivial con rutinas init_module y cleanup_module

Lo siguiente se debe realizar parados en
el directorio en donde se encuentra este README.txt

+ Compilacion (puede ser en modo usuario):
% make
...
% ls
... hello.ko ...

+ Instalacion (en modo root)

# insmod hello.ko
# dmesg
...
[ ... ] Hello world
#

+ Desinstalar el modulo

# rmmod hello.ko
# dmesg
...
[ ... ] Goodbye world
#
