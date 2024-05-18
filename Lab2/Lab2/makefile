main : main.cpp my_print.o
	g++ -g main.cpp my_print.o -o main

my_print.o:my_print.asm
	nasm -f elf64 my_print.asm -o my_print.o
	
clean:
	rm -rf *.o main