include ../../sdk/app.mk

NAME=gcc
SRC=gccdriver.c
EXE=gcc.exe
CC=gcc
ICSOS_ROOT=../..
SDK=../../sdk
CFLAGS=$(APP_CFLAGS) -nostdinc -I$(SDK)/include
LIBS=$(SDK)/tccsdk.c $(SDK)/posix.c $(SDK)/libtcc1.c $(SDK)/crt1.c

# gccdriver: the thin in-OS gcc front-end that drives cc1/as/ld (see gccdriver.c).
$(EXE): $(SRC) $(SDK)/posix.c $(SDK)/tccsdk.c
	$(CC) $(CFLAGS) $(APP_LDFLAGS) -o${EXE} $(SRC) $(LIBS)

install: $(EXE)
	mkdir -p $(ICSOS_ROOT)/apps
	cp $(EXE) $(ICSOS_ROOT)/apps

uninstall:
	rm -f $(ICSOS_ROOT)/apps/$(EXE)

clean:
	rm -f $(EXE)
