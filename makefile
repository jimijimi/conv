all:
	@echo "Hello there."
	@echo "conv makefile."
	@echo "Available targets:"
	@echo "tccwin"
	@echo "tcclinux"
	@echo "cl"
	@echo "gcc"
	@echo "clang"
	@echo "crosscompilewin"

tccwin:
	tcc -DWINDOWS conv.c -o conv.exe

tcc:
	tcc conv.c -o conv

cl:
	cl /DWINDOWS conv.c /Feconv.exe

gcc:
	gcc conv.c -o conv

clang:
	clang conv.c -o conv

crosscompilewin:
	i686-w64-mingw32-gcc -DWINDOWS conv.c -o conv.exe

