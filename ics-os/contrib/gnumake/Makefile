include ../../sdk/app.mk

NAME=make
EXE=make.exe
CC=gcc
ICSOS_ROOT=../..
SDK=../../sdk
STAGE=/tmp/icsos-make
CFLAGS=$(APP_CFLAGS) -nostdinc -I$(SDK)/include -I$(STAGE) -I$(STAGE)/glob -DHAVE_CONFIG_H
LIBS=$(SDK)/tccsdk.c $(SDK)/posix.c $(SDK)/libtcc1.c $(SDK)/crt1.c $(SDK)/setjmp.c
MAKE_C=ar.c arscan.c commands.c default.c dir.c expand.c file.c function.c \
	getopt.c getopt1.c implicit.c job.c main.c misc.c read.c remake.c \
	rule.c signame.c strcache.c variable.c version.c vpath.c hash.c \
	remote-stub.c glob/glob.c glob/fnmatch.c

$(EXE): $(SDK)/posix.c $(SDK)/tccsdk.c $(SDK)/crt1.c
	chmod +x $(ICSOS_ROOT)/scripts/stage-make.sh
	$(ICSOS_ROOT)/scripts/stage-make.sh $(STAGE)
	$(CC) $(CFLAGS) $(APP_LDFLAGS) -o$(EXE) \
		$(addprefix $(STAGE)/,$(MAKE_C)) $(LIBS)

install: $(EXE)
	mkdir -p $(ICSOS_ROOT)/apps
	cp $(EXE) $(ICSOS_ROOT)/apps

uninstall:
	rm -f $(ICSOS_ROOT)/apps/$(EXE)

clean:
	rm -f $(EXE)
