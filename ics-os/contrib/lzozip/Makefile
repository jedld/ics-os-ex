include ../../sdk/app.mk

CC=gcc
ICSOS_ROOT=../..
KERNEL=$(ICSOS_ROOT)/kernel
SDK=../../sdk
CFLAGS=$(APP_CFLAGS)
LIBS=$(SDK)/tccsdk.c $(SDK)/libtcc1.c $(SDK)/crt1.c
EXE=lzozip.exe

$(EXE): lzozip.c
	$(CC) $(CFLAGS) $(APP_LDFLAGS) -olzozip.exe lzozip.c minilzo.c $(LIBS)

install: $(EXE)
	mkdir -p $(ICSOS_ROOT)/apps
	cp $(EXE) $(ICSOS_ROOT)/apps

uninstall: $(EXE)
	rm -f $(ICSOS_ROOT)/apps/$(EXE)

clean:
	rm -f $(EXE)
