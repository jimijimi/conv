conv
====

conv is a unit converter for the command line. It has been written in C.

The program has been tested in Linux x86, Linux ARM and WIndows x86 64 bit.

The program looks complicated and I know it can be written in a few lines of python. But I wanted to have a go at pointers, memory management and all that C stuff.



Database:
=========

conv utilizes an external database file where the information for the conversion is stored. Here a database entry is defined by four pieces of data (1) the original units, (2) the target units, (3) a numerical factor and (4) a numerical constant. Ezample

m   km   0.001    0.0


One line comments can be included in the database by itializing them with a '#'


Conversion Formula:
===================

A generalized conversion expression can be:

y = Fx + C          (1)

where:
y = the quantity in the target units
x = the quantity in the initial units
F = conversion factor
C = conversion constant.

Usually C is zero for most conversions except for temperature. ( See the example database provided ).

Consider now the conversion km/L to L/100km ( fuel consumption ), the conversion is carried out using:

y = 100 / x         (2)

or

y = 100 x^(-1)      (3)


so equation (1) can be further generalized as

y = F x^n + C   (4)


conv v2.0 includes this modifications.




Requirements:
=============

A C compiler.

Tested compilers include:

(1) gcc 4.9 linux
(2) gcc 5.0 linux
(3) clang 3.5 linux
(4) tcc 0.9.26 linux
(5) tcc 0.9.26 windows
(6) i686-w64-mingw32-gcc Windows crosscompile under Linux
(7) cl windows visual studio

License:
========

The conv program is Copyright(C) 2015 Jaime Ortiz
See license.md file for licensing information

Document last updated August 1, 2015


