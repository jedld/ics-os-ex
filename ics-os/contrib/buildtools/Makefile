include ../../sdk/app.mk

CC=gcc
SDK=../../sdk
ICSOS_ROOT=../..
CFLAGS=$(APP_CFLAGS) -nostdinc -I$(SDK)/include
LIBS=$(SDK)/tccsdk.c $(SDK)/posix.c $(SDK)/libtcc1.c $(SDK)/crt1.c $(SDK)/setjmp.c
TOOLS=cp.exe mkdir.exe rm.exe

buildtool.exe: buildtool.c
	$(CC) $(CFLAGS) $(APP_LDFLAGS) -o$@ buildtool.c $(LIBS)

$(TOOLS): buildtool.exe
	cp buildtool.exe $@

all: $(TOOLS)

buildtest.exe: buildtest.c
	$(CC) $(CFLAGS) $(APP_LDFLAGS) -o$@ buildtest.c $(LIBS)

install: all buildtest.exe
	mkdir -p $(ICSOS_ROOT)/apps
	cp $(TOOLS) buildtest.exe $(ICSOS_ROOT)/apps/

clean:
	rm -f buildtool.exe buildtest.exe $(TOOLS)

.PHONY: all install clean
