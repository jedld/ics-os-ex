include ../../sdk/app.mk

NAME=hello

SRC=$(NAME).c
EXE=$(NAME).exe

CC=gcc
ICSOS_ROOT=../..
SDK=../../sdk
CFLAGS=$(APP_CFLAGS)
LIBS=$(SDK)/tccsdk.c $(SDK)/libtcc1.c $(SDK)/crt1.c 


$(EXE): $(SRC)
	$(CC) $(CFLAGS) $(APP_LDFLAGS) -o${EXE} $(SRC) $(LIBS)

install: $(EXE)
	mkdir -p $(ICSOS_ROOT)/apps
	cp $(EXE) $(ICSOS_ROOT)/apps

uninstall:
	rm -f $(ICSOS_ROOT)/apps/$(EXE)

clean:
	rm -f $(EXE)
