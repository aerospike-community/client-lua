###############################################################################
##  SETTINGS                                                                 ##
###############################################################################


CFLAGS = -std=gnu99 -g -rdynamic -Wall 
CFLAGS += -fno-common -fno-strict-aliasing -fPIC 
CFLAGS += -DMARCH_$(ARCH) -D_FILE_OFFSET_BITS=64 
CFLAGS += -D_REENTRANT -D_GNU_SOURCE -DMEM_COUNT

LDFLAGS = 
LDFLAGS += -lssl -lcrypto -lpthread -lrt
LDFLAGS += -llua
LDFLAGS += -lm
LDFLAGS += -laerospike

###############################################################################
##  MAIN TARGETS                                                             ##
###############################################################################

all: build

build: as_lua


as_lua: 
	gcc $(CFLAGS) src/as_lua.c $(LDFLAGS) -o as_lua


