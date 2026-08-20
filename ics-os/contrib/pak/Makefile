include ../../sdk/app.mk

CC=gcc
ICSOS_ROOT=../..
SDK=../../sdk
CFLAGS=$(APP_CFLAGS)
LIBS=$(SDK)/tccsdk.c $(SDK)/libtcc1.c $(SDK)/crt1.c 
EXE=pak.exe

$(EXE): pak.c
	$(CC) $(CFLAGS) $(APP_LDFLAGS) -opak.exe pak.c $(LIBS)

install: $(EXE)
	mkdir -p $(ICSOS_ROOT)/apps
	cp $(EXE) $(ICSOS_ROOT)/apps

uninstall:
	rm -f $(ICSOS_ROOT)/apps/$(EXE)

clean:
	rm -f $(EXE)
