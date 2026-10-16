#
# minimum Makefile for PVM at ESIEE
#

# includes arch. specific

include $(PVM_ROOT)/conf/$(PVM_ARCH).def

OPTIONS	=	-g -Wno-implicit-int -Wno-implicit-function-declaration
# OPTIONS	=	-O
CFLAGS	=	$(OPTIONS) -I$(PVM_ROOT)/include $(ARCHCFLAGS)

LIBS	=	-lpvm3 -lgpvm3 -lm -ltirpc 
LFLAGS	=	-L$(PVM_ROOT)/lib/$(PVM_ARCH)

LDFLAGS	=	$(LFLAGS) $(LIBS)

# new default rule includes mv to bin/$PVM_ARCH (default PATH for pvmd)

.c:
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	mv $@ $(HOME)/pvm3/bin/$(PVM_ARCH)

# dependances go here

# hello: hello.c
# 	$(CC) $(CFLAGS) -o $@ $@.c $(LDFLAGS)
# 	mv $@ $(HOME)/pvm3/bin/$(PVM_ARCH)

# monprog: $$@.c $$@.h
monprog: monprog.c monprog.h
	$(CC) $(CFLAGS) -o $@ $@.c $(LDFLAGS)
	mv $@ $(HOME)/pvm3/bin/$(PVM_ARCH)

gauss: gauss.c
	$(CC) $(CFLAGS) -o $@ $@.c $(LDFLAGS)

gaussp: gaussp.c
	$(CC) $(CFLAGS) -o $@ $@.c $(LDFLAGS)
