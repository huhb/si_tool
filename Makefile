all: si_tool

si_tool: driver.o main.o
	gcc -o si_tool  driver.o main.o -lpci
driver.o: driver.c
	gcc -c driver.c -lpci
main.o:	main.c
	gcc -c  main.c	
