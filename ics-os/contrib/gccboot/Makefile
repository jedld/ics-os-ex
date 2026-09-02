include ../../sdk/app.mk

CC=gcc
SDK=../../sdk
EXE=gccboot.exe
SRC=gccboot.c
CFLAGS=$(APP_CFLAGS) -nostdinc -I$(SDK)/include
LIBS=$(SDK)/tccsdk.c $(SDK)/posix.c $(SDK)/libtcc1.c $(SDK)/crt1.c

$(EXE): $(SRC)
	$(CC) $(CFLAGS) $(APP_LDFLAGS) -o $@ $(SRC) $(LIBS)

install: $(EXE)
	mkdir -p ../../apps
	cp $(EXE) ../../apps/$(EXE)

clean:
	rm -f $(EXE)
