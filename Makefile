all: si_tool

si_tool: driver.o main.o rtl8186.o
	gcc -o si_tool -g driver.o rtl8186.o main.o -lpci
driver.o: driver.c
	gcc -c -g driver.c -lpci
main.o:	main.c
	gcc -c  -g main.c
rtl8186.o: rtl8186.c
	gcc -c -g rtl8186.c
clean:
	rm *.o	
