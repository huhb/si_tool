CC=gcc
CFLAGS=-g -O2
OBJECTS= driver.o main.o interface.o rtl8186.o sb7xx_usb.o

all: si_tool
si_tool: $(OBJECTS)
	$(CC) $(CFLAGS)  $^ -o $@ -lpci

.PHONY: clean
clean:
	rm -f $(OBJECTS) si_tool
