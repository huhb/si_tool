all: si_tool

si_tool: driver.o main.o interface.o rtl8186.o sb710_usb.o
	gcc -o si_tool -g driver.o interface.o rtl8186.o sb710_usb.o main.o -lpci
driver.o: driver.c
	gcc -c -g driver.c -lpci
main.o:	main.c
	gcc -c  -g main.c
rtl8186.o: rtl8186.c
	gcc -c -g rtl8186.c
sb710_usb.o: sb710_usb.c
	gcc -c -g sb710_usb.c
interface.o: interface.c
	gcc -c -g interface.c
clean:
	rm *.o	
